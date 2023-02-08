#ifndef OUTPUTNODE_H
#define OUTPUTNODE_H

#include  "editornode.h"

class OutputNode : public EditorNode {
    Q_OBJECT
public:
    OutputNode(QGraphicsScene *scene, qreal x = 0, qreal y = 0);
    virtual ~OutputNode();
    void Update() override;
};

#endif // OUTPUTNODE_H
