#include "vectornodes_docker_widget.h"
#include "ui_vectornodes_docker_widget.h"

#include "kis_debug.h"
#include "kis_canvas2.h"

#include "kactioncollection.h"

#include <QAction>
#include <QToolButton>

struct VectorNodesDockerWidget::Private {
};

VectorNodesDockerWidget::VectorNodesDockerWidget(QWidget *parent) : QWidget(parent), ui(new Ui::VectorNodesDockerWidget) {
    ui->setupUi(this);

    Scene = new NodeScene(this);
    ui->graphicsView->setScene(Scene);
    ui->debugLabel->setText(QString("Hellooooo"));
}
VectorNodesDockerWidget::~VectorNodesDockerWidget() {}

void VectorNodesDockerWidget::on_styleBox_currentIndexChanged(int index) {
    Scene->setStyle(index);
}

void VectorNodesDockerWidget::setCanvas(KoCanvasBase *canvas) {
    Scene->setCanvas(canvas);
}

void replaceAction(QToolButton *button, QAction *newAction) {
    Q_FOREACH (QAction *action, button->actions()) {
        button->removeAction(action);
    }

    if (newAction) {
        button->setDefaultAction(newAction);
    }
}
