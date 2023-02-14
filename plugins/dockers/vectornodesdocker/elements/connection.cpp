#include "connection.h"

#include "../nodestyles.h"

Connection::Connection(QGraphicsScene *parent, qreal x, qreal y, bool left) : QGraphicsPathItem() {
    // Init
    input = nullptr;
    output = nullptr;
    toLeft = left;
    path = new QPainterPath();
    pen = new QPen();
    // Configure
    path->moveTo(left? x-6: x+6, y);
    DrawTo(x, y);
    pen->setColor(NodeStyles::Color::Connection_Normie);
    pen->setWidth(3);
    setPen(*pen);
    parent->addItem(this);
}

void Connection::DrawTo(int xPos, int yPos) {
    qreal xRoot = path->elementAt(0).x;
    qreal yRoot = path->elementAt(0).y;
    qreal distX = abs(xPos - xRoot);
    distX = (distX < 60)? distX : 60;
    path->clear();
    path->moveTo(xRoot, yRoot);
    if(toLeft) {
        path->cubicTo(xRoot-distX, yRoot, xPos+distX, yPos, xPos, yPos);
    } else {
        path->cubicTo(xRoot+distX, yRoot, xPos-distX, yPos, xPos, yPos);
    }
    setPath(*path);
}
void Connection::DrawMove() {
    qreal xRoot = input->scenePos().x() + 6;
    qreal yRoot = input->scenePos().y() + 6;
    qreal xTo = output->scenePos().x() + 6;
    qreal yTo = output->scenePos().y() + 6;
    qreal distX = abs(xTo - xRoot);
    distX = (distX < 60)? distX : 60;
    path->clear();
    path->moveTo(xRoot, yRoot);
    path->cubicTo(xRoot-distX, yRoot, xTo+distX, yTo, xTo, yTo);
    setPath(*path);
}

void Connection::ColorActive(bool active) {
    if(active)
        pen->setColor(NodeStyles::Color::Connection_Selected_Normie);
    else
        pen->setColor(NodeStyles::Color::Connection_Normie);
    update();
    setPen(*pen);
}

IOElement *Connection::Input() const { return input; }
void Connection::setInput(IOElement *newInput) {
    input = newInput;
    input->Connect(this);
}

IOElement *Connection::Output() const { return output; }
void Connection::setOutput(IOElement *newOutput) {
    output = newOutput;
    output->Connect(this);
}

IOElement *Connection::CurrentIO() const { return input? input : output; }
