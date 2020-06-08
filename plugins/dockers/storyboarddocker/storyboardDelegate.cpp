/*
 *  Copyright (c) 2020 Saurabh Kumar <saurabhk660@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "storyboardDelegate.h"

#include <QLineEdit>
#include <QDebug>
#include <QStyle>
#include <QPainter>
#include <QApplication>
#include <QSize>
#include <QMouseEvent>
#include <QListView>
#include <QSpinBox>

#include <kis_icon.h>
#include "storyboardModel.h"

StoryboardDelegate::StoryboardDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

StoryboardDelegate::~StoryboardDelegate()
{
}

void StoryboardDelegate::paint(QPainter *p, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    p->save();
    {
        QStyle *style = option.widget ? option.widget->style() : QApplication::style();
        style->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, p, option.widget);

        p->setFont(option.font);
        if (!index.isValid()){
            p->restore();
            return;
        }
        if (!index.parent().isValid()){
            QRect parentRect = option.rect;
            p->drawRect(parentRect);

            parentRect.setTopLeft(parentRect.topLeft() + QPoint(5, 5));
            parentRect.setBottomRight(parentRect.bottomRight() - QPoint(5, 5));
            //TODO: change highlight color and the area that is highlighted
        }
        else{
            //paint Child index (the indices that hold data)
            QModelIndex parent = index.parent();

            //draw the child items
            int rowNum = index.model()->rowCount(parent);
            int childNum = index.row();
            QString data = index.model()->data(index, Qt::DisplayRole).toString();

            switch (childNum)
            {
                case 0:
                {
                    QRect frameNumRect = option.rect;
                    frameNumRect.setHeight(m_view->fontMetrics().height()+3);
                    frameNumRect.setWidth(3 * m_view->fontMetrics().width("0")+2);
                    frameNumRect.moveBottom(option.rect.top()-1);
                    p->drawRect(frameNumRect);
                    p->drawText(frameNumRect, Qt::AlignHCenter | Qt::AlignVCenter, data);

                    QIcon icon = KisIconUtils::loadIcon("krita-base");
                    icon.paint(p, option.rect);
                    p->drawRect(option.rect);
                    break;
                }
                case 1:
                {
                    QRect itemNameRect = option.rect;
                    itemNameRect.setLeft(option.rect.left() + 5);
                    p->drawText(itemNameRect, Qt::AlignLeft | Qt::AlignVCenter, data);
                    p->drawRect(option.rect);
                    break;
                }
                case 2:    //time duration
                case 3:    //frame duration
                {
                    drawSpinBox(p, option, data);
                    break;
                }
                default:
                {
                    p->drawRect(option.rect);
                    break;
                }
            }
        }
    }
    p->restore();
}

void StoryboardDelegate::drawSpinBox(QPainter *p, const QStyleOptionViewItem &option, QString data) const
{
    QStyle *style = option.widget ? option.widget->style() : QApplication::style();
    QStyleOptionSpinBox spinBoxOption;
    spinBoxOption.stepEnabled = QAbstractSpinBox::StepDownEnabled | QAbstractSpinBox::StepUpEnabled;
    spinBoxOption.subControls = QStyle::SC_SpinBoxUp | QStyle::SC_SpinBoxDown;
    spinBoxOption.rect = option.rect;
    style->drawComplexControl(QStyle::CC_SpinBox, &spinBoxOption, p, option.widget);

    QRect rect = style->subControlRect(QStyle::CC_SpinBox, &spinBoxOption,
                    QStyle::QStyle::SC_SpinBoxEditField);
    rect.moveTopLeft(option.rect.topLeft());
    p->drawText(rect, Qt::AlignHCenter | Qt::AlignVCenter, data);
}

QSize StoryboardDelegate::sizeHint(const QStyleOptionViewItem &option,
                                const QModelIndex &index) const
{
    if (!index.parent().isValid()){
        int width = option.widget->width() - 17;
        const StoryboardModel* model = dynamic_cast<const StoryboardModel*>(index.model());
        int numComments = model->commentCount();
        int numItem = width/250;
        if(numItem <=0){
            return QSize(0, 0);
        }
        return QSize(width / numItem, 140 + numComments*100);
    }
    else {
        return option.rect.size();
    }
    return QSize(0,0);
}


QWidget *StoryboardDelegate::createEditor(QWidget *parent,
    const QStyleOptionViewItem &option ,
    const QModelIndex &index) const
{
    //only create editor for children
    if (index.parent().isValid()){
        int row = index.row();
        switch (row)
        {
            case 0:             //frame thumbnail is uneditable
            return nullptr;
            case 2:            //second and frame spin box
            case 3:
            {
                QSpinBox *spinbox = new QSpinBox(parent);
                return spinbox;
            }
            default:             // for itemName and comments
            {
                QLineEdit *editor = new QLineEdit(parent);
                return editor;
            }
        }
    }
    return nullptr;
}

bool StoryboardDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if ((event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick)
        && (index.flags() & Qt::ItemIsEnabled))
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

        //handle the duration edit event
        if (index.parent().isValid() && (index.row() == 2 || index.row() == 3)){
            QRect upButton = spinBoxUpButton(option);
            QRect downButton = spinBoxDownButton(option);

            bool upButtonClicked = upButton.isValid() && upButton.contains(mouseEvent->pos());
            bool downButtonClicked = downButton.isValid() && downButton.contains(mouseEvent->pos());
            const bool leftButton = mouseEvent->buttons() & Qt::LeftButton;

            if (leftButton && upButtonClicked){
                model->setData(index, index.data().toInt() + 1);
                return true;
            }
            else if (leftButton && downButtonClicked){
                model->setData(index, std::max(0,index.data().toInt() - 1));
                return true;
            }
        }
    }
    return false;
}

//set the existing data in the editor
void StoryboardDelegate::setEditorData(QWidget *editor,
                                    const QModelIndex &index) const
{
    QVariant value = index.data();
    if (index.parent().isValid()){
        int row = index.row();
        switch (row)
        {
            case 0:             //frame thumbnail is uneditable
                return;
            case 2:            //second and frame spin box
            case 3:
            {
                QSpinBox *spinbox = static_cast<QSpinBox*>(editor);
                spinbox->setValue(value.toInt());
                return;
            }
            default:             // for itemName and comments
            {
                QLineEdit *lineEdit = static_cast<QLineEdit*>(editor);
                lineEdit->setText(value.toString());
                return;
            }
        }
    }
}

void StoryboardDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                   const QModelIndex &index) const
{
    QVariant value = index.data();
    if (index.parent().isValid()){
        int row = index.row();
        switch (row)
        {
            case 0:             //frame thumbnail is uneditable
                return;
            case 2:            //second and frame spin box
            case 3:
            {
                QSpinBox *spinbox = static_cast<QSpinBox*>(editor);
                int value = spinbox->value();
                model->setData(index, value, Qt::EditRole);
                return;
            }
            default:             // for itemName and comments
            {
                QLineEdit *lineEdit = static_cast<QLineEdit*>(editor);
                QString value = lineEdit->text();
                model->setData(index, value, Qt::EditRole);
                return;
            }
        }
    }
}

void StoryboardDelegate::updateEditorGeometry(QWidget *editor,
    const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    editor->setGeometry(option.rect);
    qDebug()<<"setting geometry";
}

void StoryboardDelegate::setView(QListView *view)
{
    m_view = view;
}

QRect StoryboardDelegate::spinBoxUpButton(const QStyleOptionViewItem &option)
{
    QStyle *style = option.widget ? option.widget->style() : QApplication::style();
    QStyleOptionSpinBox spinOption;
    spinOption.rect = option.rect;
    QRect rect = style->subControlRect(QStyle::CC_SpinBox, &spinOption,
                    QStyle::QStyle::SC_SpinBoxUp);
    rect.moveTopRight(option.rect.topRight());
    return rect;
}

QRect StoryboardDelegate::spinBoxDownButton(const QStyleOptionViewItem &option)
{
    QStyle *style = option.widget ? option.widget->style() : QApplication::style();
    QStyleOptionSpinBox spinOption;
    spinOption.rect = option.rect;
    QRect rect = style->subControlRect(QStyle::CC_SpinBox, &spinOption,
                    QStyle::QStyle::SC_SpinBoxDown);
    rect.moveBottomRight(option.rect.bottomRight());
    return rect;
}

QRect StoryboardDelegate::spinBoxEditField(const QStyleOptionViewItem &option)
{
    QStyle *style = option.widget ? option.widget->style() : QApplication::style();
    QStyleOptionSpinBox spinOption;
    spinOption.rect = option.rect;
    QRect rect = style->subControlRect(QStyle::CC_SpinBox, &spinOption,
                    QStyle::QStyle::SC_SpinBoxEditField);
    rect.moveTopLeft(option.rect.topLeft());
    return rect;
}