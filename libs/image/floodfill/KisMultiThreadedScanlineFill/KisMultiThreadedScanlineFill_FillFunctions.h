/*
 *  SPDX-FileCopyrightText: 2022 Deif Lou <ginoba@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KISMULTITHREADEDSCANLINEFILL_FILLFUNCTIONS_H
#define KISMULTITHREADEDSCANLINEFILL_FILLFUNCTIONS_H

#include <kis_default_bounds.h>

#include "KisMultiThreadedScanlineFill_Filler.h"
#include "KisMultiThreadedScanlineFill_SelectionPolicies.h"

namespace KisMultiThreadedScanlineFillNS
{

template <typename SelectionPolicyType_>
void fillColorDevice(KisPaintDeviceSP referenceDevice,
                     const KoColor &fillColor,
                     const QRect &workingRect,
                     const QPoint &startPoint,
                     const SelectionPolicyType_ &selectionPolicy)
{
    KIS_SAFE_ASSERT_RECOVER_RETURN(referenceDevice);
    KIS_SAFE_ASSERT_RECOVER_RETURN(workingRect.contains(startPoint));

    KisPixelSelectionSP maskDevice = new KisPixelSelection(new KisSelectionDefaultBounds(referenceDevice));
    maskDevice->moveTo(referenceDevice->offset());

    Filler<SelectionPolicyType_, WriteToReferenceDeviceTilePolicy<SelectionPolicyType_>>
        filler(referenceDevice, nullptr, maskDevice, nullptr, fillColor, workingRect, selectionPolicy);
    filler.fill(startPoint);
}

template <typename SelectionPolicyType_>
void fillExternalColorDevice(KisPaintDeviceSP referenceDevice,
                             KisPaintDeviceSP externalDevice,
                             const KoColor &fillColor,
                             const QRect &workingRect,
                             const QPoint &startPoint,
                             const SelectionPolicyType_ &selectionPolicy)
{
    KIS_SAFE_ASSERT_RECOVER_RETURN(referenceDevice);
    KIS_SAFE_ASSERT_RECOVER_RETURN(externalDevice);
    KIS_SAFE_ASSERT_RECOVER_RETURN(workingRect.contains(startPoint));

    KisPixelSelectionSP maskDevice = new KisPixelSelection(new KisSelectionDefaultBounds(referenceDevice));
    maskDevice->moveTo(referenceDevice->offset());

    const QPoint tileGridOffset(referenceDevice->x() % tileSize.width(), referenceDevice->y() % tileSize.height());

    const bool externalDeviceIsAligned =
        QPoint(externalDevice->x() % tileSize.width(), externalDevice->y() % tileSize.height()) == tileGridOffset;

    if (externalDeviceIsAligned) {
        Filler<SelectionPolicyType_, WriteToAlignedExternalDeviceTilePolicy<SelectionPolicyType_>>
            filler(referenceDevice, externalDevice, maskDevice, nullptr, fillColor, workingRect, selectionPolicy);
        filler.fill(startPoint);
    } else {
        Filler<SelectionPolicyType_, WriteToUnalignedExternalDeviceTilePolicy<SelectionPolicyType_>>
            filler(referenceDevice, externalDevice, maskDevice, nullptr, fillColor, workingRect, selectionPolicy);
        filler.fill(startPoint);
    }
}

template <typename SelectionPolicyType_>
void fillMaskDevice(KisPaintDeviceSP referenceDevice,
                    KisPixelSelectionSP maskDevice,
                    KisPixelSelectionSP boundarySelectionDevice,
                    const QRect &workingRect,
                    const QPoint &startPoint,
                    SelectionPolicyType_ &selectionPolicy)
{
    KIS_SAFE_ASSERT_RECOVER_RETURN(referenceDevice);
    KIS_SAFE_ASSERT_RECOVER_RETURN(maskDevice);
    KIS_SAFE_ASSERT_RECOVER_RETURN(workingRect.contains(startPoint));

    const QPoint tileGridOffset(referenceDevice->x() % tileSize.width(), referenceDevice->y() % tileSize.height());

    const bool maskDeviceIsAligned =
        QPoint(maskDevice->x() % tileSize.width(), maskDevice->y() % tileSize.height()) == tileGridOffset;

    const bool useBoundarySelectionDevice = boundarySelectionDevice;
    const bool boundarySelectionDeviceIsAligned =
        useBoundarySelectionDevice &&
        QPoint(boundarySelectionDevice->x() % tileSize.width(), boundarySelectionDevice->y() % tileSize.height()) == tileGridOffset;

    if (useBoundarySelectionDevice) {
        if (boundarySelectionDeviceIsAligned) {
            if (maskDeviceIsAligned) {
                Filler<SelectionPolicyType_, WriteToAlignedMaskDeviceWithAlignedBoundarySelectionDeviceTilePolicy<SelectionPolicyType_>>
                    filler(referenceDevice, nullptr, maskDevice, boundarySelectionDevice, KoColor(), workingRect, selectionPolicy);
                filler.fill(startPoint);
            } else {
                Filler<SelectionPolicyType_, WriteToUnalignedMaskDeviceWithAlignedBoundarySelectionDeviceTilePolicy<SelectionPolicyType_>>
                    filler(referenceDevice, nullptr, maskDevice, boundarySelectionDevice, KoColor(), workingRect, selectionPolicy);
                filler.fill(startPoint);
            }
        } else {
            if (maskDeviceIsAligned) {
                Filler<SelectionPolicyType_, WriteToAlignedMaskDeviceWithUnalignedBoundarySelectionDeviceTilePolicy<SelectionPolicyType_>>
                    filler(referenceDevice, nullptr, maskDevice, boundarySelectionDevice, KoColor(), workingRect, selectionPolicy);
                filler.fill(startPoint);
            } else {
                Filler<SelectionPolicyType_, WriteToUnalignedMaskDeviceWithUnalignedBoundarySelectionDeviceTilePolicy<SelectionPolicyType_>>
                    filler(referenceDevice, nullptr, maskDevice, boundarySelectionDevice, KoColor(), workingRect, selectionPolicy);
                filler.fill(startPoint);
            }
        }
    } else {
        if (maskDeviceIsAligned) {
            Filler<SelectionPolicyType_, WriteToAlignedMaskDeviceTilePolicy<SelectionPolicyType_>>
                filler(referenceDevice, nullptr, maskDevice, nullptr, KoColor(), workingRect, selectionPolicy);
            filler.fill(startPoint);
        } else {
            Filler<SelectionPolicyType_, WriteToUnalignedMaskDeviceTilePolicy<SelectionPolicyType_>>
                filler(referenceDevice, nullptr, maskDevice, nullptr, KoColor(), workingRect, selectionPolicy);
            filler.fill(startPoint);
        }
    }
}

void fillContiguousGroup(KisPaintDeviceSP scribbleDevice,
                         KisPaintDeviceSP groupMapDevice,
                         qint32 groupIndex,
                         quint8 referenceValue,
                         qint32 threshold,
                         const QRect &workingRect,
                         const QPoint &startPoint)
{
    KIS_SAFE_ASSERT_RECOVER_RETURN(scribbleDevice);
    KIS_SAFE_ASSERT_RECOVER_RETURN(groupMapDevice);
    KIS_SAFE_ASSERT_RECOVER_RETURN(workingRect.contains(startPoint));

    KisPixelSelectionSP maskDevice = new KisPixelSelection(new KisSelectionDefaultBounds(scribbleDevice));
    maskDevice->moveTo(scribbleDevice->offset());

    const QPoint tileGridOffset(scribbleDevice->x() % tileSize.width(), scribbleDevice->y() % tileSize.height());

    const bool groupMapDeviceIsAligned =
        QPoint(groupMapDevice->x() % tileSize.width(), groupMapDevice->y() % tileSize.height()) == tileGridOffset;

    GroupSplitSelectionPolicy selectionPolicy(referenceValue, threshold);

    if (groupMapDeviceIsAligned) {
        using FillerType = Filler<GroupSplitSelectionPolicy, AlignedGroupSplitTilePolicy<GroupSplitSelectionPolicy>>;
        FillerType filler(scribbleDevice, groupMapDevice, maskDevice, nullptr, KoColor(), workingRect, selectionPolicy);
        for (FillerType::PoolEntry &poolEntry : filler.m_pool) {
            poolEntry.tilePolicy.setGroupIndex(groupIndex);
        }
        filler.fill(startPoint);
    } else {
        using FillerType = Filler<GroupSplitSelectionPolicy, UnalignedGroupSplitTilePolicy<GroupSplitSelectionPolicy>>;
        FillerType filler(scribbleDevice, groupMapDevice, maskDevice, nullptr, KoColor(), workingRect, selectionPolicy);
        for (FillerType::PoolEntry &poolEntry : filler.m_pool) {
            poolEntry.tilePolicy.setGroupIndex(groupIndex);
        }
        filler.fill(startPoint);
    }
}

}

#endif
