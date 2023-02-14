#include "nodeitem.h"

#include <QBrush>
#include <QPen>
#include <QDebug>

#include "nodestyles.h"
#include "nodes/editornode.h"

NodeItem::NodeItem(EditorNode *nodeParent, qreal x, qreal y, int w, int h) :
    bodyRect(new QGraphicsRectItem(0, 0, w, h)),
    headerRect(new QGraphicsRectItem(0, -NodeStyles::Style::NodeHeaderHeight, w, NodeStyles::Style::NodeHeaderHeight)),
    outlineRect(new NodeOutlineRectItem(0, -NodeStyles::Style::NodeHeaderHeight, w, h + NodeStyles::Style::NodeHeaderHeight, this))
{
    node = nodeParent;
    QBrush brush = QBrush(Qt::SolidPattern);
    // Body
    brush.setColor(NodeStyles::Color::NodeFill_Normie);
    bodyRect->setBrush(brush);
    bodyRect->setPen(Qt::NoPen);
    addToGroup(bodyRect);
    // Header
    addToGroup(headerRect);
    titleText = new QGraphicsTextItem();
    titleText->setPos(0, -NodeStyles::Style::NodeHeaderHeight-4);
    titleText->setAcceptHoverEvents(false);
    addToGroup(titleText);
    // This
    setFlags(QGraphicsItem::GraphicsItemFlag::ItemIsMovable);
    setAcceptHoverEvents(true);
    setHandlesChildEvents(false);
    setPos(x, y);
}
NodeItem::~NodeItem() {
//    inputs.clear();
//    outputs.clear();
}

void NodeItem::mousePressEvent(QGraphicsSceneMouseEvent *event) { node->OnClick(event); }

void NodeItem::SetTitle(QString text) { titleText->setPlainText(text); }

void NodeItem::AddInput(IOElement *input) {
    addToGroup(input);
    inputs.push_back(input);
    resizeBody();
}
void NodeItem::AddOutput(IOElement *output) {
    addToGroup(output);
    outputs.push_back(output);
    resizeBody();
}

void NodeItem::SetupFromType(int type) {
    QBrush brush = QBrush(Qt::SolidPattern);
    switch (type) {
    case 0:// Empty
    case 3:// Layer
    case 6:// Output
        brush.setColor(NodeStyles::Color::HeaderIO_Normie);
        break;
    case 8:// Shape
    case 2:// GetShapes
        brush.setColor(NodeStyles::Color::HeaderGeometry_Normie);
        break;
    case 4:// Math
    case 5:// Noise
        brush.setColor(NodeStyles::Color::HeaderAdjust_Normie);
        break;
    case 1:// Color
        brush.setColor(NodeStyles::Color::HeaderColor_Normie);
        break;
    case 7:// Preview
    case 9:// Value
        brush.setColor(NodeStyles::Color::HeaderValue_Normie);
        break;
    default:
        break;
    }
    headerRect->setBrush(brush);
    headerRect->setPen(Qt::NoPen);
}

void NodeItem::resizeBody() {
    int w = outlineRect->rect().width();
    int inputsHeight = NodeStyles::Style::NodeMarginV;
    for(int i = 0; i < inputs.size(); i++) {
        int space = inputs[i]->VSpacing();
        inputs[i]->setRect(0, 0, 12, 12);
        inputs[i]->setPos(-6, inputsHeight + space/2 - 6);
        inputsHeight += space;
    }
    int outputsHeight = NodeStyles::Style::NodeMarginV;
    for(int i = 0; i < outputs.size(); i++) {
        int space = outputs[i]->VSpacing();
        outputs[i]->setRect(0, 0, 12, 12);
        outputs[i]->setPos(w - 6, outputsHeight + space/2 - 6);
        outputsHeight += space;
    }

    int h = (outputsHeight > inputsHeight)? outputsHeight : inputsHeight;
    h += NodeStyles::Style::NodeMarginV;
    bodyRect->setRect(0, 0, w, h);
    headerRect->setRect(0, -NodeStyles::Style::NodeHeaderHeight, w, NodeStyles::Style::NodeHeaderHeight);
    outlineRect->setRect(0, -NodeStyles::Style::NodeHeaderHeight, w, h + NodeStyles::Style::NodeHeaderHeight);
    // Note : Necessary for resizing
    QGraphicsRectItem *rect = new QGraphicsRectItem(0, -NodeStyles::Style::NodeHeaderHeight, w, h + NodeStyles::Style::NodeHeaderHeight);
    addToGroup(rect);
    removeFromGroup(rect);
    delete rect;
}

void NodeItem::ApplyStyle(int style) {
    qDebug() << QString("From Node Item: %1").arg(style);
    switch (style) {
    case 0:
        break;
    default:
        break;
    }
}
