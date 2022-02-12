/*
 * SPDX-FileCopyrightText: 2022 Alvin Wong <alvin@alvinhc.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef KIS_TOP_LEVEL_WIDGET_CHANGE_NOTIFIER_H
#define KIS_TOP_LEVEL_WIDGET_CHANGE_NOTIFIER_H

#include <QObject>

#include "kritawidgetutils_export.h"

class QWidget;

/**
 * Helper class to receive notification when the top-level widget of a QWidget
 * has changed. The top-level widget is the widget returned by
 * `QWidget::window()`. It may change when the widget itself or any of its
 * ancestors are reparented. This class works by installing an event filter
 * onto QApplication to receive ParentChange events sent to widgets.
 */
class KRITAWIDGETUTILS_EXPORT KisTopLevelWidgetChangeNotifier
    : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(KisTopLevelWidgetChangeNotifier)
    friend class KisTopLevelWidgetChangeNotifierBackend;

    class Private;
    Private *d;

public:
    explicit KisTopLevelWidgetChangeNotifier(QWidget *widgetToWatch);
    KisTopLevelWidgetChangeNotifier(QWidget *widgetToWatch, QObject *parent);
    ~KisTopLevelWidgetChangeNotifier() override;

Q_SIGNALS:
    /**
     * This signal is emitted when the top-level widget of the watched QWidget
     * has changed.
     * 
     * @param windowWidget The new top-level widget
     */
    void topLevelWidgetChanged(QWidget *windowWidget);

private Q_SLOTS:
    void slotWidgetDestroyed();
};

#endif // KIS_TOP_LEVEL_WIDGET_CHANGE_NOTIFIER_H
