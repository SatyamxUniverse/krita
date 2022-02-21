/*
 * SPDX-FileCopyrightText: 2022 Alvin Wong <alvin@alvinhc.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "KisCanvasToolOutlineOpenGLRenderNode_p.h"

#include <kis_canvas2.h>
#include <KoToolProxy.h>
#include <KoShapeManager.h>

#include <QQuickWindow>
#include <QOpenGLPaintDevice>
#include <QPainter>

KisCanvasToolOutlineOpenGLRenderNode::KisCanvasToolOutlineOpenGLRenderNode(KisCanvas2 *canvas)
    : m_canvas(canvas)
{}

KisCanvasToolOutlineOpenGLRenderNode::~KisCanvasToolOutlineOpenGLRenderNode() {}

QSGRenderNode::StateFlags KisCanvasToolOutlineOpenGLRenderNode::changedStates() const
{
    return DepthState | StencilState | ScissorState | ColorState | BlendState | ViewportState;
}

QSGRenderNode::RenderingFlags KisCanvasToolOutlineOpenGLRenderNode::flags() const
{
    return BoundedRectRendering;
}

QRectF KisCanvasToolOutlineOpenGLRenderNode::rect() const
{
    return QRectF(0, 0, m_size.width(), m_size.height());
}

void KisCanvasToolOutlineOpenGLRenderNode::render(const RenderState *state)
{
    Q_UNUSED(state)

    // TODO: This assumes the QQuickItem fills the whole surface/viewport
    //       and has no additional transformations. Check what happens when
    //       this is not true.
    QOpenGLPaintDevice pd((m_size * m_dpr).toSize());
    pd.setDevicePixelRatio(m_dpr);
    QPainter gc(&pd);

    // This should do the same thing as KisCanvasWidgetBase::drawDecorations

    // XXX: This is safe only because we are rendering QtQuick scenes on the
    //      GUI thread.
    auto coordinatesConverter = m_canvas->coordinatesConverter();

    // HACK: Paint the global shapes together with the tool outline so that
    //       we don't need to make an extra render node for this.
    // Note: According to Dmitry on IRC this probably paints nothing because
    //       it is mostly a fallback object for the Ko-based tools...
    // TODO: Can we remove this?
    gc.setTransform(coordinatesConverter->documentToWidgetTransform());
    // Paint the shapes (other than the layers)
    m_canvas->globalShapeManager()->paint(gc, false);

    gc.end();
    gc.begin(&pd);

    gc.setTransform(coordinatesConverter->flakeToWidgetTransform());
    m_canvas->toolProxy()->paint(gc, *m_canvas->viewConverter());
}

void KisCanvasToolOutlineOpenGLRenderNode::sync(QQuickItem *item)
{
    m_size = item->size().toSize();
    m_dpr = item->window()->effectiveDevicePixelRatio();
}
