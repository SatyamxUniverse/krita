/*
 *  Copyright (c) 2006-2007 Cyrille Berger <cberger@cberger.net>
 *  Copyright (c) 2016-2017 L. E. Segovia <leo.segovia@siggraph.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#ifndef _KO_LAB_COLORSPACE_TRAITS_H_
#define _KO_LAB_COLORSPACE_TRAITS_H_

#include <KoLabColorSpaceMaths.h>

/**
 * LAB traits, it provides some convenient functions to
 * access LAB channels through an explicit API.
 *
 * Use this class in conjonction with KoColorSpace::toLabA16 and
 * KoColorSpace::fromLabA16 data.
 *
 * Example:
 * quint8* p = KoLabU16Traits::allocate(1);
 * oneKoColorSpace->toLabA16(somepointertodata, p, 1);
 * KoLabU16Traits::setL( p, KoLabU16Traits::L(p) / 10 );
 * oneKoColorSpace->fromLabA16(p, somepointertodata, 1);
 */
template<typename _channels_type_>
struct KoLabTraits : public KoColorSpaceTrait<_channels_type_, 4, 3> {
    typedef _channels_type_ channels_type;
    typedef KoColorSpaceTrait<_channels_type_, 4, 3> parent;
    static const qint32 L_pos = 0;
    static const qint32 a_pos = 1;
    static const qint32 b_pos = 2;

    /**
     * An Lab pixel
     */
    struct Pixel {
        channels_type L;
        channels_type a;
        channels_type b;
        channels_type alpha;
    };

    /// @return the L component
    inline static channels_type L(quint8* data) {
        channels_type* d = parent::nativeArray(data);
        return d[L_pos];
    }
    /// Set the L component
    inline static void setL(quint8* data, channels_type nv) {
        channels_type* d = parent::nativeArray(data);
        d[L_pos] = nv;
    }
    /// @return the a component
    inline static channels_type a(quint8* data) {
        channels_type* d = parent::nativeArray(data);
        return d[a_pos];
    }
    /// Set the a component
    inline static void setA(quint8* data, channels_type nv) {
        channels_type* d = parent::nativeArray(data);
        d[a_pos] = nv;
    }
    /// @return the b component
    inline static channels_type b(quint8* data) {
        channels_type* d = parent::nativeArray(data);
        return d[b_pos];
    }
    /// Set the a component
    inline static void setB(quint8* data, channels_type nv) {
        channels_type* d = parent::nativeArray(data);
        d[b_pos] = nv;
    }
};

//For quint* values must range from 0 to 1 - see KoColorSpaceMaths<double, quint*>

// https://github.com/mm2/Little-CMS/blob/master/src/cmspcs.c
//PCS in Lab2 is encoded as:
//              8 bit Lab PCS:
//                     L*      0..100 into a 0..ff byte.
//                     a*      t + 128 range is -128.0  +127.0
//                     b*
//             16 bit Lab PCS:
//                     L*     0..100  into a 0..ff00 word.
//                     a*     t + 128  range is  -128.0  +127.9961
//                     b*
//Version 4
//---------
//CIELAB (16 bit)     L*            0 -> 100.0          0x0000 -> 0xffff
//CIELAB (16 bit)     a*            -128.0 -> +127      0x0000 -> 0x8080 -> 0xffff
//CIELAB (16 bit)     b*            -128.0 -> +127      0x0000 -> 0x8080 -> 0xffff

struct KoLabU8Traits : public KoLabTraits<quint8> {

    // Maps each channel to the ranges above.
    // It works when using integers b/c it is always 0..max -- which is not in fp
    // I've redone the implementation to be consistent with the fp version (for clarity) --LES
    inline static void normalisedChannelsValue(const quint8 *pixel, QVector<float> &channels) {
        Q_ASSERT((int)channels.count() >= (int)parent::channels_nb);
        channels_type c;
        for (uint i = 0; i < parent::channels_nb; i++) {
            c = nativeArray(pixel)[i];
            switch (i) {
            case L_pos:
                channels[i] = ((qreal)c) / KoLabColorSpaceMathsTraits<channels_type>::unitValueL;
                break;
            case a_pos:
            case b_pos:
               channels[i] = (((qreal)c) - KoLabColorSpaceMathsTraits<channels_type>::halfValueAB) / KoLabColorSpaceMathsTraits<channels_type>::unitValueAB;
                break;
            case 3:
                channels[i] = ((qreal)c) / UINT8_MAX;
                break;
            default:
                channels[i] = ((qreal)c) / KoLabColorSpaceMathsTraits<channels_type>::unitValue;
                break;
            }
        }
    }

    inline static QString normalisedChannelValueText(const quint8 *pixel, quint32 channelIndex) {
        if (channelIndex > parent::channels_nb) return QString("Error");
        channels_type c = nativeArray(pixel)[channelIndex];
        switch (channelIndex) {
        case L_pos:
            return QString().setNum(100.0 * ((qreal)c) / KoLabColorSpaceMathsTraits<channels_type>::unitValueL);
        case a_pos:
        case b_pos:
            return QString().setNum(100.0 * ((((qreal)c) - KoLabColorSpaceMathsTraits<channels_type>::halfValueAB) / KoLabColorSpaceMathsTraits<channels_type>::unitValueAB));
        case 3:
            return QString().setNum(100.0 * ((qreal)c) / UINT8_MAX);
        default:
            return QString("Error");
        }
    }

    inline static void fromNormalisedChannelsValue(quint8 *pixel, const QVector<float> &values) {
        Q_ASSERT((int)values.count() >= (int)parent::channels_nb);
        channels_type c;
        for (uint i = 0; i < channels_nb; i++) {
            float b = 0;

            switch (i) {
            case L_pos:
                b = qBound((float)0,
                           (float)KoLabColorSpaceMathsTraits<channels_type>::unitValueL * values[i],
                           (float)KoLabColorSpaceMathsTraits<channels_type>::unitValueL);
                break;
            case a_pos:
            case b_pos:
                b = qBound((float)0,
                           (float)KoLabColorSpaceMathsTraits<channels_type>::unitValueAB * values[i],
                           (float)KoLabColorSpaceMathsTraits<channels_type>::unitValueAB);
                break;
            default:
                b = qBound((float)KoLabColorSpaceMathsTraits<channels_type>::min,
                           (float)KoLabColorSpaceMathsTraits<channels_type>::unitValue * values[i],
                           (float)KoLabColorSpaceMathsTraits<channels_type>::max);
                break;
            }
            c = (channels_type)b;
            nativeArray(pixel)[i] = c;
        }
    }
};

struct KoLabU16Traits : public KoLabTraits<quint16> {

    inline static void normalisedChannelsValue(const quint8 *pixel, QVector<float> &channels) {
        Q_ASSERT((int)channels.count() >= (int)parent::channels_nb);
        channels_type c;
        for (uint i = 0; i < parent::channels_nb; i++) {
            c = nativeArray(pixel)[i];
            switch (i) {
            case L_pos:
                channels[i] = ((qreal)c) / KoLabColorSpaceMathsTraits<channels_type>::unitValueL;
                break;
            case a_pos:
            case b_pos:
                channels[i] = (((qreal)c) - KoLabColorSpaceMathsTraits<channels_type>::halfValueAB) / KoLabColorSpaceMathsTraits<channels_type>::unitValueAB;
                break;
            case 3:
                channels[i] = ((qreal)c) / UINT16_MAX;
                break;
            default:
                channels[i] = ((qreal)c) / KoLabColorSpaceMathsTraits<channels_type>::unitValue;
                break;
            }
        }
    }

    inline static QString normalisedChannelValueText(const quint8 *pixel, quint32 channelIndex) {
        if (channelIndex > parent::channels_nb) return QString("Error");
        channels_type c = nativeArray(pixel)[channelIndex];
        switch (channelIndex) {
        case L_pos:
            return QString().setNum(100.0 * ((qreal)c) / KoLabColorSpaceMathsTraits<channels_type>::unitValueL);
        case a_pos:
        case b_pos:
            return QString().setNum(100.0 * ((((qreal)c) - KoLabColorSpaceMathsTraits<channels_type>::halfValueAB) / KoLabColorSpaceMathsTraits<channels_type>::unitValueAB));
        case 3:
            return QString().setNum(100.0 * ((qreal)c) / UINT16_MAX);
        default:
            return QString("Error");
        }
    }

    inline static void fromNormalisedChannelsValue(quint8 *pixel, const QVector<float> &values) {
        Q_ASSERT((int)values.count() >= (int)parent::channels_nb);
        channels_type c;
        for (uint i = 0; i < channels_nb; i++) {
            float b = 0;

            switch (i) {
            case L_pos:
                b = qBound((float)0,
                           (float)KoLabColorSpaceMathsTraits<channels_type>::unitValueL * values[i],
                           (float)KoLabColorSpaceMathsTraits<channels_type>::unitValueL);
                break;
            case a_pos:
            case b_pos:
                b = qBound((float)0,
                           (float)KoLabColorSpaceMathsTraits<channels_type>::unitValueAB * values[i],
                           (float)KoLabColorSpaceMathsTraits<channels_type>::unitValueAB);
                break;
            default:
                b = qBound((float)KoLabColorSpaceMathsTraits<channels_type>::min,
                           (float)KoLabColorSpaceMathsTraits<channels_type>::unitValue * values[i],
                           (float)KoLabColorSpaceMathsTraits<channels_type>::max);
                break;
            }
            c = (channels_type)b;
            nativeArray(pixel)[i] = c;
        }
    }
};

// Float values are normalized to [0..100], [-128..+127], [-128..+127] - out of range values are clipped

#include <KoConfig.h>
#ifdef HAVE_OPENEXR
#include <half.h>

struct KoLabF16Traits : public KoLabTraits<half> {

    inline static QString normalisedChannelValueText(const quint8 *pixel, quint32 channelIndex) {
        if (channelIndex > parent::channels_nb) return QString("Error");
        channels_type c = nativeArray(pixel)[channelIndex];
        switch (channelIndex) {
        case L_pos:
            return QString().setNum(100.0 * qBound((qreal)0,
                                                   ((qreal)c) / KoLabColorSpaceMathsTraits<channels_type>::unitValueL,
                                                   (qreal)KoLabColorSpaceMathsTraits<channels_type>::unitValueL));
        case a_pos:
        case b_pos:
            return QString().setNum(100.0 * qBound((qreal)0,
                                                   (((qreal)c) - KoLabColorSpaceMathsTraits<channels_type>::halfValueAB) / KoLabColorSpaceMathsTraits<channels_type>::unitValueAB,
                                                   (qreal)KoLabColorSpaceMathsTraits<channels_type>::unitValueAB));
        case 3:
            return QString().setNum(100.0 * qBound((qreal)0,
                                                   ((qreal)c) / KoLabColorSpaceMathsTraits<channels_type>::unitValue,
                                                   (qreal)KoLabColorSpaceMathsTraits<channels_type>::unitValue));
        default:
            return QString("Error");
        }
    }
    inline static void normalisedChannelsValue(const quint8 *pixel, QVector<float> &channels) {
        Q_ASSERT((int)channels.count() >= (int)parent::channels_nb);
        channels_type c;
        for (uint i = 0; i < parent::channels_nb; i++) {
            c = nativeArray(pixel)[i];
            switch (i) {
            case L_pos:
                channels[i] = qBound((qreal)0,
                                     ((qreal)c) / KoLabColorSpaceMathsTraits<channels_type>::unitValueL,
                                     (qreal)KoLabColorSpaceMathsTraits<channels_type>::unitValueL);
                break;
            case a_pos:
            case b_pos:
                channels[i] = qBound((qreal)0,
                                     (((qreal)c) - KoLabColorSpaceMathsTraits<channels_type>::halfValueAB) / KoLabColorSpaceMathsTraits<channels_type>::unitValueAB,
                                     (qreal)KoLabColorSpaceMathsTraits<channels_type>::unitValueAB);
                break;
            // As per KoChannelInfo alpha channels are [0..1]
            case 3:
            default:
                channels[i] = qBound((qreal)0,
                                     ((qreal)c) / KoLabColorSpaceMathsTraits<channels_type>::unitValue,
                                     (qreal)KoLabColorSpaceMathsTraits<channels_type>::unitValue);
                break;
            }
        }
    }
    inline static void fromNormalisedChannelsValue(quint8 *pixel, const QVector<float> &values) {
        Q_ASSERT((int)values.count() >= (int)parent::channels_nb);
        channels_type c;
        for (uint i = 0; i < parent::channels_nb; i++) {
            float b = 0;
            switch(i) {
            case L_pos:
                b = qBound((float)0,
                           (float)KoLabColorSpaceMathsTraits<channels_type>::unitValueL * values[i],
                           (float)KoLabColorSpaceMathsTraits<channels_type>::unitValueL);
                break;
            case a_pos:
            case b_pos:
                b = qBound((float)0,
                           (float)KoLabColorSpaceMathsTraits<channels_type>::unitValueAB * values[i],
                           (float)KoLabColorSpaceMathsTraits<channels_type>::unitValueAB);
                break;
            case 3:
                b = qBound((float)KoLabColorSpaceMathsTraits<channels_type>::min,
                           (float)KoLabColorSpaceMathsTraits<channels_type>::unitValue * values[i],
                           (float)KoLabColorSpaceMathsTraits<channels_type>::max);
            default:
                break;
            }
            c = (channels_type)b;
            parent::nativeArray(pixel)[i] = c;
        }
    }
};

#endif

struct KoLabF32Traits : public KoLabTraits<float> {
    // Lab has some... particulars
    inline static QString normalisedChannelValueText(const quint8 *pixel, quint32 channelIndex) {
        if (channelIndex > parent::channels_nb) return QString("Error");
        channels_type c = nativeArray(pixel)[channelIndex];
        switch (channelIndex) {
        case L_pos:
            return QString().setNum(100.0 * qBound((qreal)0,
                                                   ((qreal)c) / KoLabColorSpaceMathsTraits<channels_type>::unitValueL,
                                                   (qreal)KoLabColorSpaceMathsTraits<channels_type>::unitValueL));
        case a_pos:
        case b_pos:
            return QString().setNum(100.0 * qBound((qreal)0,
                                                   (((qreal)c) - KoLabColorSpaceMathsTraits<channels_type>::halfValueAB) / KoLabColorSpaceMathsTraits<channels_type>::unitValueAB,
                                                   (qreal)KoLabColorSpaceMathsTraits<channels_type>::unitValueAB));
        case 3:
            return QString().setNum(100.0 * qBound((qreal)0,
                                                   ((qreal)c) / KoLabColorSpaceMathsTraits<channels_type>::unitValue,
                                                   (qreal)KoLabColorSpaceMathsTraits<channels_type>::unitValue));
        default:
            return QString("Error");
        }
    }
    inline static void normalisedChannelsValue(const quint8 *pixel, QVector<float> &channels) {
        Q_ASSERT((int)channels.count() >= (int)parent::channels_nb);
        channels_type c;
        for (uint i = 0; i < parent::channels_nb; i++) {
            c = nativeArray(pixel)[i];
            switch (i) {
            case L_pos:
                channels[i] = qBound((qreal)0,
                                     ((qreal)c) / KoLabColorSpaceMathsTraits<channels_type>::unitValueL,
                                     (qreal)KoLabColorSpaceMathsTraits<channels_type>::unitValueL);
                break;
            case a_pos:
            case b_pos:
                channels[i] = qBound((qreal)0,
                                     (((qreal)c) - KoLabColorSpaceMathsTraits<channels_type>::halfValueAB) / KoLabColorSpaceMathsTraits<channels_type>::unitValueAB,
                                     (qreal)KoLabColorSpaceMathsTraits<channels_type>::unitValueAB);
                break;
            // As per KoChannelInfo alpha channels are [0..1]
            case 3:
            default:
                channels[i] = qBound((qreal)0,
                                     ((qreal)c) / KoLabColorSpaceMathsTraits<channels_type>::unitValue,
                                     (qreal)KoLabColorSpaceMathsTraits<channels_type>::unitValue);
                break;
            }
        }
    }
    inline static void fromNormalisedChannelsValue(quint8 *pixel, const QVector<float> &values) {
        Q_ASSERT((int)values.count() >= (int)parent::channels_nb);
        channels_type c;
        for (uint i = 0; i < parent::channels_nb; i++) {
            float b = 0;
            switch(i) {
            case L_pos:
                b = qBound((float)0,
                           (float)KoLabColorSpaceMathsTraits<channels_type>::unitValueL * values[i],
                           (float)KoLabColorSpaceMathsTraits<channels_type>::unitValueL);
                break;
            case a_pos:
            case b_pos:
                b = qBound((float)0,
                           (float)KoLabColorSpaceMathsTraits<channels_type>::unitValueAB * values[i],
                           (float)KoLabColorSpaceMathsTraits<channels_type>::unitValueAB);
                break;
            case 3:
                b = qBound((float)KoLabColorSpaceMathsTraits<channels_type>::min,
                           (float)KoLabColorSpaceMathsTraits<channels_type>::unitValue * values[i],
                           (float)KoLabColorSpaceMathsTraits<channels_type>::max);
            default:
                break;
            }
            c = (channels_type)b;
            parent::nativeArray(pixel)[i] = c;
        }
    }
};

struct KoLabF64Traits : public KoLabTraits<double> {
    // Lab has some... particulars
    inline static QString normalisedChannelValueText(const quint8 *pixel, quint32 channelIndex) {
        if (channelIndex > parent::channels_nb) return QString("Error");
        channels_type c = nativeArray(pixel)[channelIndex];
        switch (channelIndex) {
        case L_pos:
            return QString().setNum(100.0 * qBound((qreal)0,
                                                   ((qreal)c) / KoLabColorSpaceMathsTraits<channels_type>::unitValueL,
                                                   (qreal)KoLabColorSpaceMathsTraits<channels_type>::unitValueL));
        case a_pos:
        case b_pos:
            return QString().setNum(100.0 * qBound((qreal)0,
                                                   (((qreal)c) - KoLabColorSpaceMathsTraits<channels_type>::halfValueAB) / KoLabColorSpaceMathsTraits<channels_type>::unitValueAB,
                                                   (qreal)KoLabColorSpaceMathsTraits<channels_type>::unitValueAB));
        case 3:
            return QString().setNum(100.0 * qBound((qreal)0,
                                                   ((qreal)c) / KoLabColorSpaceMathsTraits<channels_type>::unitValue,
                                                   (qreal)KoLabColorSpaceMathsTraits<channels_type>::unitValue));
        default:
            return QString("Error");
        }
    }
    inline static void normalisedChannelsValue(const quint8 *pixel, QVector<float> &channels) {
        Q_ASSERT((int)channels.count() >= (int)parent::channels_nb);
        channels_type c;
        for (uint i = 0; i < parent::channels_nb; i++) {
            c = nativeArray(pixel)[i];
            switch (i) {
            case L_pos:
                channels[i] = qBound((qreal)0,
                                     ((qreal)c) / KoLabColorSpaceMathsTraits<channels_type>::unitValueL,
                                     (qreal)KoLabColorSpaceMathsTraits<channels_type>::unitValueL);
                break;
            case a_pos:
            case b_pos:
                channels[i] = qBound((qreal)0,
                                     (((qreal)c) - KoLabColorSpaceMathsTraits<channels_type>::halfValueAB) / KoLabColorSpaceMathsTraits<channels_type>::unitValueAB,
                                     (qreal)KoLabColorSpaceMathsTraits<channels_type>::unitValueAB);
                break;
            // As per KoChannelInfo alpha channels are [0..1]
            case 3:
            default:
                channels[i] = qBound((qreal)0,
                                     ((qreal)c) / KoLabColorSpaceMathsTraits<channels_type>::unitValue,
                                     (qreal)KoLabColorSpaceMathsTraits<channels_type>::unitValue);
                break;
            }
        }
    }
    inline static void fromNormalisedChannelsValue(quint8 *pixel, const QVector<float> &values) {
        Q_ASSERT((int)values.count() >= (int)parent::channels_nb);
        channels_type c;
        for (uint i = 0; i < parent::channels_nb; i++) {
            float b = 0;
            switch(i) {
            case L_pos:
                b = qBound((float)0,
                           (float)KoLabColorSpaceMathsTraits<channels_type>::unitValueL * values[i],
                           (float)KoLabColorSpaceMathsTraits<channels_type>::unitValueL);
                break;
            case a_pos:
            case b_pos:
                b = qBound((float)0,
                           (float)KoLabColorSpaceMathsTraits<channels_type>::unitValueAB * values[i],
                           (float)KoLabColorSpaceMathsTraits<channels_type>::unitValueAB);
                break;
            case 3:
                b = qBound((float)KoLabColorSpaceMathsTraits<channels_type>::min,
                           (float)KoLabColorSpaceMathsTraits<channels_type>::unitValue * values[i],
                           (float)KoLabColorSpaceMathsTraits<channels_type>::max);
            default:
                break;
            }
            c = (channels_type)b;
            parent::nativeArray(pixel)[i] = c;
        }
    }
};

#endif
