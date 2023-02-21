#ifndef KOCOMPOSITEOPWETCOLORBURN_H
#define KOCOMPOSITEOPWETCOLORBURN_H

#include "KoCompositeOpBase.h"
#include "KoCompositeOpFunctions.h"
#include "KoCompositeOpRegistry.h"

/**
 * Wet Normal compositor - uses the greater of two alpha values as final alpha,
 * color channels are blended with Over function (Normal blend).
 * Mathematicaly it's basically Greather-than for Alpha channel.
 *
 * Why prefix Wet **** mode.
 * Name Wet is used to help artist understand how it can be used.
 * In traditional mediums when paint is still wet two separate strokes of paint
 * tend to merge into one, blending pigments at the same time.
 * This blending produces similart effect to described above while
 * Normal mode (Over) is more similar on to applying wet stroke over dry stroke.
 *
 * Second part of the mode name is what blending function is used for
 * the color channels data.
 */

template<class CS_Traits>
class KoCompositeOpWetColorBurn : public KoCompositeOpBase<CS_Traits, KoCompositeOpWetColorBurn<CS_Traits>>
{
    typedef KoCompositeOpBase<CS_Traits, KoCompositeOpWetColorBurn<CS_Traits>> base_class;
    typedef typename CS_Traits::channels_type channels_type;
    typedef typename KoColorSpaceMathsTraits<typename CS_Traits::channels_type>::compositetype composite_type;

    static const qint8 channels_nb = CS_Traits::channels_nb;
    static const qint8 alpha_pos = CS_Traits::alpha_pos;

public:
    KoCompositeOpWetColorBurn(const KoColorSpace *cs)
        : base_class(cs, COMPOSITE_WET_COLOR_BURN, KoCompositeOp::categoryMix())
    {
    }

public:
    template<bool alphaLocked, bool allChannelFlags>
    inline static channels_type composeColorChannels(const channels_type *src,
                                                     channels_type srcAlpha,
                                                     channels_type *dst,
                                                     channels_type dstAlpha,
                                                     channels_type maskAlpha,
                                                     channels_type opacity,
                                                     const QBitArray &channelFlags)
    {
        using namespace Arithmetic;

        // Standard color channels blending (check KoCompositeOpGeneric.h).
        srcAlpha = mul(srcAlpha, maskAlpha, opacity);

        if (alphaLocked) {
            if (dstAlpha != zeroValue<channels_type>()) {
                for (qint32 i = 0; i < channels_nb; i++) {
                    if (i != alpha_pos && (allChannelFlags || channelFlags.testBit(i)))
                        dst[i] = lerp(dst[i], cfColorBurn(src[i], dst[i]), srcAlpha);
                }
            }

            return dstAlpha;
        } else {
            channels_type newDstAlpha = unionShapeOpacity(srcAlpha, dstAlpha);

            if (newDstAlpha != zeroValue<channels_type>()) {
                for (qint32 i = 0; i < channels_nb; i++) {
                    if (i != alpha_pos && (allChannelFlags || channelFlags.testBit(i))) {
                        //channels_type result = blend(src[i], srcAlpha, dst[i], dstAlpha, src[i]);
                        channels_type result = blend(src[i], srcAlpha, dst[i], dstAlpha, cfColorBurn(src[i], dst[i]));
                        dst[i] = div(result, newDstAlpha);
                    }
                }
            }

            /************************************************************
             * This section calculates and returns blended (Wet) Alpha. *
             ************************************************************/

            /*
             * Original Greater mode formula. It creates small smooth dip
             * on the edge where two Alpha channels meet.
             *
             * 1/(1+exp(-x)) is the sigmoid function, there d-s would be x,
             * the scalar (40) makes the slope more pronounced.
             * Then that is used to interpolate between the two channels.
             * It might be not the best solution, it produces values that are
             * not present in any of the curves. This might produce halo on
             * the edges of strokes.
            */
#if 0
            float dA = scale<float>(dstAlpha);

            float w = 1.0 / (1.0 + exp(-40.0 * (dA - scale<float>(srcAlpha))));
            float a = dA * w + scale<float>(srcAlpha) * (1.0 - w);
            if (a < 0.0f) {
                a = 0.0f;
            }
            if (a > 1.0f) {
                a = 1.0f;
            }

            if (a < dA)
                a = dA;
            newDstAlpha = scale<channels_type>(a);
#endif

            /*
             * Use max(d, s) for simple union of Alpha channels.
             * Standard blending would be:
             * newDstAlpha = unionShapeOpacity(srcAlpha, dstAlpha)
            */

            // TODO Add 2 value max to Arithemtic?
            // float a = max(scale<float>(dstAlpha), scale<float>(srcAlpha));

            float dA = scale<float>(dstAlpha);
            float a = scale<float>(srcAlpha);

            if (a < dA)
                a = dA;
            newDstAlpha = scale<channels_type>(a);

            return newDstAlpha;
        }
    }
};

#endif // KOCOMPOSITEOPWETCOLORBURN_H
