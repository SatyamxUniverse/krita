/*
 *  SPDX-FileCopyrightText: 2022 Deif Lou <ginoba@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef COMMONFUNCTIONS_H
#define COMMONFUNCTIONS_H

#include <simpletest.h>

#include <QColor>
#include <QPoint>
#include <QImage>
#include <QPainter>
#include <QElapsedTimer>

#include <floodfill/kis_scanline_fill.h>
#include <floodfill/KisMultiThreadedScanlineFill.h>
#include <KoColorSpaceRegistry.h>
#include <KoColor.h>
#include <kis_pixel_selection.h>
#include <kis_default_bounds.h>

#define MAX_IMAGE_SIZE 12000
#define ITERATIONS_PER_TEST 1

template <typename NonMultiThreadedTestFn, typename MultiThreadedTestFn>
void benchmarkGeneric(NonMultiThreadedTestFn nonMultiThreadedTestFn, MultiThreadedTestFn multiThreadedTestFn, bool prepareBoundarySelection = false)
{
    const KoColorSpace * cs = KoColorSpaceRegistry::instance()->rgb8();

    QImage srcImage(QString(FILES_DATA_DIR) + '/' + "concentric_rings_labyrinth.png");
    QVERIFY(!srcImage.isNull());

    qint32 testImageSize = srcImage.width();

    qDebug() << "Size\t\tNon MT\t\tMT";

    for (; testImageSize <= MAX_IMAGE_SIZE; testImageSize += 256) {
        KisPaintDeviceSP referenceDevice = new KisPaintDevice(cs);
        KisPixelSelectionSP boundarySelection = {nullptr};
        KisPaintDeviceSP workingDevice;
        QRect rect;

        {
            const QImage testImage = srcImage.scaled(testImageSize, testImageSize);
            referenceDevice->convertFromQImage(testImage, 0);
            rect = testImage.rect();

            if (prepareBoundarySelection) {
                boundarySelection = new KisPixelSelection(new KisSelectionDefaultBounds(referenceDevice));
                boundarySelection->fill(rect.adjusted(0, 32, 0, -32), KoColor(Qt::white, boundarySelection->colorSpace()));
            }
        }

        const KoColor fillColor(Qt::red, referenceDevice->colorSpace());

        const QPoint seedPoint(0, testImageSize / 2);

        QElapsedTimer timer;
        qreal time1 = 0.0;
        qreal time2 = 0.0;

        for (qint32 i = 0; i < ITERATIONS_PER_TEST; ++i) {
            workingDevice = new KisPaintDevice(*referenceDevice);
            timer.start();
            nonMultiThreadedTestFn(workingDevice, rect, seedPoint, boundarySelection);
            time1 += timer.nsecsElapsed();
        }
        time1 /= ITERATIONS_PER_TEST * 1000000.0;

        for (qint32 i = 0; i < ITERATIONS_PER_TEST; ++i) {
            workingDevice = new KisPaintDevice(*referenceDevice);
            timer.start();
            multiThreadedTestFn(workingDevice, rect, seedPoint, boundarySelection);
            time2 += timer.nsecsElapsed();
        }
        time2 /= ITERATIONS_PER_TEST * 1000000.0;

        qDebug() << fixed << qSetRealNumberPrecision(2) << testImageSize << "\t\t" << time1 << "\t\t" << time2;
    }
}

#endif
