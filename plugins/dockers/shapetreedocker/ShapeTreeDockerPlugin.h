/*
 *  SPDX-FileCopyrightText: 2023 Alvin Wong <alvin@alvinhc.com>
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SHAPE_TREE_DOCKER_PLUGIN_H
#define SHAPE_TREE_DOCKER_PLUGIN_H

#include <QObject>
#include <QVariant>

class ShapeTreeDockerPlugin : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ShapeTreeDockerPlugin)

public:
    ShapeTreeDockerPlugin(QObject *parent, const QVariantList &);
    ~ShapeTreeDockerPlugin() override;
};

#endif /* SHAPE_TREE_DOCKER_PLUGIN_H */
