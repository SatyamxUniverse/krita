/*
 *  This file is part of the KDE project
 *
 *  SPDX-FileCopyrightText: 2006 Cyrille Berger <cberger@cberger.net>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_random_sub_accessor.h"
#include <QtGlobal>

#include <KoColorSpace.h>
#include <KoMixColorsOp.h>
#include <QtMath>

#include "kis_paint_device.h"

KisRandomSubAccessor::KisRandomSubAccessor(KisPaintDeviceSP device)
        : m_device(device)
        , m_currentPoint(0, 0)
        , m_randomAccessor(device->createRandomConstAccessorNG())
{
}


KisRandomSubAccessor::~KisRandomSubAccessor()
{
}


void KisRandomSubAccessor::sampledOldRawData(quint8* dst)
{
    const quint8* pixels[4];
    qint16 weights[4];
    int sumOfWeights = 0;
    const QPointF currentPoint = m_currentPoint - QPointF(0.5, 0.5);
    const int x = qFloor(currentPoint.x());
    const int y = qFloor(currentPoint.y());
    const double hsub = currentPoint.x() - x;
    const double vsub = currentPoint.y() - y;
    
    weights[0] = qRound((1.0 - hsub) * (1.0 - vsub) * 255);
    sumOfWeights += weights[0];
    m_randomAccessor->moveTo(x, y);
    pixels[0] = m_randomAccessor->oldRawData();
    weights[1] = qRound((1.0 - vsub) * hsub * 255);
    sumOfWeights += weights[1];
    m_randomAccessor->moveTo(x + 1, y);
    pixels[1] = m_randomAccessor->oldRawData();
    weights[2] = qRound(vsub * (1.0 - hsub) * 255);
    sumOfWeights += weights[2];
    m_randomAccessor->moveTo(x, y + 1);
    pixels[2] = m_randomAccessor->oldRawData();
    weights[3] = qRound(hsub * vsub * 255);
    sumOfWeights += weights[3];
    m_randomAccessor->moveTo(x + 1, y + 1);
    pixels[3] = m_randomAccessor->oldRawData();

    m_device->colorSpace()->mixColorsOp()->mixColors(pixels, weights, 4, dst, sumOfWeights);
}


void KisRandomSubAccessor::sampledRawData(quint8* dst)
{
    const quint8* pixels[4];
    qint16 weights[4];
    int sumOfWeights = 0;
    const QPointF currentPoint = m_currentPoint - QPointF(0.5, 0.5);
    const int x = qFloor(currentPoint.x());
    const int y = qFloor(currentPoint.y());
    const double hsub = currentPoint.x() - x;
    const double vsub = currentPoint.y() - y;
    
    weights[0] = qRound((1.0 - hsub) * (1.0 - vsub) * 255);
    sumOfWeights += weights[0];
    m_randomAccessor->moveTo(x, y);
    pixels[0] = m_randomAccessor->rawDataConst();
    weights[1] = qRound((1.0 - vsub) * hsub * 255);
    sumOfWeights += weights[1];
    m_randomAccessor->moveTo(x + 1, y);
    pixels[1] = m_randomAccessor->rawDataConst();
    weights[2] = qRound(vsub * (1.0 - hsub) * 255);
    sumOfWeights += weights[2];
    m_randomAccessor->moveTo(x, y + 1);
    pixels[2] = m_randomAccessor->rawDataConst();
    weights[3] = qRound(hsub * vsub * 255);
    sumOfWeights += weights[3];
    m_randomAccessor->moveTo(x + 1, y + 1);
    pixels[3] = m_randomAccessor->rawDataConst();
    m_device->colorSpace()->mixColorsOp()->mixColors(pixels, weights, 4, dst, sumOfWeights);
}
