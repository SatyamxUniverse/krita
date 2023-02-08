#include "nodeoutlinerectitem.h"

#include <QPen>

#include "../nodestyles.h"

NodeOutlineRectItem::NodeOutlineRectItem(qreal x, qreal y, qreal width, qreal height, QGraphicsItemGroup *parent) :
    QGraphicsRectItem(x, y, width, height, parent) {
    setPen(NodeStyles::Color::Outline_Normie);
    parent->addToGroup(this);
    setAcceptHoverEvents(true);
}

void NodeOutlineRectItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event){
    QPen pen;
    pen.setColor(NodeStyles::Color::Pen_Hover_Normie);
    pen.setWidth(4);
    setPen(pen);
    update();
    QGraphicsRectItem::hoverEnterEvent(event);
}

void NodeOutlineRectItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event){
    QPen pen;
    pen.setColor(NodeStyles::Color::Outline_Normie);
    pen.setWidth(1);
    setPen(pen);
    update();
    QGraphicsRectItem::hoverLeaveEvent(event);
}
