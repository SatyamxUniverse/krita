/*
 * SPDX-FileCopyrightText: 2005 Adrian Page <adrian@pagenet.plus.com>
 * SPDX-FileCopyrightText: 2011 Silvio Heinrich <plassy@web.de>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
*/
#ifndef KOCOMPOSITEOPREGISTRY_H
#define KOCOMPOSITEOPREGISTRY_H

#include <QString>
#include <QList>
#include <QMultiMap>
#include <QBitArray>

#include "kritapigment_export.h"

class KoColorSpace;
#include <KoID.h>

// TODO : convert this data blob into a modern design with an enum class.
// This will reduce the need for runtime string comparisons.

const QString COMPOSITE_OVER         = "normal";
const QString COMPOSITE_ERASE        = "erase";
const QString COMPOSITE_IN           = "in";
const QString COMPOSITE_OUT          = "out";
const QString COMPOSITE_ALPHA_DARKEN = "alphadarken";
const QString COMPOSITE_DESTINATION_IN = "destination-in";
const QString COMPOSITE_DESTINATION_ATOP = "destination-atop";

const QString COMPOSITE_XOR                   = "xor";
const QString COMPOSITE_OR                    = "or";
const QString COMPOSITE_AND                   = "and";
const QString COMPOSITE_NAND                  = "nand";
const QString COMPOSITE_NOR                   = "nor";
const QString COMPOSITE_XNOR                  = "xnor";
const QString COMPOSITE_IMPLICATION           = "implication";
const QString COMPOSITE_NOT_IMPLICATION       = "not_implication";
const QString COMPOSITE_CONVERSE              = "converse";
const QString COMPOSITE_NOT_CONVERSE          = "not_converse";

const QString COMPOSITE_PLUS                  = "plus";
const QString COMPOSITE_MINUS                 = "minus";
const QString COMPOSITE_ADD                   = "add";
const QString COMPOSITE_SUBTRACT              = "subtract";
const QString COMPOSITE_INVERSE_SUBTRACT      = "inverse_subtract";
const QString COMPOSITE_DIFF                  = "diff";
const QString COMPOSITE_MULT                  = "multiply";
const QString COMPOSITE_DIVIDE                = "divide";
const QString COMPOSITE_ARC_TANGENT           = "arc_tangent";
const QString COMPOSITE_GEOMETRIC_MEAN        = "geometric_mean";
const QString COMPOSITE_ADDITIVE_SUBTRACTIVE  = "additive_subtractive";
const QString COMPOSITE_NEGATION              = "negation";

const QString COMPOSITE_MOD                = "modulo";
const QString COMPOSITE_MOD_CON            = "modulo_continuous";
const QString COMPOSITE_DIVISIVE_MOD       = "divisive_modulo";
const QString COMPOSITE_DIVISIVE_MOD_CON   = "divisive_modulo_continuous";
const QString COMPOSITE_MODULO_SHIFT       = "modulo_shift";
const QString COMPOSITE_MODULO_SHIFT_CON   = "modulo_shift_continuous";

const QString COMPOSITE_EQUIVALENCE   = "equivalence";
const QString COMPOSITE_ALLANON       = "allanon";
const QString COMPOSITE_PARALLEL      = "parallel";
const QString COMPOSITE_GRAIN_MERGE   = "grain_merge";
const QString COMPOSITE_GRAIN_EXTRACT = "grain_extract";
const QString COMPOSITE_EXCLUSION     = "exclusion";
const QString COMPOSITE_HARD_MIX      = "hard mix";
const QString COMPOSITE_HARD_MIX_PHOTOSHOP = "hard_mix_photoshop";
const QString COMPOSITE_HARD_MIX_SOFTER_PHOTOSHOP = "hard_mix_softer_photoshop";
const QString COMPOSITE_OVERLAY       = "overlay";
const QString COMPOSITE_BEHIND        = "behind";
const QString COMPOSITE_GREATER       = "greater";
const QString COMPOSITE_HARD_OVERLAY  = "hard overlay";
const QString COMPOSITE_INTERPOLATION = "interpolation";
const QString COMPOSITE_INTERPOLATIONB = "interpolation 2x";
const QString COMPOSITE_PENUMBRAA     = "penumbra a";
const QString COMPOSITE_PENUMBRAB     = "penumbra b";
const QString COMPOSITE_PENUMBRAC     = "penumbra c";
const QString COMPOSITE_PENUMBRAD     = "penumbra d";

const QString COMPOSITE_DARKEN      = "darken";
const QString COMPOSITE_BURN        = "burn";//this is also known as 'color burn'.
const QString COMPOSITE_LINEAR_BURN = "linear_burn";
const QString COMPOSITE_GAMMA_DARK  = "gamma_dark";
const QString COMPOSITE_SHADE_IFS_ILLUSIONS = "shade_ifs_illusions";
const QString COMPOSITE_FOG_DARKEN_IFS_ILLUSIONS = "fog_darken_ifs_illusions";
const QString COMPOSITE_EASY_BURN        = "easy burn";

const QString COMPOSITE_LIGHTEN      = "lighten";
const QString COMPOSITE_DODGE        = "dodge";
const QString COMPOSITE_LINEAR_DODGE = "linear_dodge";
const QString COMPOSITE_SCREEN       = "screen";
const QString COMPOSITE_HARD_LIGHT   = "hard_light";
const QString COMPOSITE_SOFT_LIGHT_IFS_ILLUSIONS = "soft_light_ifs_illusions";
const QString COMPOSITE_SOFT_LIGHT_PEGTOP_DELPHI = "soft_light_pegtop_delphi";
const QString COMPOSITE_SOFT_LIGHT_PHOTOSHOP = "soft_light";
const QString COMPOSITE_SOFT_LIGHT_SVG  = "soft_light_svg";
const QString COMPOSITE_GAMMA_LIGHT  = "gamma_light";
const QString COMPOSITE_GAMMA_ILLUMINATION  = "gamma_illumination";
const QString COMPOSITE_VIVID_LIGHT  = "vivid_light";
const QString COMPOSITE_FLAT_LIGHT   = "flat_light";
const QString COMPOSITE_LINEAR_LIGHT = "linear light";
const QString COMPOSITE_PIN_LIGHT    = "pin_light";
const QString COMPOSITE_PNORM_A        = "pnorm_a";
const QString COMPOSITE_PNORM_B        = "pnorm_b";
const QString COMPOSITE_SUPER_LIGHT  = "super_light";
const QString COMPOSITE_TINT_IFS_ILLUSIONS = "tint_ifs_illusions";
const QString COMPOSITE_FOG_LIGHTEN_IFS_ILLUSIONS = "fog_lighten_ifs_illusions";
const QString COMPOSITE_EASY_DODGE        = "easy dodge";
const QString COMPOSITE_LUMINOSITY_SAI        = "luminosity_sai";


const QString COMPOSITE_HUE            = "hue";
const QString COMPOSITE_COLOR          = "color";
const QString COMPOSITE_SATURATION     = "saturation";
const QString COMPOSITE_INC_SATURATION = "inc_saturation";
const QString COMPOSITE_DEC_SATURATION = "dec_saturation";
const QString COMPOSITE_LUMINIZE       = "luminize";
const QString COMPOSITE_INC_LUMINOSITY = "inc_luminosity";
const QString COMPOSITE_DEC_LUMINOSITY = "dec_luminosity";

const QString COMPOSITE_HUE_HSV            = "hue_hsv";
const QString COMPOSITE_COLOR_HSV          = "color_hsv";
const QString COMPOSITE_SATURATION_HSV     = "saturation_hsv";
const QString COMPOSITE_INC_SATURATION_HSV = "inc_saturation_hsv";
const QString COMPOSITE_DEC_SATURATION_HSV = "dec_saturation_hsv";
const QString COMPOSITE_VALUE              = "value";
const QString COMPOSITE_INC_VALUE          = "inc_value";
const QString COMPOSITE_DEC_VALUE          = "dec_value";

const QString COMPOSITE_HUE_HSL            = "hue_hsl";
const QString COMPOSITE_COLOR_HSL          = "color_hsl";
const QString COMPOSITE_SATURATION_HSL     = "saturation_hsl";
const QString COMPOSITE_INC_SATURATION_HSL = "inc_saturation_hsl";
const QString COMPOSITE_DEC_SATURATION_HSL = "dec_saturation_hsl";
const QString COMPOSITE_LIGHTNESS          = "lightness";
const QString COMPOSITE_INC_LIGHTNESS      = "inc_lightness";
const QString COMPOSITE_DEC_LIGHTNESS      = "dec_lightness";

const QString COMPOSITE_HUE_HSI            = "hue_hsi";
const QString COMPOSITE_COLOR_HSI          = "color_hsi";
const QString COMPOSITE_SATURATION_HSI     = "saturation_hsi";
const QString COMPOSITE_INC_SATURATION_HSI = "inc_saturation_hsi";
const QString COMPOSITE_DEC_SATURATION_HSI = "dec_saturation_hsi";
const QString COMPOSITE_INTENSITY          = "intensity";
const QString COMPOSITE_INC_INTENSITY      = "inc_intensity";
const QString COMPOSITE_DEC_INTENSITY      = "dec_intensity";

const QString COMPOSITE_COPY         = "copy";
const QString COMPOSITE_COPY_RED     = "copy_red";
const QString COMPOSITE_COPY_GREEN   = "copy_green";
const QString COMPOSITE_COPY_BLUE    = "copy_blue";
const QString COMPOSITE_TANGENT_NORMALMAP    = "tangent_normalmap";

const QString COMPOSITE_COLORIZE     = "colorize";
const QString COMPOSITE_BUMPMAP      = "bumpmap";
const QString COMPOSITE_COMBINE_NORMAL = "combine_normal";
const QString COMPOSITE_CLEAR        = "clear";
const QString COMPOSITE_DISSOLVE     = "dissolve";
const QString COMPOSITE_DISPLACE     = "displace";
const QString COMPOSITE_NO           = "nocomposition";
const QString COMPOSITE_PASS_THROUGH = "pass through"; // XXX: not implemented anywhere yet
const QString COMPOSITE_DARKER_COLOR = "darker color";
const QString COMPOSITE_LIGHTER_COLOR = "lighter color";
const QString COMPOSITE_UNDEF        = "undefined";

const QString COMPOSITE_REFLECT   = "reflect";
const QString COMPOSITE_GLOW      = "glow";
const QString COMPOSITE_FREEZE    = "freeze";
const QString COMPOSITE_HEAT      = "heat";
const QString COMPOSITE_GLEAT     = "glow_heat";
const QString COMPOSITE_HELOW     = "heat_glow";
const QString COMPOSITE_REEZE     = "reflect_freeze";
const QString COMPOSITE_FRECT     = "freeze_reflect";
const QString COMPOSITE_FHYRD     = "heat_glow_freeze_reflect_hybrid";

const QString COMPOSITE_LAMBERT_LIGHTING   = "lambert_lighting";
const QString COMPOSITE_LAMBERT_LIGHTING_GAMMA_2_2   = "lambert_lighting_gamma2.2";

// "Wet" Modes
const QString COMPOSITE_WET_OVER = "wet_normal";
//const QString COMPOSITE_WET_ERASE = "wet_erase";
//const QString COMPOSITE_WET_IN = "wet_in";
//const QString COMPOSITE_WET_OUT = "wet_out";
//const QString COMPOSITE_WET_ALPHA_DARKEN = "wet_alphadarken";
//const QString COMPOSITE_WET_DESTINATION_IN = "wet_destination_in";
//const QString COMPOSITE_WET_DESTINATION_ATOP = "wet_destination_atop";

const QString COMPOSITE_WET_XOR = "wet_xor";
const QString COMPOSITE_WET_OR = "wet_or";
const QString COMPOSITE_WET_AND = "wet_and";
const QString COMPOSITE_WET_NAND = "wet_nand";
const QString COMPOSITE_WET_NOR = "wet_nor";
const QString COMPOSITE_WET_XNOR = "wet_xnor";
const QString COMPOSITE_WET_IMPLICATION = "wet_implication";
const QString COMPOSITE_WET_NOT_IMPLICATION = "wet_not_implication";
const QString COMPOSITE_WET_CONVERSE = "wet_converse";
const QString COMPOSITE_WET_NOT_CONVERSE = "wet_not_converse";

const QString COMPOSITE_WET_PLUS = "wet_plus";
const QString COMPOSITE_WET_MINUS = "wet_minus";
const QString COMPOSITE_WET_ADD = "wet_add";
const QString COMPOSITE_WET_SUBTRACT = "wet_subtract";
const QString COMPOSITE_WET_INVERSE_SUBTRACT = "wet_inverse_subtract";
const QString COMPOSITE_WET_DIFF = "wet_diff";
const QString COMPOSITE_WET_MULT = "wet_multiply";
const QString COMPOSITE_WET_DIVIDE = "wet_divide";
const QString COMPOSITE_WET_ARC_TANGENT = "wet_arc_tangent";
const QString COMPOSITE_WET_GEOMETRIC_MEAN = "wet_geometric_mean";
const QString COMPOSITE_WET_ADDITIVE_SUBTRACTIVE = "wet_additive_subtractive";
const QString COMPOSITE_WET_NEGATION = "wet_negation";

const QString COMPOSITE_WET_MOD = "wet_modulo";
const QString COMPOSITE_WET_MOD_CON = "wet_modulo_continuous";
const QString COMPOSITE_WET_DIVISIVE_MOD = "wet_divisive_modulo";
const QString COMPOSITE_WET_DIVISIVE_MOD_CON = "wet_divisive_modulo_continuous";
const QString COMPOSITE_WET_MODULO_SHIFT = "wet_modulo_shift";
const QString COMPOSITE_WET_MODULO_SHIFT_CON = "wet_modulo_shift_continuous";

const QString COMPOSITE_WET_EQUIVALENCE = "wet_equivalence";
const QString COMPOSITE_WET_ALLANON = "wet_allanon";
const QString COMPOSITE_WET_PARALLEL = "wet_parallel";
const QString COMPOSITE_WET_GRAIN_MERGE = "wet_grain_merge";
const QString COMPOSITE_WET_GRAIN_EXTRACT = "wet_grain_extract";
const QString COMPOSITE_WET_EXCLUSION = "wet_exclusion";
const QString COMPOSITE_WET_HARD_MIX = "wet_hard_mix";
const QString COMPOSITE_WET_HARD_MIX_PHOTOSHOP = "wet_hard_mix_photoshop";
const QString COMPOSITE_WET_HARD_MIX_SOFTER_PHOTOSHOP = "wet_hard_mix_softer_photoshop";
const QString COMPOSITE_WET_OVERLAY = "wet_overlay";
const QString COMPOSITE_WET_BEHIND = "wet_behind";

const QString COMPOSITE_WET_HARD_OVERLAY = "wet_hard_overlay";
const QString COMPOSITE_WET_INTERPOLATION = "wet_interpolation";
const QString COMPOSITE_WET_INTERPOLATIONB = "wet_interpolation_2x";
const QString COMPOSITE_WET_PENUMBRAA = "wet_penumbra_a";
const QString COMPOSITE_WET_PENUMBRAB = "wet_penumbra_b";
const QString COMPOSITE_WET_PENUMBRAC = "wet_penumbra_c";
const QString COMPOSITE_WET_PENUMBRAD = "wet_penumbra_d";

const QString COMPOSITE_WET_DARKEN = "wet_darken";
const QString COMPOSITE_WET_BURN = "wet_burn"; // this is also known as 'color burn'.
const QString COMPOSITE_WET_LINEAR_BURN = "wet_linear_burn";
const QString COMPOSITE_WET_GAMMA_DARK = "wet_gamma_dark";
const QString COMPOSITE_WET_SHADE_IFS_ILLUSIONS = "wet_shade_ifs_illusions";
const QString COMPOSITE_WET_FOG_DARKEN_IFS_ILLUSIONS = "wet_fog_darken_ifs_illusions";
const QString COMPOSITE_WET_EASY_BURN = "wet_easy_burn";

const QString COMPOSITE_WET_LIGHTEN = "wet_lighten";
const QString COMPOSITE_WET_DODGE = "wet_dodge";
const QString COMPOSITE_WET_LINEAR_DODGE = "wet_linear_dodge";
const QString COMPOSITE_WET_SCREEN = "wet_screen";
const QString COMPOSITE_WET_HARD_LIGHT = "wet_hard_light";
const QString COMPOSITE_WET_SOFT_LIGHT_IFS_ILLUSIONS = "wet_soft_light_ifs_illusions";
const QString COMPOSITE_WET_SOFT_LIGHT_PEGTOP_DELPHI = "wet_soft_light_pegtop_delphi";
const QString COMPOSITE_WET_SOFT_LIGHT_PHOTOSHOP = "wet_soft_light";
const QString COMPOSITE_WET_SOFT_LIGHT_SVG = "wet_soft_light_svg";
const QString COMPOSITE_WET_GAMMA_LIGHT = "wet_gamma_light";
const QString COMPOSITE_WET_GAMMA_ILLUMINATION = "wet_gamma_illumination";
const QString COMPOSITE_WET_VIVID_LIGHT = "wet_vivid_light";
const QString COMPOSITE_WET_FLAT_LIGHT = "wet_flat_light";
const QString COMPOSITE_WET_LINEAR_LIGHT = "wet_linear_light";
const QString COMPOSITE_WET_PIN_LIGHT = "wet_pin_light";
const QString COMPOSITE_WET_PNORM_A = "wet_pnorm_a";
const QString COMPOSITE_WET_PNORM_B = "wet_pnorm_b";
const QString COMPOSITE_WET_SUPER_LIGHT = "wet_super_light";
const QString COMPOSITE_WET_TINT_IFS_ILLUSIONS = "wet_tint_ifs_illusions";
const QString COMPOSITE_WET_FOG_LIGHTEN_IFS_ILLUSIONS = "wet_fog_lighten_ifs_illusions";
const QString COMPOSITE_WET_EASY_DODGE = "wet_easy_dodge";
const QString COMPOSITE_WET_LUMINOSITY_SAI = "wet_luminosity_sai";

const QString COMPOSITE_WET_HUE            = "wet_hue";
const QString COMPOSITE_WET_COLOR          = "wet_color";
const QString COMPOSITE_WET_SATURATION     = "wet_saturation";
const QString COMPOSITE_WET_INC_SATURATION = "wet_inc_saturation";
const QString COMPOSITE_WET_DEC_SATURATION = "wet_dec_saturation";
const QString COMPOSITE_WET_LUMINIZE       = "wet_luminize";
const QString COMPOSITE_WET_INC_LUMINOSITY = "wet_inc_luminosity";
const QString COMPOSITE_WET_DEC_LUMINOSITY = "wet_dec_luminosity";

const QString COMPOSITE_WET_HUE_HSV = "wet_hue_hsv";
const QString COMPOSITE_WET_COLOR_HSV = "wet_color_hsv";
const QString COMPOSITE_WET_SATURATION_HSV = "wet_saturation_hsv";
const QString COMPOSITE_WET_INC_SATURATION_HSV = "wet_inc_saturation_hsv";
const QString COMPOSITE_WET_DEC_SATURATION_HSV = "wet_dec_saturation_hsv";
const QString COMPOSITE_WET_VALUE = "wet_value";
const QString COMPOSITE_WET_INC_VALUE = "wet_inc_value";
const QString COMPOSITE_WET_DEC_VALUE = "wet_dec_value";

const QString COMPOSITE_WET_HUE_HSL = "wet_hue_hsl";
const QString COMPOSITE_WET_COLOR_HSL = "wet_color_hsl";
const QString COMPOSITE_WET_SATURATION_HSL = "wet_saturation_hsl";
const QString COMPOSITE_WET_INC_SATURATION_HSL = "wet_inc_saturation_hsl";
const QString COMPOSITE_WET_DEC_SATURATION_HSL = "wet_dec_saturation_hsl";
const QString COMPOSITE_WET_LIGHTNESS = "wet_lightness";
const QString COMPOSITE_WET_INC_LIGHTNESS = "wet_inc_lightness";
const QString COMPOSITE_WET_DEC_LIGHTNESS = "wet_dec_lightness";

const QString COMPOSITE_WET_HUE_HSI = "wet_hue_hsi";
const QString COMPOSITE_WET_COLOR_HSI = "wet_color_hsi";
const QString COMPOSITE_WET_SATURATION_HSI = "wet_saturation_hsi";
const QString COMPOSITE_WET_INC_SATURATION_HSI = "wet_inc_saturation_hsi";
const QString COMPOSITE_WET_DEC_SATURATION_HSI = "wet_dec_saturation_hsi";
const QString COMPOSITE_WET_INTENSITY = "wet_intensity";
const QString COMPOSITE_WET_INC_INTENSITY = "wet_inc_intensity";
const QString COMPOSITE_WET_DEC_INTENSITY = "wet_dec_intensity";

const QString COMPOSITE_WET_TANGENT_NORMALMAP = "wet_tangent_normalmap";

const QString COMPOSITE_WET_COLORIZE = "wet_colorize";
const QString COMPOSITE_WET_BUMPMAP = "wet_bumpmap";
const QString COMPOSITE_WET_COMBINE_NORMAL = "wet_combine_normal";
const QString COMPOSITE_WET_CLEAR = "wet_clear";
const QString COMPOSITE_WET_DISSOLVE = "wet_dissolve";
const QString COMPOSITE_WET_DISPLACE = "wet_displace";
const QString COMPOSITE_WET_NO = "wet_nocomposition";
const QString COMPOSITE_WET_PASS_THROUGH = "wet_pass_through"; // XXX: not implemented anywhere yet
const QString COMPOSITE_WET_DARKER_COLOR = "wet_darker_color";
const QString COMPOSITE_WET_LIGHTER_COLOR = "wet_lighter_color";
const QString COMPOSITE_WET_UNDEF = "wet_undefined";

const QString COMPOSITE_WET_REFLECT = "wet_reflect";
const QString COMPOSITE_WET_GLOW = "wet_glow";
const QString COMPOSITE_WET_FREEZE = "wet_freeze";
const QString COMPOSITE_WET_HEAT = "wet_heat";
const QString COMPOSITE_WET_GLEAT = "wet_glow_heat";
const QString COMPOSITE_WET_HELOW = "wet_heat_glow";
const QString COMPOSITE_WET_REEZE = "wet_reflect_freeze";
const QString COMPOSITE_WET_FRECT = "wet_freeze_reflect";
const QString COMPOSITE_WET_FHYRD = "wet_heat_glow_freeze_reflect_hybrid";

const QString COMPOSITE_WET_LAMBERT_LIGHTING = "wet_lambert_lighting";
const QString COMPOSITE_WET_LAMBERT_LIGHTING_GAMMA_2_2 = "wet_lambert_lighting_gamma2.2";


class KRITAPIGMENT_EXPORT KoCompositeOpRegistry
{
    typedef QMultiMap<KoID,KoID> KoIDMap;
    typedef QList<KoID>          KoIDList;

public:
    KoCompositeOpRegistry();
    static const KoCompositeOpRegistry& instance();

    KoID     getDefaultCompositeOp() const;
    KoID     getKoID(const QString& compositeOpID) const;
    QString  getCompositeOpDisplayName(const QString& compositeOpID) const;
    KoIDMap  getCompositeOps() const;
    KoIDMap  getLayerStylesCompositeOps() const;
    KoIDList getCategories() const;
    QString  getCategoryDisplayName(const QString& categoryID) const;
    KoIDList getCompositeOps(const KoColorSpace* colorSpace) const;
    KoIDList getCompositeOps(const KoID& category, const KoColorSpace* colorSpace=0) const;
    bool     colorSpaceHasCompositeOp(const KoColorSpace* colorSpace, const KoID& compositeOp) const;

    template<class TKoIdIterator>
    KoIDList filterCompositeOps(TKoIdIterator begin, TKoIdIterator end, const KoColorSpace* colorSpace, bool removeInvaliOps=true) const {
        KoIDList list;

        for(; begin!=end; ++begin){
            if (colorSpaceHasCompositeOp(colorSpace, *begin) == removeInvaliOps) {
                list.push_back(*begin);
            }
        }

        return list;
    }

private:
    KoIDList m_categories;
    KoIDMap  m_map;
};


#endif // KOCOMPOSITEOPREGISTRY_H
