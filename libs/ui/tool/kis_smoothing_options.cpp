/*
 *  SPDX-FileCopyrightText: 2012 Boudewijn Rempt <boud@valdyas.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "kis_smoothing_options.h"

#include "kis_config.h"
#include "kis_signal_compressor.h"

#include <iostream>

struct KisSmoothingOptions::Private {

    Private()
        : writeCompressor(500, KisSignalCompressor::FIRST_ACTIVE)
    {

    }
    ~Private()
    {
        
    }

    virtual void saveConfig() {}

    KisSignalCompressor writeCompressor;

    SmoothingType smoothingType;
    qreal smoothnessDistance;
    qreal tailAggressiveness;
    bool smoothPressure;
    bool useScalableDistance;
    qreal delayDistance;
    bool useDelayDistance;
    bool finishStabilizedCurve;
    bool stabilizeSensors;
};

struct KisSmoothingOptions::Private_line : public KisSmoothingOptions::Private {
    Private_line(bool useSavedSmoothing)
        : Private()
    {
        loadFromConfig(useSavedSmoothing);
    }
    void loadFromConfig(bool useSavedSmoothing)
    {
        KisConfig cfg(true);
        smoothingType = (SmoothingType)cfg.lineSmoothingType(!useSavedSmoothing);
        smoothnessDistance = cfg.lineSmoothingDistance(!useSavedSmoothing);
        tailAggressiveness = cfg.lineSmoothingTailAggressiveness(!useSavedSmoothing);
        smoothPressure = cfg.lineSmoothingSmoothPressure(!useSavedSmoothing);
        useScalableDistance = cfg.lineSmoothingScalableDistance(!useSavedSmoothing);
        delayDistance = cfg.lineSmoothingDelayDistance(!useSavedSmoothing);
        useDelayDistance = cfg.lineSmoothingUseDelayDistance(!useSavedSmoothing);
        finishStabilizedCurve = cfg.lineSmoothingFinishStabilizedCurve(!useSavedSmoothing);
        stabilizeSensors = cfg.lineSmoothingStabilizeSensors(!useSavedSmoothing);
    }
    void saveConfig() 
    {
        std::cout<<"Save line smoothing options\n";
        KisConfig cfg(false);
        cfg.setLineSmoothingType(smoothingType);
        cfg.setLineSmoothingDistance(smoothnessDistance);
        cfg.setLineSmoothingTailAggressiveness(tailAggressiveness);
        cfg.setLineSmoothingSmoothPressure(smoothPressure);
        cfg.setLineSmoothingScalableDistance(useScalableDistance);
        cfg.setLineSmoothingDelayDistance(delayDistance);
        cfg.setLineSmoothingUseDelayDistance(useDelayDistance);
        cfg.setLineSmoothingFinishStabilizedCurve(finishStabilizedCurve);
        cfg.setLineSmoothingStabilizeSensors(stabilizeSensors);
    }

};

struct KisSmoothingOptions::Private_eraser : public KisSmoothingOptions::Private {
    Private_eraser(bool useSavedSmoothing)
        : Private()
    {
        loadFromConfig(useSavedSmoothing);
    }

    void loadFromConfig(bool useSavedSmoothing)
    {
        KisConfig cfg(true);
        smoothingType = (SmoothingType)cfg.eraserSmoothingType(!useSavedSmoothing);
        smoothnessDistance = cfg.eraserSmoothingDistance(!useSavedSmoothing);
        tailAggressiveness = cfg.eraserSmoothingTailAggressiveness(!useSavedSmoothing);
        smoothPressure = cfg.eraserSmoothingSmoothPressure(!useSavedSmoothing);
        useScalableDistance = cfg.eraserSmoothingScalableDistance(!useSavedSmoothing);
        delayDistance = cfg.eraserSmoothingDelayDistance(!useSavedSmoothing);
        useDelayDistance = cfg.eraserSmoothingUseDelayDistance(!useSavedSmoothing);
        finishStabilizedCurve = cfg.eraserSmoothingFinishStabilizedCurve(!useSavedSmoothing);
        stabilizeSensors = cfg.eraserSmoothingStabilizeSensors(!useSavedSmoothing);
    }
    void saveConfig() 
    {
        std::cout<<"Save eraser smoothing options\n";
        KisConfig cfg(false);
        cfg.setEraserSmoothingType(smoothingType);
        cfg.setEraserSmoothingDistance(smoothnessDistance);
        cfg.setEraserSmoothingTailAggressiveness(tailAggressiveness);
        cfg.setEraserSmoothingSmoothPressure(smoothPressure);
        cfg.setEraserSmoothingScalableDistance(useScalableDistance);
        cfg.setEraserSmoothingDelayDistance(delayDistance);
        cfg.setEraserSmoothingUseDelayDistance(useDelayDistance);
        cfg.setEraserSmoothingFinishStabilizedCurve(finishStabilizedCurve);
        cfg.setEraserSmoothingStabilizeSensors(stabilizeSensors);
    }
};

KisSmoothingOptions::KisSmoothingOptions(bool forEraser, bool useSavedSmoothing)
{
    if(forEraser) {
        m_d.reset(new Private_eraser(useSavedSmoothing));
    } else {
        m_d.reset(new Private_line(useSavedSmoothing));
    }
    connect(&m_d->writeCompressor, SIGNAL(timeout()), this, SLOT(slotWriteConfig()));
}

KisSmoothingOptions::~KisSmoothingOptions()
{
}

KisSmoothingOptions::SmoothingType KisSmoothingOptions::smoothingType() const
{
    return m_d->smoothingType;
}

void KisSmoothingOptions::setSmoothingType(KisSmoothingOptions::SmoothingType value)
{
    m_d->smoothingType = value;
    emit sigSmoothingTypeChanged();
    m_d->writeCompressor.start();
}

qreal KisSmoothingOptions::smoothnessDistance() const
{
    return m_d->smoothnessDistance;
}

void KisSmoothingOptions::setSmoothnessDistance(qreal value)
{
    m_d->smoothnessDistance = value;
    m_d->writeCompressor.start();
}

qreal KisSmoothingOptions::tailAggressiveness() const
{
    return m_d->tailAggressiveness;
}

void KisSmoothingOptions::setTailAggressiveness(qreal value)
{
    m_d->tailAggressiveness = value;
    m_d->writeCompressor.start();
}

bool KisSmoothingOptions::smoothPressure() const
{
    return m_d->smoothPressure;
}

void KisSmoothingOptions::setSmoothPressure(bool value)
{
    m_d->smoothPressure = value;
    m_d->writeCompressor.start();
}

bool KisSmoothingOptions::useScalableDistance() const
{
    return m_d->smoothingType != STABILIZER ? m_d->useScalableDistance : true;
}

void KisSmoothingOptions::setUseScalableDistance(bool value)
{
    // stabilizer mush always have scalable distance on
    // see bug 421314
    KIS_SAFE_ASSERT_RECOVER_RETURN(m_d->smoothingType != STABILIZER);

    m_d->useScalableDistance = value;
    m_d->writeCompressor.start();
}

qreal KisSmoothingOptions::delayDistance() const
{
    return m_d->delayDistance;
}

void KisSmoothingOptions::setDelayDistance(qreal value)
{
    m_d->delayDistance = value;
    m_d->writeCompressor.start();
}

bool KisSmoothingOptions::useDelayDistance() const
{
    return m_d->useDelayDistance;
}

void KisSmoothingOptions::setUseDelayDistance(bool value)
{
    m_d->useDelayDistance = value;
    m_d->writeCompressor.start();
}

void KisSmoothingOptions::setFinishStabilizedCurve(bool value)
{
    m_d->finishStabilizedCurve = value;
    m_d->writeCompressor.start();
}

bool KisSmoothingOptions::finishStabilizedCurve() const
{
    return m_d->finishStabilizedCurve;
}

void KisSmoothingOptions::setStabilizeSensors(bool value)
{
    m_d->stabilizeSensors = value;
    m_d->writeCompressor.start();
}

bool KisSmoothingOptions::stabilizeSensors() const
{
    return m_d->stabilizeSensors;
}

void KisSmoothingOptions::slotWriteConfig()
{
    m_d->saveConfig();
}
