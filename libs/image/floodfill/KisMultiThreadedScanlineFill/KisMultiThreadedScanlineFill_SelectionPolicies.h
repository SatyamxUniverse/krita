/*
 *  SPDX-FileCopyrightText: 2014 Dmitry Kazakov <dimula73@gmail.com>
 *  SPDX-FileCopyrightText: 2022 Deif Lou <ginoba@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KISMULTITHREADEDSCANLINEFILL_SELECTIONPOLICIES_H
#define KISMULTITHREADEDSCANLINEFILL_SELECTIONPOLICIES_H

#include <KoAlwaysInline.h>

#include <QHash>

#include <KoColor.h>
#include <KoColorSpace.h>

namespace KisMultiThreadedScanlineFillNS
{

class DifferencePolicySlow
{
public:
    ALWAYS_INLINE void initDifferences(KisPaintDeviceSP device, const KoColor &srcPixel, int threshold) {
        m_colorSpace = device->colorSpace();
        m_srcPixel = srcPixel;
        m_srcPixelPtr = m_srcPixel.data();
        m_threshold = threshold;
    }

    ALWAYS_INLINE quint8 calculateDifference(const quint8 *pixelPtr) const {
        if (m_threshold == 1) {
            if (memcmp(m_srcPixelPtr, pixelPtr, m_colorSpace->pixelSize()) == 0) {
                return 0;
            }
            return quint8_MAX;
        }
        else {
            return m_colorSpace->differenceA(m_srcPixelPtr, pixelPtr);
        }
    }

private:
    const KoColorSpace *m_colorSpace {nullptr};
    KoColor m_srcPixel;
    const quint8 *m_srcPixelPtr {nullptr};
    int m_threshold {0};
};

template <typename SrcPixelType>
class DifferencePolicyOptimized
{
    typedef SrcPixelType HashKeyType;
    typedef QHash<HashKeyType, quint8> HashType;

public:
    ALWAYS_INLINE void initDifferences(KisPaintDeviceSP device, const KoColor &srcPixel, int threshold) {
        m_colorSpace = device->colorSpace();
        m_srcPixel = srcPixel;
        m_srcPixelPtr = m_srcPixel.data();
        m_threshold = threshold;
    }

    ALWAYS_INLINE quint8 calculateDifference(const quint8 *pixelPtr) const {
        HashKeyType key = *reinterpret_cast<const HashKeyType*>(pixelPtr);

        quint8 result;

        typename HashType::iterator it = m_differences.find(key);

        if (it != m_differences.end()) {
            result = *it;
        } else {
            if (m_threshold == 1) {
                if (memcmp(m_srcPixelPtr, pixelPtr, m_colorSpace->pixelSize()) == 0) {
                    result = 0;
                }
                else {
                    result = quint8_MAX;
                }
            }
            else {
                result = m_colorSpace->differenceA(m_srcPixelPtr, pixelPtr);
            }
            m_differences.insert(key, result);
        }

        return result;
    }

private:
    mutable HashType m_differences;

    const KoColorSpace *m_colorSpace {nullptr};
    KoColor m_srcPixel;
    const quint8 *m_srcPixelPtr {nullptr};
    int m_threshold {0};
};

class ColorOrTransparentDifferencePolicySlow
{
public:
    ALWAYS_INLINE void initDifferences(KisPaintDeviceSP device, const KoColor &srcPixel, int threshold) {
        m_colorSpace = device->colorSpace();
        m_srcPixel = srcPixel;
        m_srcPixelPtr = m_srcPixel.data();
        m_threshold = threshold;
    }

    ALWAYS_INLINE quint8 calculateDifference(const quint8 *pixelPtr) const {
        if (m_threshold == 1) {
            if (memcmp(m_srcPixelPtr, pixelPtr, m_colorSpace->pixelSize()) == 0 ||
                m_colorSpace->opacityU8(pixelPtr) == 0) {
                return 0;
            }
            return quint8_MAX;
        }
        else {
            const quint8 colorDifference = m_colorSpace->difference(m_srcPixelPtr, pixelPtr);
            const quint8 opacityDifference = m_colorSpace->opacityU8(pixelPtr) * 100 / quint8_MAX;
            return qMin(colorDifference, opacityDifference);
        }
    }

private:
    const KoColorSpace *m_colorSpace {nullptr};
    KoColor m_srcPixel;
    const quint8 *m_srcPixelPtr {nullptr};
    int m_threshold {0};
};

template <typename SrcPixelType>
class ColorOrTransparentDifferencePolicyOptimized
{
    typedef SrcPixelType HashKeyType;
    typedef QHash<HashKeyType, quint8> HashType;

public:
    ALWAYS_INLINE void initDifferences(KisPaintDeviceSP device, const KoColor &srcPixel, int threshold) {
        m_colorSpace = device->colorSpace();
        m_srcPixel = srcPixel;
        m_srcPixelPtr = m_srcPixel.data();
        m_threshold = threshold;
    }

    ALWAYS_INLINE quint8 calculateDifference(const quint8 *pixelPtr) const {
        HashKeyType key = *reinterpret_cast<const HashKeyType*>(pixelPtr);

        quint8 result;

        typename HashType::iterator it = m_differences.find(key);

        if (it != m_differences.end()) {
            result = *it;
        } else {
            const quint8 colorDifference = m_colorSpace->difference(m_srcPixelPtr, pixelPtr);
            const quint8 opacityDifference = m_colorSpace->opacityU8(pixelPtr) * 100 / quint8_MAX;
            result = qMin(colorDifference, opacityDifference);
            m_differences.insert(key, result);
        }

        return result;
    }

private:
    mutable HashType m_differences;

    const KoColorSpace *m_colorSpace {nullptr};
    KoColor m_srcPixel;
    const quint8 *m_srcPixelPtr {nullptr};
    int m_threshold {0};
};

class IsNonNullPolicySlow
{
public:
    ALWAYS_INLINE void initDifferences(KisPaintDeviceSP device, const KoColor &srcPixel, int /*threshold*/)
    {
        Q_UNUSED(srcPixel);
        m_pixelSize = device->pixelSize();
        m_testPixel.resize(m_pixelSize);
    }

    ALWAYS_INLINE quint8 calculateDifference(const quint8 *pixelPtr) const {
        if (memcmp(m_testPixel.data(), pixelPtr, m_pixelSize) == 0) {
            return 0;
        }
        return quint8_MAX;
    }

private:
    int m_pixelSize {0};
    QByteArray m_testPixel;
};

template <typename SrcPixelType>
class IsNonNullPolicyOptimized
{
public:
    ALWAYS_INLINE void initDifferences(KisPaintDeviceSP device, const KoColor &srcPixel, int /*threshold*/) {
        Q_UNUSED(device);
        Q_UNUSED(srcPixel);
    }

    ALWAYS_INLINE quint8 calculateDifference(const quint8 *pixelPtr) const {
        const SrcPixelType *pixel = reinterpret_cast<const SrcPixelType*>(pixelPtr);
        return *pixel == 0;
    }
};

template <class DifferencePolicy>
class HardSelectionPolicy: public DifferencePolicy
{
public:
    HardSelectionPolicy(KisPaintDeviceSP device, const KoColor &srcPixel, int threshold)
        : m_threshold(threshold)
    {
        this->initDifferences(device, srcPixel, threshold);
    }

    ALWAYS_INLINE quint8 calculateOpacity(const quint8 *pixelPtr) const {
        return this->calculateDifference(pixelPtr) <= m_threshold ? MAX_SELECTED : MIN_SELECTED;
    }

protected:
    int m_threshold;
};

template <class DifferencePolicy>
class SoftSelectionPolicy : public HardSelectionPolicy<DifferencePolicy>
{
public:
    SoftSelectionPolicy(KisPaintDeviceSP device, const KoColor &srcPixel, int threshold, int softness)
        : HardSelectionPolicy<DifferencePolicy>(device, srcPixel, threshold)
        , m_softness(softness)
    {}

    ALWAYS_INLINE quint8 calculateOpacity(const quint8 *pixelPtr) const {
        if (m_threshold == 0) {
            return MIN_SELECTED;
        }
        // Integer version of: (threshold - diff) / (threshold * softness)
        const int diff = this->calculateDifference(pixelPtr);
        if (diff < m_threshold) {
            const int v = (m_threshold - diff) * MAX_SELECTED * 100 / (m_threshold * m_softness);
            return v > MAX_SELECTED ? MAX_SELECTED : v;
        } else {
            return MIN_SELECTED;
        }
    }

protected:
    using HardSelectionPolicy<DifferencePolicy>::m_threshold;
    int m_softness;
};

template <class DifferencePolicy>
class SelectAllUntilColorHardSelectionPolicy: public HardSelectionPolicy<DifferencePolicy>
{
public:
    SelectAllUntilColorHardSelectionPolicy(KisPaintDeviceSP device, const KoColor &referenceColor, int threshold)
        : HardSelectionPolicy<DifferencePolicy>(device, referenceColor, threshold)
    {}

    ALWAYS_INLINE quint8 calculateOpacity(const quint8 *pixelPtr) const {
        return this->calculateDifference(pixelPtr) > m_threshold ? MAX_SELECTED : MIN_SELECTED;
    }

protected:
    using HardSelectionPolicy<DifferencePolicy>::m_threshold;
};

template <class DifferencePolicy>
class SelectAllUntilColorSoftSelectionPolicy : public SoftSelectionPolicy<DifferencePolicy>
{
public:
    SelectAllUntilColorSoftSelectionPolicy(KisPaintDeviceSP device, const KoColor &referenceColor, int threshold, int softness)
        : SoftSelectionPolicy<DifferencePolicy>(device, referenceColor, threshold, softness)
    {}

    ALWAYS_INLINE quint8 calculateOpacity(const quint8 *pixelPtr) const {
        if (m_threshold == 0) {
            return MAX_SELECTED;
        }
        // Integer version of: 1 - ((1-threshold) - diff) / ((1-threshold) * softness)
        const int diff = this->calculateDifference(pixelPtr);
        if (diff < m_threshold) {
            const int v = MAX_SELECTED - (m_threshold - diff) * MAX_SELECTED * 100 / (m_threshold * m_softness);
            return v < MIN_SELECTED ? MIN_SELECTED : v;
        } else {
            return MAX_SELECTED;
        }
    }

protected:
    using SoftSelectionPolicy<DifferencePolicy>::m_threshold;
    using SoftSelectionPolicy<DifferencePolicy>::m_softness;
};

class GroupSplitSelectionPolicy
{
public:
    GroupSplitSelectionPolicy(quint8 referenceValue, int threshold)
        : m_threshold(threshold)
        , m_referenceValue(referenceValue)
    {}

    ALWAYS_INLINE quint8 calculateOpacity(const quint8* pixelPtr) const {
        // TODO: either threshold should always be null, or there should be a special
        //       case for *pixelPtr == 0, which is different from all the other groups,
        //       whatever the threshold is
        int diff = qAbs(int(*pixelPtr) - m_referenceValue);
        return diff <= m_threshold ? MAX_SELECTED : MIN_SELECTED;
    }

private:
    int m_threshold;
    quint8 m_referenceValue;
};

}

#endif
