/*
 * SPDX-FileCopyrightText: 2008 Cyrille Berger <cberger@cberger.net>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _KIS_CANVAS_DECORATION_H_
#define _KIS_CANVAS_DECORATION_H_

#include <QObject>
#include <QPointer>

#include <kritaui_export.h>
#include <kis_image.h>
#include "KisView.h"
#include <kis_shared.h>

class KisCanvas2;
class QRectF;
class QPainter;
class KisCoordinatesConverter;
class QQuickItem;
class QQmlEngine;

class KisCanvasDecoration;
typedef KisSharedPtr<KisCanvasDecoration> KisCanvasDecorationSP;

/**
 * This class is the base class for object that draw a decoration on the canvas,
 * for instance, selections, grids, tools, ...
 */
class KRITAUI_EXPORT KisCanvasDecoration : public QObject, public KisShared
{
    Q_OBJECT
public:
    KisCanvasDecoration(const QString& id, QPointer<KisView>parent);

    ~KisCanvasDecoration() override;

    void setView(QPointer<KisView> imageView);

    const QString& id() const;

    /**
     * @return whether the decoration is visible.
     */
    bool visible() const;

    /**
     * Will paint the decoration on the QPainter, if the visible is set to @c true.
     *
     * @param gc the painter
     * @param updateRect dirty rect in document pixels
     * @param converter coordinate converter
     * @param canvas the canvas
     */
    void paint(QPainter& gc, const QRectF& updateRect, const KisCoordinatesConverter *converter,KisCanvas2* canvas);

    /**
     * When called for the first time, the decoration shall construct a
     * QQuickItem using the provided engine. Subsequent calls may or may not
     * return the same item, though there is no need for any implementations
     * to construct a new one. The item shall be owned by the decoration.
     *
     * KisCanvasDecoration will set the `visible` property of the item when
     * the visibility of the decoration is changed, but implementations are
     * expected to set the initial visibility of the item.
     *
     * The canvas will add the item to the canvas QtQuick scene. The item
     * can safely assume that its parent item will be a container that fills
     * the whole viewport area for positioning and sizing. The typical approach
     * to arrange this item is to bind the property `anchors.fill` to `parent`.
     *
     * Decorations which overrides this method must also override `getQuickItem()`.
     * Overriding `updateQuickItem()` is optional.
     *
     * The default implementation returns a QQuickItem which paints the
     * decoration using the `paint(...)` method. If for any reason the
     * implementation is unable to provide a QQuickItem, it may either return
     * nullptr or use the base implementation as a fallback.
     *
     * @return A QQuickItem for this decoration, or nullptr.
     */
    virtual QQuickItem *initOrGetQuickItem(QQmlEngine *engine);

    /**
     * @return the QQuickItem associated with this decoration, or nullptr
     * if it has not been constructed.
     */
    virtual QQuickItem *quickItem() const;

    /**
     * Provides an opportunity for the decoration to update its QQuickItem
     * before the scene graph is synchronized and rendered. The decoration
     * may still update the item at anytime outside of this method, and is
     * not required to override this method.
     */
    virtual void updateQuickItem();

    /**
     * Return z-order priority of the decoration. The higher the priority, the higher
     * the decoration is painted.
     */
    int priority() const;

    static bool comparePriority(KisCanvasDecorationSP decoration1, KisCanvasDecorationSP decoration2);

public Q_SLOTS:
    /**
     * Set if the decoration is visible or not.
     */
    virtual void setVisible(bool v);
    /**
     * If decoration is visible, hide it, if not show it.
     */
    void toggleVisibility();
protected:
    virtual void drawDecoration(QPainter& gc, const QRectF& updateArea, const KisCoordinatesConverter *converter,KisCanvas2* canvas) = 0;

    /**
     * @return the parent KisView
     */
    QPointer<KisView> view() const;

    /**
     * Set the priority of the decoration. The higher the priority, the higher
     * the decoration is painted.
     */
    void setPriority(int value);

private:
    struct Private;
    Private* const d;
};

#endif
