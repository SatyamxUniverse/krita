/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2006-2013 Boudewijn Rempt <boud@valdyas.org>
 * SPDX-FileCopyrightText: 2015 Michael Abrahams <miabraha@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#define GL_GLEXT_PROTOTYPES

#include "opengl/KisQuickWidgetCanvas.h"
#include "opengl/KisOpenGLCanvasRenderer.h"

#include "canvas/kis_canvas2.h"
#include "kis_config.h"
#include "kis_config_notifier.h"
#include "kis_debug.h"

#include <QPointer>
#include "KisOpenGLModeProber.h"

static bool OPENGL_SUCCESS = false;

class KisQuickWidgetCanvas::CanvasBridge
    : public KisOpenGLCanvasRenderer::CanvasBridge
{
    friend class KisQuickWidgetCanvas;
    explicit CanvasBridge(KisQuickWidgetCanvas *canvas)
        : m_canvas(canvas)
    {}
    ~CanvasBridge() override = default;
    Q_DISABLE_COPY(CanvasBridge)
    KisQuickWidgetCanvas *m_canvas;
protected:
    KisCanvas2 *canvas() const override {
        return m_canvas->canvas();
    }
    QOpenGLContext *openglContext() const override {
        return m_canvas->context();
    }
    qreal devicePixelRatioF() const override {
        return m_canvas->devicePixelRatioF();
    }
    KisCoordinatesConverter *coordinatesConverter() const override {
        return m_canvas->coordinatesConverter();
    }
    QColor borderColor() const override {
        return m_canvas->borderColor();
    }
    QWidget *widget() const override {
        return m_canvas;
    }
    void drawDecorations(QPainter &gc, const QRect &updateWidgetRect) const override {
        m_canvas->drawDecorations(gc, updateWidgetRect);
    }
};

struct KisQuickWidgetCanvas::Private
{
public:
    ~Private() {
        delete renderer;
    }

    boost::optional<QRect> updateRect;
    KisOpenGLCanvasRenderer *renderer;
};

KisQuickWidgetCanvas::KisQuickWidgetCanvas(KisCanvas2 *canvas,
                                           KisCoordinatesConverter *coordinatesConverter,
                                           QWidget *parent,
                                           KisImageWSP image,
                                           KisDisplayColorConverter *colorConverter)
    : QOpenGLWidget(parent)
    , KisCanvasWidgetBase(canvas, coordinatesConverter)
    , d(new Private())
{
    KisConfig cfg(false);
    cfg.setCanvasState("OPENGL_STARTED");

    d->renderer = new KisOpenGLCanvasRenderer(new CanvasBridge(this), image, colorConverter);

    connect(d->renderer->openGLImageTextures().data(),
            SIGNAL(sigShowFloatingMessage(QString, int, bool)),
            SLOT(slotShowFloatingMessage(QString, int, bool)));

    setAcceptDrops(true);
    setAutoFillBackground(false);

    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_NoSystemBackground, true);
#ifdef Q_OS_MACOS
    setAttribute(Qt::WA_AcceptTouchEvents, false);
#else
    setAttribute(Qt::WA_AcceptTouchEvents, true);
#endif
    setAttribute(Qt::WA_InputMethodEnabled, false);
    setAttribute(Qt::WA_DontCreateNativeAncestors, true);
    setUpdateBehavior(PartialUpdate);

#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    // we should make sure the texture doesn't have alpha channel,
    // otherwise blending will not work correctly.
    if (KisOpenGLModeProber::instance()->useHDRMode()) {
        setTextureFormat(GL_RGBA16F);
    } else {
        /**
         * When in pure OpenGL mode, the canvas surface will have alpha
         * channel. Therefore, if our canvas blending algorithm produces
         * semi-transparent pixels (and it does), then Krita window itself
         * will become transparent. Which is not good.
         *
         * In Angle mode, GL_RGB8 is not available (and the transparence effect
         * doesn't exist at all).
         */
        if (!KisOpenGL::hasOpenGLES()) {
            setTextureFormat(GL_RGB8);
        }
    }
#endif

    connect(KisConfigNotifier::instance(), SIGNAL(configChanged()), SLOT(slotConfigChanged()));
    connect(KisConfigNotifier::instance(), SIGNAL(pixelGridModeChanged()), SLOT(slotPixelGridModeChanged()));
    slotConfigChanged();
    slotPixelGridModeChanged();
    cfg.writeEntry("canvasState", "OPENGL_SUCCESS");
}

KisQuickWidgetCanvas::~KisQuickWidgetCanvas()
{
    /**
     * Since we delete openGL resources, we should make sure the
     * context is initialized properly before they are deleted.
     * Otherwise resources from some other (current) context may be
     * deleted due to resource id aliasing.
     *
     * The main symptom of resources being deleted from wrong context,
     * the canvas being locked/backened-out after some other document
     * is closed.
     */

    makeCurrent();

    delete d;

    doneCurrent();
}

void KisQuickWidgetCanvas::setDisplayFilter(QSharedPointer<KisDisplayFilter> displayFilter)
{
    d->renderer->setDisplayFilter(displayFilter);
}

void KisQuickWidgetCanvas::notifyImageColorSpaceChanged(const KoColorSpace *cs)
{
    d->renderer->notifyImageColorSpaceChanged(cs);
}

void KisQuickWidgetCanvas::setWrapAroundViewingMode(bool value)
{
    d->renderer->setWrapAroundViewingMode(value);
    update();
}

bool KisQuickWidgetCanvas::wrapAroundViewingMode() const
{
    return d->renderer->wrapAroundViewingMode();
}

void KisQuickWidgetCanvas::initializeGL()
{
    d->renderer->initializeGL();
}

void KisQuickWidgetCanvas::resizeGL(int width, int height)
{
    d->renderer->resizeGL(width, height);
}

void KisQuickWidgetCanvas::paintGL()
{
    const QRect updateRect = d->updateRect ? *d->updateRect : QRect();

    if (!OPENGL_SUCCESS) {
        KisConfig cfg(false);
        cfg.writeEntry("canvasState", "OPENGL_PAINT_STARTED");
    }

    d->renderer->paintGL(updateRect);

    if (!OPENGL_SUCCESS) {
        KisConfig cfg(false);
        cfg.writeEntry("canvasState", "OPENGL_SUCCESS");
        OPENGL_SUCCESS = true;
    }
}

void KisQuickWidgetCanvas::paintEvent(QPaintEvent *e)
{
    KIS_SAFE_ASSERT_RECOVER_RETURN(!d->updateRect);

    d->updateRect = e->rect();
    QOpenGLWidget::paintEvent(e);
    d->updateRect = boost::none;
}

void KisQuickWidgetCanvas::paintToolOutline(const QPainterPath &path)
{
    d->renderer->paintToolOutline(path);
}

bool KisQuickWidgetCanvas::isBusy() const
{
    return d->renderer->isBusy();
}

void KisQuickWidgetCanvas::setLodResetInProgress(bool value)
{
    d->renderer->setLodResetInProgress(value);
}

void KisQuickWidgetCanvas::slotConfigChanged()
{
    d->renderer->updateConfig();

    notifyConfigChanged();
}

void KisQuickWidgetCanvas::slotPixelGridModeChanged()
{
    d->renderer->updatePixelGridMode();

    update();
}

void KisQuickWidgetCanvas::slotShowFloatingMessage(const QString &message, int timeout, bool priority)
{
    canvas()->imageView()->showFloatingMessage(message, QIcon(), timeout, priority ? KisFloatingMessage::High : KisFloatingMessage::Medium);
}

QVariant KisQuickWidgetCanvas::inputMethodQuery(Qt::InputMethodQuery query) const
{
    return processInputMethodQuery(query);
}

void KisQuickWidgetCanvas::inputMethodEvent(QInputMethodEvent *event)
{
    processInputMethodEvent(event);
}

void KisQuickWidgetCanvas::setDisplayColorConverter(KisDisplayColorConverter *colorConverter)
{
    d->renderer->setDisplayColorConverter(colorConverter);
}

void KisQuickWidgetCanvas::channelSelectionChanged(const QBitArray &channelFlags)
{
    d->renderer->channelSelectionChanged(channelFlags);
}


void KisQuickWidgetCanvas::finishResizingImage(qint32 w, qint32 h)
{
    d->renderer->finishResizingImage(w, h);
}

KisUpdateInfoSP KisQuickWidgetCanvas::startUpdateCanvasProjection(const QRect & rc, const QBitArray &channelFlags)
{
    return d->renderer->startUpdateCanvasProjection(rc, channelFlags);
}


QRect KisQuickWidgetCanvas::updateCanvasProjection(KisUpdateInfoSP info)
{
    return d->renderer->updateCanvasProjection(info);
}

QVector<QRect> KisQuickWidgetCanvas::updateCanvasProjection(const QVector<KisUpdateInfoSP> &infoObjects)
{
#if defined(Q_OS_MACOS) || defined(Q_OS_ANDROID)
    /**
     * On OSX openGL different (shared) contexts have different execution queues.
     * It means that the textures uploading and their painting can be easily reordered.
     * To overcome the issue, we should ensure that the textures are uploaded in the
     * same openGL context as the painting is done.
     */

    QOpenGLContext *oldContext = QOpenGLContext::currentContext();
    QSurface *oldSurface = oldContext ? oldContext->surface() : 0;

    this->makeCurrent();
#endif

    QVector<QRect> result = KisCanvasWidgetBase::updateCanvasProjection(infoObjects);

#if defined(Q_OS_MACOS) || defined(Q_OS_ANDROID)
    if (oldContext) {
        oldContext->makeCurrent(oldSurface);
    } else {
        this->doneCurrent();
    }
#endif

    return result;
}

bool KisQuickWidgetCanvas::callFocusNextPrevChild(bool next)
{
    return focusNextPrevChild(next);
}

KisOpenGLImageTexturesSP KisQuickWidgetCanvas::openGLImageTextures() const
{
    return d->renderer->openGLImageTextures();
}
