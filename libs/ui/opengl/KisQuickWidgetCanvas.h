/*
 * SPDX-FileCopyrightText: 2006 Boudewijn Rempt <boud@valdyas.org>
 * SPDX-FileCopyrightText: 2015 Michael Abrahams <miabraha@gmail.com>
 * SPDX-FileCopyrightText: 2022 Alvin Wong <alvin@alvinhc.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_QUICK_WIDGET_CANVAS_H
#define KIS_QUICK_WIDGET_CANVAS_H

#include <QOpenGLWidget>
#include "canvas/kis_canvas_widget_base.h"
#include "opengl/kis_opengl_image_textures.h"

#include "kritaui_export.h"
#include "kis_ui_types.h"

class KisCanvas2;
class KisDisplayColorConverter;
class QOpenGLShaderProgram;
class QPainterPath;

/**
 * KisQuickWidgetCanvas is KisOpenGLCanvas but with the ability to overlay a Qt Quick 2 scene.
 *
 * NOTE: if you change something in the event handling here, also change it
 * in the qpainter canvas.
 *
 */
class KRITAUI_EXPORT KisQuickWidgetCanvas
        : public QOpenGLWidget
        , public KisCanvasWidgetBase
{
    Q_OBJECT

public:

    KisQuickWidgetCanvas(KisCanvas2 *canvas, KisCoordinatesConverter *coordinatesConverter, QWidget *parent, KisImageWSP image, KisDisplayColorConverter *colorConverter);

    ~KisQuickWidgetCanvas() override;

private Q_SLOTS:
    void slotComponentStatusChanged();
    void slotRenderRequested();
    void slotSceneChanged();

public: // QOpenGLWidget
    void resizeGL(int width, int height) override;
    void initializeGL() override;
    void paintGL() override;

public: // QWidget
    void paintEvent(QPaintEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;

    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;
    void inputMethodEvent(QInputMethodEvent *event) override;

public:
    void paintToolOutline(const QPainterPath &path);


public: // Implement kis_abstract_canvas_widget interface
    void setDisplayFilter(QSharedPointer<KisDisplayFilter> displayFilter) override;
    void notifyImageColorSpaceChanged(const KoColorSpace *cs) override;

    void setWrapAroundViewingMode(bool value) override;
    bool wrapAroundViewingMode() const override;

    void channelSelectionChanged(const QBitArray &channelFlags) override;
    void setDisplayColorConverter(KisDisplayColorConverter *colorConverter) override;
    void finishResizingImage(qint32 w, qint32 h) override;
    KisUpdateInfoSP startUpdateCanvasProjection(const QRect & rc, const QBitArray &channelFlags) override;
    QRect updateCanvasProjection(KisUpdateInfoSP info) override;
    QVector<QRect> updateCanvasProjection(const QVector<KisUpdateInfoSP> &infoObjects) override;

    QWidget *widget() override {
        return this;
    }

    bool isBusy() const override;
    void setLodResetInProgress(bool value) override;

    KisOpenGLImageTexturesSP openGLImageTextures() const;

public Q_SLOTS:
    void slotConfigChanged();
    void slotPixelGridModeChanged();

private Q_SLOTS:
    void slotShowFloatingMessage(const QString &message, int timeout, bool priority);

protected: // KisCanvasWidgetBase
    bool callFocusNextPrevChild(bool next) override;

private:
    struct Private;
    Private * const d;

    class CanvasBridge;
    class RenderControl;
};

#endif // KIS_QUICK_WIDGET_CANVAS_H
