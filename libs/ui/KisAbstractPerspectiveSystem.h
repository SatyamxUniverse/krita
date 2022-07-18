/*
 * This file is part of Krita
 *
 *  SPDX-FileCopyrightText: 2022 Agata Cacko <cacko.azh@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef KIS_ABSTRACT_PERSPECTIVE_SYSTEM_H
#define KIS_ABSTRACT_PERSPECTIVE_SYSTEM_H

#include <QPointF>
#include <QObject>

#include "kritaui_export.h"

class KRITAUI_EXPORT KisAbstractPerspectiveSystem
{
public:

    KisAbstractPerspectiveSystem() {}

    virtual ~KisAbstractPerspectiveSystem() {}

	// returns a list of the horizon lines in the perspective system
    virtual QList<QLineF> additionalHorizonLines() = 0;
    // returns a list of the vanishing points in the perspective system
    virtual QList<QPointF> vanishingPoints() = 0;
};

#endif
