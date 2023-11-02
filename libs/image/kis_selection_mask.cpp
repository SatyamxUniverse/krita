/*
 *  SPDX-FileCopyrightText: 2006 Boudewijn Rempt <boud@valdyas.org>
 *
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_selection_mask.h"

#include "kis_image.h"
#include "kis_layer.h"
#include "kis_selection.h"
#include <KoColorSpaceRegistry.h>
#include <KoColorSpace.h>
#include <KoMixColorsOp.h>
#include <KoProperties.h>
#include "kis_fill_painter.h"
#include <KoCompositeOp.h>
#include "kis_node_visitor.h"
#include "kis_processing_visitor.h"
#include "kis_pixel_selection.h"
#include "kis_undo_adapter.h"
#include "kis_iterator_ng.h"
#include <KoIcon.h>
#include <kis_icon.h>
#include "kis_thread_safe_signal_compressor.h"
#include "kis_layer_properties_icons.h"
#include "kis_cached_paint_device.h"
#include "kis_image_config.h"
#include "KisImageConfigNotifier.h"

struct Q_DECL_HIDDEN KisSelectionMask::Private
{
public:
    Private(KisSelectionMask *_q)
        : q(_q)
        , updatesCompressor(0)
        , updateMergeCompressor(0)
        , maskColor1(Qt::transparent, KoColorSpaceRegistry::instance()->rgb8())
        , maskColor2(Qt::green, KoColorSpaceRegistry::instance()->rgb8())
        , thumbnailDevice(new KisPaintDevice(KoColorSpaceRegistry::instance()->rgb8()))
    {}
    KisSelectionMask *q;
    KisThreadSafeSignalCompressor *updatesCompressor;
    KisThreadSafeSignalCompressor *updateMergeCompressor;
    KoColor maskColor1;
    KoColor maskColor2;
    KisPaintDeviceSP thumbnailDevice;
    QRect effectiveRect;
 
    void slotSelectionChangedCompressed();
    void slotMergeSelectionChangedCompressed();
    void slotConfigChangedImpl(bool blockUpdates);
    void slotConfigChanged();
};

KisSelectionMask::KisSelectionMask(KisImageWSP image, const QString &name)
    : KisEffectMask(image, name)
    , m_d(new Private(this))
{
    setActive(false);
    setSupportsLodMoves(false);

    m_d->updatesCompressor =
            new KisThreadSafeSignalCompressor(50, KisSignalCompressor::FIRST_ACTIVE);

    connect(m_d->updatesCompressor, SIGNAL(timeout()), SLOT(slotSelectionChangedCompressed()));

    m_d->updateMergeCompressor =
            new KisThreadSafeSignalCompressor(50, KisSignalCompressor::FIRST_ACTIVE);

    connect(m_d->updateMergeCompressor, SIGNAL(timeout()), SLOT(slotMergeSelectionChangedCompressed()));

    connect(KisImageConfigNotifier::instance(), SIGNAL(configChanged()), SLOT(slotConfigChanged()));
    m_d->slotConfigChangedImpl(false);
}

KisSelectionMask::KisSelectionMask(const KisSelectionMask& rhs)
    : KisEffectMask(rhs)
    , m_d(new Private(this))
{
    m_d->updatesCompressor =
            new KisThreadSafeSignalCompressor(300, KisSignalCompressor::POSTPONE);

    connect(m_d->updatesCompressor, SIGNAL(timeout()), SLOT(slotSelectionChangedCompressed()));

    m_d->updateMergeCompressor =
            new KisThreadSafeSignalCompressor(300, KisSignalCompressor::POSTPONE);

    connect(m_d->updateMergeCompressor, SIGNAL(timeout()), SLOT(slotMergeSelectionChangedCompressed()));

    connect(KisImageConfigNotifier::instance(), SIGNAL(configChanged()), SLOT(slotConfigChanged()));
    m_d->slotConfigChangedImpl(false);
}

KisSelectionMask::~KisSelectionMask()
{
    m_d->updatesCompressor->deleteLater();
    m_d->updateMergeCompressor->deleteLater();
    delete m_d;
}

QIcon KisSelectionMask::icon() const {
    return KisIconUtils::loadIcon("selectionMask");
}

QImage KisSelectionMask::thumbnailImage() const
{
    QImage image = QImage();

    if (m_d->effectiveRect.isValid()) {
        image = m_d->thumbnailDevice->convertToQImage(0, m_d->effectiveRect);
    }

    return image;
}

QTransform KisSelectionMask::thumbnailImageTransform() const
{
    QTransform transform = QTransform();

    if (m_d->effectiveRect.isValid()) {
        transform = QTransform::fromTranslate(m_d->effectiveRect.x(), m_d->effectiveRect.y());
    }

    return transform;
}

void KisSelectionMask::mergeInMaskInternal(KisPaintDeviceSP projection,
                                           KisSelectionSP effectiveSelection,
                                           const QRect &applyRect,
                                           const QRect &preparedNeedRect,
                                           KisNode::PositionToFilthy maskPos) const
{
    Q_UNUSED(projection);
    Q_UNUSED(preparedNeedRect);
    Q_UNUSED(maskPos);

    if (!effectiveSelection) {
        m_d->effectiveRect = QRect();
        return;
    }

    KisSelectionSP mainMaskSelection = this->selection();
    if (mainMaskSelection &&
        (!mainMaskSelection->isVisible() ||
         mainMaskSelection->pixelSelection()->defaultBounds()->externalFrameActive())) {

        m_d->effectiveRect = QRect();
        return;
    }

    m_d->effectiveRect = effectiveSelection->selectedExactRect();
    KisSequentialIterator thumbnailDeviceIt(m_d->thumbnailDevice, applyRect);

    if (m_d->effectiveRect.contains(applyRect) || m_d->effectiveRect.intersects(applyRect)) {
        KisSequentialConstIterator effectiveSelectionIt(effectiveSelection->pixelSelection(), applyRect);

        const quint8 *colors[2];
        colors[0] = m_d->maskColor2.data();
        colors[1] = m_d->maskColor1.data();
        qint16 weights[2];

        while (effectiveSelectionIt.nextPixel() && thumbnailDeviceIt.nextPixel()) {
            weights[1] = *effectiveSelectionIt.rawDataConst();
            weights[0] = MAX_SELECTED - weights[1];

            m_d->thumbnailDevice->colorSpace()->mixColorsOp()->mixColors(colors, weights, 2, thumbnailDeviceIt.rawData(), MAX_SELECTED);
        }
    } else {
        const int pixelSize = m_d->thumbnailDevice->colorSpace()->pixelSize();
        while (thumbnailDeviceIt.nextPixel()) {
            memcpy(thumbnailDeviceIt.rawData(), m_d->maskColor2.data(), pixelSize);
        }
    }

    m_d->updateMergeCompressor->start();
}

bool KisSelectionMask::paintsOutsideSelection() const
{
    return true;
}

void KisSelectionMask::setSelection(KisSelectionSP selection)
{
    if (selection) {
        KisEffectMask::setSelection(selection);
    } else {
        KisEffectMask::setSelection(new KisSelection());

        const KoColorSpace * cs = KoColorSpaceRegistry::instance()->alpha8();
        KisFillPainter gc(KisPaintDeviceSP(this->selection()->pixelSelection().data()));
        gc.fillRect(image()->bounds(), KoColor(Qt::white, cs), MAX_SELECTED);
        gc.end();
    }
    setDirty();
}

bool KisSelectionMask::accept(KisNodeVisitor &v)
{
    return v.visit(this);
}

void KisSelectionMask::accept(KisProcessingVisitor &visitor, KisUndoAdapter *undoAdapter)
{
    return visitor.visit(this, undoAdapter);
}

KisBaseNode::PropertyList KisSelectionMask::sectionModelProperties() const
{
    KisBaseNode::PropertyList l = KisBaseNode::sectionModelProperties();
    l << KisLayerPropertiesIcons::getProperty(KisLayerPropertiesIcons::selectionActive, active());
    return l;
}

void KisSelectionMask::setSectionModelProperties(const KisBaseNode::PropertyList &properties)
{
    KisEffectMask::setSectionModelProperties(properties);
    setActive(properties.at(2).state.toBool());
}

void KisSelectionMask::setVisible(bool visible, bool isLoading)
{
    const bool oldVisible = this->visible(false);
    setNodeProperty("visible", visible);

    if (!isLoading && visible != oldVisible) {
        if (selection())
            selection()->setVisible(visible);
    }
}

bool KisSelectionMask::active() const
{
    return nodeProperties().boolProperty("active", true);
}

void KisSelectionMask::setActive(bool active)
{
    KisImageSP image = this->image();
    KisLayerSP parentLayer = qobject_cast<KisLayer*>(parent().data());

    if (active && parentLayer) {
        KisSelectionMaskSP activeMask = parentLayer->selectionMask();
        if (activeMask && activeMask != this) {
            activeMask->setActive(false);
        }
    }

    const bool oldActive = this->active();
    setNodeProperty("active", active);

    /**
     * WARNING: we have a direct link to the image here, but we
     * must not use it for notification until we are a part of
     * the node graph! Notifications should be emitted iff we
     * have graph listener link set up.
     */
    if (graphListener() &&
        image && oldActive != active) {

        baseNodeChangedCallback();
        image->undoAdapter()->emitSelectionChanged();
    }
}

QRect KisSelectionMask::needRect(const QRect &rect, KisNode::PositionToFilthy pos) const
{
    Q_UNUSED(pos);
    
    // selection masks just add an overlay, so the needed rect is simply passed through
    return rect;
}

QRect KisSelectionMask::changeRect(const QRect &rect, KisNode::PositionToFilthy pos) const
{
    Q_UNUSED(pos);

    // selection masks just add an overlay, so the changed rect is simply passed through
    return rect;
}

QRect KisSelectionMask::extent() const
{
    // since mask overlay is inverted, the mask paints over
    // the entire image bounds
    QRect resultRect;
    KisSelectionSP selection = this->selection();
    
    if (selection) {
        resultRect = selection->pixelSelection()->defaultBounds()->bounds();
        
        if (KisNodeSP parent = this->parent()) {
            resultRect |= parent->extent();
        }
        
    } else if (KisNodeSP parent = this->parent()) {
        KisPaintDeviceSP dev = parent->projection();
        if (dev) {
            resultRect = dev->defaultBounds()->bounds();
        }
    }

    return resultRect;
}

QRect KisSelectionMask::exactBounds() const
{
    return extent();
}

void KisSelectionMask::notifySelectionChangedCompressed()
{
    m_d->updatesCompressor->start();
}

bool KisSelectionMask::decorationsVisible() const
{
    return selection()->isVisible();
}

void KisSelectionMask::setDecorationsVisible(bool value, bool update)
{
    if (value == decorationsVisible()) return;

    const QRect oldExtent = extent();

    selection()->setVisible(value);

    if (update) {
        setDirty(oldExtent | extent());
    }
}

void KisSelectionMask::setDirty(const QVector<QRect> &rects)
{
    KisImageSP image = this->image();

    if (image && image->overlaySelectionMask() == this) {
        KisEffectMask::setDirty(rects);
    }
}

void KisSelectionMask::flattenSelectionProjection(KisSelectionSP selection, const QRect &dirtyRect) const
{
    Q_UNUSED(selection);
    Q_UNUSED(dirtyRect);
}

void KisSelectionMask::Private::slotSelectionChangedCompressed()
{
    KisSelectionSP currentSelection = q->selection();
    if (!currentSelection) return;

    currentSelection->notifySelectionChanged();
}

void KisSelectionMask::Private::slotMergeSelectionChangedCompressed()
{
    updateMergeCompressor->stop();

    KisImageSP image = q->image();
    if (image && q->graphListener()) {
        image->undoAdapter()->emitSelectionChanged();
    }
}

void KisSelectionMask::Private::slotConfigChangedImpl(bool doUpdates)
{
    KisImageConfig cfg(true);
    const KoColorSpace *cs = KoColorSpaceRegistry::instance()->rgb8();

    maskColor1 = KoColor(cfg.selectionOverlayMaskColorSelected(), cs);
    maskColor2 = KoColor(cfg.selectionOverlayMaskColor(), cs);

    thumbnailDevice->setDefaultPixel(maskColor2);

    KisImageSP image = q->image();
    if (doUpdates && image && image->overlaySelectionMask() == q) {
        q->setDirty();
    }
}

void KisSelectionMask::Private::slotConfigChanged()
{
    slotConfigChangedImpl(true);
}

#include "moc_kis_selection_mask.cpp"
