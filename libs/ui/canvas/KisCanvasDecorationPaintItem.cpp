/*
 * SPDX-FileCopyrightText: 2022 Alvin Wong <alvin@alvinhc.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "KisCanvasDecorationPaintItem_p.h"

#include "KisCanvasDecorationOpenGLRenderNode_p.h"
#include "kis_canvas_decoration.h"
#include "kis_canvas2.h"
#include "kis_coordinates_converter.h"

#include <QQmlProperty>
#include <QSGRenderNode>
#include <QSGRendererInterface>
#include <QQuickWindow>

class KisCanvas2;

class KisCanvasDecorationPaintItem::Private
{
    friend class KisCanvasDecorationPaintItem;

    KisCanvasDecoration *decoration;
    KisCanvas2 *canvas {nullptr};
};

KisCanvasDecorationPaintItem::KisCanvasDecorationPaintItem(KisCanvasDecoration *decoration)
    : QQuickItem()
    , d(new Private)
{
    setParent(decoration);
    QString decoName(decoration->objectName());
    if (decoName.isEmpty()) {
        decoName = decoration->metaObject()->className();
    }
    setObjectName(QStringLiteral("PaintItem/") + decoName);

    d->decoration = decoration;
    setFlag(ItemHasContents);
    // setRenderTarget(FramebufferObject);
}

KisCanvasDecorationPaintItem::~KisCanvasDecorationPaintItem()
{
    delete d;
}

QSGNode *KisCanvasDecorationPaintItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updatePaintNodeData)
{
    Q_UNUSED(updatePaintNodeData)

    QSGRenderNode *node = static_cast<QSGRenderNode *>(oldNode);

    QSGRendererInterface *ri = window()->rendererInterface();
    if (!ri)
        return nullptr;

    switch (ri->graphicsApi()) {
    case QSGRendererInterface::OpenGL:
        if (!node) {
            node = new KisCanvasDecorationOpenGLRenderNode(d->decoration);
        }
        static_cast<KisCanvasDecorationOpenGLRenderNode *>(node)->sync(this, d->canvas);
        break;
    case QSGRendererInterface::Software:
        {
            static auto once = []() {
                qWarning() << "KisCanvasDecorationPaintItem: Does not yet support the Qt Quick software adaptation";
                return true;
            }();
            Q_UNUSED(once)
        }
        break;
    default:
        {
            static auto once = [ri]() {
                qWarning() << "KisCanvasDecorationPaintItem: Unsupported render interface" << ri->graphicsApi();
                return true;
            }();
            Q_UNUSED(once)
        }
        break;
    }
    return node;
}

void KisCanvasDecorationPaintItem::setAnchorsFill(QQuickItem *item)
{
    QQmlProperty(this, "anchors.fill").write(QVariant::fromValue(item));
}

void KisCanvasDecorationPaintItem::setCanvas(KisCanvas2 *canvas)
{
    d->canvas = canvas;
}
