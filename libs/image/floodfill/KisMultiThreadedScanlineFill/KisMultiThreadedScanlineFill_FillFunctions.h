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

template <typename TilePolicyType_>
struct TilePolicyFactory
{
    using TilePolicyType = TilePolicyType_;
    using SelectionPolicyType = typename TilePolicyType::SelectionPolicyType;

    TilePolicyType operator() () {
        return TilePolicyType(m_fillColor, m_selectionPolicy);
    }

    TilePolicyFactory(const KoColor &color, const SelectionPolicyType &selectionPolicy)
        : m_fillColor(color)
        , m_selectionPolicy(selectionPolicy)
    {}

    KoColor m_fillColor;
    SelectionPolicyType m_selectionPolicy;
};

template <typename SelectionPolicyType_>
void fillColorDevice(KisPaintDeviceSP referenceDevice,
                     const KoColor &fillColor,
                     const QRect &workingRect,
                     const QPoint &startPoint,
                     const SelectionPolicyType_ &selectionPolicy,
                     KisRunnableStrokeJobsInterface *jobsInterface)
{
    KIS_SAFE_ASSERT_RECOVER_RETURN(referenceDevice);
    KIS_SAFE_ASSERT_RECOVER_RETURN(workingRect.contains(startPoint));

    KisPixelSelectionSP maskDevice = new KisPixelSelection(new KisSelectionDefaultBounds(referenceDevice));
    maskDevice->moveTo(referenceDevice->offset());

    TilePolicyFactory<WriteToReferenceDeviceTilePolicy<SelectionPolicyType_>> tilePolicy(fillColor, selectionPolicy);
    startFillProcess(startPoint, referenceDevice, nullptr, maskDevice, nullptr, workingRect, tilePolicy, jobsInterface);
}

template <typename SelectionPolicyType_>
void fillExternalColorDevice(KisPaintDeviceSP referenceDevice,
                             KisPaintDeviceSP externalDevice,
                             const KoColor &fillColor,
                             const QRect &workingRect,
                             const QPoint &startPoint,
                             const SelectionPolicyType_ &selectionPolicy,
                             KisRunnableStrokeJobsInterface *jobsInterface)
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
        TilePolicyFactory<WriteToAlignedExternalDeviceTilePolicy<SelectionPolicyType_>> tilePolicy(fillColor, selectionPolicy);
        startFillProcess(startPoint, referenceDevice, externalDevice, maskDevice, nullptr, workingRect, tilePolicy, jobsInterface);
    } else {
        TilePolicyFactory<WriteToUnalignedExternalDeviceTilePolicy<SelectionPolicyType_>> tilePolicy(fillColor, selectionPolicy);
        startFillProcess(startPoint, referenceDevice, externalDevice, maskDevice, nullptr, workingRect, tilePolicy, jobsInterface);
    }
}

template <typename SelectionPolicyType_>
void fillMaskDevice(KisPaintDeviceSP referenceDevice,
                    KisPixelSelectionSP maskDevice,
                    KisPixelSelectionSP boundarySelectionDevice,
                    const QRect &workingRect,
                    const QPoint &startPoint,
                    SelectionPolicyType_ &selectionPolicy,
                    KisRunnableStrokeJobsInterface *jobsInterface)
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
                TilePolicyFactory<WriteToAlignedMaskDeviceWithAlignedBoundarySelectionDeviceTilePolicy<SelectionPolicyType_>> tilePolicy(KoColor(), selectionPolicy);
                startFillProcess(startPoint, referenceDevice, nullptr, maskDevice, boundarySelectionDevice, workingRect, tilePolicy, jobsInterface);
            } else {
                TilePolicyFactory<WriteToUnalignedMaskDeviceWithAlignedBoundarySelectionDeviceTilePolicy<SelectionPolicyType_>> tilePolicy(KoColor(), selectionPolicy);
                startFillProcess(startPoint, referenceDevice, nullptr, maskDevice, boundarySelectionDevice, workingRect, tilePolicy, jobsInterface);
            }
        } else {
            if (maskDeviceIsAligned) {
                TilePolicyFactory<WriteToAlignedMaskDeviceWithUnalignedBoundarySelectionDeviceTilePolicy<SelectionPolicyType_>> tilePolicy(KoColor(), selectionPolicy);
                startFillProcess(startPoint, referenceDevice, nullptr, maskDevice, boundarySelectionDevice, workingRect, tilePolicy, jobsInterface);
            } else {
                TilePolicyFactory<WriteToUnalignedMaskDeviceWithUnalignedBoundarySelectionDeviceTilePolicy<SelectionPolicyType_>> tilePolicy(KoColor(), selectionPolicy);
                startFillProcess(startPoint, referenceDevice, nullptr, maskDevice, boundarySelectionDevice, workingRect, tilePolicy, jobsInterface);
            }
        }
    } else {
        if (maskDeviceIsAligned) {
            TilePolicyFactory<WriteToAlignedMaskDeviceTilePolicy<SelectionPolicyType_>> tilePolicy(KoColor(), selectionPolicy);
            startFillProcess(startPoint, referenceDevice, nullptr, maskDevice, nullptr, workingRect, tilePolicy, jobsInterface);
        } else {
            TilePolicyFactory<WriteToUnalignedMaskDeviceTilePolicy<SelectionPolicyType_>> tilePolicy(KoColor(), selectionPolicy);
            startFillProcess(startPoint, referenceDevice, nullptr, maskDevice, nullptr, workingRect, tilePolicy, jobsInterface);
        }
    }
}

template <typename TilePolicyType_>
struct GroupSplitTilePolicyFactory : TilePolicyFactory<TilePolicyType_>
{
    using TilePolicyType = typename TilePolicyFactory<TilePolicyType_>::TilePolicyType;
    using SelectionPolicyType = typename TilePolicyFactory<TilePolicyType_>::SelectionPolicyType;

    TilePolicyType operator() () {
        TilePolicyType tilePolicy =
            TilePolicyFactory<TilePolicyType_>::operator()();
        tilePolicy.setGroupIndex(this->m_groupIndex);
        return tilePolicy;
    }

    GroupSplitTilePolicyFactory(const KoColor &color, const SelectionPolicyType &selectionPolicy, int groupIndex)
        : TilePolicyFactory<TilePolicyType_>(color, selectionPolicy)
        , m_groupIndex(groupIndex)
    {}

    int m_groupIndex;
};


void fillContiguousGroup(KisPaintDeviceSP scribbleDevice,
                         KisPaintDeviceSP groupMapDevice,
                         qint32 groupIndex,
                         quint8 referenceValue,
                         qint32 threshold,
                         const QRect &workingRect,
                         const QPoint &startPoint,
                         KisRunnableStrokeJobsInterface *jobsInterface)
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
        GroupSplitTilePolicyFactory<AlignedGroupSplitTilePolicy<GroupSplitSelectionPolicy>> tilePolicy(KoColor(), selectionPolicy, groupIndex);
        startFillProcess(startPoint, scribbleDevice, groupMapDevice, maskDevice, nullptr, workingRect, tilePolicy, jobsInterface);
    } else {
        GroupSplitTilePolicyFactory<UnalignedGroupSplitTilePolicy<GroupSplitSelectionPolicy>> tilePolicy(KoColor(), selectionPolicy, groupIndex);
        startFillProcess(startPoint, scribbleDevice, groupMapDevice, maskDevice, nullptr, workingRect, tilePolicy, jobsInterface);
    }
}

}

#endif
