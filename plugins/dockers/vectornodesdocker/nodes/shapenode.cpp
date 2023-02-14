#include "shapenode.h"

ShapeNode::ShapeNode(QGraphicsScene *scene, qreal x, qreal y) : EditorNode(scene, x, y) {
    type = EditorNodes::Type::Shape;
    nodeItem->SetupFromType(static_cast<int>(type));
    nodeItem->SetTitle("Shape");
/*
 * description :
 * TODO : combo box w primitive or library(future task)
*/
}
ShapeNode::~ShapeNode() {}

void ShapeNode::Update() {
}
