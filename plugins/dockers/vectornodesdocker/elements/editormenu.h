#ifndef EDITORMENU_H
#define EDITORMENU_H

#include <vector>

#include <QGraphicsRectItem>

#include "editormenuitem.h"

class EditorMenu : public QGraphicsItemGroup {
public:
    EditorMenu(qreal x = 0, qreal y = 0);
    void AddMenuItem(QString text, bool selectable = true);

protected:
    void Resize();
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private:
    QGraphicsRectItem *bodyRect;
    std::vector<EditorMenuItem *> menuItems;
};

#endif // EDITORMENU_H
