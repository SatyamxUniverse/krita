#ifndef IOELEMENT_H
#define IOELEMENT_H

#include <QVector>
#include <QGraphicsEllipseItem>
#include <QGraphicsItemGroup>
#include <QPen>
#include <QGraphicsSceneDragDropEvent>

class EditorNode;

class IOElement : public QGraphicsEllipseItem {
public:
    IOElement(QSharedPointer<QGraphicsItemGroup> parent = nullptr, EditorNode *nodeParent = nullptr);
    virtual ~IOElement();
    EditorNode *Node();

    int index;

    int  VSpacing() const;
    void SetVSpacing(int v);

    bool IsOutput() const;
    void SetOutput(bool output);

    void SetHovering(bool newIsHovering);
    void HoverDraw(bool hovering);

    class Connection *Connection() const;
    void Connect(class Connection *newConnection);
    void Disconnect(class Connection *conn = nullptr);

protected:
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    EditorNode *node;
    int vSpacing;
    bool isOutput;
    bool isHovering;
    class Connection *connection;
    QVector<class Connection *> connections;
};

#endif // IOELEMENT_H
