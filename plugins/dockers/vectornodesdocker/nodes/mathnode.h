#ifndef MATHNODE_H
#define MATHNODE_H

#include  "editornode.h"

class MathNode : public EditorNode {
    Q_OBJECT
public:
    MathNode(QGraphicsScene *scene, qreal x = 0, qreal y = 0);
    virtual ~MathNode();
    void Update() override;
};

#endif // MATHNODE_H
