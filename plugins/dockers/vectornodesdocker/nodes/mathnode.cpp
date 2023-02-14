#include "mathnode.h"

MathNode::MathNode(QGraphicsScene *scene, qreal x, qreal y) : EditorNode(scene, x, y) {
    type = EditorNodes::Type::Math;
    nodeItem->SetupFromType(static_cast<int>(type));
    nodeItem->SetTitle("Math");
/*
 * description :
 * TODO : combo box to select operation
 * types
*/
}
MathNode::~MathNode() {}

void MathNode::Update() {
}
