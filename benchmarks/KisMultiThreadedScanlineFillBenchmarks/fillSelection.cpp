/*
 *  SPDX-FileCopyrightText: 2022 Deif Lou <ginoba@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "commonFunctions.h"
#include "fillSelection.h"

void FillSelectionBenchmark::benchmarkFillSelection_Aligned()
{
    benchmarkGeneric(
        [](KisPaintDeviceSP referenceDevice, const QRect &workingRect, const QPoint &seedPoint, KisPixelSelectionSP) -> void
        {
            KisPixelSelectionSP mask = new KisPixelSelection();
            KisScanlineFill gc(referenceDevice, seedPoint, workingRect);
            gc.setThreshold(50);
            gc.fillSelection(mask);
        },
        [](KisPaintDeviceSP referenceDevice, const QRect &workingRect, const QPoint &seedPoint, KisPixelSelectionSP) -> void
        {
            KisPixelSelectionSP mask = new KisPixelSelection();
            KisMultiThreadedScanlineFill gc(referenceDevice, seedPoint, workingRect);
            gc.setThreshold(50);
            gc.fillSelection(mask);
        }
    );
}

void FillSelectionBenchmark::benchmarkFillSelection_Unaligned()
{
    benchmarkGeneric(
        [](KisPaintDeviceSP referenceDevice, const QRect &workingRect, const QPoint &seedPoint, KisPixelSelectionSP) -> void
        {
            KisPixelSelectionSP mask = new KisPixelSelection();
            mask->moveTo(32, 32);
            KisScanlineFill gc(referenceDevice, seedPoint, workingRect);
            gc.setThreshold(50);
            gc.fillSelection(mask);
        },
        [](KisPaintDeviceSP referenceDevice, const QRect &workingRect, const QPoint &seedPoint, KisPixelSelectionSP) -> void
        {
            KisPixelSelectionSP mask = new KisPixelSelection();
            mask->moveTo(32, 32);
            KisMultiThreadedScanlineFill gc(referenceDevice, seedPoint, workingRect);
            gc.setThreshold(50);
            gc.fillSelection(mask);
        }
    );
}

SIMPLE_TEST_MAIN(FillSelectionBenchmark)
