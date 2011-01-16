/* This file is part of the KDE project
 * Copyright (C) Boudewijn Rempt <boud@valdyas.org>, (C) 2008
 * Copyright (C) Silvio Heinrich <plassy@web.de>, (C) 2011
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef KIS_PRESSURE_RATE_OPTION_H
#define KIS_PRESSURE_RATE_OPTION_H

#include "kis_curve_option.h"
#include <kis_paint_information.h>
#include <krita_export.h>
#include <kis_types.h>

class QSlider;
class KisPropertiesConfiguration;
class KisPainter;

/**
 * The pressure opacity option defines a curve that is used to
 * calculate the effect of pressure on the rate of the dab
 */
class PAINTOP_EXPORT KisPressureRateOption : public KisCurveOption
{
public:
    KisPressureRateOption(const QString& name, const QString& label="", bool checked=true, const QString& category=KisPaintOpOption::brushCategory());

    /**
     * Set the opacity of the painter based on the rate
     * and the curve (if checked)
     */
    void apply(KisPainter* painter, const KisPaintInformation& info, qreal scaleMin=0.0, qreal scaleMax=1.0) const;

    void writeOptionSetting(KisPropertiesConfiguration* setting) const;
    void readOptionSetting(const KisPropertiesConfiguration* setting);

    void setRate(qreal rate) { m_rate = qBound<qreal>(0.0, rate, 1.0); }
    qreal getRate() const { return m_rate; }
    
private:
    qreal m_rate;
};

#endif
