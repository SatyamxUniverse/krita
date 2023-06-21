/*
 *  SPDX-FileCopyrightText: 2022 Deif Lou <ginoba@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "commonFunctions.h"
#include "fillColor.h"
#include "MultithreadedFillTestHelpers.h"


void FillColorBenchmark::benchmarkFillColor()
{
    benchmarkGeneric(
        [](KisPaintDeviceSP referenceDevice, const QRect &workingRect, const QPoint &seedPoint, KisPixelSelectionSP) -> void
        {
            KisScanlineFill gc(referenceDevice, seedPoint, workingRect);
            gc.setThreshold(50);
            gc.fill(KoColor(Qt::red, referenceDevice->colorSpace()));
        },
        [](KisPaintDeviceSP referenceDevice, const QRect &workingRect, const QPoint &seedPoint, KisPixelSelectionSP) -> void
        {
            KisImageSP image = new KisImage(0, workingRect.width(), workingRect.height(), referenceDevice->colorSpace(), "");
            TestUtil::runFillStroke(image, [=] (KisRunnableStrokeJobsInterface *iface) {
                KisMultiThreadedScanlineFill gc(referenceDevice, seedPoint, workingRect, iface);
                gc.setThreshold(50);
                gc.fill(KoColor(Qt::red, referenceDevice->colorSpace()));
            });
        }
    );
}

SIMPLE_TEST_MAIN(FillColorBenchmark)
