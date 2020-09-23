/*
 *  Copyright (c) 2020 Saurabh Kumar <saurabhk660@gmail.com>
 *
 *  This library is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "storyboarddocker_dock.h"
#include "commentDelegate.h"
#include "commentModel.h"
#include "storyboardModel.h"
#include "storyboardDelegate.h"

#include <QMenu>
#include <QButtonGroup>
#include <QDebug>
#include <QStringListModel>
#include <QListView>

#include <klocalizedstring.h>

#include <KisPart.h>
#include <KisViewManager.h>
#include <KisDocument.h>
#include <kis_icon.h>

#include "ui_wdgstoryboarddock.h"
#include "ui_wdgcommentmenu.h"
#include "ui_wdgarrangemenu.h"

class CommentMenu: public QMenu
{
    Q_OBJECT
public:
    CommentMenu(QWidget *parent, CommentModel *m_model)
        : QMenu(parent)
        , m_menuUI(new Ui_WdgCommentMenu())
        , model(m_model)
        , delegate(new CommentDelegate(this))
    {
        QWidget* commentWidget = new QWidget(this);
        m_menuUI->setupUi(commentWidget);

        m_menuUI->fieldListView->setDragEnabled(true);
        m_menuUI->fieldListView->setAcceptDrops(true);
        m_menuUI->fieldListView->setDropIndicatorShown(true);
        m_menuUI->fieldListView->setDragDropMode(QAbstractItemView::InternalMove);

        m_menuUI->fieldListView->setModel(model);
        m_menuUI->fieldListView->setItemDelegate(delegate);

        m_menuUI->fieldListView->setEditTriggers(QAbstractItemView::AnyKeyPressed |
                                                    QAbstractItemView::DoubleClicked  );

        m_menuUI->btnAddField->setIcon(KisIconUtils::loadIcon("list-add"));
        m_menuUI->btnDeleteField->setIcon(KisIconUtils::loadIcon("trash-empty"));
        m_menuUI->btnAddField->setIconSize(QSize(22, 22));
        m_menuUI->btnDeleteField->setIconSize(QSize(22, 22));
        connect(m_menuUI->btnAddField, SIGNAL(clicked()), this, SLOT(slotaddItem()));
        connect(m_menuUI->btnDeleteField, SIGNAL(clicked()), this, SLOT(slotdeleteItem()));

        KisAction *commentAction = new KisAction(commentWidget);
        commentAction->setDefaultWidget(commentWidget);
        this->addAction(commentAction);
    }

private Q_SLOTS:
    void slotaddItem()
    {
        int row = m_menuUI->fieldListView->currentIndex().row()+1;
        model->insertRows(row, 1);

        QModelIndex index = model->index(row);
        m_menuUI->fieldListView->setCurrentIndex(index);
        m_menuUI->fieldListView->edit(index);
    }

    void slotdeleteItem()
    {
        model->removeRows(m_menuUI->fieldListView->currentIndex().row(), 1);
    }

private:
    QScopedPointer<Ui_WdgCommentMenu> m_menuUI;
    CommentModel *model;
    CommentDelegate *delegate;
};

class ArrangeMenu: public QMenu
{
public:
    ArrangeMenu(QWidget *parent)
        : QMenu(parent)
        , m_menuUI(new Ui_WdgArrangeMenu())
        , modeGroup(new QButtonGroup(this))
        , viewGroup(new QButtonGroup(this))
    {
        QWidget* arrangeWidget = new QWidget(this);
        m_menuUI->setupUi(arrangeWidget);

        modeGroup->addButton(m_menuUI->btnColumnMode, 0);
        modeGroup->addButton(m_menuUI->btnRowMode, 1);
        modeGroup->addButton(m_menuUI->btnGridMode, 2);

        viewGroup->addButton(m_menuUI->btnAllView, 0);
        viewGroup->addButton(m_menuUI->btnThumbnailsView, 1);
        viewGroup->addButton(m_menuUI->btnCommentsView, 2);

        KisAction *arrangeAction = new KisAction(arrangeWidget);
        arrangeAction->setDefaultWidget(arrangeWidget);
        this->addAction(arrangeAction);
    }

    QButtonGroup* getModeGroup(){ return modeGroup;}
    QButtonGroup* getViewGroup(){ return viewGroup;}

private:
    QScopedPointer<Ui_WdgArrangeMenu> m_menuUI;
    QButtonGroup *modeGroup;
    QButtonGroup *viewGroup;
};

StoryboardDockerDock::StoryboardDockerDock( )
    : QDockWidget(i18n("Storyboard"))
    , m_ui(new Ui_WdgStoryboardDock())
{
    QWidget* mainWidget = new QWidget(this);
    setWidget(mainWidget);
    m_ui->setupUi(mainWidget);

    m_exportMenu = new QMenu(this);
    m_ui->btnExport->setMenu(m_exportMenu);
    m_ui->btnExport->setPopupMode(QToolButton::MenuButtonPopup);

    m_exportAsPdfAction = new KisAction("Export as PDF", m_exportMenu);
    m_exportMenu->addAction(m_exportAsPdfAction);

    m_exportAsSvgAction = new KisAction("Export as SVG");
    m_exportMenu->addAction(m_exportAsSvgAction);
    connect(m_exportAsPdfAction, SIGNAL(triggered()), this, SLOT(slotExportAsPdf()));
    connect(m_exportAsSvgAction, SIGNAL(triggered()), this, SLOT(slotExportAsSvg()));

    m_commentModel = new CommentModel(this);
    m_commentMenu = new CommentMenu(this, m_commentModel);

    m_ui->btnComment->setMenu(m_commentMenu);
    m_ui->btnComment->setPopupMode(QToolButton::MenuButtonPopup);

    m_lockAction = new KisAction(KisIconUtils::loadIcon("unlocked"), "Lock", m_ui->btnLock);
    m_lockAction->setCheckable(true);
    m_ui->btnLock->setDefaultAction(m_lockAction);
    m_ui->btnLock->setIconSize(QSize(22, 22));
    connect(m_lockAction, SIGNAL(toggled(bool)), this, SLOT(slotLockClicked(bool)));

    m_arrangeMenu = new ArrangeMenu(this);
    m_ui->btnArrange->setMenu(m_arrangeMenu);
    m_ui->btnArrange->setPopupMode(QToolButton::InstantPopup);
    m_ui->btnArrange->setIcon(KisIconUtils::loadIcon("view-choose"));
    m_ui->btnArrange->setIconSize(QSize(22, 22));

    m_modeGroup = m_arrangeMenu->getModeGroup();
    m_viewGroup = m_arrangeMenu->getViewGroup();

    connect(m_modeGroup, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(slotModeChanged(QAbstractButton*)));
    connect(m_viewGroup, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(slotViewChanged(QAbstractButton*)));

    m_storyboardModel = new StoryboardModel(this);
    StoryboardDelegate *delegate = new StoryboardDelegate(this);
    delegate->setView(m_ui->listView);
    m_ui->listView->setModel(m_storyboardModel);
    m_ui->listView->setItemDelegate(delegate);
    delegate->setView(m_ui->listView);

    m_storyboardModel->insertRows(0, 10);
    m_storyboardModel->setCommentModel(m_commentModel);

    m_modeGroup->button(2)->click();
    m_viewGroup->button(0)->click();
}

StoryboardDockerDock::~StoryboardDockerDock()
{
}

void StoryboardDockerDock::setCanvas(KoCanvasBase *canvas)
{
}

void StoryboardDockerDock::setViewManager(KisViewManager* kisview)
{
}

void StoryboardDockerDock::slotExportAsPdf()
{
    qDebug()<<"export as pdf";
    slotExport("pdf");
}

void StoryboardDockerDock::slotExportAsSvg()
{
    qDebug()<<"export as svg";
    slotExport("svg");
}

void StoryboardDockerDock::slotExport(QString mode)
{
    qDebug()<<"mode is "<<mode;
}

void StoryboardDockerDock::slotLockClicked(bool isLocked){
    if (isLocked) {
        m_lockAction->setIcon(KisIconUtils::loadIcon("locked"));
    }
    else {
        m_lockAction->setIcon(KisIconUtils::loadIcon("unlocked"));
    }
}

void StoryboardDockerDock::slotModeChanged(QAbstractButton* button)
{
    QString mode = button->text();
    if (mode == "Column") {
        m_ui->listView->setFlow(QListView::LeftToRight);
        m_ui->listView->setWrapping(false);
        m_ui->listView->setItemOrientation(Qt::Vertical);
        m_viewGroup->button(2)->setEnabled(true);
    }
    else if (mode == "Row") {
        m_ui->listView->setFlow(QListView::TopToBottom);
        m_ui->listView->setWrapping(false);
        m_ui->listView->setItemOrientation(Qt::Horizontal);
        m_viewGroup->button(2)->setEnabled(false);           //disable the comments only view
    }
    else if (mode == "Grid") {
        m_ui->listView->setFlow(QListView::LeftToRight);
        m_ui->listView->setWrapping(true);
        m_ui->listView->setItemOrientation(Qt::Vertical);
        m_viewGroup->button(2)->setEnabled(true);
    }
    m_storyboardModel->layoutChanged();
}

void StoryboardDockerDock::slotViewChanged(QAbstractButton* button)
{
    QString view = button->text();
    if (view == "All") {
        m_ui->listView->setCommentVisibility(true);
        m_ui->listView->setThumbnailVisibility(true);
        m_modeGroup->button(1)->setEnabled(true);
    }
    else if (view == "Thumbnails Only") {
        m_ui->listView->setCommentVisibility(false);
        m_ui->listView->setThumbnailVisibility(true);
        m_modeGroup->button(1)->setEnabled(true);
    }

    else if (view == "Comments Only") {
        m_ui->listView->setCommentVisibility(true);
        m_ui->listView->setThumbnailVisibility(false);
        m_modeGroup->button(1)->setEnabled(false);               //disable the row mode
    }
    m_storyboardModel->layoutChanged();
}

#include "storyboarddocker_dock.moc"