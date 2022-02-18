/*
 * SPDX-FileCopyrightText: 2008 Cyrille Berger <cberger@cberger.net>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_canvas_decoration.h"
#include "KisCanvasDecorationPaintItem_p.h"
#include "kis_canvas2.h"
#include "kis_debug.h"

#include <QQmlEngine>

struct KisCanvasDecoration::Private {
    bool visible;
    QPointer<KisView> view;
    QString id;
    int priority = 0;
    KisCanvasDecorationPaintItem *paintQuickItem {nullptr};
};

KisCanvasDecoration::KisCanvasDecoration(const QString& id, QPointer<KisView>parent)
    : QObject(parent)
    , d(new Private)
{
    d->visible = false;
    d->view = parent;
    d->id = id;
}

KisCanvasDecoration::~KisCanvasDecoration()
{
    delete d;
}

void KisCanvasDecoration::setView(QPointer<KisView>imageView)
{
    d->view = imageView;
}


const QString& KisCanvasDecoration::id() const
{
    return d->id;
}

void KisCanvasDecoration::setVisible(bool v)
{
    if (d->visible == v) {
        return;
    }
    d->visible = v;
    if (quickItem()) {
        quickItem()->setVisible(v);
    }
    if (d->view &&
            d->view->canvasBase()) {

        d->view->canvasBase()->updateCanvas();
    }
}

bool KisCanvasDecoration::visible() const
{
    return d->visible;
}

void KisCanvasDecoration::toggleVisibility()
{
    setVisible(!visible());
}

void KisCanvasDecoration::paint(QPainter& gc, const QRectF& updateArea, const KisCoordinatesConverter *converter, KisCanvas2 *canvas = 0)
{
    KIS_SAFE_ASSERT_RECOVER_RETURN(visible());

    if (!canvas) {
        dbgFile<<"canvas does not exist:"<<canvas;
    }

    drawDecoration(gc, updateArea, converter,canvas);
}

QQuickItem *KisCanvasDecoration::initOrGetQuickItem(QQmlEngine *engine)
{
    if (d->paintQuickItem) {
        return d->paintQuickItem;
    }

    d->paintQuickItem = new KisCanvasDecorationPaintItem(this);
    engine->setContextForObject(d->paintQuickItem, engine->rootContext());
    connect(d->paintQuickItem, &KisCanvasDecorationPaintItem::parentChanged,
            d->paintQuickItem, &KisCanvasDecorationPaintItem::setAnchorsFill);
    d->paintQuickItem->setVisible(d->visible);
    return d->paintQuickItem;
}

QQuickItem *KisCanvasDecoration::quickItem() const
{
    return d->paintQuickItem;
}

void KisCanvasDecoration::updateQuickItem()
{
    if (!d->paintQuickItem) {
        return;
    }
    d->paintQuickItem->setCanvas(d->view ? d->view->canvasBase() : nullptr);
    if (d->paintQuickItem->isVisible()) {
        d->paintQuickItem->update();
    }
}

int KisCanvasDecoration::priority() const
{
    return d->priority;
}

void KisCanvasDecoration::setPriority(int value)
{
    d->priority = value;
}

bool KisCanvasDecoration::comparePriority(KisCanvasDecorationSP decoration1, KisCanvasDecorationSP decoration2)
{
    return decoration1->priority() < decoration2->priority();
}

QPointer<KisView>KisCanvasDecoration::view() const
{
    return d->view;
}
