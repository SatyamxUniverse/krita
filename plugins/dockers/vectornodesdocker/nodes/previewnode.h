#ifndef PREVIEWNODE_H
#define PREVIEWNODE_H

#include  "editornode.h"

class PreviewNode : public EditorNode {
    Q_OBJECT
public:
    PreviewNode(QGraphicsScene *scene, qreal x = 0, qreal y = 0);
    virtual ~PreviewNode();
    void Update() override;

private:
    QGraphicsTextItem *textItem;
    int times;
    QGraphicsPixmapItem *imageItem;
    IOElement *inputIO;
};

#endif // PREVIEWNODE_H
