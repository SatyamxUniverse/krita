/*
 *  SPDX-FileCopyrightText: 2022 Deif Lou <ginoba@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KISTOOLOPTIONSTOOLBARCONTAINER_H
#define KISTOOLOPTIONSTOOLBARCONTAINER_H

#include "kritaui_export.h"

#include <QWidget>
#include <QScopedPointer>

class KRITAUI_EXPORT KisToolOptionsToolbarContainer : public QWidget
{
    Q_OBJECT

public:
    KisToolOptionsToolbarContainer(QWidget *parent = nullptr);
    ~KisToolOptionsToolbarContainer() override;

    void setOptionWidgets(const QList<QPointer<QWidget>> &optionWidgetList);

    QSize sizeHint() const override;
    bool eventFilter(QObject *o, QEvent *e) override;
    void resizeEvent(QResizeEvent*) override;
    void enterEvent(QEvent*) override;
    void leaveEvent(QEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;

private:
    class Private;
    QScopedPointer<Private> m_d;
};

#endif
