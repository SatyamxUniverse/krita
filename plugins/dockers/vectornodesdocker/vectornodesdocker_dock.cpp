#include "vectornodesdocker_dock.h"
#include <klocalizedstring.h>

#include "kis_canvas2.h"
#include <KisViewManager.h>
#include "vectornodes_docker_widget.h"

#include <KoToolProxy.h>
#include <KoShapeManager.h>


VectorNodesDockerDock::VectorNodesDockerDock( ) : QDockWidget(i18n("Vector Nodes")), m_canvas(0) {
    m_configWidget = new VectorNodesDockerWidget(this);
    setWidget(m_configWidget);
    setEnabled(m_canvas);
}

VectorNodesDockerDock::~VectorNodesDockerDock() {
}

void VectorNodesDockerDock::setCanvas(KoCanvasBase * canvas) {
    if(canvas && m_canvas == canvas)
        return;

    if (m_canvas) {
        m_canvasConnections.clear();
        m_canvas->disconnectCanvasObserver(this);
        m_canvas->image()->disconnect(this);
    }

    m_canvas = canvas ? dynamic_cast<KisCanvas2*>(canvas) : 0;
    setEnabled(m_canvas);

    if (m_canvas) {
        // Pass canvas reference to widget, then scene
        m_configWidget->setCanvas(m_canvas);
    }
}

void VectorNodesDockerDock::unsetCanvas() {
    setCanvas(0);
}
