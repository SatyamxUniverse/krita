#ifndef CONNECTION_H
#define CONNECTION_H

#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QPainterPath>

#include "ioelement.h"

class Connection : public QGraphicsPathItem {
public:
    Connection(QGraphicsScene *parent = nullptr, qreal x = 0, qreal y = 0, bool left = false);
    void DrawTo(int xPos, int yPos);
    void DrawMove();
    void ColorActive(bool active);

    IOElement *Input() const;
    void setInput(IOElement *newInput);
    IOElement *Output() const;
    void setOutput(IOElement *newOutput);
    IOElement *CurrentIO() const;

protected:
    QPainterPath *path;
    QPen *pen;

private:
    IOElement *input;
    IOElement *output;
    bool toLeft;
};

#endif // CONNECTION_H
