/*
 *  SPDX-FileCopyrightText: 2022 Deif Lou <ginoba@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "commonFunctions.h"
#include "fillSelectionWithBoundary.h"

void FillSelectionWithBoundaryBenchmark::benchmarkFillSelectionWithBoundary_Aligned_Aligned()
{
    benchmarkGeneric(
        [](KisPaintDeviceSP referenceDevice, const QRect &workingRect, const QPoint &seedPoint, KisPixelSelectionSP boundarySelection) -> void
        {
            KisPixelSelectionSP mask = new KisPixelSelection();
            KisScanlineFill gc(referenceDevice, seedPoint, workingRect);
            gc.setThreshold(50);
            gc.fillSelection(mask, boundarySelection);
        },
        [](KisPaintDeviceSP referenceDevice, const QRect &workingRect, const QPoint &seedPoint, KisPixelSelectionSP boundarySelection) -> void
        {
            KisPixelSelectionSP mask = new KisPixelSelection();
            KisMultiThreadedScanlineFill gc(referenceDevice, seedPoint, workingRect);
            gc.setThreshold(50);
            gc.fillSelection(mask, boundarySelection);
        },
        true
    );
}

void FillSelectionWithBoundaryBenchmark::benchmarkFillSelectionWithBoundary_Aligned_Unaligned()
{
    benchmarkGeneric(
        [](KisPaintDeviceSP referenceDevice, const QRect &workingRect, const QPoint &seedPoint, KisPixelSelectionSP boundarySelection) -> void
        {
            boundarySelection->moveTo(-32, -32);
            KisPixelSelectionSP mask = new KisPixelSelection();
            KisScanlineFill gc(referenceDevice, seedPoint, workingRect);
            gc.setThreshold(50);
            gc.fillSelection(mask, boundarySelection);
        },
        [](KisPaintDeviceSP referenceDevice, const QRect &workingRect, const QPoint &seedPoint, KisPixelSelectionSP boundarySelection) -> void
        {
            boundarySelection->moveTo(-32, -32);
            KisPixelSelectionSP mask = new KisPixelSelection();
            KisMultiThreadedScanlineFill gc(referenceDevice, seedPoint, workingRect);
            gc.setThreshold(50);
            gc.fillSelection(mask, boundarySelection);
        },
        true
    );
}

void FillSelectionWithBoundaryBenchmark::benchmarkFillSelectionWithBoundary_Unaligned_Aligned()
{
    benchmarkGeneric(
        [](KisPaintDeviceSP referenceDevice, const QRect &workingRect, const QPoint &seedPoint, KisPixelSelectionSP boundarySelection) -> void
        {
            KisPixelSelectionSP mask = new KisPixelSelection();
            mask->moveTo(32, 32);
            KisScanlineFill gc(referenceDevice, seedPoint, workingRect);
            gc.setThreshold(50);
            gc.fillSelection(mask, boundarySelection);
        },
        [](KisPaintDeviceSP referenceDevice, const QRect &workingRect, const QPoint &seedPoint, KisPixelSelectionSP boundarySelection) -> void
        {
            KisPixelSelectionSP mask = new KisPixelSelection();
            mask->moveTo(32, 32);
            KisMultiThreadedScanlineFill gc(referenceDevice, seedPoint, workingRect);
            gc.setThreshold(50);
            gc.fillSelection(mask, boundarySelection);
        },
        true
    );
}

void FillSelectionWithBoundaryBenchmark::benchmarkFillSelectionWithBoundary_Unaligned_Unaligned()
{
    benchmarkGeneric(
        [](KisPaintDeviceSP referenceDevice, const QRect &workingRect, const QPoint &seedPoint, KisPixelSelectionSP boundarySelection) -> void
        {
            boundarySelection->moveTo(-32, -32);
            KisPixelSelectionSP mask = new KisPixelSelection();
            mask->moveTo(32, 32);
            KisScanlineFill gc(referenceDevice, seedPoint, workingRect);
            gc.setThreshold(50);
            gc.fillSelection(mask, boundarySelection);
        },
        [](KisPaintDeviceSP referenceDevice, const QRect &workingRect, const QPoint &seedPoint, KisPixelSelectionSP boundarySelection) -> void
        {
            boundarySelection->moveTo(-32, -32);
            KisPixelSelectionSP mask = new KisPixelSelection();
            mask->moveTo(32, 32);
            KisMultiThreadedScanlineFill gc(referenceDevice, seedPoint, workingRect);
            gc.setThreshold(50);
            gc.fillSelection(mask, boundarySelection);
        },
        true
    );
}

SIMPLE_TEST_MAIN(FillSelectionWithBoundaryBenchmark)
