#ifndef EDITORNODE_H
#define EDITORNODE_H

#include <vector>
#include <QVector>
#include <QSharedPointer>
#include <QPointer>

#include <QObject>
#include <QGraphicsScene>

#include "../nodeitem.h"
#include "../nodestyles.h"

namespace EditorNodes {
enum Type {
    Empty,
    Color,
    GetShapes,
    Layer,
    Math,
    Noise,
    Output,
    Preview,
    Shape,
    Value,
};
}

class EditorNode : public QObject {
    Q_OBJECT
public:
    explicit EditorNode(QGraphicsScene *scene, qreal x = 0, qreal y = 0, QObject *parent = nullptr);
    virtual ~EditorNode();
    void ApplyStyle(int style);
    IOElement *AddInput();
    IOElement *AddOutput();
    void ConnectInput(IOElement *elementIn, IOElement *elementOut);
    void ConnectOutput(IOElement *elementOut, IOElement *elementIn);
//    void ConnectInput(IOElement *element, QSharedPointer<EditorNode> node);
//    void ConnectOutput(IOElement *element, QSharedPointer<EditorNode> node);
    void DisconnectInput(IOElement *elementIn, IOElement *elementOut);
    void DisconnectOutput(IOElement *elementOut, IOElement *elementIn);
    virtual void Update();
    void DownstreamUpdate();
    virtual void OnClick(QGraphicsSceneMouseEvent *event);
    QSharedPointer<NodeItem> nodeItem;// TODO : Move back to protected

protected:
    QMap<QString, EditorNode *> inputNodes;
    QMap<QString, EditorNode *> outputNodes;
    QVector<IOElement *> inputs;
    QVector<IOElement *> outputs;
    EditorNodes::Type type;
};

#endif // EDITORNODE_H
