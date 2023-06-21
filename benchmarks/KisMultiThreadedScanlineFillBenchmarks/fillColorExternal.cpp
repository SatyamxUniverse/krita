/*
 *  SPDX-FileCopyrightText: 2022 Deif Lou <ginoba@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "commonFunctions.h"
#include "fillColorExternal.h"
#include "MultithreadedFillTestHelpers.h"

void FillColorExternalBenchmark::benchmarkFillColorExternal_Aligned()
{
    benchmarkGeneric(
        [](KisPaintDeviceSP referenceDevice, const QRect &workingRect, const QPoint &seedPoint, KisPixelSelectionSP) -> void
        {
            KisPaintDeviceSP externalDevice = new KisPaintDevice(referenceDevice->colorSpace());
            KisScanlineFill gc(referenceDevice, seedPoint, workingRect);
            gc.setThreshold(50);
            gc.fill(KoColor(Qt::red, referenceDevice->colorSpace()), externalDevice);
        },
        [](KisPaintDeviceSP referenceDevice, const QRect &workingRect, const QPoint &seedPoint, KisPixelSelectionSP) -> void
        {
            KisImageSP image = new KisImage(0, workingRect.width(), workingRect.height(), referenceDevice->colorSpace(), "");
            TestUtil::runFillStroke(image, [=] (KisRunnableStrokeJobsInterface *iface) {
                KisPaintDeviceSP externalDevice = new KisPaintDevice(referenceDevice->colorSpace());
                KisMultiThreadedScanlineFill gc(referenceDevice, seedPoint, workingRect, iface);
                gc.setThreshold(50);
                gc.fill(KoColor(Qt::red, referenceDevice->colorSpace()), externalDevice);
            });
        }
    );
}

void FillColorExternalBenchmark::benchmarkFillColorExternal_Unaligned()
{
    benchmarkGeneric(
        [](KisPaintDeviceSP referenceDevice, const QRect &workingRect, const QPoint &seedPoint, KisPixelSelectionSP) -> void
        {
            KisPaintDeviceSP externalDevice = new KisPaintDevice(referenceDevice->colorSpace());
            externalDevice->moveTo(32, 32);
            KisScanlineFill gc(referenceDevice, seedPoint, workingRect);
            gc.setThreshold(50);
            gc.fill(KoColor(Qt::red, referenceDevice->colorSpace()), externalDevice);
        },
        [](KisPaintDeviceSP referenceDevice, const QRect &workingRect, const QPoint &seedPoint, KisPixelSelectionSP) -> void
        {
            KisImageSP image = new KisImage(0, workingRect.width(), workingRect.height(), referenceDevice->colorSpace(), "");
            TestUtil::runFillStroke(image, [=] (KisRunnableStrokeJobsInterface *iface) {
                KisPaintDeviceSP externalDevice = new KisPaintDevice(referenceDevice->colorSpace());
                externalDevice->moveTo(32, 32);
                KisMultiThreadedScanlineFill gc(referenceDevice, seedPoint, workingRect, iface);
                gc.setThreshold(50);
                gc.fill(KoColor(Qt::red, referenceDevice->colorSpace()), externalDevice);
            });
        }
    );
}

SIMPLE_TEST_MAIN(FillColorExternalBenchmark)
