#ifndef VALUENODE_H
#define VALUENODE_H

#include <QComboBox>
#include <QSpinBox>

#include  "editornode.h"

class ValueNode : public EditorNode {
    Q_OBJECT
public:
    ValueNode(QGraphicsScene *scene, qreal x = 0, qreal y = 0);
    virtual ~ValueNode();
    void Update() override;
    QVector<int> value;

private:
    IOElement *outputIO;
    QComboBox *typeComboBox;
    QSpinBox *minSpinBox;
    QSpinBox *maxSpinBox;
    Q_SLOT void selectValueType(int index);
    Q_SLOT void minValueChanged(int i);
    Q_SLOT void maxValueChanged(int i);
};

#endif // VALUENODE_H
