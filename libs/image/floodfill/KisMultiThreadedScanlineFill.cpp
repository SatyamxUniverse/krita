/*
 *  SPDX-FileCopyrightText: 2014 Dmitry Kazakov <dimula73@gmail.com>
 *  SPDX-FileCopyrightText: 2022 Deif Lou <ginoba@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisMultiThreadedScanlineFill.h"
#include "KisMultiThreadedScanlineFill/KisMultiThreadedScanlineFill_FillFunctions.h"
#include "KisMultiThreadedScanlineFill/KisMultiThreadedScanlineFill_SelectionPolicies.h"

#include <KoColor.h>
#include <KoColorSpace.h>
#include "kis_pixel_selection.h"

#include <KisFakeRunnableStrokeJobsExecutor.h>

struct Q_DECL_HIDDEN KisMultiThreadedScanlineFill::Private
{
    KisPaintDeviceSP referenceDevice;
    QPoint startPoint;
    QRect boundingRect;
    int threshold;
    int opacitySpread;
    KisRunnableStrokeJobsInterface *jobsInterface = 0;
};

KisMultiThreadedScanlineFill::KisMultiThreadedScanlineFill(KisPaintDeviceSP referenceDevice, const QPoint &startPoint, const QRect &boundingRect, KisRunnableStrokeJobsInterface *jobsInterface)
    : m_d(new Private)
{

    // TODO: remove and pass this responsibility to the caller!
    if (!jobsInterface) {
        qWarning() << "WARNING: Created leaking fake stroke jobs executor!";
        jobsInterface = new KisFakeRunnableStrokeJobsExecutor();
    }

    m_d->referenceDevice = referenceDevice;
    m_d->startPoint = startPoint;
    m_d->boundingRect = boundingRect;
    m_d->threshold = 0;
    m_d->opacitySpread = 0;
    m_d->jobsInterface = jobsInterface;
}

KisMultiThreadedScanlineFill::~KisMultiThreadedScanlineFill()
{}

void KisMultiThreadedScanlineFill::setThreshold(int threshold)
{
    m_d->threshold = threshold;
}

void KisMultiThreadedScanlineFill::setOpacitySpread(int opacitySpread)
{
    m_d->opacitySpread = opacitySpread;
}

void KisMultiThreadedScanlineFill::fill(const KoColor &originalFillColor)
{
    using namespace KisMultiThreadedScanlineFillNS;

    KoColor srcColor(m_d->referenceDevice->pixel(m_d->startPoint));
    KoColor fillColor(originalFillColor);
    fillColor.convertTo(m_d->referenceDevice->colorSpace());

    const int pixelSize = m_d->referenceDevice->pixelSize();

    if (pixelSize == 1) {
        HardSelectionPolicy<DifferencePolicyOptimized<quint8>>
            selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
        fillColorDevice(m_d->referenceDevice, fillColor, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
    } else if (pixelSize == 2) {
        HardSelectionPolicy<DifferencePolicyOptimized<quint16>>
            selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
        fillColorDevice(m_d->referenceDevice, fillColor, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
    } else if (pixelSize == 4) {
        HardSelectionPolicy<DifferencePolicyOptimized<quint32>>
            selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
        fillColorDevice(m_d->referenceDevice, fillColor, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
    } else if (pixelSize == 8) {
        HardSelectionPolicy<DifferencePolicyOptimized<quint64>>
            selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
        fillColorDevice(m_d->referenceDevice, fillColor, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
    } else {
        HardSelectionPolicy<DifferencePolicySlow>
            selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
        fillColorDevice(m_d->referenceDevice, fillColor, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
    }
}

void KisMultiThreadedScanlineFill::fillUntilColor(const KoColor &originalFillColor, const KoColor &boundaryColor)
{
    using namespace KisMultiThreadedScanlineFillNS;

    KoColor srcColor(boundaryColor);
    srcColor.convertTo(m_d->referenceDevice->colorSpace());
    KoColor fillColor(originalFillColor);
    fillColor.convertTo(m_d->referenceDevice->colorSpace());

    const int pixelSize = m_d->referenceDevice->pixelSize();

    if (pixelSize == 1) {
        SelectAllUntilColorHardSelectionPolicy<DifferencePolicyOptimized<quint8>>
            selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
        fillColorDevice(m_d->referenceDevice, fillColor, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
    } else if (pixelSize == 2) {
        SelectAllUntilColorHardSelectionPolicy<DifferencePolicyOptimized<quint16>>
            selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
        fillColorDevice(m_d->referenceDevice, fillColor, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
    } else if (pixelSize == 4) {
        SelectAllUntilColorHardSelectionPolicy<DifferencePolicyOptimized<quint32>>
            selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
        fillColorDevice(m_d->referenceDevice, fillColor, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
    } else if (pixelSize == 8) {
        SelectAllUntilColorHardSelectionPolicy<DifferencePolicyOptimized<quint64>>
            selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
        fillColorDevice(m_d->referenceDevice, fillColor, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
    } else {
        SelectAllUntilColorHardSelectionPolicy<DifferencePolicySlow>
            selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
        fillColorDevice(m_d->referenceDevice, fillColor, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
    }
}

void KisMultiThreadedScanlineFill::fill(const KoColor &originalFillColor, KisPaintDeviceSP externalDevice)
{
    using namespace KisMultiThreadedScanlineFillNS;

    KoColor srcColor(m_d->referenceDevice->pixel(m_d->startPoint));
    KoColor fillColor(originalFillColor);
    fillColor.convertTo(m_d->referenceDevice->colorSpace());

    const int pixelSize = m_d->referenceDevice->pixelSize();

    if (pixelSize == 1) {
        HardSelectionPolicy<DifferencePolicyOptimized<quint8>>
            selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
        fillExternalColorDevice(m_d->referenceDevice, externalDevice, fillColor, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
    } else if (pixelSize == 2) {
        HardSelectionPolicy<DifferencePolicyOptimized<quint16>>
            selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
        fillExternalColorDevice(m_d->referenceDevice, externalDevice, fillColor, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
    } else if (pixelSize == 4) {
        HardSelectionPolicy<DifferencePolicyOptimized<quint32>>
            selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
        fillExternalColorDevice(m_d->referenceDevice, externalDevice, fillColor, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
    } else if (pixelSize == 8) {
        HardSelectionPolicy<DifferencePolicyOptimized<quint64>>
            selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
        fillExternalColorDevice(m_d->referenceDevice, externalDevice, fillColor, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
    } else {
        HardSelectionPolicy<DifferencePolicySlow>
            selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
        fillExternalColorDevice(m_d->referenceDevice, externalDevice, fillColor, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
    }
}

void KisMultiThreadedScanlineFill::fillUntilColor(const KoColor &originalFillColor, const KoColor &boundaryColor, KisPaintDeviceSP externalDevice)
{
    using namespace KisMultiThreadedScanlineFillNS;

    KoColor srcColor(boundaryColor);
    srcColor.convertTo(m_d->referenceDevice->colorSpace());
    KoColor fillColor(originalFillColor);
    fillColor.convertTo(m_d->referenceDevice->colorSpace());

    const int pixelSize = m_d->referenceDevice->pixelSize();

    if (pixelSize == 1) {
        SelectAllUntilColorHardSelectionPolicy<DifferencePolicyOptimized<quint8>>
            selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
        fillExternalColorDevice(m_d->referenceDevice, externalDevice, fillColor, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
    } else if (pixelSize == 2) {
        SelectAllUntilColorHardSelectionPolicy<DifferencePolicyOptimized<quint16>>
            selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
        fillExternalColorDevice(m_d->referenceDevice, externalDevice, fillColor, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
    } else if (pixelSize == 4) {
        SelectAllUntilColorHardSelectionPolicy<DifferencePolicyOptimized<quint32>>
            selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
        fillExternalColorDevice(m_d->referenceDevice, externalDevice, fillColor, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
    } else if (pixelSize == 8) {
        SelectAllUntilColorHardSelectionPolicy<DifferencePolicyOptimized<quint64>>
            selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
        fillExternalColorDevice(m_d->referenceDevice, externalDevice, fillColor, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
    } else {
        SelectAllUntilColorHardSelectionPolicy<DifferencePolicySlow>
            selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
        fillExternalColorDevice(m_d->referenceDevice, externalDevice, fillColor, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
    }
}

void KisMultiThreadedScanlineFill::fillSelection(KisPixelSelectionSP pixelSelection, KisPaintDeviceSP originalBoundarySelection)
{
    using namespace KisMultiThreadedScanlineFillNS;

    KoColor srcColor(m_d->referenceDevice->pixel(m_d->startPoint));
    KisPixelSelectionSP boundarySelection = static_cast<KisPixelSelection*>(originalBoundarySelection.data());

    const int pixelSize = m_d->referenceDevice->pixelSize();
    const int softness = 100 - m_d->opacitySpread;

    if (softness == 0) {
        if (pixelSize == 1) {
            HardSelectionPolicy<DifferencePolicyOptimized<quint8>>
                selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
            fillMaskDevice(m_d->referenceDevice, pixelSelection, boundarySelection, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
        } else if (pixelSize == 2) {
            HardSelectionPolicy<DifferencePolicyOptimized<quint16>>
                selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
            fillMaskDevice(m_d->referenceDevice, pixelSelection, boundarySelection, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
        } else if (pixelSize == 4) {
            HardSelectionPolicy<DifferencePolicyOptimized<quint32>>
                selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
            fillMaskDevice(m_d->referenceDevice, pixelSelection, boundarySelection, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
        } else if (pixelSize == 8) {
            HardSelectionPolicy<DifferencePolicyOptimized<quint64>>
                selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
            fillMaskDevice(m_d->referenceDevice, pixelSelection, boundarySelection, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
        } else {
            HardSelectionPolicy<DifferencePolicySlow>
                selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
            fillMaskDevice(m_d->referenceDevice, pixelSelection, boundarySelection, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
        }
    } else {
        if (pixelSize == 1) {
            SoftSelectionPolicy<DifferencePolicyOptimized<quint8>>
                selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold, softness);
            fillMaskDevice(m_d->referenceDevice, pixelSelection, boundarySelection, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
        } else if (pixelSize == 2) {
            SoftSelectionPolicy<DifferencePolicyOptimized<quint16>>
                selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold, softness);
            fillMaskDevice(m_d->referenceDevice, pixelSelection, boundarySelection, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
        } else if (pixelSize == 4) {
            SoftSelectionPolicy<DifferencePolicyOptimized<quint32>>
                selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold, softness);
            fillMaskDevice(m_d->referenceDevice, pixelSelection, boundarySelection, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
        } else if (pixelSize == 8) {
            SoftSelectionPolicy<DifferencePolicyOptimized<quint64>>
                selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold, softness);
            fillMaskDevice(m_d->referenceDevice, pixelSelection, boundarySelection, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
        } else {
            SoftSelectionPolicy<DifferencePolicySlow>
                selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold, softness);
            fillMaskDevice(m_d->referenceDevice, pixelSelection, boundarySelection, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
        }
    }
}

void KisMultiThreadedScanlineFill::fillSelection(KisPixelSelectionSP pixelSelection)
{
    fillSelection(pixelSelection, nullptr);
}

void KisMultiThreadedScanlineFill::fillSelectionUntilColor(KisPixelSelectionSP pixelSelection, const KoColor &referenceColor, KisPaintDeviceSP originalBoundarySelection)
{
    using namespace KisMultiThreadedScanlineFillNS;

    KoColor srcColor(referenceColor);
    srcColor.convertTo(m_d->referenceDevice->colorSpace());

    KisPixelSelectionSP boundarySelection = static_cast<KisPixelSelection*>(originalBoundarySelection.data());

    const int pixelSize = m_d->referenceDevice->pixelSize();
    const int softness = 100 - m_d->opacitySpread;

    if (softness == 0) {
        if (pixelSize == 1) {
            SelectAllUntilColorHardSelectionPolicy<DifferencePolicyOptimized<quint8>>
                selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
            fillMaskDevice(m_d->referenceDevice, pixelSelection, boundarySelection, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
        } else if (pixelSize == 2) {
            SelectAllUntilColorHardSelectionPolicy<DifferencePolicyOptimized<quint16>>
                selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
            fillMaskDevice(m_d->referenceDevice, pixelSelection, boundarySelection, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
        } else if (pixelSize == 4) {
            SelectAllUntilColorHardSelectionPolicy<DifferencePolicyOptimized<quint32>>
                selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
            fillMaskDevice(m_d->referenceDevice, pixelSelection, boundarySelection, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
        } else if (pixelSize == 8) {
            SelectAllUntilColorHardSelectionPolicy<DifferencePolicyOptimized<quint64>>
                selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
            fillMaskDevice(m_d->referenceDevice, pixelSelection, boundarySelection, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
        } else {
            SelectAllUntilColorHardSelectionPolicy<DifferencePolicySlow>
                selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
            fillMaskDevice(m_d->referenceDevice, pixelSelection, boundarySelection, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
        }
    } else {
        if (pixelSize == 1) {
            SelectAllUntilColorSoftSelectionPolicy<DifferencePolicyOptimized<quint8>>
                selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold, softness);
            fillMaskDevice(m_d->referenceDevice, pixelSelection, boundarySelection, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
        } else if (pixelSize == 2) {
            SelectAllUntilColorSoftSelectionPolicy<DifferencePolicyOptimized<quint16>>
                selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold, softness);
            fillMaskDevice(m_d->referenceDevice, pixelSelection, boundarySelection, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
        } else if (pixelSize == 4) {
            SelectAllUntilColorSoftSelectionPolicy<DifferencePolicyOptimized<quint32>>
                selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold, softness);
            fillMaskDevice(m_d->referenceDevice, pixelSelection, boundarySelection, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
        } else if (pixelSize == 8) {
            SelectAllUntilColorSoftSelectionPolicy<DifferencePolicyOptimized<quint64>>
                selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold, softness);
            fillMaskDevice(m_d->referenceDevice, pixelSelection, boundarySelection, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
        } else {
            SelectAllUntilColorSoftSelectionPolicy<DifferencePolicySlow>
                selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold, softness);
            fillMaskDevice(m_d->referenceDevice, pixelSelection, boundarySelection, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
        }
    }
}

void KisMultiThreadedScanlineFill::fillSelectionUntilColor(KisPixelSelectionSP pixelSelection, const KoColor &referenceColor)
{
    fillSelectionUntilColor(pixelSelection, referenceColor, nullptr);
}

void KisMultiThreadedScanlineFill::fillSelectionUntilColorOrTransparent(KisPixelSelectionSP pixelSelection, const KoColor &referenceColor, KisPaintDeviceSP existingSelection)
{
    using namespace KisMultiThreadedScanlineFillNS;

    KoColor srcColor(referenceColor);
    srcColor.convertTo(m_d->referenceDevice->colorSpace());

    KisPixelSelectionSP boundarySelection = static_cast<KisPixelSelection*>(existingSelection.data());

    const int pixelSize = m_d->referenceDevice->pixelSize();
    const int softness = 100 - m_d->opacitySpread;

    if (softness == 0) {
        if (pixelSize == 1) {
            SelectAllUntilColorHardSelectionPolicy<ColorOrTransparentDifferencePolicyOptimized<quint8>>
                selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
            fillMaskDevice(m_d->referenceDevice, pixelSelection, boundarySelection, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
        } else if (pixelSize == 2) {
            SelectAllUntilColorHardSelectionPolicy<ColorOrTransparentDifferencePolicyOptimized<quint16>>
                selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
            fillMaskDevice(m_d->referenceDevice, pixelSelection, boundarySelection, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
        } else if (pixelSize == 4) {
            SelectAllUntilColorHardSelectionPolicy<ColorOrTransparentDifferencePolicyOptimized<quint32>>
                selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
            fillMaskDevice(m_d->referenceDevice, pixelSelection, boundarySelection, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
        } else if (pixelSize == 8) {
            SelectAllUntilColorHardSelectionPolicy<ColorOrTransparentDifferencePolicyOptimized<quint64>>
                selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
            fillMaskDevice(m_d->referenceDevice, pixelSelection, boundarySelection, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
        } else {
            SelectAllUntilColorHardSelectionPolicy<ColorOrTransparentDifferencePolicySlow>
                selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
            fillMaskDevice(m_d->referenceDevice, pixelSelection, boundarySelection, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
        }
    } else {
        if (pixelSize == 1) {
            SelectAllUntilColorSoftSelectionPolicy<ColorOrTransparentDifferencePolicyOptimized<quint8>>
                selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold, softness);
            fillMaskDevice(m_d->referenceDevice, pixelSelection, boundarySelection, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
        } else if (pixelSize == 2) {
            SelectAllUntilColorSoftSelectionPolicy<ColorOrTransparentDifferencePolicyOptimized<quint16>>
                selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold, softness);
            fillMaskDevice(m_d->referenceDevice, pixelSelection, boundarySelection, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
        } else if (pixelSize == 4) {
            SelectAllUntilColorSoftSelectionPolicy<ColorOrTransparentDifferencePolicyOptimized<quint32>>
                selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold, softness);
            fillMaskDevice(m_d->referenceDevice, pixelSelection, boundarySelection, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
        } else if (pixelSize == 8) {
            SelectAllUntilColorSoftSelectionPolicy<ColorOrTransparentDifferencePolicyOptimized<quint64>>
                selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold, softness);
            fillMaskDevice(m_d->referenceDevice, pixelSelection, boundarySelection, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
        } else {
            SelectAllUntilColorSoftSelectionPolicy<ColorOrTransparentDifferencePolicySlow>
                selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold, softness);
            fillMaskDevice(m_d->referenceDevice, pixelSelection, boundarySelection, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
        }
    }
}

void KisMultiThreadedScanlineFill::fillSelectionUntilColorOrTransparent(KisPixelSelectionSP pixelSelection, const KoColor &referenceColor)
{
    fillSelectionUntilColorOrTransparent(pixelSelection, referenceColor, nullptr);
}

void KisMultiThreadedScanlineFill::clearNonZeroComponent()
{
    using namespace KisMultiThreadedScanlineFillNS;

    const int pixelSize = m_d->referenceDevice->pixelSize();
    KoColor srcColor(Qt::transparent, m_d->referenceDevice->colorSpace());

    if (pixelSize == 1) {
        HardSelectionPolicy<IsNonNullPolicyOptimized<quint8>> selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
        fillColorDevice(m_d->referenceDevice, srcColor, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
    } else if (pixelSize == 2) {
        HardSelectionPolicy<IsNonNullPolicyOptimized<quint16>> selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
        fillColorDevice(m_d->referenceDevice, srcColor, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
    } else if (pixelSize == 4) {
        HardSelectionPolicy<IsNonNullPolicyOptimized<quint32>> selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
        fillColorDevice(m_d->referenceDevice, srcColor, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
    } else if (pixelSize == 8) {
        HardSelectionPolicy<IsNonNullPolicyOptimized<quint64>> selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
        fillColorDevice(m_d->referenceDevice, srcColor, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
    } else {
        HardSelectionPolicy<IsNonNullPolicySlow> selectionPolicy(m_d->referenceDevice, srcColor, m_d->threshold);
        fillColorDevice(m_d->referenceDevice, srcColor, m_d->boundingRect, m_d->startPoint, selectionPolicy, m_d->jobsInterface);
    }
}

void KisMultiThreadedScanlineFill::fillContiguousGroup(KisPaintDeviceSP groupMapDevice, qint32 groupIndex)
{
    KIS_SAFE_ASSERT_RECOVER_RETURN(m_d->referenceDevice->pixelSize() == 1);
    KIS_SAFE_ASSERT_RECOVER_RETURN(groupMapDevice->pixelSize() == 4);

    const quint8 referenceValue = *m_d->referenceDevice->pixel(m_d->startPoint).data();

    KisMultiThreadedScanlineFillNS::fillContiguousGroup(m_d->referenceDevice, groupMapDevice, groupIndex,
                                                        referenceValue, m_d->threshold, m_d->boundingRect, m_d->startPoint, m_d->jobsInterface);
}
