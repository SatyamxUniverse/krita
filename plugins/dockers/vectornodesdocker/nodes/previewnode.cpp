#include "previewnode.h"

#include "layernode.h"
#include "kis_shape_layer.h"

//#include "../nodestyles.h"

PreviewNode::PreviewNode(QGraphicsScene *scene, qreal x, qreal y) : EditorNode(scene, x, y) {
    type = EditorNodes::Type::Preview;
    nodeItem->SetupFromType(static_cast<int>(type));
    nodeItem->SetTitle("Preview");
    // Init
    times = 0;
    // Inputs
    inputIO = AddInput();
    inputIO->SetVSpacing(26);
    // Display
    imageItem = new QGraphicsPixmapItem();
    imageItem->setPos(nodeItem->scenePos().x() + 15, nodeItem->scenePos().y() + 15);
    nodeItem->addToGroup(imageItem);

    textItem = new QGraphicsTextItem();
    textItem->setPos(nodeItem->scenePos().x(), nodeItem->scenePos().y() + NodeStyles::Style::NodeHeaderHeight);
    textItem->setAcceptHoverEvents(false);
    nodeItem->addToGroup(textItem);
}
PreviewNode::~PreviewNode() {}

void PreviewNode::Update() {
    imageItem->hide();
    inputIO->SetVSpacing(26);
    nodeItem->resizeBody();

    if(inputNodes.size() < 1) {
        textItem->setPlainText(QString("No input."));
        return;
    }
    if(inputNodes.first()->inherits("LayerNode")) {
        LayerNode * layerNode = qobject_cast<LayerNode *>(inputNodes.first());
        KisNodeSP node = layerNode->value;
        if(!node) {
            textItem->setPlainText(QString("No input value set."));
            return;
        }
        KisShapeLayer *shapeLayer = qobject_cast<KisShapeLayer*>(node.data());
        if(!shapeLayer) {
            textItem->setPlainText(QString("Not a vector layer."));
            return;
        }
        textItem->setPlainText(QString(""));
        imageItem->show();
        inputIO->SetVSpacing(130);
        nodeItem->resizeBody();
        QImage thumbnail = shapeLayer->createThumbnail(130, 130, Qt::KeepAspectRatio);
        imageItem->setPixmap(QPixmap::fromImage(thumbnail));
    } else if(inputNodes.first()->inherits("ShapeNode"))  {
        //
    } else {
        textItem->setPlainText(QString("Incorrect input type."));
        return;
    }
}
