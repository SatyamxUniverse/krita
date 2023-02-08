#ifndef EDITORMENUITEM_H
#define EDITORMENUITEM_H

#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QString>

class EditorMenuItem : public QGraphicsItemGroup {
public:
    EditorMenuItem(QString text, qreal x, qreal y, qreal w, qreal h, bool selectable);
    bool isSelectable;
    bool selected;
    QString GetText();

protected:
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void RefreshSel(bool sel);

private:
    QGraphicsRectItem *bodyRect;
    QGraphicsTextItem *textItem;
    QString m_text;
};

#endif // EDITORMENUITEM_H
