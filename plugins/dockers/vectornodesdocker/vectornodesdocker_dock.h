#ifndef _GRID_DOCK_H_
#define _GRID_DOCK_H_

#include <QDockWidget>
#include <KoCanvasObserverBase.h>
#include "kis_signal_auto_connection.h"

class KisCanvas2;
class VectorNodesDockerWidget;
class KisSignalAutoConnection;

class VectorNodesDockerDock : public QDockWidget, public KoCanvasObserverBase {
    Q_OBJECT
public:
    VectorNodesDockerDock();
    ~VectorNodesDockerDock() override;
    QString observerName() override { return "VectorNodesDockerDock"; }
    void setCanvas(KoCanvasBase *canvas) override;
    void unsetCanvas() override;

private Q_SLOTS:
    // Slots here

private:
    VectorNodesDockerWidget *m_configWidget;
    QPointer<KisCanvas2> m_canvas;
    KisSignalAutoConnectionsStore m_canvasConnections;
};


#endif
