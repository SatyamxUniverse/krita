#include "editormenuitem.h"

#include <QBrush>
#include <QPen>

#include "../nodescene.h"
#include "../nodestyles.h"

EditorMenuItem::EditorMenuItem(QString text, qreal x, qreal y, qreal w, qreal h, bool selectable) :
    bodyRect(new QGraphicsRectItem(x, y, w, h)),
    textItem(new QGraphicsTextItem(text))
{
    isSelectable = selectable;
    selected = false;
    m_text = text;
    QBrush brush = QBrush(Qt::SolidPattern);
    // Body
    if(selectable)
        brush.setColor(NodeStyles::Color::NodeFill_Normie);
    else
        brush.setColor(NodeStyles::Color::BackgroundFill);
    bodyRect->setBrush(brush);
    bodyRect->setPen(Qt::NoPen);
    addToGroup(bodyRect);
    // Text
    textItem->setPos(x, y);
    textItem->setAcceptHoverEvents(false);
    addToGroup(textItem);
    // This
    setAcceptHoverEvents(true);
    setPos(x, y);
}

QString EditorMenuItem::GetText() { return m_text; }

void EditorMenuItem::RefreshSel(bool sel) {
    if(!isSelectable) return;
    selected = sel;
    QBrush brush = QBrush(Qt::SolidPattern);
    if(selected)
        brush.setColor(NodeStyles::Color::Text_Normie);
    else
        brush.setColor(NodeStyles::Color::NodeFill_Normie);
    bodyRect->setBrush(brush);
}

void EditorMenuItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event) {
    RefreshSel(true);
    QGraphicsItemGroup::hoverLeaveEvent(event);
}
void EditorMenuItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
    RefreshSel(false);
    QGraphicsItemGroup::hoverLeaveEvent(event);
}
void EditorMenuItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    NodeScene *nodeScene = dynamic_cast<NodeScene *>(scene());
    RefreshSel(false);
    nodeScene->AddNode(GetText());
    QGraphicsItemGroup::mousePressEvent(event);
}
