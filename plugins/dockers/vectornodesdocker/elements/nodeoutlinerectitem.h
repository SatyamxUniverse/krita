#ifndef NODEOUTLINERECTITEM_H
#define NODEOUTLINERECTITEM_H

#include <QGraphicsRectItem>

class NodeOutlineRectItem : public QGraphicsRectItem {
public:
    NodeOutlineRectItem(qreal x, qreal y, qreal width, qreal height, QGraphicsItemGroup *parent = nullptr);

protected:
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
};

#endif // NODEOUTLINERECTITEM_H
