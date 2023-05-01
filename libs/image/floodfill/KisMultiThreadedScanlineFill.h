/*
 *  SPDX-FileCopyrightText: 2014 Dmitry Kazakov <dimula73@gmail.com>
 *  SPDX-FileCopyrightText: 2022 Deif Lou <ginoba@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KISMULTITHREADEDSCANLINEFILL_H
#define KISMULTITHREADEDSCANLINEFILL_H

#include <QScopedPointer>

#include <kritaimage_export.h>
#include <kis_types.h>
#include <kis_paint_device.h>

class KisRunnableStrokeJobsInterface;


class KRITAIMAGE_EXPORT KisMultiThreadedScanlineFill
{
public:
    KisMultiThreadedScanlineFill(KisPaintDeviceSP referenceDevice, const QPoint &startPoint, const QRect &boundingRect, KisRunnableStrokeJobsInterface *jobsInterface = 0);
    ~KisMultiThreadedScanlineFill();

    /**
     * Fill the source device with \p fillColor
     */
    void fill(const KoColor &fillColor);

    /**
     * Fill the source device with \p fillColor until \p boundaryColor is reached
     */
    void fillUntilColor(const KoColor &fillColor, const KoColor &boundaryColor);

    /**
     * Fill \p externalDevice with \p fillColor basing on the contents
     * of the source device.
     */
    void fill(const KoColor &fillColor, KisPaintDeviceSP externalDevice);

    /**
     * Fill \p externalDevice with \p fillColor basing on the contents
     * of the source device. Fills until \p boundaryColor is reached
     */
    void fillUntilColor(const KoColor &fillColor, const KoColor &boundaryColor, KisPaintDeviceSP externalDevice);

    /**
     * Fill \p pixelSelection with the opacity of the contiguous area.
     * This method uses an existing selection as boundary for the flood fill.
     */
    void fillSelection(KisPixelSelectionSP pixelSelection, KisPaintDeviceSP boundarySelection);

    /**
     * Fill \p pixelSelection with the opacity of the contiguous area
     */
    void fillSelection(KisPixelSelectionSP pixelSelection);

    /**
     * Fill \p pixelSelection with the opacity of the contiguous area, which
     * encompass all the connected pixels as long as the color in the
     * pixels of the source device is not similar to \p boundaryColor.
     * This method uses an existing selection as boundary for the flood fill.
     */
    void fillSelectionUntilColor(KisPixelSelectionSP pixelSelection, const KoColor &boundaryColor, KisPaintDeviceSP boundarySelection);

    /**
     * Fill \p pixelSelection with the opacity of the contiguous area, which
     * encompass all the connected pixels as long as the color in the
     * pixels of the source device is not similar to \p boundaryColor.
     */
    void fillSelectionUntilColor(KisPixelSelectionSP pixelSelection, const KoColor &boundaryColor);

    /**
     * Fill \p pixelSelection with the opacity of the contiguous area, which
     * encompass all the connected pixels as long as the color in the
     * pixels of the source device is not similar to \p boundaryColor or transparent.
     * This method uses an existing selection as boundary for the flood fill.
     */
    void fillSelectionUntilColorOrTransparent(KisPixelSelectionSP pixelSelection, const KoColor &boundaryColor, KisPaintDeviceSP boundarySelection);

    /**
     * Fill \p pixelSelection with the opacity of the contiguous area, which
     * encompass all the connected pixels as long as the color in the
     * pixels of the source device is not similar to \p boundaryColor or transparent.
     */
    void fillSelectionUntilColorOrTransparent(KisPixelSelectionSP pixelSelection, const KoColor &boundaryColor);

    /**
     * Clear the contiguous non-zero area of the device
     *
     * WARNING: the threshold parameter is not counted!
     */
    void clearNonZeroComponent();

    /**
     * A special filler algorithm for the Watershed initialization routine:
     *
     * 1) Clear the contiguous area in the destination device
     * 2) At the same time, fill the corresponding area of \p groupMapDevice with
     *    value \p groupIndex
     * 3) \p groupMapDevice **must** store 4 bytes per pixel
     */
    void fillContiguousGroup(KisPaintDeviceSP groupMapDevice, qint32 groupIndex);

    /**
     * Set the threshold of the filling operation
     *
     * Used in all functions except clearNonZeroComponent()
     */
    void setThreshold(int threshold);

    /**
     * Set the opacity spread for floodfill. The range is 0-100: 0% means that
     * the fully opaque area only encompasses the pixels exactly equal to the
     * seed point with the other pixels of the selected region being
     * semi-transparent (depending on how similar they are to the seed pixel)
     * up to the region boundary (given by the threshold value). 100 means that
     * the fully opaque area will emcompass all the pixels of the selected
     * region up to the contour. Any value inbetween will make the fully opaque
     * portion of the region vary in size, with semi-transparent pixels
     * inbetween it and  the region boundary
     */
    void setOpacitySpread(int opacitySpread);

private:
    struct Private;
    const QScopedPointer<Private> m_d;
};

#endif
