/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2006-2013 Boudewijn Rempt <boud@valdyas.org>
 * SPDX-FileCopyrightText: 2015 Michael Abrahams <miabraha@gmail.com>
 * SPDX-FileCopyrightText: 2022 Alvin Wong <alvin@alvinhc.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#define GL_GLEXT_PROTOTYPES

#include "opengl/KisQuickWidgetCanvas.h"
#include "opengl/KisOpenGLCanvasRenderer.h"
#include "opengl/KisOpenGLSync.h"
#include "opengl/kis_opengl_canvas_debugger.h"

#include "canvas/kis_canvas2.h"
#include "kis_config.h"
#include "kis_config_notifier.h"
#include "kis_debug.h"
#include "KisWidgetScreenChangeNotifier.h"
#include "KisRepaintDebugger.h"

#include <QPointer>
#include "KisOpenGLModeProber.h"

#include <QQuickRenderControl>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQmlEngine>
#include <QQmlComponent>

static bool OPENGL_SUCCESS = false;

/**
 * The shared QQmlEngine to be used for the canvas. This should only be used
 * when constructing the root component of the canvas. In other cases, get
 * the engine directly via the root QQuickItem instead.
 *
 * TODO: Perhaps make this application-global.
 */
QQmlEngine *sharedQmlEngine()
{
    static QQmlEngine s_engine;
    // TODO: Create a QQmlIncubationController for the engine.
    return &s_engine;
}

class KisQuickWidgetCanvas::RenderControl : public QQuickRenderControl
{
public:
    RenderControl(KisQuickWidgetCanvas *canvas)
        : QQuickRenderControl(canvas)
        , m_canvas(canvas)
    {}
    ~RenderControl() override = default;

    QWindow *renderWindow(QPoint *offset) override {
        if (offset) {
            *offset = m_canvas->mapTo(m_canvas->window(), QPoint());
        }
        return m_canvas->window()->windowHandle();
    }

private:
    KisQuickWidgetCanvas *m_canvas;
};

struct KisQuickWidgetCanvas::Private
{
public:
    Private(KisQuickWidgetCanvas *parent)
        : q(parent)
        , screenChangeNotifier(parent)
    {}
    ~Private() {
        delete rootItem;
        delete component;
        delete renderControl;
        delete offscreenQuickWindow;
        delete renderer;
    }

    KisQuickWidgetCanvas *const q;
    KisOpenGLCanvasRenderer *renderer {nullptr};
    QScopedPointer<KisOpenGLSync> glSyncObject;
    boost::optional<QRect> updateRect;
    KisRepaintDebugger repaintDbg;

    RenderControl *renderControl;
    QQuickWindow *offscreenQuickWindow;
    QQmlComponent *component;
    QQuickItem *rootItem {nullptr};
    QQuickItem *decorationsContainer {nullptr};
    bool quickSceneUpdatePending {true};
    bool blockQuickSceneRenderRequest {false};

    KisWidgetScreenChangeNotifier screenChangeNotifier;

    void readdSceneDecorations();
};

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
};

KisQuickWidgetCanvas::KisQuickWidgetCanvas(KisCanvas2 *canvas,
                                           KisCoordinatesConverter *coordinatesConverter,
                                           QWidget *parent,
                                           KisImageWSP image,
                                           KisDisplayColorConverter *colorConverter)
    : QOpenGLWidget(parent)
    , KisCanvasWidgetBase(canvas, coordinatesConverter)
    , d(new Private(this))
{
    KisConfig cfg(false);
    cfg.setCanvasState("OPENGL_STARTED");

    d->renderControl = new RenderControl(this);
    d->offscreenQuickWindow = new QQuickWindow(d->renderControl);
    QWindow *topWindow = window()->windowHandle();
    if (topWindow) {
        d->offscreenQuickWindow->setScreen(topWindow->screen());
    }
    connect(&d->screenChangeNotifier, SIGNAL(screenChanged(QScreen *)),
            SLOT(slotScreenChanged(QScreen *)));
    d->offscreenQuickWindow->setTitle(QLatin1String("KisQuickWidgetCanvas Offscreen Window"));
    d->offscreenQuickWindow->setObjectName(d->offscreenQuickWindow->title());

    connect(d->renderControl, SIGNAL(renderRequested()), SLOT(slotRenderRequested()));
    connect(d->renderControl, SIGNAL(sceneChanged()), SLOT(slotSceneChanged()));

    d->offscreenQuickWindow->setClearBeforeRendering(false);
    d->offscreenQuickWindow->setPersistentOpenGLContext(true);

    d->component = new QQmlComponent(sharedQmlEngine(), this);
    connect(d->component, SIGNAL(statusChanged(QQmlComponent::Status)),
            SLOT(slotComponentStatusChanged()));
    d->component->loadUrl(QUrl("qrc:/kisqml/canvas/KisQuickWidgetCanvas.qml"), QQmlComponent::PreferSynchronous);

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

    d->renderControl->disconnect(this);
    d->renderControl->invalidate();
    delete d;

    doneCurrent();
}

void KisQuickWidgetCanvas::slotComponentStatusChanged()
{
    if (d->component->isLoading()) {
        return;
    }
    auto printComponetErrors = [this]() {
        const QList<QQmlError> errorList = d->component->errors();
        Q_FOREACH(const QQmlError &error, errorList) {
            qWarning() << "KisQuickWidgetCanvas:" << error.url() << error.line() << error;
        }
    };
    if (d->component->isError()) {
        printComponetErrors();
        return;
    }
    QObject *rootObject = d->component->create();
    if (d->component->isError()) {
        printComponetErrors();
        qWarning() << "KisQuickWidgetCanvas: Trying to use the component anyway...";
    }
    d->rootItem = qobject_cast<QQuickItem *>(rootObject);
    if (!d->rootItem) {
        qWarning() << "KisQuickWidgetCanvas: Failed to get root QQuickItem.";
        delete rootObject;
        return;
    }
    d->rootItem->setParentItem(d->offscreenQuickWindow->contentItem());
    disconnect(d->component, SIGNAL(statusChanged(QQmlComponent::Status)),
               this, SLOT(slotComponentStatusChanged()));

    d->decorationsContainer = d->rootItem->findChild<QQuickItem *>(QLatin1String("canvasDecorationsContainer"));
    if (!d->decorationsContainer) {
        qWarning() << "KisQuickWidgetCanvas: Failed to get canvasDecorationsContainer.";
    }
}

void KisQuickWidgetCanvas::slotRenderRequested()
{
    if (d->quickSceneUpdatePending) {
        return;
    }
    // Note: Although the docs of QQuickRenderControl::renderRequested
    // states that "it is not necessary to call sync()", it seems that
    // GammaRay does not capture the quick scene properly without calling
    // polish() and sync(). Probably safer to do it anyway.
    d->quickSceneUpdatePending = true;
    if (d->blockQuickSceneRenderRequest) {
        return;
    }
    canvas()->updateCanvas();
}

void KisQuickWidgetCanvas::slotSceneChanged()
{
    if (d->quickSceneUpdatePending) {
        return;
    }
    d->quickSceneUpdatePending = true;
    if (d->blockQuickSceneRenderRequest) {
        return;
    }
    canvas()->updateCanvas();
}

void KisQuickWidgetCanvas::slotScreenChanged(QScreen *screen)
{
    d->offscreenQuickWindow->setScreen(screen);
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
    d->renderControl->initialize(context());
    KisOpenGLSync::init(context());
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

    QRect decorationsBoundingRect = coordinatesConverter()->imageRectInWidgetPixels().toAlignedRect();

    // In case the QtQuick Scene requested updates from changes inside paintGL,
    // don't call KisCanvas2::updateCanvas because that will schedule another
    // update in the future. If it updates things like canvas FPS, calling
    // updateCanvas will even cause a self-sustaining render loop.
    //
    // We are going to sync and render the QtQuick scene immediately after
    // this too, so there is no point scheduling updates during paintGL.
    d->blockQuickSceneRenderRequest = true;
    KisOpenglCanvasDebugger::instance()->nofityPaintRequested();
    d->renderer->paintCanvasOnly(updateRect, true);
    {
        QPainter gc(this);
        setDrawDecorationsMask(Shapes);
        drawDecorations(gc, decorationsBoundingRect);
    }
    Q_FOREACH(KisCanvasDecorationSP deco, decorations()) {
        deco->updateQuickItem();
    }
    d->blockQuickSceneRenderRequest = false;

    d->offscreenQuickWindow->resetOpenGLState();

    if (d->quickSceneUpdatePending) {
        d->quickSceneUpdatePending = false;
        d->renderControl->polishItems();
        d->renderControl->sync();
    }
    d->renderControl->render();

    {
        QPainter gc(this);
        setDrawDecorationsMask(ToolOutline);
        drawDecorations(gc, decorationsBoundingRect);
    }

    d->repaintDbg.paint(this, updateRect.isEmpty() ? rect() : updateRect);

    d->glSyncObject.reset(new KisOpenGLSync());

    if (!OPENGL_SUCCESS) {
        KisConfig cfg(false);
        cfg.writeEntry("canvasState", "OPENGL_SUCCESS");
        OPENGL_SUCCESS = true;
    }
}

void KisQuickWidgetCanvas::paintEvent(QPaintEvent *e)
{
    KIS_SAFE_ASSERT_RECOVER_RETURN(!d->updateRect);

    // TODO: Track the modified image rect instead. This should be decoupled
    //       from the QWidget hierarchy.
    d->updateRect = e->rect();
    QOpenGLWidget::paintEvent(e);
    d->updateRect = boost::none;
}

void KisQuickWidgetCanvas::resizeEvent(QResizeEvent *e)
{
    if (d->rootItem) {
        QSize viewportSize = size() * devicePixelRatioF();
        qreal scaledWidth = viewportSize.width() / devicePixelRatioF();
        qreal scaledHeight = viewportSize.height() / devicePixelRatioF();
        d->rootItem->setSize(QSizeF(scaledWidth, scaledHeight));
    }
    d->offscreenQuickWindow->resize(size());

    QOpenGLWidget::resizeEvent(e);
}

void KisQuickWidgetCanvas::paintToolOutline(const QPainterPath &path)
{
    d->renderer->paintToolOutline(path);
}

bool KisQuickWidgetCanvas::isBusy() const
{
    const bool isBusyStatus = d->glSyncObject && !d->glSyncObject->isSignaled();
    KisOpenglCanvasDebugger::instance()->nofitySyncStatus(isBusyStatus);
    return isBusyStatus;
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

void KisQuickWidgetCanvas::addDecoration(KisCanvasDecorationSP deco)
{
    KisCanvasWidgetBase::addDecoration(deco);
    d->readdSceneDecorations();
}

void KisQuickWidgetCanvas::removeDecoration(const QString& id)
{
    KisCanvasWidgetBase::removeDecoration(id);
    d->readdSceneDecorations();
}

void KisQuickWidgetCanvas::setDecorations(const QList<KisCanvasDecorationSP > &decorations)
{
    KisCanvasWidgetBase::setDecorations(decorations);
    d->readdSceneDecorations();
}

void KisQuickWidgetCanvas::Private::readdSceneDecorations()
{
    if (!decorationsContainer) {
        return;
    }
    QQmlEngine *engine = qmlEngine(rootItem);
    QVector<QQuickItem *> decoItems;
    Q_FOREACH(KisCanvasDecorationSP deco, q->decorations()) {
        QQuickItem *item = deco->initOrGetQuickItem(engine);
        if (item) {
            decoItems.append(item);
        } else {
            QString name(deco->objectName());
            if (name.isEmpty()) {
                name = deco->metaObject()->className();
            }
            static QSet<QString> loggedDecos;
            if (!loggedDecos.contains(name)) {
                qWarning() << "Decoration" << name << item << "did not return a QQuickItem!";
                loggedDecos.insert(name);
            }
        }
    }
    Q_FOREACH(QQuickItem *item, decorationsContainer->childItems()) {
        if (!decoItems.contains(item)) {
            item->setParentItem(nullptr);
        }
    }
    Q_FOREACH(QQuickItem *item, decoItems) {
        item->setParentItem(decorationsContainer);
    }
    for (int i = 1; i < decoItems.size(); i++) {
        decoItems[i]->stackAfter(decoItems[i - 1]);
    }
}

KisOpenGLImageTexturesSP KisQuickWidgetCanvas::openGLImageTextures() const
{
    return d->renderer->openGLImageTextures();
}
