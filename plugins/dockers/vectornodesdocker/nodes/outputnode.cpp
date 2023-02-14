#include "outputnode.h"

OutputNode::OutputNode(QGraphicsScene *scene, qreal x, qreal y) : EditorNode(scene, x, y) {
    type = EditorNodes::Type::Output;
    nodeItem->SetupFromType(static_cast<int>(type));
    nodeItem->SetTitle("Output");
/*
 * description :
 * TODO : combo box with create or select layer output
 * clear and rebuild layer on update
*/
}
OutputNode::~OutputNode() {}

void OutputNode::Update() {
}
