
/*
 *  SPDX-FileCopyrightText: 2021 Know Zero
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "KisWindowStateTransition.h"



#include <kis_debug.h>
#include "kis_global.h"
#include "kis_config.h"
#include "kis_canvas_controller.h"
#include "KisView.h"
#include "KisViewManager.h"
#include "KoZoomController.h"
#include "kactioncollection.h"

#include <QApplication>
#include <QEvent>
#include <QEventLoop>
#include <QPainter>
#include <QMdiSubWindow>
#include <QScreen>



KisWindowStateTransition::KisWindowStateTransition(QWidget *parent)
    : QWidget(parent)
{

    setWindowFlags(Qt::FramelessWindowHint | Qt::ToolTip | Qt::WindowTransparentForInput);
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_ShowWithoutActivating);

    setAutoFillBackground(true);
}

void KisWindowStateTransition::runAction()
{
    m_status = TransitionStatus::RUNNING;

    if (m_currentAction->objectName() == "fullscreen") {
        m_currentAction->triggered(m_currentAction->isChecked());
    } else {
        m_currentAction->toggled(m_currentAction->isChecked());
    }
            
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
}


bool KisWindowStateTransition::startTransition(QAction* action, QMdiArea* mdiArea, bool fullscreen, int timeout)
{
    
    if (m_status == TransitionStatus::READY) {

        m_status = TransitionStatus::STARTED;
        m_currentAction = action;
        m_timeout = timeout;
        m_mapToGlobal = fullscreen;
        m_mdiArea = mdiArea;

        configureTransition();

        m_runtime = 0;
        refreshTarget();

        m_mdiArea->installEventFilter(this);

        QWidget *main = parentWidget();

        if (m_timeout > 0) {
            QRect overlayRect = m_mapToGlobal ? qApp->screenAt(main->pos())->geometry() : main->geometry();

            setGeometry(overlayRect);
            show();
            m_timerId = startTimer(1);
        } else {
            runAction();
            cleanup();
        }

        return false;
    } else if (m_status == TransitionStatus::FINISHED) {
        m_runtime = m_timeout+1;

        return true;
    } else if (m_currentAction != action) {
        return true;
    } else {
        return true;
    }
}

void KisWindowStateTransition::endTransition(const QString &actionName)
{
    if ((m_status == TransitionStatus::STARTED || m_status == TransitionStatus::RUNNING) && m_currentAction->objectName() == actionName) m_status = TransitionStatus::FINISHED; 
}

void KisWindowStateTransition::configureTransition()
{
    
    KisConfig cfg(false);
    bool useMdiColor = true;

    QMdiSubWindow* currentSubWindow = m_mdiArea->currentSubWindow();

    if (m_mdiArea->viewMode() == QMdiArea::TabbedView || currentSubWindow->isMaximized()) {
        m_targetWidget = currentSubWindow->widget()->focusWidget();
    } else {
        m_targetWidget = m_mdiArea;
    }

    m_mdiSubWinPos.clear();

    m_mdiStartPos = m_mdiArea->mapToGlobal(QPoint());

    for (QMdiSubWindow *subwin : m_mdiArea->subWindowList()) {
        KisView *view = qobject_cast<KisView *>(subwin->widget());

        if (m_mdiArea->viewMode() == QMdiArea::SubWindowView && !subwin->isMaximized()) {
            m_mdiSubWinPos[subwin] = subwin->mapToGlobal(QPoint());
        } else if (view->zoomController()->zoomMode() == KoZoomMode::ZOOM_CONSTANT) {
            m_mdiSubWinPos[subwin] = view->canvasController()->scrollBarValue();

            useMdiColor = false;
        }
    }

    QPalette pal = palette();

    pal.setColor(QPalette::Window, useMdiColor ? m_mdiArea->background().color() : cfg.canvasBorderColor());
    setPalette(pal);
}

void KisWindowStateTransition::refreshTarget()
{
    m_targetPixelmap = m_targetWidget->grab();

    QPoint targetPos = m_mapToGlobal ? m_targetWidget->mapToGlobal(QPoint()) : m_targetWidget->mapTo(parentWidget(),QPoint());

    m_targetRect = QRect(targetPos.x(),  targetPos.y(), m_targetWidget->width(), m_targetWidget->height());
}


void KisWindowStateTransition::cleanup()
{
    m_mdiArea->removeEventFilter(this);
    m_status = TransitionStatus::READY;
    m_currentAction = nullptr;
}


void KisWindowStateTransition::syncPos()
{
    if (m_mdiArea) {
        QPoint mdiEndPos = m_mdiArea->mapToGlobal(QPoint());
        QPoint mdiPosDiff = mdiEndPos - m_mdiStartPos;

        for (QMdiSubWindow *subwin : m_mdiArea->subWindowList()) {
            if (m_mdiSubWinPos.contains(subwin)) {
                KisView *view = qobject_cast<KisView *>(subwin->widget());

                if (m_mdiArea->viewMode() == QMdiArea::SubWindowView  && !subwin->isMaximized()) {
                    subwin->move(m_mdiSubWinPos[subwin] - mdiEndPos);
                } else if (view->zoomController()->zoomMode() == KoZoomMode::ZOOM_CONSTANT) {
                    view->canvasController()->setScrollBarValue(mdiPosDiff + m_mdiSubWinPos[subwin]);
                }
            }
        }
    }
}

void KisWindowStateTransition::timerEvent(QTimerEvent* e)
{
    if (e->timerId() == m_timerId) {
        m_runtime++;

        if ((m_status == TransitionStatus::RUNNING || m_status == TransitionStatus::FINISHED) && m_runtime < ceil(m_timeout / 2) + 10 ) syncPos();
        if (m_runtime == ceil((double)m_timeout / 2.00) ) runAction();

        if (m_runtime >= m_timeout) {
            killTimer(m_timerId);
            cleanup();
            hide();

        } else if (m_runtime % 50 == 0) {
            refreshTarget();
            update();
        } else if (m_runtime > 1000) {
            killTimer(m_timerId);
            cleanup();
            hide();
        }
    }
}


void KisWindowStateTransition::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);

    QPainter p(this);

    p.drawPixmap(m_targetRect, m_targetPixelmap);
}


bool KisWindowStateTransition::eventFilter(QObject *object, QEvent *e)
{
    Q_UNUSED(object);

    if (e->type() == QEvent::Move) {
        syncPos();
    }

    return false;
}
