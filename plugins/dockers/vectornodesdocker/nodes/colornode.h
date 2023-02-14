#ifndef COLORNODE_H
#define COLORNODE_H

#include  "editornode.h"

class ColorNode : public EditorNode {
    Q_OBJECT
public:
    ColorNode(QGraphicsScene *scene, qreal x = 0, qreal y = 0);
    virtual ~ColorNode();
    void Update() override;

private:
};

#endif // COLORNODE_H
