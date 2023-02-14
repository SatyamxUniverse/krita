#include "valuenode.h"

#include <QGraphicsProxyWidget>

ValueNode::ValueNode(QGraphicsScene *scene, qreal x, qreal y) : EditorNode(scene, x, y) {
    // Init
    type = EditorNodes::Type::Value;
    nodeItem->SetupFromType(static_cast<int>(type));
    nodeItem->SetTitle("Value");
    value.push_back(0);
    value.push_back(0);
    value.push_back(0);
    // I/O
    outputIO = AddOutput();
    outputIO->SetVSpacing(64);
    nodeItem->resizeBody();
    // Type Compo Box
    typeComboBox = new QComboBox();
    typeComboBox->addItem("int value");
    typeComboBox->addItem("int range");
    QGraphicsProxyWidget *comboProxy = new QGraphicsProxyWidget(nodeItem.data());
    comboProxy->setWidget(typeComboBox);
    comboProxy->setGeometry(QRectF(15, NodeStyles::Style::NodeMarginV, 130, 40));
    connect(typeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(selectValueType(int)));
    nodeItem->addToGroup(comboProxy);
    // Value Spinners
    minSpinBox = new QSpinBox();
    minSpinBox->setValue(0);
    minSpinBox->setMinimum(0);
    minSpinBox->setMaximum(10000);
    QGraphicsProxyWidget *minProxy = new QGraphicsProxyWidget(nodeItem.data());
    minProxy->setWidget(minSpinBox);
    minProxy->setGeometry(QRectF(15, NodeStyles::Style::NodeMarginV + 44, 130, 20));
    connect(minSpinBox, SIGNAL(valueChanged(int)), this, SLOT(minValueChanged(int)));

    maxSpinBox = new QSpinBox();
    maxSpinBox->setValue(0);
    maxSpinBox->setMinimum(0);
    maxSpinBox->setMaximum(10000);
    maxSpinBox->hide();
    QGraphicsProxyWidget *maxProxy = new QGraphicsProxyWidget(nodeItem.data());
    maxProxy->setWidget(maxSpinBox);
    maxProxy->setGeometry(QRectF(15, NodeStyles::Style::NodeMarginV + 68, 130, 20));
    connect(maxSpinBox, SIGNAL(valueChanged(int)), this, SLOT(maxValueChanged(int)));
}
ValueNode::~ValueNode() {}

void ValueNode::selectValueType(int index) {
    if(index == 0) {
        outputIO->SetVSpacing(64);
        maxSpinBox->hide();
        minSpinBox->setMaximum(10000);
    } else if(index == 1) {
        outputIO->SetVSpacing(88);
        maxSpinBox->show();
        minSpinBox->setMaximum(maxSpinBox->value());
    }
    nodeItem->resizeBody();
    value[2] = index;
    DownstreamUpdate();
}
void ValueNode::minValueChanged(int i) {
    value[0] = minSpinBox->value();
    DownstreamUpdate();
}
void ValueNode::maxValueChanged(int i) {
    minSpinBox->setMaximum(maxSpinBox->value());
    value[1] = maxSpinBox->value();
    DownstreamUpdate();
}

void ValueNode::Update() {}
