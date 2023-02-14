#ifndef NODEITEM_H
#define NODEITEM_H

#include <QVector>
#include <QGraphicsItemGroup>
#include <QGraphicsRectItem>

#include "elements/nodeoutlinerectitem.h"
#include "elements/ioelement.h"

class NodeItem : public QGraphicsItemGroup {
public:
    explicit NodeItem(class EditorNode *nodeParent, qreal x = 0, qreal y = 0, int w = 160, int h = 50);
    virtual ~NodeItem();
    void SetupFromType(int type);
    void ApplyStyle(int style);
    void SetTitle(QString text);
    void AddInput(IOElement *input);
    void AddOutput(IOElement *output);
    void resizeBody();
    QGraphicsTextItem *titleText;// TODO : Move back to private

protected:
    void  mousePressEvent(QGraphicsSceneMouseEvent *event);

private:
    QGraphicsRectItem *bodyRect;
    QGraphicsRectItem *headerRect;
    NodeOutlineRectItem *outlineRect;
    QVector<IOElement *> inputs;
    QVector<IOElement *> outputs;
    class EditorNode *node;
};

#endif // NODEITEM_H
