#include "vectornodesdocker.h"

#include <kis_debug.h>
#include <kpluginfactory.h>
#include <klocalizedstring.h>

#include <KoDockFactoryBase.h>
#include <KoDockRegistry.h>
#include "KisViewManager.h"

#include "vectornodesdocker_dock.h"

K_PLUGIN_FACTORY_WITH_JSON(VectorNodesDockerPluginFactory, "krita_vectornodesdocker.json", registerPlugin<VectorNodesDockerPlugin>();)

class VectorNodesDockerDockFactory : public KoDockFactoryBase {
public:
    VectorNodesDockerDockFactory() {
    }

    QString id() const override {
        return QString( "VectorNodesDocker" );
    }

    virtual Qt::DockWidgetArea defaultDockWidgetArea() const {
        return Qt::RightDockWidgetArea;
    }

    QDockWidget* createDockWidget() override {
        VectorNodesDockerDock * dockWidget = new VectorNodesDockerDock();
        dockWidget->setObjectName(id());

        return dockWidget;
    }

    DockPosition defaultDockPosition() const override {
        return DockMinimized;
    }
private:

};


VectorNodesDockerPlugin::VectorNodesDockerPlugin(QObject *parent, const QVariantList &) : QObject(parent) {
    KoDockRegistry::instance()->add(new VectorNodesDockerDockFactory());
}

VectorNodesDockerPlugin::~VectorNodesDockerPlugin() {
    m_view = 0;
}

#include "vectornodesdocker.moc"
