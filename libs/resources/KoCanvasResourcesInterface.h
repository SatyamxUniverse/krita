/*
 *  SPDX-FileCopyrightText: 2020 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KOCANVASRESOURCESINTERFACE_H
#define KOCANVASRESOURCESINTERFACE_H

#include "kritaresources_export.h"
#include <QSharedPointer>
#include <QObject>

class QVariant;

#include <kritaresources_export.h>

/**
 * @brief An abstract class for providing access to canvas resources
 * like current gradient and Fg/Bg colors.
 *
 * Specific implementations may forward the requests either to
 * KoCanvasResourceProvider or to a local storage.
 */
class KRITARESOURCES_EXPORT KoCanvasResourcesInterface : public QObject
{
    Q_OBJECT
public:
    virtual ~KoCanvasResourcesInterface();

    virtual QVariant resource(int key) const = 0;

Q_SIGNALS:
    /**
     * This signal is emitted every time a resource is set that is either
     * new or different from the previous set value.
     * @param key the identifying key for the resource
     * @param value the variants new value.
     * @see KoCanvasResource::CanvasResourceId
     */
    void canvasResourceChanged(int key, const QVariant &value);
};

using KoCanvasResourcesInterfaceSP = QSharedPointer<KoCanvasResourcesInterface>;

#endif // KOCANVASRESOURCESINTERFACE_H
