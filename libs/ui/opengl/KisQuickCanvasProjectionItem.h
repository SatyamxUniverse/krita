/*
 * SPDX-FileCopyrightText: 2022 Alvin Wong <alvin@alvinhc.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef KIS_QUICK_CANVAS_PROJECTION_ITEM_H
#define KIS_QUICK_CANVAS_PROJECTION_ITEM_H

#include <QQuickFramebufferObject>

class KisOpenGLCanvasRenderer;

class KisQuickCanvasProjectionItem
    : public QQuickFramebufferObject
{
    Q_OBJECT
    Q_DISABLE_COPY(KisQuickCanvasProjectionItem)

    class Renderer;
    friend class KisQuickWidgetCanvas;

public:
    explicit KisQuickCanvasProjectionItem(QQuickItem *parent = nullptr);
    ~KisQuickCanvasProjectionItem() override;

    QQuickFramebufferObject::Renderer *createRenderer() const override;

private:
    KisOpenGLCanvasRenderer *m_renderer {nullptr};
    QRect m_canvasUpdateRect;
};

#endif // KIS_QUICK_CANVAS_PROJECTION_ITEM_H
