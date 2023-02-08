#ifndef NOISENODE_H
#define NOISENODE_H

#include  "editornode.h"

class NoiseNode : public EditorNode {
    Q_OBJECT
public:
    NoiseNode(QGraphicsScene *scene, qreal x = 0, qreal y = 0);
    virtual ~NoiseNode();
    void Update() override;
};

#endif // NOISENODE_H
