#include "ioelement.h"

#include <QBrush>
#include <QDebug>

#include "../nodescene.h"
#include "../nodestyles.h"

IOElement::IOElement(QSharedPointer<QGraphicsItemGroup> parent, EditorNode *nodeParent) : QGraphicsEllipseItem(parent.data()) {
    //  Init
    index = -1;
    vSpacing = 26;
    isOutput = false;
    isHovering = false;
    connection = nullptr;
    node = nodeParent;

    QBrush brush = QBrush(Qt::SolidPattern);
    brush.setColor(NodeStyles::Color::IO_Normie);
    setBrush(brush);
    setPen(QPen(NodeStyles::Color::IO_Pen_Normie));
    setAcceptHoverEvents(true);
    setFlags(QGraphicsItem::GraphicsItemFlag::ItemIsSelectable |
             QGraphicsItem::GraphicsItemFlag::ItemSendsScenePositionChanges |
             QGraphicsItem::GraphicsItemFlag::ItemSendsGeometryChanges);
}
IOElement::~IOElement() {}

EditorNode *IOElement::Node() { return node; }

void IOElement::HoverDraw(bool hovering) {
    QPen pen;
    if(hovering) {
        pen.setColor(NodeStyles::Color::Pen_Hover_Normie);
        pen.setWidth(2);
    } else {
        pen.setColor(NodeStyles::Color::IO_Pen_Normie);
        pen.setWidth(1);
    }
    setPen(pen);
    update();
}

void IOElement::hoverEnterEvent(QGraphicsSceneHoverEvent *event) {
    SetHovering(true);
    QGraphicsEllipseItem::hoverEnterEvent(event);
}
void IOElement::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
    SetHovering(false);
    QGraphicsEllipseItem::hoverLeaveEvent(event);
}

QVariant IOElement::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value) {
    if(change == QGraphicsItem::ItemScenePositionHasChanged) {
        if(!isOutput && connection)
            connection->DrawMove();
        if(isOutput) {
            for(int i = 0; i < connections.size(); i++){
                connections[i]->DrawMove();
            }
        }
    }
    return QGraphicsEllipseItem::itemChange(change, value);
}

void IOElement::SetHovering(bool hovering) {
    isHovering = hovering;
    HoverDraw(isHovering);
    NodeScene *nodeScene = dynamic_cast<NodeScene *>(scene());
    if(isHovering)
        nodeScene->setHoverIO(this);
    else
        nodeScene->setHoverIO(nullptr);
}

class Connection * IOElement::Connection() const { return connection; }
void IOElement::Connect(class Connection *conn) {
    if(!isOutput)
        connection = conn;
    else
        connections.push_back(conn);
}
void IOElement::Disconnect(class Connection *conn) {
    if(!isOutput)
        connection = nullptr;
    else
        for(int i = 0; i < connections.size(); i++) {
            if(connections[i] == conn) {
                connections.erase(connections.begin() + i);
            }
        }
}

bool IOElement::IsOutput() const { return isOutput; }
void IOElement::SetOutput(bool output) { isOutput = output; }

int IOElement::VSpacing() const { return vSpacing; }
void IOElement::SetVSpacing(int v) { vSpacing = v; }
