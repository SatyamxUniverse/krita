/*
 * SPDX-FileCopyrightText: 2022 Alvin Wong <alvin@alvinhc.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef KIS_CANVAS_TOOL_OUTLINE_PAINT_ITEM_P_H
#define KIS_CANVAS_TOOL_OUTLINE_PAINT_ITEM_P_H

#include <QQuickItem>

class KisCanvas2;

class KisCanvasToolOutlinePaintItem
    : public QQuickItem
{
    Q_OBJECT
    Q_DISABLE_COPY(KisCanvasToolOutlinePaintItem)

    class Private;
    Private *const d;

public:
    KisCanvasToolOutlinePaintItem(QQuickItem *parent, KisCanvas2 *canvas);
    ~KisCanvasToolOutlinePaintItem() override;

    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updatePaintNodeData) override;

public Q_SLOTS:
    void setAnchorsFill(QQuickItem *item);
};

#endif // KIS_CANVAS_TOOL_OUTLINE_PAINT_ITEM_P_H
