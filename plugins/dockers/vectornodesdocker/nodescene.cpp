#include "nodescene.h"

#include "KisDocument.h"
#include <kis_layer.h>
#include <kis_group_layer.h>
#include <kis_node.h>
#include <KoProperties.h>

#include "nodestyles.h"
#include "nodeitem.h"

#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>

NodeScene::NodeScene(QObject *parent) : QGraphicsScene(parent) {
    //  Init
    addNodeMenu = new EditorMenu();
    hoverIO = nullptr;
    selectedIO = nullptr;
    activeConnection = nullptr;
    mainCanvas = nullptr;
    shiftKey = false;
    ctrlKey = false;
    altKey = false;
    previousPos = QPointF(0, 0);

    debugTextItem = new QGraphicsTextItem();
    debugTextItem->setAcceptHoverEvents(false);
    debugTextItem->setPos(-300, -100);
    addItem(debugTextItem);

    // Style stuffs
    style = Normie;

    // Center
    this->addLine(0, -20, 0, 20, QPen(Qt::red));
    this->addLine(-20, 0, 20, 0, QPen(Qt::red));
    nodes.push_back(new PreviewNode(this, 400, previousPos.y()));
    nodes.push_back(new GetShapesNode(this, 200, previousPos.y()));
    nodes.push_back(new ValueNode(this, previousPos.x(), 100));

    // Menu
    addItem(addNodeMenu);
    addNodeMenu->hide();

    // Events
    installEventFilter(this);
}

KoCanvasBase *NodeScene::Canvas() const { return mainCanvas; }
void NodeScene::setCanvas(KoCanvasBase *canvas) {
    mainCanvas = canvas;
    nodes.push_back(new LayerNode(this, previousPos.x(), -100));
} // TODO : Propagate to specific node types

void NodeScene::AddNode(QString nodeName) {
    debugTextItem->setPlainText(nodeName);
    if(nodeName == "Layer")
        nodes.push_back(new LayerNode(this, previousPos.x(), previousPos.y()));
    else if(nodeName == "Output")
        nodes.push_back(new OutputNode(this, previousPos.x(), previousPos.y()));
    else if(nodeName == "Preview")
        nodes.push_back(new PreviewNode(this, previousPos.x(), previousPos.y()));
    else if(nodeName == "Shape")
        nodes.push_back(new ShapeNode(this, previousPos.x(), previousPos.y()));
    else if(nodeName == "Get Shapes")
        nodes.push_back(new GetShapesNode(this, previousPos.x(), previousPos.y()));
    else if(nodeName == "Math")
        nodes.push_back(new MathNode(this, previousPos.x(), previousPos.y()));
    else if(nodeName == "Noise")
        nodes.push_back(new NoiseNode(this, previousPos.x(), previousPos.y()));
    else if(nodeName == "Color")
        nodes.push_back(new ColorNode(this, previousPos.x(), previousPos.y()));
    else if(nodeName == "Value")
        nodes.push_back(new ValueNode(this, previousPos.x(), previousPos.y()));
    addNodeMenu->hide();
    nodes.last()->Update();
}

void NodeScene::Debug(QString text) { debugTextItem->setPlainText(text); }

void NodeScene::setHoverIO(IOElement *hover) {
    if(hoverIO != hover)
        hoverIO = hover;
}

void NodeScene::setStyle(int index) {
    style = static_cast<NodeStyle>(index); // TODO  : Add bg styles
    for(int i = 0; i < nodes.size(); i++)
        nodes[i]->ApplyStyle(index);
}

void NodeScene::DeleteConnection(Connection *conn) {
    if(conn->Input())
        conn->Input()->Disconnect();
    if(conn->Output())
        conn->Output()->Disconnect(conn);
    removeItem(conn);
    delete conn;
}

bool NodeScene::eventFilter(QObject *watched, QEvent *event) {
    if(watched == this) {
        QGraphicsSceneMouseEvent *mouseSceneEvent;
        if(event->type() == QEvent::GraphicsSceneMousePress) { // Mouse Down
            if(hoverIO) {
                if(hoverIO->IsOutput()) {
                    // New connection
                    activeConnection = new Connection(this, hoverIO->scenePos().x()+6, hoverIO->scenePos().y()+6, false);
                    activeConnection->setOutput(hoverIO);
                } else {
                    if(hoverIO->Connection()) {
                        // Disconnect
                        IOElement *outOld = hoverIO->Connection()->Output();
                        DeleteConnection(hoverIO->Connection());
                        hoverIO->Node()->DisconnectInput(hoverIO, outOld);
                        outOld->Node()->DisconnectOutput(outOld, hoverIO);
                        hoverIO->Node()->DownstreamUpdate();
                        // New connection
                        activeConnection = new Connection(this, outOld->scenePos().x()+6, outOld->scenePos().y()+6, false);
                        activeConnection->setOutput(outOld);
                    } else {
                        // New connection
                        activeConnection = new Connection(this, hoverIO->scenePos().x()+6, hoverIO->scenePos().y()+6, true);
                        activeConnection->setInput(hoverIO);
                    }
                }
                activeConnection->ColorActive(true);
            }
        }
        else if (event->type() == QEvent::GraphicsSceneMouseRelease) { // Mouse Up
            if(activeConnection) {
                if(hoverIO) {
                    if(hoverIO->IsOutput()) {
                        // Connect
                        activeConnection->setOutput(hoverIO);
                        hoverIO->Node()->ConnectOutput(hoverIO, activeConnection->Input());
                        activeConnection->Input()->Node()->ConnectInput(activeConnection->Input(), hoverIO);
                        activeConnection->Input()->Node()->DownstreamUpdate();
                    } else {
                        if(hoverIO->Connection()) {
                            // Disconnect
                            IOElement *outOld = hoverIO->Connection()->Output();
                            DeleteConnection(hoverIO->Connection());
                            hoverIO->Node()->DisconnectInput(hoverIO, outOld);
                            outOld->Node()->DisconnectOutput(outOld, hoverIO);
                        }
                        // Connect
                        activeConnection->setInput(hoverIO);
                        hoverIO->Node()->ConnectInput(hoverIO, activeConnection->Output());
                        activeConnection->Output()->Node()->ConnectOutput(activeConnection->Output(), hoverIO);
                        hoverIO->Node()->DownstreamUpdate();
                    }
                    activeConnection->ColorActive(false);
                } else {
                    DeleteConnection(activeConnection);
                }
                activeConnection = nullptr;
            }
        }
        else if (event->type() == QEvent::GraphicsSceneMouseMove) { // Mouse Move
            mouseSceneEvent = static_cast<QGraphicsSceneMouseEvent *>(event);
            previousPos = mouseSceneEvent->scenePos();
            // If dragging activeConnection
            if(activeConnection) {
                int x = mouseSceneEvent->scenePos().rx();
                int y = mouseSceneEvent->scenePos().ry();
                bool snap = false;
                // Set tempHover objptr
                IOElement *tempHover = nullptr;
                // For each graphics item
                foreach (QGraphicsItem *item, this->items()) {
                    IOElement *ioElement = qgraphicsitem_cast<IOElement *>(item);
                    IOElement *selectedElement = activeConnection->CurrentIO();
                    // process only if item is an IOElement
                    if (!ioElement) continue;
                    // if under the mouse
                    if(ioElement->isUnderMouse()) {
                        if(ioElement->IsOutput() == selectedElement->IsOutput()) continue;
                        if(ioElement->Node() == selectedElement->Node()) continue;
                        // Draw to snapping position and set snap to prevent drawing to mouse pos
                       activeConnection->DrawTo(ioElement->scenePos().x()+6, ioElement->scenePos().y()+6);
                       snap = true;
                        // Draw hover - NOTE : Also sets hoverIO
                        ioElement->HoverDraw(true);
                        // Set tempHover
                        tempHover = ioElement;
                    // if not under mouse
                    } else {
                        // Draw non hovering - NOTE : Also sets hoverIO to nullptr
                        ioElement->HoverDraw(false);
                    }
                }
                // Sets hoverIO to nullptr OR to tempHover if it was set
                setHoverIO(tempHover);
                if(!snap)
                    activeConnection->DrawTo(x, y);
            }
        }
        QKeyEvent *keyEvent;
        if(event->type() == QEvent::KeyPress) { // Key Press
            keyEvent = static_cast<QKeyEvent *>(event);
            switch(keyEvent->key()) {
            case Qt::Key_Shift:
                shiftKey = true;
                break;
            case Qt::Key_Alt:
                altKey = true;
                break;
            case Qt::Key_Control:
            case Qt::Key_Meta:
                ctrlKey = true;
                break;
            case Qt::Key_A:
                if(shiftKey) {
                    debugTextItem->setPlainText(QString("Add Element"));
                    addNodeMenu->setPos(previousPos.x(), previousPos.y());
                    addNodeMenu->show();
                }
                break;
            default:
                break;
            }
        }
        if(event->type() == QEvent::KeyRelease) { // Key Release
            keyEvent = static_cast<QKeyEvent *>(event);
            switch(keyEvent->key()) {
            case Qt::Key_Shift:
                shiftKey = false;
                break;
            case Qt::Key_Alt:
                altKey = false;
                break;
            case Qt::Key_Control:
            case Qt::Key_Meta:
                ctrlKey = false;
                break;
            default:
                break;
            }
        }
    }
    return QGraphicsScene::eventFilter(watched, event);
}
