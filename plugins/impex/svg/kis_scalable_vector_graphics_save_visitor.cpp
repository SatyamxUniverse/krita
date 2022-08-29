#include "kis_scalable_vector_graphics_save_visitor.h"
#include <math.h>

#include <QDomElement>
#include <QImage>

#include <KoCompositeOpRegistry.h>

#include "kis_adjustment_layer.h"
#include "filter/kis_filter.h"
#include "filter/kis_filter_configuration.h"
#include "kis_group_layer.h"
#include "kis_paint_layer.h"
#include <generator/kis_generator_layer.h>
#include <kis_scalable_vector_graphics_save_context.h>
#include <kis_clone_layer.h>
#include <kis_external_layer_iface.h>
#include <QTextStream>

struct KisScalableVectorGraphicsSaveVisitor::Private {
    Private() {}
    KisScalableVectorGraphicsSaveContext* saveContext {nullptr};
    QDomDocument layerStack;
    QDomElement currentElement;
    vKisNodeSP activeNodes;
    QTextStream* saveDevice;
};


KisScalableVectorGraphicsSaveVisitor::KisScalableVectorGraphicsSaveVisitor(QIODevice* saveDevice, vKisNodeSP activeNodes)
    : d(new Private)
{
    QTextStream svgStream(saveDevice);
    svgStream.setCodec("UTF-8");
    d->saveDevice = &svgStream;
    d->activeNodes = activeNodes;
    qDebug() << "create KisScalableVectorGraphicsSaveVisitor";
}

KisScalableVectorGraphicsSaveVisitor::~KisScalableVectorGraphicsSaveVisitor()
{
    delete d;
}

bool KisScalableVectorGraphicsSaveVisitor::visit(KisPaintLayer *layer)
{
    qDebug() << "visit paint layer";
    return saveLayer(layer);
}

bool KisScalableVectorGraphicsSaveVisitor::visit(KisGroupLayer *layer)
{
    qDebug() << "visit KisGroupLayer";
    visitAll(layer);
    return true;
}

bool KisScalableVectorGraphicsSaveVisitor::visit(KisAdjustmentLayer *layer)
{
    qDebug() << "visit KisAdjustmentLayer";

    return true;
}

bool KisScalableVectorGraphicsSaveVisitor::visit(KisExternalLayer *layer)
{
    qDebug() << "visit KisExternalLayer";
    return true;
}

bool KisScalableVectorGraphicsSaveVisitor::visit(KisGeneratorLayer *layer)
{
    qDebug() << "visit KisGeneratorLayer";
    return true;
}

bool KisScalableVectorGraphicsSaveVisitor::visit(KisCloneLayer *layer)
{
    qDebug() << "visit group layer";
    return true;
}

bool KisScalableVectorGraphicsSaveVisitor::saveLayer(KisLayer *layer)
{
    qDebug() << "save layer";
    if (layer->isFakeNode()) {
        // don't save grids, reference images layers etc.
        return true;
    }

    // here we adjust the bounds to encompass the entire area of the layer, including transforms
    QRect adjustedBounds = layer->exactBounds();

    if (adjustedBounds.isEmpty()) {
        // in case of an empty layer, artificially increase the size of the saved rectangle
        // to just save an empty layer file
        adjustedBounds.adjust(0, 0, 1, 1);
    }

    //QString filename = d->saveContext->saveDeviceData(layer->projection(), layer->metaData(), adjustedBounds, layer->image()->xRes(), layer->image()->yRes());

    QDomElement elt = d->layerStack.createElement("layer");
    saveLayerInfo(elt, layer);
    //elt.setAttribute("src", filename);
    d->currentElement.insertBefore(elt, QDomNode());
    d->saveDevice << layer->name() << endl;
    return true;
}

void KisScalableVectorGraphicsSaveVisitor::saveLayerInfo(QDomElement& elt, KisLayer* layer)
{
    qDebug() << "void KisScalableVectorGraphicsSaveVisitor::saveLayerInfo(QDomElement& elt, KisLayer* layer)";
}