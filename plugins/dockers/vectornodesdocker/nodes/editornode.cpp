#include "editornode.h"

#include "../nodescene.h"

//#include <QDebug>

EditorNode::EditorNode(QGraphicsScene *scene, qreal x, qreal y, QObject *parent) : QObject(parent) {
    // Init
    type = EditorNodes::Type::Empty;
    NodeItem *n = new NodeItem(this, x, y);
    nodeItem = QSharedPointer<NodeItem>(n);
    scene->addItem(nodeItem.data());
    nodeItem.data()->SetupFromType(static_cast<int>(type));
}
EditorNode::~EditorNode() {}
void EditorNode::OnClick(QGraphicsSceneMouseEvent *event) {
    if(event) return; // Suppresses unused event warning. Saved for virtual overrides
}

IOElement *EditorNode::AddInput() {
    IOElement *inputElement = new IOElement(nodeItem, this);
    inputElement->index = inputs.size();
    nodeItem->AddInput(inputElement);
    inputs.push_back(inputElement);
//    qDebug() << QString("Adding input %1").arg(inputElement->index);
    return inputElement;
}
IOElement *EditorNode::AddOutput() {
    IOElement *outputElement = new IOElement(nodeItem, this);
    outputElement->index = outputs.size();
    outputElement->SetOutput(true);
    nodeItem->AddOutput(outputElement);
    outputs.push_back(outputElement);
//    qDebug() << QString("Adding output %1").arg(outputElement->index);
    return outputElement;
}

void EditorNode::ConnectInput(IOElement *elementIn, IOElement *elementOut) {
    QString key = QString("%1-%2").arg(elementIn->index).arg(elementOut->index);
    inputNodes.insert(key, elementOut->Node().data());
//    qDebug() << QString("Connecting INPUT node at socket %1").arg(key);
//    qDebug() << QString("from %1").arg(elementOut->Node()->nodeItem->titleText->toPlainText());
//    qDebug() << QString("new inputNodes size: %1").arg(inputNodes.count());
//    qDebug() << QString("-----------------------------------------------");
}
void EditorNode::ConnectOutput(IOElement *elementOut, IOElement *elementIn) {
    QString key = QString("%1-%2").arg(elementOut->index).arg(elementIn->index);
    outputNodes.insert(key, elementIn->Node().data());
//    qDebug() << QString("Connecting OUTPUT node at socket %1").arg(key);
//    qDebug() << QString("from %1").arg(elementIn->Node()->nodeItem->titleText->toPlainText());
//    qDebug() << QString("new outputNodes size: %1").arg(outputNodes.count());
//    qDebug() << QString("-----------------------------------------------");
}

void EditorNode::DisconnectInput(IOElement *elementIn, IOElement *elementOut) {
    QString key = QString("%1-%2").arg(elementIn->index).arg(elementOut->index);
    if(inputNodes.contains(key)) {
//        qDebug() << QString("Disconnecting INPUT node at socket %1").arg(key);
//        qDebug() << QString("from %1").arg(elementOut->Node()->nodeItem->titleText->toPlainText());
        inputNodes.remove(key);
    }
//    qDebug() << QString("new inputNodes size: %1").arg(inputNodes.count());
//    qDebug() << QString("-----------------------------------------------");
}
void EditorNode::DisconnectOutput(IOElement *elementOut, IOElement *elementIn) {
    QString key = QString("%1-%2").arg(elementOut->index).arg(elementIn->index);
    if(outputNodes.contains(key)) {
//        qDebug() << QString("Disconnecting OUTPUT node at socket %1").arg(key);
//        qDebug() << QString("from %1").arg(elementIn->Node()->nodeItem->titleText->toPlainText());
        outputNodes.remove(key);
    }
//    qDebug() << QString("new outputNodes size: %1").arg(outputNodes.count());
//    qDebug() << QString("-----------------------------------------------");
}

void EditorNode::Update() {}
void EditorNode::DownstreamUpdate() {
    Update();
    for(EditorNode * node : qAsConst(outputNodes))
        node->DownstreamUpdate();
}

void EditorNode::ApplyStyle(int style) { nodeItem->ApplyStyle(style); }
