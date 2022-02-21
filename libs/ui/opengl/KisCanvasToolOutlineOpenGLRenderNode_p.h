/*
 * SPDX-FileCopyrightText: 2022 Alvin Wong <alvin@alvinhc.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef KIS_CANVAS_TOOL_OUTLINE_OPENGL_RENDER_NODE_P_H
#define KIS_CANVAS_TOOL_OUTLINE_OPENGL_RENDER_NODE_P_H

#include <QQuickItem>
#include <QSGRenderNode>

class KisCanvas2;

class KisCanvasToolOutlineOpenGLRenderNode
    : public QSGRenderNode
{
    friend class KisCanvasToolOutlinePaintItem;

    KisCanvasToolOutlineOpenGLRenderNode(KisCanvas2 *canvas);
    ~KisCanvasToolOutlineOpenGLRenderNode() override;

    StateFlags changedStates() const override;
    RenderingFlags flags() const override;
    QRectF rect() const override;
    void render(const RenderState *state) override;

    void sync(QQuickItem *item);

    KisCanvas2 *m_canvas;
    QSizeF m_size;
    qreal m_dpr;
};

#endif // KIS_CANVAS_TOOL_OUTLINE_OPENGL_RENDER_NODE_P_H
