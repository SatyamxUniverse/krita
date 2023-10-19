/*
 *  SPDX-FileCopyrightText: 2015 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_layer_projection_plane.h"

#include <QBitArray>
#include <KoColorSpace.h>
#include <KoChannelInfo.h>
#include <KoCompositeOpRegistry.h>
#include "kis_painter.h"
#include "kis_projection_leaf.h"
#include "kis_cached_paint_device.h"
#include "kis_sequential_iterator.h"
#include "kis_paint_layer.h"


struct KisLayerProjectionPlane::Private
{
    KisLayer *layer;
    KisCachedPaintDevice cachedDevice;
};


KisLayerProjectionPlane::KisLayerProjectionPlane(KisLayer *layer)
    : m_d(new Private)
{
    m_d->layer = layer;
}

KisLayerProjectionPlane::~KisLayerProjectionPlane()
{
}

QRect KisLayerProjectionPlane::recalculate(const QRect& rect, KisNodeSP filthyNode)
{
    return m_d->layer->updateProjection(rect, filthyNode);
}

void KisLayerProjectionPlane::applyImpl(KisPainter *painter, const QRect &rect, KritaUtils::ThresholdMode thresholdMode)
{
    KisPaintDeviceSP device = m_d->layer->projection();

    if (!device) return;

    //if current layer is clipped we don't need to update projection
    //because it should be updated already in the base layer
    bool clip = m_d->layer->alphaChannelDisabled();
    if (clip) {
        return;
    }


    QRect needRect = rect;

    if (m_d->layer->compositeOpId() != COMPOSITE_COPY &&
        m_d->layer->compositeOpId() != COMPOSITE_DESTINATION_IN  &&
        m_d->layer->compositeOpId() != COMPOSITE_DESTINATION_ATOP) {

        needRect &= device->extent();
    }

    if(needRect.isEmpty()) return;

    const QBitArray channelFlags = m_d->layer->projectionLeaf()->channelFlags();

    QScopedPointer<KisCachedPaintDevice::Guard> d1;

    if (thresholdMode != KritaUtils::ThresholdNone) {
        d1.reset(new KisCachedPaintDevice::Guard(device, m_d->cachedDevice));
        KisPaintDeviceSP tmp = d1->device();
        tmp->makeCloneFromRough(device, needRect);

        KritaUtils::thresholdOpacity(tmp, needRect, thresholdMode);

        device = tmp;
    }

    painter->setChannelFlags(channelFlags);
    painter->setCompositeOpId(m_d->layer->compositeOpId());
    painter->setOpacity(m_d->layer->projectionLeaf()->opacity());


    /* if layer above current have clipping enabled we gather all layers
     * with clipping and apply them on copy of current layer
     * then apply that copy to global projection
     */
    bool clipping = false;
    KisNodeSP nextNode = m_d->layer->nextSibling();
    KisLayer* nextLayer;
    if(nextNode){
        nextLayer = qobject_cast<KisLayer*>(nextNode.data());
        if (nextLayer){
            clipping = nextLayer->alphaChannelDisabled();
        }
    }


    if (!clipping){
        //if no clipping layers use regular bitblt
        painter->bitBlt(needRect.topLeft(), device, needRect);
    } else {

        QList<KisNodeSP> nodes;
        if (nextNode->visible()) nodes.append(nextNode);

        while (true) {
            nextNode = nextLayer->nextSibling();
            if (!nextNode) break;
            nextLayer = qobject_cast<KisLayer*>(nextNode.data());
            if (!nextLayer) break;
            if (nextNode->visible() && nextLayer->alphaChannelDisabled())  {
                nodes.append(nextNode);}
            else if (!nextLayer->alphaChannelDisabled()){
                break;
            }
        }

        KisPaintDeviceSP deviceClone = new KisPaintDevice(dstCS);
        deviceClone->makeCloneFrom(device,needRect);

        foreach (KisNodeSP node, nodes) {
            KisPaintLayerSP clipLayer = qobject_cast<KisPaintLayer*>(node.data());

            //merge with alpha disabled for proper results
            QBitArray newChannelFlags = clipLayer->colorSpace()->channelFlags(true, false);

            KisPainter clipMerger(deviceClone);
            clipMerger.setChannelFlags(newChannelFlags);
            clipMerger.setCompositeOpId(clipLayer->compositeOpId());
            clipMerger.setOpacity(clipLayer->projectionLeaf()->opacity());
            clipMerger.bitBlt(needRect.topLeft(), clipLayer->projection(), needRect);

        }

        painter->bitBlt(needRect.topLeft(), deviceClone, needRect);
    }
}

void KisLayerProjectionPlane::apply(KisPainter *painter, const QRect &rect)
{
    applyImpl(painter, rect, KritaUtils::ThresholdNone);
}

void KisLayerProjectionPlane::applyMaxOutAlpha(KisPainter *painter, const QRect &rect, KritaUtils::ThresholdMode thresholdMode)
{
    applyImpl(painter, rect, thresholdMode);
}

KisPaintDeviceList KisLayerProjectionPlane::getLodCapableDevices() const
{
    return KisPaintDeviceList() << m_d->layer->projection();
}

QRect KisLayerProjectionPlane::needRect(const QRect &rect, KisLayer::PositionToFilthy pos) const
{
    return m_d->layer->needRect(rect, pos);
}

QRect KisLayerProjectionPlane::changeRect(const QRect &rect, KisLayer::PositionToFilthy pos) const
{
    return m_d->layer->changeRect(rect, pos);
}

QRect KisLayerProjectionPlane::accessRect(const QRect &rect, KisLayer::PositionToFilthy pos) const
{
    return m_d->layer->accessRect(rect, pos);
}

QRect KisLayerProjectionPlane::needRectForOriginal(const QRect &rect) const
{
    return m_d->layer->needRectForOriginal(rect);
}

QRect KisLayerProjectionPlane::tightUserVisibleBounds() const
{
    return m_d->layer->tightUserVisibleBounds();
}

