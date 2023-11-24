/*
 *  SPDX-FileCopyrightText: 2023 Alvin Wong <alvin@alvinhc.com>
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ShapeTreeDockerPlugin.h"

#include <kpluginfactory.h>
#include <klocalizedstring.h>

#include <KoDockFactoryBase.h>
#include <KoDockRegistry.h>

#include "ShapeTreeDockerDock.h"

K_PLUGIN_FACTORY_WITH_JSON(ShapeTreeDockerPluginFactory,
                           "krita_shapetreedocker.json",
                           registerPlugin<ShapeTreeDockerPlugin>();)

class ShapeTreeDockerDockFactory : public KoDockFactoryBase {
    Q_DISABLE_COPY_MOVE(ShapeTreeDockerDockFactory)

public:
    ShapeTreeDockerDockFactory() = default;
    ~ShapeTreeDockerDockFactory() override = default;

    QString id() const override
    {
        return QLatin1String("ShapeTreeDocker");
    }

    // Qt::DockWidgetArea defaultDockWidgetArea() const
    // {
    //     return Qt::RightDockWidgetArea;
    // }

    QDockWidget* createDockWidget() override
    {
        ShapeTreeDockerDock *dockWidget = new ShapeTreeDockerDock();
        dockWidget->setObjectName(id());
        dockWidget->setProperty("ShowOnWelcomePage", false);
        return dockWidget;
    }

    DockPosition defaultDockPosition() const override
    {
        return DockMinimized;
    }
};


ShapeTreeDockerPlugin::ShapeTreeDockerPlugin(QObject *parent, const QVariantList &)
    : QObject(parent)
{
    KoDockRegistry::instance()->add(new ShapeTreeDockerDockFactory());
}

ShapeTreeDockerPlugin::~ShapeTreeDockerPlugin() = default;

#include "ShapeTreeDockerPlugin.moc"
