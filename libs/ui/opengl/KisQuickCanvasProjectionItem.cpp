/*
 * SPDX-FileCopyrightText: 2022 Alvin Wong <alvin@alvinhc.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "opengl/KisQuickCanvasProjectionItem.h"
#include "opengl/KisOpenGLCanvasRenderer.h"
#include "opengl/KisOpenGLModeProber.h"

#include <QOpenGLFramebufferObject>
#include <QOpenGLFramebufferObjectFormat>
#include <QQuickWindow>


class KisQuickCanvasProjectionItem::Renderer
    : public QQuickFramebufferObject::Renderer
{
    friend class KisQuickCanvasProjectionItem;

    explicit Renderer(KisOpenGLCanvasRenderer *renderer);
    ~Renderer() override;

    Q_DISABLE_COPY(Renderer)

protected:
    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override;
    void render() override;
    void synchronize(QQuickFramebufferObject *item) override;

private:
    QQuickWindow *m_window {nullptr};
    KisOpenGLCanvasRenderer *m_renderer {nullptr};
    QRect m_canvasUpdateRect;
    QSize m_size;
    qreal m_dpr;
};


KisQuickCanvasProjectionItem::KisQuickCanvasProjectionItem(QQuickItem *parent)
    : QQuickFramebufferObject(parent)
{
    setMirrorVertically(true);
}

KisQuickCanvasProjectionItem::~KisQuickCanvasProjectionItem()
{}

QQuickFramebufferObject::Renderer *KisQuickCanvasProjectionItem::createRenderer() const
{
    return new Renderer(m_renderer);
}

// ---

KisQuickCanvasProjectionItem::Renderer::Renderer(KisOpenGLCanvasRenderer *renderer)
    : m_renderer(renderer)
{}

KisQuickCanvasProjectionItem::Renderer::~Renderer()
{}

QOpenGLFramebufferObject *KisQuickCanvasProjectionItem::Renderer::createFramebufferObject(const QSize &size)
{
    QOpenGLFramebufferObjectFormat format;
    if (KisOpenGLModeProber::instance()->useHDRMode()) {
        format.setInternalTextureFormat(GL_RGBA16F);
    }
    auto newFBO = new QOpenGLFramebufferObject(size, format);
    m_renderer->resizeWithFBO(m_size.width(), m_size.height(), newFBO);
    return newFBO;
}

void KisQuickCanvasProjectionItem::Renderer::render()
{
    m_renderer->paintCanvasOnly(m_canvasUpdateRect);
    // XXX: m_window may already be deleted if the QtQuick renderer is multithreaded
    m_window->resetOpenGLState();
}

void KisQuickCanvasProjectionItem::Renderer::synchronize(QQuickFramebufferObject *item_)
{
    KisQuickCanvasProjectionItem *item = static_cast<KisQuickCanvasProjectionItem *>(item_);
    m_window = item->window();
    m_size = item->size().toSize();
    m_dpr = item->window()->effectiveDevicePixelRatio();
    m_canvasUpdateRect = item->m_canvasUpdateRect;
    item->m_canvasUpdateRect = QRect();
}
