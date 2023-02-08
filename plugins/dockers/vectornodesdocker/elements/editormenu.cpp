#include "editormenu.h"

#include <QPen>

#include "../nodestyles.h"

EditorMenu::EditorMenu(qreal x, qreal y) : // TODO : Make modular
    bodyRect(new QGraphicsRectItem(-80, -20, 160, 0))
{
    // Body
    bodyRect->setPen(Qt::NoPen);
    addToGroup(bodyRect);
    // This
    setAcceptHoverEvents(true);
    setHandlesChildEvents(false);
    setPos(x, y);
    // TODO : Remove
    AddMenuItem("I/O", false);
    AddMenuItem("Layer", true);
    AddMenuItem("Output", true);
    AddMenuItem("Preview", true);

    AddMenuItem("Geometric", false);
    AddMenuItem("Shape", true);
    AddMenuItem("Get Shapes", true);

    AddMenuItem("Operational", false);
    AddMenuItem("Math", true);
    AddMenuItem("Noise", true);

    AddMenuItem("Values", false);
    AddMenuItem("Color", true);
    AddMenuItem("Value", true);
}

void EditorMenu::AddMenuItem(QString text, bool selectable) {
    EditorMenuItem *menuItem = new EditorMenuItem(text, bodyRect->x(), bodyRect->y(), bodyRect->boundingRect().width(), 24, selectable);
    menuItems.push_back(menuItem);
    addToGroup(menuItem);
    Resize();
}

void EditorMenu::Resize() {
    qreal yCursor = 0;
    for(int i = 0; i < menuItems.size(); i++)
        yCursor += menuItems[i]->boundingRect().height();
    yCursor /= 2;
    yCursor = bodyRect->x() - yCursor;
    for(int i = 0; i < menuItems.size(); i++) {
        menuItems[i]->setPos(-80, yCursor);
        yCursor += menuItems[i]->boundingRect().height();
    }
}
void EditorMenu::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
    hide();
    QGraphicsItemGroup::hoverLeaveEvent(event);
}
