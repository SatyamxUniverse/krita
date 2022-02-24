/*
 * SPDX-FileCopyrightText: 2022 Alvin Wong <alvin@alvinhc.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "KisCanvasDecorationOpenGLRenderNode_p.h"

#include "KisCanvasDecorationPaintItem_p.h"
#include "kis_canvas2.h"
#include "kis_coordinates_converter.h"

#include <QQuickWindow>
#include <QOpenGLPaintDevice>
#include <QPainter>

KisCanvasDecorationOpenGLRenderNode::KisCanvasDecorationOpenGLRenderNode(KisCanvasDecoration *decoration)
    : m_decoration(decoration)
{}

KisCanvasDecorationOpenGLRenderNode::~KisCanvasDecorationOpenGLRenderNode() {}

QSGRenderNode::StateFlags KisCanvasDecorationOpenGLRenderNode::changedStates() const
{
    return DepthState | StencilState | ScissorState | ColorState | BlendState | ViewportState;
}

QSGRenderNode::RenderingFlags KisCanvasDecorationOpenGLRenderNode::flags() const
{
    return BoundedRectRendering;
}

QRectF KisCanvasDecorationOpenGLRenderNode::rect() const
{
    return QRectF(0, 0, m_size.width(), m_size.height());
}

void KisCanvasDecorationOpenGLRenderNode::render(const RenderState *state)
{
    Q_UNUSED(state)

    if (!m_canvas) {
        return;
    }

    // TODO: This assumes the QQuickItem fills the whole surface/viewport
    //       and has no additional transformations. Check what happens when
    //       this is not true.
    QOpenGLPaintDevice pd((m_size * m_dpr).toSize());
    pd.setDevicePixelRatio(m_dpr);
    QPainter painter(&pd);
    // XXX: This is safe only because we are rendering QtQuick scenes on the
    //      GUI thread.
    auto coordinatesConverter = m_canvas->coordinatesConverter();
    QRectF docRect(coordinatesConverter->widgetToDocument(QRectF(QPointF(), m_size)));
    m_decoration->paint(painter, docRect, coordinatesConverter, m_canvas);

    painter.end();
}

void KisCanvasDecorationOpenGLRenderNode::sync(QQuickItem *item, KisCanvas2 *canvas)
{
    m_canvas = canvas;
    m_size = item->size();
    m_dpr = item->window()->effectiveDevicePixelRatio();
}
