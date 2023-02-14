#ifndef SHAPENODE_H
#define SHAPENODE_H

#include  "editornode.h"

class ShapeNode : public EditorNode {
    Q_OBJECT
public:
    ShapeNode(QGraphicsScene *scene, qreal x = 0, qreal y = 0);
    virtual ~ShapeNode();
    void Update() override;
};

#endif // SHAPENODE_H
