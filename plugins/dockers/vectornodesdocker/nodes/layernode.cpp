#include "layernode.h"

#include <QList>
#include <QGraphicsProxyWidget>

#include "kis_types.h"
#include "kis_shape_layer.h"
#include "kis_canvas2.h"
#include "KisDocument.h"
#include <kis_layer.h>
#include <kis_group_layer.h>
#include <KoProperties.h>

#include "../nodescene.h"

LayerNode::LayerNode(QGraphicsScene *scene, qreal x, qreal y) : EditorNode(scene, x, y) {
    type = EditorNodes::Type::Layer;
    nodeItem->SetupFromType(static_cast<int>(type));
    nodeItem->SetTitle("Layer");
    value = nullptr;

    IOElement *outputIO = AddOutput();
    outputIO->SetVSpacing(40);
    nodeItem->resizeBody();

    spinner = new QComboBox();
    spinner->addItem("Select to refresh");

    QGraphicsProxyWidget *spinProxy = new QGraphicsProxyWidget(nodeItem.data());
    spinProxy->setWidget(spinner);
    spinProxy->setGeometry(QRectF(15, NodeStyles::Style::NodeMarginV, 130, 40));
    connect(spinner, SIGNAL(currentIndexChanged(int)), this, SLOT(selectLayer(int)));
    nodeItem->addToGroup(spinProxy);

    updateLayers();
}
LayerNode::~LayerNode() {}

void LayerNode::selectLayer(int index) {
    NodeScene *scene = dynamic_cast<NodeScene*>(nodeItem.data()->scene()); // TODO : Check cleanup
    QPointer<KisCanvas2> mainCanvas = dynamic_cast<KisCanvas2*>(scene->Canvas()); // TODO : Check cleanup
    if(mainCanvas) {
        QString name = spinner->itemText(index);
        value = findLayer(mainCanvas.data()->currentImage().data()->rootLayer(), name);
        DownstreamUpdate();
    } else {
        value = nullptr;
        DownstreamUpdate();
    }
}

void LayerNode::updateLayers() {
    NodeScene *scene = dynamic_cast<NodeScene*>(nodeItem.data()->scene()); // TODO : Check cleanup
    QPointer<KisCanvas2> mainCanvas = dynamic_cast<KisCanvas2*>(scene->Canvas()); // TODO : Check cleanup
    if (mainCanvas) {
        spinner->clear();
        QList<KisNodeSP> layers = getLayers(mainCanvas.data()->currentImage().data()->rootLayer());
        for(int i = 0; i < layers.length(); i++) {
            spinner->addItem(layers[i].data()->objectName());
        }
    }
}

void LayerNode::Update() {}

QList<KisNodeSP> LayerNode::getLayers(KisNodeSP layer) {
    QList<KisNodeSP> shapeLayers = layer.data()->childNodes(QStringList("KisShapeLayer"), KoProperties());
    QList<KisNodeSP> groupLayers = layer.data()->childNodes(QStringList("KisGroupLayer"), KoProperties());
    for(int i = 0; i < groupLayers.size(); i++) {
        QList<KisNodeSP> children = getLayers(groupLayers[i]);
        for(int j = children.size()-1; j >= 0; j--)
            shapeLayers.append(children[j]);
    }
    return shapeLayers;
}
KisNodeSP LayerNode::findLayer(KisNodeSP layer, QString name) {
    KisNodeSP found = nullptr;
    for(int i = 0; i < layer.data()->childCount(); i++) {
        KisNodeSP child = layer.data()->childNodes(QStringList(), KoProperties())[i];
        if(child.data()->name() == name)
            return child;
        else
            found = findLayer(child, name);
        // TODO : Deref both
    }
    return found;
}
