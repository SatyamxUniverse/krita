/*
 * SPDX-FileCopyrightText: 2022 Alvin Wong <alvin@alvinhc.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef KIS_WIDGET_SCREEN_CHANGE_NOTIFIER_H
#define KIS_WIDGET_SCREEN_CHANGE_NOTIFIER_H

#include <QObject>

#include "kritawidgetutils_export.h"

class QWidget;
class QScreen;

/**
 * Helper class to receive notification when the QScreen associated with a
 * QWidget has changed. This class tracks screen changes by using the signal
 * `QWindow::screenChanged(QScreen *)` on the top-level window. It also tracks
 * when the widget is reparented to another window and reconnects to the signal
 * on the new QWindow.
 *
 * This class uses `widget->window()->windowHandle()->screen()` to get the
 * associated screen. If when constructed the widget does not yet have a
 * top-level widget that is a Qt::Window, then the screenChanged signal will
 * be emitted when the widget is first reparented to another widget with a
 * top-level window. It does not consider the return value of
 * `QWidget::screen()`, which is only available starting from Qt 5.14.
 */
class KRITAWIDGETUTILS_EXPORT KisWidgetScreenChangeNotifier
    : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(KisWidgetScreenChangeNotifier)

    class Private;
    Private *d;

public:
    explicit KisWidgetScreenChangeNotifier(QWidget *widgetToWatch);
    KisWidgetScreenChangeNotifier(QWidget *widgetToWatch, QObject *parent);
    ~KisWidgetScreenChangeNotifier() override;

Q_SIGNALS:
    /**
     * This signal is emitted when the screen of the QWindow associated with
     * the watched QWidget has changed.
     */
    void screenChanged(QScreen *screen);

private:
    void setWindow(QWidget *);
    bool eventFilter(QObject *watched, QEvent *event) override;

private Q_SLOTS:
    void slotWidgetDestroyed();
    void slotWindowScreenChanged(QScreen *);
    void slotTopLevelWidgetChanged(QWidget *);
};

#endif // KIS_WIDGET_SCREEN_CHANGE_NOTIFIER_H
