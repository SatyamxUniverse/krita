/*
 * SPDX-FileCopyrightText: 2022 Alvin Wong <alvin@alvinhc.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "KisTopLevelWidgetChangeNotifier.h"

#include <QApplication>
#include <QWidget>
#include <QDebug>
#include <QTimer>
#include <QPointer>

class KisTopLevelWidgetChangeNotifierBackend
    : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(KisTopLevelWidgetChangeNotifierBackend)

public:
    KisTopLevelWidgetChangeNotifierBackend();
    ~KisTopLevelWidgetChangeNotifierBackend();

    void addNotifier(KisTopLevelWidgetChangeNotifier *notifier);
    void removeNotifier(KisTopLevelWidgetChangeNotifier *notifier);

    bool eventFilter(QObject *watched, QEvent *event) override;

private Q_SLOTS:
    void slotAboutToQuit();

private:
    QSet<KisTopLevelWidgetChangeNotifier *> m_notifiers;
};


class KisTopLevelWidgetChangeNotifier::Private
{
    friend class KisTopLevelWidgetChangeNotifier;
    friend class KisTopLevelWidgetChangeNotifierBackend;

    Private(QWidget *widget)
        : widget(widget)
        , lastTopWidget(widget->window())
    {}
    QWidget *widget;
    QPointer<QWidget> lastTopWidget;
};


static KisTopLevelWidgetChangeNotifierBackend *backendInstance {nullptr};

KisTopLevelWidgetChangeNotifierBackend::KisTopLevelWidgetChangeNotifierBackend()
{
    connect(qApp, &QApplication::aboutToQuit,
            this, &KisTopLevelWidgetChangeNotifierBackend::slotAboutToQuit);
    qApp->installEventFilter(this);
}

KisTopLevelWidgetChangeNotifierBackend::~KisTopLevelWidgetChangeNotifierBackend()
{
    if (backendInstance != this) {
        qWarning() << "KisTopLevelWidgetChangeNotifierBackend instance mismatch!";
    } else {
        backendInstance = nullptr;
    }
}

void KisTopLevelWidgetChangeNotifierBackend::slotAboutToQuit()
{
    delete this;
}

void KisTopLevelWidgetChangeNotifierBackend::addNotifier(KisTopLevelWidgetChangeNotifier *notifier)
{
    m_notifiers.insert(notifier);
}

void KisTopLevelWidgetChangeNotifierBackend::removeNotifier(KisTopLevelWidgetChangeNotifier *notifier)
{
    m_notifiers.remove(notifier);
    if (m_notifiers.isEmpty()) {
        delete this;
    }
}

bool KisTopLevelWidgetChangeNotifierBackend::eventFilter(QObject *watched, QEvent *event)
{
    if (watched->isWidgetType() && event->type() == QEvent::ParentChange) {
        QWidget *watchedWidget = static_cast<QWidget *>(watched);
        QVector<QPointer<KisTopLevelWidgetChangeNotifier>> pendingNotifiers;
        Q_FOREACH(KisTopLevelWidgetChangeNotifier *notifier, m_notifiers) {
            if (watchedWidget == notifier->d->widget || watchedWidget->isAncestorOf(notifier->d->widget)) {
                pendingNotifiers.append(notifier);
            }
        }
        Q_FOREACH(const QPointer<KisTopLevelWidgetChangeNotifier> &notifier, pendingNotifiers) {
            if (notifier && notifier->d) {
                QWidget *topWidget = notifier->d->widget->window();
                if (notifier->d->lastTopWidget != topWidget) {
                    notifier->d->lastTopWidget = topWidget;
                    emit notifier->topLevelWidgetChanged(topWidget);
                }
            }
        }
    }
    return false;
}


KisTopLevelWidgetChangeNotifier::KisTopLevelWidgetChangeNotifier(QWidget *widgetToWatch)
    : KisTopLevelWidgetChangeNotifier(widgetToWatch, widgetToWatch)
{}

KisTopLevelWidgetChangeNotifier::KisTopLevelWidgetChangeNotifier(QWidget *widgetToWatch, QObject *parent)
    : QObject(parent)
    , d(nullptr)
{
    if (!widgetToWatch) {
        qWarning() << "KisTopLevelWidgetChangeNotifier constructed without widget!";
        return;
    }
    if (thread() != qApp->thread()) {
        qWarning() << "KisTopLevelWidgetChangeNotifier constructed on non-GUI thread!";
        return;
    }
    if (!backendInstance) {
        backendInstance = new KisTopLevelWidgetChangeNotifierBackend();
    }

    d = new Private(widgetToWatch);
    connect(widgetToWatch, &QObject::destroyed,
            this, &KisTopLevelWidgetChangeNotifier::slotWidgetDestroyed);
    backendInstance->addNotifier(this);
}

KisTopLevelWidgetChangeNotifier::~KisTopLevelWidgetChangeNotifier()
{
    if (d) {
        backendInstance->removeNotifier(this);
    }
    delete d;
}

void KisTopLevelWidgetChangeNotifier::slotWidgetDestroyed()
{
    delete d;
    d = nullptr;
    backendInstance->removeNotifier(this);
}

#include "KisTopLevelWidgetChangeNotifier.moc"
