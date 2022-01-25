
/*
 *  SPDX-FileCopyrightText: 2021 Know Zero
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */


#ifndef KIS_WINDOW_STATE_TRANSITION_H
#define KIS_WINDOW_STATE_TRANSITION_H

#include <QWidget>
#include <QHash>
#include <QPixmap>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QAction>

#include <kritaui_export.h>




/**
 * @brief The KisWindowStateTransition class is a widget to help make smoother transition to full screen and toggling of dockers.
 */
class KRITAUI_EXPORT KisWindowStateTransition : public QWidget
{
    Q_OBJECT

public:

    explicit KisWindowStateTransition(QWidget *parent);

    bool startTransition(QAction* action, QMdiArea* mdiArea, bool fullscreen = true, int timeout = 200);
    void endTransition(const QString &actionName);

private:
    void refreshTarget();
    void syncPos();
    void cleanup();
    void runAction();
    void configureTransition();

    
protected:
    void paintEvent(QPaintEvent *e) override;
    void timerEvent(QTimerEvent *e) override;
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    enum TransitionStatus {
        READY = 0,
        RUNNING,
        STARTED,
        FINISHED
    };
    int m_timerId;
    int m_runtime;
    int m_timeout;
    bool m_mapToGlobal;
    QRect m_targetRect;
    QWidget* m_targetWidget = nullptr;
    QPixmap m_targetPixelmap;
    QMdiArea* m_mdiArea;
    QPoint m_mdiStartPos;
    QHash<QMdiSubWindow*, QPoint> m_mdiSubWinPos;
    QAction* m_currentAction;
    TransitionStatus m_status = TransitionStatus::READY;

};

#endif // KIS_WINDOW_STATE_TRANSITION_H

