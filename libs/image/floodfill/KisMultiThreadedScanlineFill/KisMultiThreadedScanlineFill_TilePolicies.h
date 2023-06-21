/*
 *  SPDX-FileCopyrightText: 2022 Deif Lou <ginoba@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KISMULTITHREADEDSCANLINEFILL_TILEPOLICIES_H
#define KISMULTITHREADEDSCANLINEFILL_TILEPOLICIES_H

#include <QRect>

#include <kis_sequential_iterator.h>
#include <kis_random_accessor_ng.h>
#include <kis_assert.h>
#include <tiles3/kis_tile_data_interface.h>

namespace KisMultiThreadedScanlineFillNS
{

using TileId = QPoint;
static const QSize tileSize {KisTileData::WIDTH, KisTileData::HEIGHT};

namespace detail {

template <bool IsReadOnly_>
struct DeviceAccessorBase
{
protected:
    using RandomAccessorType = typename std::conditional<IsReadOnly_, KisRandomConstAccessorSP, KisRandomAccessorSP>::type;
    using DataPointerType = typename std::conditional<IsReadOnly_, const quint8*, quint8*>::type;

    DataPointerType m_deviceTileDataBegin {nullptr};
    DataPointerType m_deviceTileRowDataBegin {nullptr};
    qint32 m_devicePixelSize;
    qint32 m_deviceRowStride;

    template <bool B = IsReadOnly_, typename = typename std::enable_if<B>::type>
    KisRandomConstAccessorSP getRandomAccessor(KisPaintDeviceSP device) const
    {
        return device->createRandomConstAccessorNG();
    }

    template <bool B = IsReadOnly_, typename = typename std::enable_if<!B>::type>
    KisRandomAccessorSP getRandomAccessor(KisPaintDeviceSP device) const
    {
        return device->createRandomAccessorNG();
    }

    template <bool B = IsReadOnly_, typename = typename std::enable_if<B>::type>
    DataPointerType getRawData(KisRandomConstAccessorSP accessor) const
    {
        return accessor->oldRawData();
    }

    template <bool B = IsReadOnly_, typename = typename std::enable_if<!B>::type>
    DataPointerType getRawData(KisRandomAccessorSP accessor) const
    {
        return accessor->rawData();
    }
};

template <bool IsReadOnly_>
struct AlignedDeviceAccessor : public DeviceAccessorBase<IsReadOnly_>
{
    void setWorkingRow(qint32 relativeRow)
    {
        this->m_deviceTileRowDataBegin = this->m_deviceTileDataBegin + relativeRow * this->m_deviceRowStride;
    }

    typename DeviceAccessorBase<IsReadOnly_>::DataPointerType getPixel(qint32 relativeColumn) const
    {
        return this->m_deviceTileRowDataBegin + relativeColumn * this->m_devicePixelSize;
    }

    template <bool B = IsReadOnly_, typename = typename std::enable_if<!B>::type>
    void setPixel(qint32 relativeColumn, const quint8 *valuePtr) const
    {
        memcpy(this->m_deviceTileRowDataBegin + relativeColumn * this->m_devicePixelSize, valuePtr, this->m_devicePixelSize);
    }

    void initialize(KisPaintDeviceSP device, const QRect &tileRect)
    {
        deviceIterator = this->getRandomAccessor(device);
        deviceIterator->moveTo(tileRect.x(), tileRect.y());
        this->m_deviceTileDataBegin = this->getRawData(deviceIterator);
        this->m_deviceTileRowDataBegin = this->m_deviceTileDataBegin;
        this->m_devicePixelSize = device->pixelSize();
        this->m_deviceRowStride = deviceIterator->rowStride(tileRect.x(), tileRect.y());
    }

    typename DeviceAccessorBase<IsReadOnly_>::RandomAccessorType deviceIterator;
};

template <bool IsReadOnly_>
struct UnalignedDeviceAccessor : public DeviceAccessorBase<IsReadOnly_>
{
    void setWorkingRow(qint32 relativeRow)
    {
        adjustRow(relativeRow);
    }

    typename DeviceAccessorBase<IsReadOnly_>::DataPointerType getPixel(qint32 relativeColumn)
    {
        adjustColumn(relativeColumn);
        return this->m_deviceTileRowDataBegin + (relativeColumn - m_columnOffset) * this->m_devicePixelSize;
    }

    template <bool B = IsReadOnly_, typename = typename std::enable_if<!B>::type>
    void setPixel(qint32 relativeColumn, const quint8 *valuePtr)
    {
        adjustColumn(relativeColumn);
        memcpy(this->m_deviceTileRowDataBegin + (relativeColumn - m_columnOffset) * this->m_devicePixelSize, valuePtr, this->m_devicePixelSize);
    }

    void initialize(KisPaintDeviceSP device, const QRect &tileRect)
    {
        deviceIterator[0] = this->getRandomAccessor(device);
        deviceIterator[1] = this->getRandomAccessor(device);
        deviceIterator[2] = this->getRandomAccessor(device);
        deviceIterator[3] = this->getRandomAccessor(device);

        m_numContiguousColumns = deviceIterator[0]->numContiguousColumns(tileRect.x());
        m_numContiguousRows = deviceIterator[0]->numContiguousRows(tileRect.y());
        this->m_devicePixelSize = device->pixelSize();
        this->m_deviceRowStride = deviceIterator[0]->rowStride(tileRect.x(), tileRect.y());

        deviceIterator[0]->moveTo(tileRect.x(), tileRect.y());
        m_partialDeviceTileDataBegin[0] = this->getRawData(deviceIterator[0]);
        deviceIterator[1]->moveTo(tileRect.x() + m_numContiguousColumns, tileRect.y());
        m_partialDeviceTileDataBegin[1] = this->getRawData(deviceIterator[1]);
        deviceIterator[2]->moveTo(tileRect.x(), tileRect.y() + m_numContiguousRows);
        m_partialDeviceTileDataBegin[2] = this->getRawData(deviceIterator[2]);
        deviceIterator[3]->moveTo(tileRect.x() + m_numContiguousColumns, tileRect.y() + m_numContiguousRows);
        m_partialDeviceTileDataBegin[3] = this->getRawData(deviceIterator[3]);

        this->m_deviceTileDataBegin = m_partialDeviceTileDataBegin[0];
        this->m_deviceTileRowDataBegin = this->m_deviceTileDataBegin;
        m_columnOffset = m_rowOffset = m_currentRelativeRowOffset = 0;
    }

private:
    typename DeviceAccessorBase<IsReadOnly_>::RandomAccessorType deviceIterator[4];
    typename DeviceAccessorBase<IsReadOnly_>::DataPointerType m_partialDeviceTileDataBegin[4] {nullptr};
    qint32 m_numContiguousColumns {0};
    qint32 m_numContiguousRows {0};
    qint32 m_columnOffset {0};
    qint32 m_rowOffset {0};
    qint32 m_currentRelativeRowOffset {0};

    void adjustColumn(qint32 relativeColumn)
    {
        if (relativeColumn >= m_numContiguousColumns) {
            if (m_columnOffset == 0) {
                m_columnOffset = m_numContiguousColumns;
                this->m_deviceTileDataBegin = m_partialDeviceTileDataBegin[(m_rowOffset > 1) * 2 + 1];
                this->m_deviceTileRowDataBegin = this->m_deviceTileDataBegin + m_currentRelativeRowOffset * this->m_deviceRowStride;
            }
        } else {
            if (m_columnOffset > 0) {
                m_columnOffset = 0;
                this->m_deviceTileDataBegin = m_partialDeviceTileDataBegin[(m_rowOffset > 1) * 2];
                this->m_deviceTileRowDataBegin = this->m_deviceTileDataBegin + m_currentRelativeRowOffset * this->m_deviceRowStride;
            }
        }
    }

    void adjustRow(qint32 relativeRow)
    {
        if (relativeRow >= m_numContiguousRows) {
            if (m_rowOffset == 0) {
                m_rowOffset = m_numContiguousRows;
                this->m_deviceTileDataBegin = m_partialDeviceTileDataBegin[2 + (m_columnOffset > 0)];
            }
            m_currentRelativeRowOffset = relativeRow - m_rowOffset;
        } else {
            if (m_rowOffset > 0) {
                m_rowOffset = 0;
                this->m_deviceTileDataBegin = m_partialDeviceTileDataBegin[m_columnOffset > 0];
            }
            m_currentRelativeRowOffset = relativeRow;
        }
        this->m_deviceTileRowDataBegin = this->m_deviceTileDataBegin + m_currentRelativeRowOffset * this->m_deviceRowStride;
    }
};

template <typename SelectionPolicyType_>
struct BasePolicy
{
    using SelectionPolicyType = SelectionPolicyType_;

    BasePolicy(const KoColor &fillColor, const SelectionPolicyType &selectionPolicy)
        : m_fillColor(fillColor)
        , m_selectionPolicy(selectionPolicy)
    {}

    const QRect& tileRect() const
    {
        return m_tileRect;
    }

    const QRect& tileSubRect() const
    {
        return m_tileSubRect;
    }

    const KoColor& fillColor() const
    {
        return m_fillColor;
    }

    SelectionPolicyType& selectionPolicy() const
    {
        return m_selectionPolicy;
    }

    qint32 relativeRow(qint32 row) const
    {
        return row - m_tileRect.top();
    }

    qint32 relativeColumn(qint32 column) const
    {
        return column - m_tileRect.left();
    }

    void assertIsProcessing() const
    {
        KIS_ASSERT(m_isProcessing);
    }

    void assertIsNotProcessing() const
    {
        KIS_ASSERT(!m_isProcessing);
    }

    void assertRowIsValid(qint32 row) const
    {
        KIS_ASSERT(row >= m_tileSubRect.top() && row <= m_tileSubRect.bottom());
    }

    void assertColumnIsValid(qint32 column) const
    {
        KIS_ASSERT(column >= this->m_tileSubRect.left() && column <= this->m_tileSubRect.right());
    }

    void assertDeviceIsValid(KisPaintDeviceSP device) const
    {
        KIS_ASSERT(device);
    }

    void beginProcessing(KisPaintDeviceSP referenceDevice,
                         const TileId &tileId,
                         const QRect &workingRect)
    {
        m_isProcessing = true;

        m_tileRect = QRect(
            referenceDevice->x() + tileId.x() * tileSize.width(),
            referenceDevice->y() + tileId.y() * tileSize.width(),
            tileSize.width(), tileSize.height()
        );
        m_tileSubRect = m_tileRect.intersected(workingRect);
    }

    void endProcessing()
    {
        m_isProcessing = false;
    }

private:
    QRect m_tileRect;
    QRect m_tileSubRect;
    KoColor m_fillColor;
    bool m_isProcessing {false};
    mutable SelectionPolicyType m_selectionPolicy;
};

template <bool IsReadOnly_>
struct ReferenceDevicePolicy : public AlignedDeviceAccessor<IsReadOnly_>
{
    void setWorkingRow(qint32 relativeRow)
    {
        AlignedDeviceAccessor<IsReadOnly_>::setWorkingRow(relativeRow);
    }

    template <typename SelectionPolicy_>
    quint8 calculateOpacity(qint32 relativeColumn, SelectionPolicy_ &selectionPolicy) const
    {
        const quint8 *colorData = AlignedDeviceAccessor<IsReadOnly_>::getPixel(relativeColumn);
        return selectionPolicy.calculateOpacity(colorData);
    }

    template <bool B = IsReadOnly_, typename = typename std::enable_if<B>::type>
    void setValue(qint32 relativeColumn, const quint8 *colorData) const
    {
        Q_UNUSED(relativeColumn);
        Q_UNUSED(colorData);
    }

    template <bool B = IsReadOnly_, typename = typename std::enable_if<!B>::type>
    void setValue(qint32 relativeColumn, const quint8 *colorData)
    {
        AlignedDeviceAccessor<IsReadOnly_>::setPixel(relativeColumn, colorData);
    }

    void beginProcessing(KisPaintDeviceSP referenceDevice, const QRect &tileRect)
    {
        AlignedDeviceAccessor<IsReadOnly_>::initialize(referenceDevice, tileRect);
    }
};

template <bool IsAligned_>
struct ExternalDevicePolicy : public std::conditional<IsAligned_, AlignedDeviceAccessor<false>, UnalignedDeviceAccessor<false>>::type
{
    using BaseClassType = typename std::conditional<IsAligned_, AlignedDeviceAccessor<false>, UnalignedDeviceAccessor<false>>::type;

    void setWorkingRow(qint32 relativeRow)
    {
        BaseClassType::setWorkingRow(relativeRow);
    }

    void setValue(qint32 relativeColumn, const quint8 *colorData)
    {
        BaseClassType::setPixel(relativeColumn, colorData);
    }

    void beginProcessing(KisPaintDeviceSP referenceDevice, const QRect &tileRect)
    {
        BaseClassType::initialize(referenceDevice, tileRect);
    }
};

template <bool IsAligned_>
struct MaskDevicePolicy : public std::conditional<IsAligned_, AlignedDeviceAccessor<false>, UnalignedDeviceAccessor<false>>::type
{
    using BaseClassType = typename std::conditional<IsAligned_, AlignedDeviceAccessor<false>, UnalignedDeviceAccessor<false>>::type;

    void setWorkingRow(qint32 relativeRow)
    {
        BaseClassType::setWorkingRow(relativeRow);
    }

    bool isAlreadySet(qint32 relativeColumn)
    {
        return *BaseClassType::getPixel(relativeColumn) > OPACITY_TRANSPARENT_U8;
    }

    void setValue(qint32 relativeColumn, quint8 value)
    {
        BaseClassType::setPixel(relativeColumn, &value);
    }

    void beginProcessing(KisPixelSelectionSP maskDevice, const QRect &tileRect)
    {
        BaseClassType::initialize(maskDevice, tileRect);
    }
};

template <bool IsAligned_>
struct BoundarySelectionDevicePolicy : public std::conditional<IsAligned_, AlignedDeviceAccessor<true>, UnalignedDeviceAccessor<true>>::type
{
    using BaseClassType = typename std::conditional<IsAligned_, AlignedDeviceAccessor<true>, UnalignedDeviceAccessor<true>>::type;

    void setWorkingRow(qint32 relativeRow)
    {
        BaseClassType::setWorkingRow(relativeRow);
    }

    bool isInsideBoundarySelection(qint32 relativeColumn)
    {
        return *BaseClassType::getPixel(relativeColumn) > OPACITY_TRANSPARENT_U8;
    }

    void beginProcessing(KisPixelSelectionSP boundarySelectionDevice, const QRect &tileRect)
    {
        BaseClassType::initialize(boundarySelectionDevice, tileRect);
    }
};

template <bool IsAligned_>
struct GroupSplitPolicy : public std::conditional<IsAligned_, AlignedDeviceAccessor<false>, UnalignedDeviceAccessor<false>>::type
{
    using BaseClassType = typename std::conditional<IsAligned_, AlignedDeviceAccessor<false>, UnalignedDeviceAccessor<false>>::type;

    void setGroupIndex(qint32 groupIndex) {
        m_groupIndex = groupIndex;
    }

    void setWorkingRow(qint32 relativeRow)
    {
        BaseClassType::setWorkingRow(relativeRow);
    }

    void setValue(qint32 relativeColumn)
    {
        // write group index into the map
        qint32 *groupMapPtr = reinterpret_cast<qint32*>(BaseClassType::getPixel(relativeColumn));
        if (*groupMapPtr != 0) {
            dbgImage << ppVar(*groupMapPtr) << ppVar(m_groupIndex);
        }
        KIS_SAFE_ASSERT_RECOVER_NOOP(*groupMapPtr == 0);
        *groupMapPtr = m_groupIndex;
    }

    void beginProcessing(KisPaintDeviceSP referenceDevice, const QRect &tileRect)
    {
        BaseClassType::initialize(referenceDevice, tileRect);
    }

private:
    qint32 m_groupIndex;
};

template <typename SelectionPolicyType_>
struct WriteToReferenceDeviceTilePolicy : public BasePolicy<SelectionPolicyType_>
{
    WriteToReferenceDeviceTilePolicy(const KoColor &fillColor, const SelectionPolicyType_ &selectionPolicy)
        : BasePolicy<SelectionPolicyType_>(fillColor, selectionPolicy)
    {}

    void setWorkingRow(qint32 row)
    {
        this->assertIsProcessing();
        this->assertRowIsValid(row);
        const qint32 relativeRow = this->relativeRow(row);
        m_referenceDevicePolicy.setWorkingRow(relativeRow);
        m_maskDevicePolicy.setWorkingRow(relativeRow);
    }

    bool isAlreadySet(qint32 column)
    {
        this->assertIsProcessing();
        this->assertColumnIsValid(column);
        const qint32 relativeColumn = this->relativeColumn(column);
        return m_maskDevicePolicy.isAlreadySet(relativeColumn);
    }

    bool isInsideBoundarySelection(qint32 column)
    {
        this->assertIsProcessing();
        this->assertColumnIsValid(column);
        return true;
    }

    quint8 calculateOpacity(qint32 column) const
    {
        this->assertIsProcessing();
        this->assertColumnIsValid(column);
        const qint32 relativeColumn = this->relativeColumn(column);
        return m_referenceDevicePolicy.calculateOpacity(relativeColumn, this->selectionPolicy());
    }

    void setValue(qint32 column, quint8 value)
    {
        this->assertIsProcessing();
        this->assertColumnIsValid(column);
        const qint32 relativeColumn = this->relativeColumn(column);
        m_referenceDevicePolicy.setValue(relativeColumn, this->fillColor().data());
        m_maskDevicePolicy.setValue(relativeColumn, value);
    }

    void beginProcessing(KisPaintDeviceSP referenceDevice,
                         KisPaintDeviceSP externalDevice,
                         KisPixelSelectionSP maskDevice,
                         KisPixelSelectionSP boundarySelectionDevice,
                         const TileId &tileId,
                         const QRect &workingRect)
    {
        Q_UNUSED(externalDevice);
        Q_UNUSED(boundarySelectionDevice);

        this->assertIsNotProcessing();
        KIS_ASSERT(referenceDevice);
        KIS_ASSERT(maskDevice);

        BasePolicy<SelectionPolicyType_>::beginProcessing(referenceDevice, tileId, workingRect);
        m_referenceDevicePolicy.beginProcessing(referenceDevice, this->tileRect());
        m_maskDevicePolicy.beginProcessing(maskDevice, this->tileRect());
        setWorkingRow(this->tileSubRect().top());
    }

    void endProcessing()
    {
        this->assertIsProcessing();
        BasePolicy<SelectionPolicyType_>::endProcessing();
    }

private:
    ReferenceDevicePolicy<false> m_referenceDevicePolicy;
    MaskDevicePolicy<true> m_maskDevicePolicy;
};

template <typename SelectionPolicyType_, typename ExternalDevicePolicy_>
struct WriteToExternalDeviceTilePolicy : public BasePolicy<SelectionPolicyType_>
{
    WriteToExternalDeviceTilePolicy(const KoColor &fillColor, const SelectionPolicyType_ &selectionPolicy)
        : BasePolicy<SelectionPolicyType_>(fillColor, selectionPolicy)
    {}

    void setWorkingRow(qint32 row)
    {
        this->assertIsProcessing();
        this->assertRowIsValid(row);
        const qint32 relativeRow = this->relativeRow(row);
        m_referenceDevicePolicy.setWorkingRow(relativeRow);
        m_maskDevicePolicy.setWorkingRow(relativeRow);
        m_externalDevicePolicy.setWorkingRow(relativeRow);
    }

    bool isAlreadySet(qint32 column)
    {
        this->assertIsProcessing();
        this->assertColumnIsValid(column);
        const qint32 relativeColumn = this->relativeColumn(column);
        return m_maskDevicePolicy.isAlreadySet(relativeColumn);
    }

    bool isInsideBoundarySelection(qint32 column)
    {
        this->assertIsProcessing();
        this->assertColumnIsValid(column);
        return true;
    }

    quint8 calculateOpacity(qint32 column) const
    {
        this->assertIsProcessing();
        this->assertColumnIsValid(column);
        const qint32 relativeColumn = this->relativeColumn(column);
        return m_referenceDevicePolicy.calculateOpacity(relativeColumn, this->selectionPolicy());
    }

    void setValue(qint32 column, quint8 value)
    {
        this->assertIsProcessing();
        this->assertColumnIsValid(column);
        const qint32 relativeColumn = this->relativeColumn(column);
        m_externalDevicePolicy.setValue(relativeColumn, this->fillColor().data());
        m_maskDevicePolicy.setValue(relativeColumn, value);
    }

    void beginProcessing(KisPaintDeviceSP referenceDevice,
                         KisPaintDeviceSP externalDevice,
                         KisPixelSelectionSP maskDevice,
                         KisPixelSelectionSP boundarySelectionDevice,
                         const TileId &tileId,
                         const QRect &workingRect)
    {
        Q_UNUSED(boundarySelectionDevice);

        this->assertIsNotProcessing();
        KIS_ASSERT(referenceDevice);
        KIS_ASSERT(externalDevice);
        KIS_ASSERT(maskDevice);

        BasePolicy<SelectionPolicyType_>::beginProcessing(referenceDevice, tileId, workingRect);
        m_referenceDevicePolicy.beginProcessing(referenceDevice, this->tileRect());
        m_externalDevicePolicy.beginProcessing(externalDevice, this->tileRect());
        m_maskDevicePolicy.beginProcessing(maskDevice, this->tileRect());
        setWorkingRow(this->tileSubRect().top());
    }

    void endProcessing()
    {
        this->assertIsProcessing();
        BasePolicy<SelectionPolicyType_>::endProcessing();
    }

private:
    ReferenceDevicePolicy<true> m_referenceDevicePolicy;
    MaskDevicePolicy<true> m_maskDevicePolicy;
    ExternalDevicePolicy_ m_externalDevicePolicy;
};

template <typename SelectionPolicyType_, typename MaskDevicePolicy_>
struct WriteToMaskDeviceTilePolicy : public BasePolicy<SelectionPolicyType_>
{
    WriteToMaskDeviceTilePolicy(const KoColor &fillColor, const SelectionPolicyType_ &selectionPolicy)
        : BasePolicy<SelectionPolicyType_>(fillColor, selectionPolicy)
    {}

    void setWorkingRow(qint32 row)
    {
        this->assertIsProcessing();
        this->assertRowIsValid(row);
        const qint32 relativeRow = this->relativeRow(row);
        m_referenceDevicePolicy.setWorkingRow(relativeRow);
        m_maskDevicePolicy.setWorkingRow(relativeRow);
    }

    bool isAlreadySet(qint32 column)
    {
        this->assertIsProcessing();
        this->assertColumnIsValid(column);
        const qint32 relativeColumn = this->relativeColumn(column);
        return m_maskDevicePolicy.isAlreadySet(relativeColumn);
    }

    bool isInsideBoundarySelection(qint32 column)
    {
        this->assertIsProcessing();
        this->assertColumnIsValid(column);
        return true;
    }

    quint8 calculateOpacity(qint32 column) const
    {
        this->assertIsProcessing();
        this->assertColumnIsValid(column);
        const qint32 relativeColumn = this->relativeColumn(column);
        return m_referenceDevicePolicy.calculateOpacity(relativeColumn, this->selectionPolicy());
    }

    void setValue(qint32 column, quint8 value)
    {
        this->assertIsProcessing();
        this->assertColumnIsValid(column);
        const qint32 relativeColumn = this->relativeColumn(column);
        m_maskDevicePolicy.setValue(relativeColumn, value);
    }

    void beginProcessing(KisPaintDeviceSP referenceDevice,
                         KisPaintDeviceSP externalDevice,
                         KisPixelSelectionSP maskDevice,
                         KisPixelSelectionSP boundarySelectionDevice,
                         const TileId &tileId,
                         const QRect &workingRect)
    {
        Q_UNUSED(externalDevice);
        Q_UNUSED(boundarySelectionDevice);

        this->assertIsNotProcessing();
        KIS_ASSERT(referenceDevice);
        KIS_ASSERT(maskDevice);

        BasePolicy<SelectionPolicyType_>::beginProcessing(referenceDevice, tileId, workingRect);
        m_referenceDevicePolicy.beginProcessing(referenceDevice, this->tileRect());
        m_maskDevicePolicy.beginProcessing(maskDevice, this->tileRect());
        setWorkingRow(this->tileSubRect().top());
    }

    void endProcessing()
    {
        this->assertIsProcessing();
        BasePolicy<SelectionPolicyType_>::endProcessing();
    }

private:
    ReferenceDevicePolicy<true> m_referenceDevicePolicy;
    MaskDevicePolicy_ m_maskDevicePolicy;
};

template <typename SelectionPolicyType_, typename MaskDevicePolicy_, typename BoundarySelectionPolicy_>
struct WriteToMaskDeviceWithBoundarySelectionDeviceTilePolicy : public BasePolicy<SelectionPolicyType_>
{
    WriteToMaskDeviceWithBoundarySelectionDeviceTilePolicy(const KoColor &fillColor, const SelectionPolicyType_ &selectionPolicy)
        : BasePolicy<SelectionPolicyType_>(fillColor, selectionPolicy)
    {}

    void setWorkingRow(qint32 row)
    {
        this->assertIsProcessing();
        this->assertRowIsValid(row);
        const qint32 relativeRow = this->relativeRow(row);
        m_referenceDevicePolicy.setWorkingRow(relativeRow);
        m_maskDevicePolicy.setWorkingRow(relativeRow);
        m_boundarySelectionPolicy.setWorkingRow(relativeRow);
    }

    bool isAlreadySet(qint32 column)
    {
        this->assertIsProcessing();
        this->assertColumnIsValid(column);
        const qint32 relativeColumn = this->relativeColumn(column);
        return m_maskDevicePolicy.isAlreadySet(relativeColumn);
    }

    bool isInsideBoundarySelection(qint32 column)
    {
        this->assertIsProcessing();
        this->assertColumnIsValid(column);
        const qint32 relativeColumn = this->relativeColumn(column);
        return m_boundarySelectionPolicy.isInsideBoundarySelection(relativeColumn);
    }

    quint8 calculateOpacity(qint32 column) const
    {
        this->assertIsProcessing();
        this->assertColumnIsValid(column);
        const qint32 relativeColumn = this->relativeColumn(column);
        return m_referenceDevicePolicy.calculateOpacity(relativeColumn, this->selectionPolicy());
    }

    void setValue(qint32 column, quint8 value)
    {
        this->assertIsProcessing();
        this->assertColumnIsValid(column);
        const qint32 relativeColumn = this->relativeColumn(column);
        m_maskDevicePolicy.setValue(relativeColumn, value);
    }

    void beginProcessing(KisPaintDeviceSP referenceDevice,
                         KisPaintDeviceSP externalDevice,
                         KisPixelSelectionSP maskDevice,
                         KisPixelSelectionSP boundarySelectionDevice,
                         const TileId &tileId,
                         const QRect &workingRect)
    {
        Q_UNUSED(externalDevice);

        this->assertIsNotProcessing();
        KIS_ASSERT(referenceDevice);
        KIS_ASSERT(maskDevice);
        KIS_ASSERT(boundarySelectionDevice);

        BasePolicy<SelectionPolicyType_>::beginProcessing(referenceDevice, tileId, workingRect);
        m_referenceDevicePolicy.beginProcessing(referenceDevice, this->tileRect());
        m_maskDevicePolicy.beginProcessing(maskDevice, this->tileRect());
        m_boundarySelectionPolicy.beginProcessing(boundarySelectionDevice, this->tileRect());
        setWorkingRow(this->tileSubRect().top());
    }

    void endProcessing()
    {
        this->assertIsProcessing();
        BasePolicy<SelectionPolicyType_>::endProcessing();
    }

private:
    ReferenceDevicePolicy<true> m_referenceDevicePolicy;
    MaskDevicePolicy_ m_maskDevicePolicy;
    BoundarySelectionPolicy_ m_boundarySelectionPolicy;
};

template <typename SelectionPolicyType_, typename GroupSplitPolicy_>
struct GroupSplitTilePolicy : public BasePolicy<SelectionPolicyType_>
{
    GroupSplitTilePolicy(const KoColor &fillColor, const SelectionPolicyType_ &selectionPolicy)
        : BasePolicy<SelectionPolicyType_>(fillColor, selectionPolicy)
    {}

    void setGroupIndex(qint32 groupIndex) {
        m_groupSplitPolicy.setGroupIndex(groupIndex);
    }

    void setWorkingRow(qint32 row)
    {
        this->assertIsProcessing();
        this->assertRowIsValid(row);
        const qint32 relativeRow = this->relativeRow(row);
        m_referenceDevicePolicy.setWorkingRow(relativeRow);
        m_maskDevicePolicy.setWorkingRow(relativeRow);
        m_groupSplitPolicy.setWorkingRow(relativeRow);
    }

    bool isAlreadySet(qint32 column)
    {
        this->assertIsProcessing();
        this->assertColumnIsValid(column);
        const qint32 relativeColumn = this->relativeColumn(column);
        return m_maskDevicePolicy.isAlreadySet(relativeColumn);
    }

    bool isInsideBoundarySelection(qint32 column)
    {
        this->assertIsProcessing();
        this->assertColumnIsValid(column);
        return true;
    }

    quint8 calculateOpacity(qint32 column) const
    {
        this->assertIsProcessing();
        this->assertColumnIsValid(column);
        const qint32 relativeColumn = this->relativeColumn(column);
        return m_referenceDevicePolicy.calculateOpacity(relativeColumn, this->selectionPolicy());
    }

    void setValue(qint32 column, quint8 value)
    {
        this->assertIsProcessing();
        this->assertColumnIsValid(column);
        const qint32 relativeColumn = this->relativeColumn(column);
        // erase the scribble
        const quint32 nullValue = 0;
        m_referenceDevicePolicy.setValue(relativeColumn, reinterpret_cast<const quint8*>(&nullValue));
        m_maskDevicePolicy.setValue(relativeColumn, value);
        m_groupSplitPolicy.setValue(relativeColumn);
    }

    void beginProcessing(KisPaintDeviceSP referenceDevice,
                         KisPaintDeviceSP externalDevice,
                         KisPixelSelectionSP maskDevice,
                         KisPixelSelectionSP boundarySelectionDevice,
                         const TileId &tileId,
                         const QRect &workingRect)
    {
        Q_UNUSED(boundarySelectionDevice);

        this->assertIsNotProcessing();
        KIS_ASSERT(referenceDevice);
        KIS_ASSERT(externalDevice);
        KIS_ASSERT(maskDevice);

        BasePolicy<SelectionPolicyType_>::beginProcessing(referenceDevice, tileId, workingRect);
        m_referenceDevicePolicy.beginProcessing(referenceDevice, this->tileRect());
        m_groupSplitPolicy.beginProcessing(externalDevice, this->tileRect());
        m_maskDevicePolicy.beginProcessing(maskDevice, this->tileRect());
        setWorkingRow(this->tileSubRect().top());
    }

    void endProcessing()
    {
        this->assertIsProcessing();
        BasePolicy<SelectionPolicyType_>::endProcessing();
    }

private:
    ReferenceDevicePolicy<false> m_referenceDevicePolicy;
    MaskDevicePolicy<true> m_maskDevicePolicy;
    GroupSplitPolicy_ m_groupSplitPolicy;
};

} // namespace detail

template <typename SelectionPolicyType_>
using WriteToReferenceDeviceTilePolicy = detail::WriteToReferenceDeviceTilePolicy<SelectionPolicyType_>;

template <typename SelectionPolicyType_>
using WriteToAlignedExternalDeviceTilePolicy = detail::WriteToExternalDeviceTilePolicy<SelectionPolicyType_, detail::ExternalDevicePolicy<true>>;

template <typename SelectionPolicyType_>
using WriteToUnalignedExternalDeviceTilePolicy = detail::WriteToExternalDeviceTilePolicy<SelectionPolicyType_, detail::ExternalDevicePolicy<false>>;

template <typename SelectionPolicyType_>
using WriteToAlignedMaskDeviceTilePolicy = detail::WriteToMaskDeviceTilePolicy<SelectionPolicyType_, detail::MaskDevicePolicy<true>>;

template <typename SelectionPolicyType_>
using WriteToUnalignedMaskDeviceTilePolicy = detail::WriteToMaskDeviceTilePolicy<SelectionPolicyType_, detail::MaskDevicePolicy<false>>;

template <typename SelectionPolicyType_>
using WriteToAlignedMaskDeviceWithAlignedBoundarySelectionDeviceTilePolicy =
    detail::WriteToMaskDeviceWithBoundarySelectionDeviceTilePolicy<SelectionPolicyType_,
                                                                   detail::MaskDevicePolicy<true>,
                                                                   detail::BoundarySelectionDevicePolicy<true>>;

template <typename SelectionPolicyType_>
using WriteToAlignedMaskDeviceWithUnalignedBoundarySelectionDeviceTilePolicy =
    detail::WriteToMaskDeviceWithBoundarySelectionDeviceTilePolicy<SelectionPolicyType_,
                                                                   detail::MaskDevicePolicy<true>,
                                                                   detail::BoundarySelectionDevicePolicy<false>>;

template <typename SelectionPolicyType_>
using WriteToUnalignedMaskDeviceWithAlignedBoundarySelectionDeviceTilePolicy =
    detail::WriteToMaskDeviceWithBoundarySelectionDeviceTilePolicy<SelectionPolicyType_,
                                                                   detail::MaskDevicePolicy<false>,
                                                                   detail::BoundarySelectionDevicePolicy<true>>;

template <typename SelectionPolicyType_>
using WriteToUnalignedMaskDeviceWithUnalignedBoundarySelectionDeviceTilePolicy =
    detail::WriteToMaskDeviceWithBoundarySelectionDeviceTilePolicy<SelectionPolicyType_,
                                                                   detail::MaskDevicePolicy<false>,
                                                                   detail::BoundarySelectionDevicePolicy<false>>;

template <typename SelectionPolicyType_>
using AlignedGroupSplitTilePolicy = detail::GroupSplitTilePolicy<SelectionPolicyType_, detail::GroupSplitPolicy<true>>;

template <typename SelectionPolicyType_>
using UnalignedGroupSplitTilePolicy = detail::GroupSplitTilePolicy<SelectionPolicyType_, detail::GroupSplitPolicy<false>>;

}

#endif
