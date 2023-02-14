#ifndef VECTORNODES_DOCKER_WIDGET_H
#define VECTORNODES_DOCKER_WIDGET_H

#include <QWidget>
#include <QScopedPointer>
#include <QGraphicsRectItem>
#include <KoCanvasObserverBase.h>

#include "kactioncollection.h"

#include "nodescene.h"

namespace Ui {
    class VectorNodesDockerWidget;
}

class VectorNodesDockerWidget : public QWidget {
    Q_OBJECT

public:
    explicit VectorNodesDockerWidget(QWidget *parent = 0);
    ~VectorNodesDockerWidget() override;
    void on_styleBox_currentIndexChanged(int index);
    void setCanvas(KoCanvasBase *canvas);
    void switchState(bool enabled);

private:
    NodeScene *Scene;
    QScopedPointer<Ui::VectorNodesDockerWidget> ui;
    struct Private;
};

#endif // VECTORNODES_DOCKER_WIDGET_H
