/*
 * SPDX-FileCopyrightText: 2022 Alvin Wong <alvin@alvinhc.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "KisCanvasToolOutlinePaintItem_p.h"
#include "KisCanvasToolOutlineOpenGLRenderNode_p.h"

#include <QQmlProperty>
#include <QSGRenderNode>
#include <QSGRendererInterface>
#include <QQuickWindow>

class KisCanvas2;

class KisCanvasToolOutlinePaintItem::Private
{
    friend class KisCanvasToolOutlinePaintItem;

    Private(KisCanvas2 *canvas)
        : canvas(canvas)
    {}

    KisCanvas2 *const canvas;
};

KisCanvasToolOutlinePaintItem::KisCanvasToolOutlinePaintItem(QQuickItem *parent, KisCanvas2 *canvas)
    : QQuickItem(parent)
    , d(new Private(canvas))
{
    setFlag(ItemHasContents);
}

KisCanvasToolOutlinePaintItem::~KisCanvasToolOutlinePaintItem()
{
    delete d;
}

QSGNode *KisCanvasToolOutlinePaintItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updatePaintNodeData)
{
    Q_UNUSED(updatePaintNodeData)

    QSGRenderNode *node = static_cast<QSGRenderNode *>(oldNode);

    QSGRendererInterface *ri = window()->rendererInterface();
    if (!ri)
        return nullptr;

    switch (ri->graphicsApi()) {
    case QSGRendererInterface::OpenGL:
        if (!node) {
            node = new KisCanvasToolOutlineOpenGLRenderNode(d->canvas);
        }
        static_cast<KisCanvasToolOutlineOpenGLRenderNode *>(node)->sync(this);
        break;
    case QSGRendererInterface::Software:
        {
            static auto once = []() {
                qWarning() << "KisCanvasToolOutlinePaintItem: Does not yet support the Qt Quick software adaptation";
                return true;
            }();
            Q_UNUSED(once)
        }
        break;
    default:
        {
            static auto once = [ri]() {
                qWarning() << "KisCanvasToolOutlinePaintItem: Unsupported render interface" << ri->graphicsApi();
                return true;
            }();
            Q_UNUSED(once)
        }
        break;
    }
    return node;
}

void KisCanvasToolOutlinePaintItem::setAnchorsFill(QQuickItem *item)
{
    QQmlProperty(this, "anchors.fill").write(QVariant::fromValue(item));
}
