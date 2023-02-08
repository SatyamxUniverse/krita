#include "getshapesnode.h"

#include <QList>

#include "layernode.h"
#include "valuenode.h"

#include "kis_shape_layer.h"
#include "kis_image.h"
#include <KisDocument.h>
#include <KoProperties.h>
#include <KoShapeManager.h>
#include <KoShape.h>
#include <KoShapeGroup.h>
#include <KoShapeControllerBase.h>

GetShapesNode::GetShapesNode(QGraphicsScene *scene, qreal x, qreal y) : EditorNode(scene, x, y) {
    type = EditorNodes::Type::GetShapes;
    nodeItem->SetupFromType(static_cast<int>(type));
    nodeItem->SetTitle("Get Shapes");
    value = nullptr;
    // Inputs
    geometryInput = AddInput();
    selectionInput = AddInput();
    // Outputs
    outputIO = AddOutput();
    // Display
    imageItem = new QGraphicsPixmapItem();
    imageItem->setPos(nodeItem->scenePos().x() + 15, nodeItem->scenePos().y() + 15);
    nodeItem->addToGroup(imageItem);
    imageItem->hide();// TODO

    textItem = new QGraphicsTextItem();
    textItem->setPos(nodeItem->scenePos().x() + 10, nodeItem->scenePos().y() + NodeStyles::Style::NodeHeaderHeight);
    textItem->setAcceptHoverEvents(false);
    nodeItem->addToGroup(textItem);
}
GetShapesNode::~GetShapesNode() {}

void GetShapesNode::Update() {
    outputIO->SetVSpacing(26);
    nodeItem->resizeBody();

    if(inputNodes.size() < 2) {
        textItem->setPlainText(QString("No input."));
        return;
    }
    KisShapeLayer *shapeLayer = nullptr;
    bool canDisplay = true;
    if(inputNodes.first()->inherits("LayerNode")) {
        LayerNode *layerNode = qobject_cast<LayerNode *>(inputNodes.first());
        KisNodeSP node = layerNode->value;
        if(!node)
            canDisplay = false;
        shapeLayer = qobject_cast<KisShapeLayer *>(node.data());
        if(!shapeLayer)
            canDisplay = false;
    }
    if(inputNodes.last()->inherits("ValueNode")) {
        ValueNode *valueNode = qobject_cast<ValueNode *>(inputNodes.last());
        if(valueNode->value[2] == 0) { // Single element
            int index = valueNode->value[0];
        } else if(valueNode->value[2] == 1) { // Range of elements
            int min = valueNode->value[0];
            int max = valueNode->value[1];
            if(canDisplay) {
                KoShapeControllerBase *shapeController = nullptr;
                value = new KisShapeLayer(shapeController, shapeLayer->image(), QString("name"), 255);
//                value = dynamic_cast<KisShapeLayer *>(shapeLayer->clone().data());
                ///////////////////////////////////////////////
                const QTransform invertedTransform = value->absoluteTransformation().inverted();
                value->shapeManager()->setUpdatesBlocked(true);
//                int i = 0;
//                Q_FOREACH (KoShape *shape, value->shapes()) {
//                    if(i < min || i > max) {
//                        value->removeShape(shape);
//                    }
//                    i++;
//                }
                int i = 0;
                Q_FOREACH (KoShape *shape, shapeLayer->shapes()) {
                    if(i >= min && i <= max) {
                        KoShape *clonedShape = shape->cloneShape();
                        KIS_SAFE_ASSERT_RECOVER(clonedShape) { continue; }
                        clonedShape->setTransformation(shape->absoluteTransformation() * invertedTransform);
                        value->addShape(clonedShape);
                        clonedShape->update();
                        value->shapeManager()->notifyShapeChanged(clonedShape);
                    }
                    i++;
                }
                value->shapeManager()->setUpdatesBlocked(false);
                value->setDirty();
                value->image()->waitForDone();
                value->forceUpdateTimedNode();
                value->image()->waitForDone();
                ///////////////////////////////////////////////
                textItem->setPlainText(QString("Num shapes %1").arg(value->shapeCount()));
                textItem->setPlainText(QString("x: %1 y: %2 w: %3 h: %4 shapes: %5")
                                       .arg(value->absoluteOutlineRect().x())
                                       .arg(value->absoluteOutlineRect().y())
                                       .arg(value->absoluteOutlineRect().width())
                                       .arg(value->absoluteOutlineRect().height())
                                       .arg(value->shapeCount())
                                       );

//                textItem->setPlainText(QString(""));
                imageItem->show();
                outputIO->SetVSpacing(130);
                nodeItem->resizeBody();
                QImage thumbnail = value->createThumbnail(130, 130, Qt::KeepAspectRatio);
                imageItem->setPixmap(QPixmap::fromImage(thumbnail));
            } else {
                QVector<int> range = valueNode->value;
                textItem->setPlainText(QString("min: %1 max: %2 sel: %3").arg(range.at(0)).arg(range.at(1)).arg(range.at(2)));
            }
        }
//        if(canDisplay) {
//            textItem->setPlainText(QString(""));
//            imageItem->show();
//            outputIO->SetVSpacing(130);
//            nodeItem->resizeBody();
//            QImage thumbnail = shapeLayer->createThumbnail(130, 130, Qt::KeepAspectRatio);
//            imageItem->setPixmap(QPixmap::fromImage(thumbnail));
//        }
    } else {
        textItem->setPlainText(QString("Incorrect input type."));
        return;
    }
}
