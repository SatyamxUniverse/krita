#include "colornode.h"

ColorNode::ColorNode(QGraphicsScene *scene, qreal x, qreal y) : EditorNode(scene, x, y) {
    type = EditorNodes::Type::Color;
    nodeItem->SetupFromType(static_cast<int>(type));
    nodeItem->SetTitle("Color");
    AddOutput();
}
ColorNode::~ColorNode() {}

void ColorNode::Update() {}
