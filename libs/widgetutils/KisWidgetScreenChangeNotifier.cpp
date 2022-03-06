/*
 * SPDX-FileCopyrightText: 2022 Alvin Wong <alvin@alvinhc.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "KisWidgetScreenChangeNotifier.h"

#include "KisTopLevelWidgetChangeNotifier.h"

#include <QWidget>
#include <QWindow>
#include <QPointer>
#include <QDebug>
#include <QScreen>
#include <QApplication>

class KisWidgetScreenChangeNotifier::Private
{
    friend class KisWidgetScreenChangeNotifier;

    Private(KisWidgetScreenChangeNotifier *parent, QWidget *widget)
        : widget(widget)
        , topLevelWidgetChangeNotifier(widget, parent)
    {}
    QWidget *widget;
    QPointer<QWindow> lastWindow;
    QPointer<QScreen> lastScreen;
    KisTopLevelWidgetChangeNotifier topLevelWidgetChangeNotifier;
};


KisWidgetScreenChangeNotifier::KisWidgetScreenChangeNotifier(QWidget *widgetToWatch)
    : KisWidgetScreenChangeNotifier(widgetToWatch, widgetToWatch)
{}

KisWidgetScreenChangeNotifier::KisWidgetScreenChangeNotifier(QWidget *widgetToWatch, QObject *parent)
    : QObject(parent)
    , d(nullptr)
{
    if (!widgetToWatch) {
        qWarning() << "KisWidgetScreenChangeNotifier constructed without widget!";
        return;
    }
    if (thread() != qApp->thread()) {
        qWarning() << "KisWidgetScreenChangeNotifier constructed on non-GUI thread!";
        return;
    }

    d = new Private(this, widgetToWatch);
    connect(&d->topLevelWidgetChangeNotifier, &KisTopLevelWidgetChangeNotifier::topLevelWidgetChanged,
            this, &KisWidgetScreenChangeNotifier::slotTopLevelWidgetChanged);
    connect(widgetToWatch, &QObject::destroyed,
            this, &KisWidgetScreenChangeNotifier::slotWidgetDestroyed);
    setWindow(widgetToWatch->window());
}

KisWidgetScreenChangeNotifier::~KisWidgetScreenChangeNotifier()
{
    delete d;
}

void KisWidgetScreenChangeNotifier::slotWidgetDestroyed()
{
    if (d->lastWindow) {
        disconnect(d->lastWindow, &QWindow::screenChanged,
                   this, &KisWidgetScreenChangeNotifier::slotWindowScreenChanged);
    }
    delete d;
    d = nullptr;
}

void KisWidgetScreenChangeNotifier::slotTopLevelWidgetChanged(QWidget *windowWidget)
{
    setWindow(windowWidget);
}

void KisWidgetScreenChangeNotifier::setWindow(QWidget *windowWidget)
{
    QWindow *newWindow = windowWidget->windowHandle();
    if (newWindow == d->lastWindow) {
        return;
    }
    if (d->lastWindow) {
        disconnect(d->lastWindow, &QWindow::screenChanged,
                   this, &KisWidgetScreenChangeNotifier::slotWindowScreenChanged);
    }
    if (!newWindow) {
        // The native window hasn't been created yet... It should be created
        // when the widget is shown, so install an event filter to catch that.
        d->widget->installEventFilter(this);
    }
    d->lastWindow = newWindow;
    if (newWindow) {
        connect(newWindow, &QWindow::screenChanged,
                this, &KisWidgetScreenChangeNotifier::slotWindowScreenChanged);
        QScreen *screen = newWindow->screen();
        if (screen && d->lastScreen != screen) {
            d->lastScreen = screen;
            emit screenChanged(screen);
        }
    }
    // If newWindow is nullptr, we do not change lastScreen. This is
    // intentional. We also don't want to emit `screenChanged(nullptr)`.
}

bool KisWidgetScreenChangeNotifier::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != d->widget) {
        // ???
        return false;
    }
    if (event->type() == QEvent::Show) {
        d->widget->removeEventFilter(this);
        setWindow(d->widget->window());
    }
    return false;
}

void KisWidgetScreenChangeNotifier::slotWindowScreenChanged(QScreen *screen)
{
    if (d->lastScreen != screen) {
        d->lastScreen = screen;
        emit screenChanged(screen);
    }
}
