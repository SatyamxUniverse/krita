/*
 * SPDX-FileCopyrightText: 2022 Alvin Wong <alvin@alvinhc.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "KisCanvasDecorationPaintItem_p.h"

#include "kis_canvas_decoration.h"
#include "kis_canvas2.h"
#include "kis_coordinates_converter.h"

#include <QQmlProperty>

class KisCanvas2;

// FIXME: Remove the overhead of a QImage or FBO of QQuickPaintedItem by
// implementing a custom QSGNode.

class KisCanvasDecorationPaintItem::Private
{
    friend class KisCanvasDecorationPaintItem;

    KisCanvasDecoration *decoration;
    KisCanvas2 *canvas {nullptr};
};

KisCanvasDecorationPaintItem::KisCanvasDecorationPaintItem(KisCanvasDecoration *decoration)
    : QQuickPaintedItem()
    , d(new Private)
{
    setParent(decoration);
    QString decoName(decoration->objectName());
    if (decoName.isEmpty()) {
        decoName = decoration->metaObject()->className();
    }
    setObjectName(QStringLiteral("PaintItem/") + decoName);

    d->decoration = decoration;
    setRenderTarget(FramebufferObject);
}

KisCanvasDecorationPaintItem::~KisCanvasDecorationPaintItem()
{
    delete d;
}

void KisCanvasDecorationPaintItem::paint(QPainter *painter)
{
    if (!d->canvas) {
        return;
    }
    if (!isVisible()) {
        return;
    }
    auto coordinatesConverter = d->canvas->coordinatesConverter();
    QRectF docRect(coordinatesConverter->widgetToDocument(QRectF(QPointF(), size())));
    d->decoration->paint(*painter, docRect, coordinatesConverter, d->canvas);
}

void KisCanvasDecorationPaintItem::setAnchorsFill(QQuickItem *item)
{
    QQmlProperty(this, "anchors.fill").write(QVariant::fromValue(item));
}

void KisCanvasDecorationPaintItem::setCanvas(KisCanvas2 *canvas)
{
    d->canvas = canvas;
}
