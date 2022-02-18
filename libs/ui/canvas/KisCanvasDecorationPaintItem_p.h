/*
 * SPDX-FileCopyrightText: 2022 Alvin Wong <alvin@alvinhc.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef KIS_CANVAS_DECORATION_PAINT_ITEM_P_H
#define KIS_CANVAS_DECORATION_PAINT_ITEM_P_H

#include <QQuickPaintedItem>

class KisCanvasDecoration;
class KisCoordinatesConverter;
class KisCanvas2;

class KisCanvasDecorationPaintItem
    : public QQuickPaintedItem
{
    Q_OBJECT
    Q_DISABLE_COPY(KisCanvasDecorationPaintItem)

    class Private;
    Private *const d;

public:
    KisCanvasDecorationPaintItem(KisCanvasDecoration *decoration);
    ~KisCanvasDecorationPaintItem() override;

    void paint(QPainter *painter) override;

    void setCanvas(KisCanvas2 *canvas);

public Q_SLOTS:
    void setAnchorsFill(QQuickItem *item);
};

#endif // KIS_CANVAS_DECORATION_PAINT_ITEM_P_H
