/*
 *  SPDX-FileCopyrightText: 2005 Adrian Page <adrian@pagenet.plus.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "KoCompositeOpRegistry.h"

#include <QGlobalStatic>
#include <QList>

#include <klocalizedstring.h>

#include <KoID.h>
#include "KoCompositeOp.h"
#include "KoColorSpace.h"

#include "kis_assert.h"
#include "DebugPigment.h"

Q_GLOBAL_STATIC(KoCompositeOpRegistry, registry)

static KoID koidCompositeOverStatic() {
    static const KoID compositeOver(COMPOSITE_OVER, i18nc("Blending mode - Normal", "Normal"));
    return compositeOver;
}

KoCompositeOpRegistry::KoCompositeOpRegistry()
{
    m_categories
        << KoID("arithmetic",  i18nc("Blending mode - category Arithmetic", "Arithmetic"))
        << KoID("binary"    ,  i18nc("Blending mode - category Binary", "Binary"))
        << KoID("dark"      ,  i18nc("Blending mode - category Darken", "Darken"))
        << KoID("light"     ,  i18nc("Blending mode - category Lighten", "Lighten"))
        << KoID("modulo"       ,  i18nc("Blending mode - category Modulo", "Modulo"))
        << KoID("negative"  ,  i18nc("Blending mode - category Negative", "Negative"))
        << KoID("mix"       ,  i18nc("Blending mode - category Mix", "Mix"))
        << KoID("misc"      ,  i18nc("Blending mode - category Misc", "Misc"))
        << KoID("hsy"       ,  i18nc("Blending mode - category HSY", "HSY"))
        << KoID("hsi"       ,  i18nc("Blending mode - category HSI", "HSI"))
        << KoID("hsl"       ,  i18nc("Blending mode - category HSL", "HSL"))
        << KoID("hsv"       ,  i18nc("Blending mode - category HSV", "HSV"))
        << KoID("quadratic" ,  i18nc("Blending mode - category Quadratic", "Quadratic"));

    m_map.insert(m_categories[0], KoID(COMPOSITE_ADD             ,  i18nc("Blending mode - Addition", "Addition")));
    m_map.insert(m_categories[0], KoID(COMPOSITE_SUBTRACT        ,  i18nc("Blending mode - Subtract", "Subtract")));
    m_map.insert(m_categories[0], KoID(COMPOSITE_MULT            ,  i18nc("Blending mode - Multiply", "Multiply")));
    m_map.insert(m_categories[0], KoID(COMPOSITE_DIVIDE          ,  i18nc("Blending mode - Divide", "Divide")));
    m_map.insert(m_categories[0], KoID(COMPOSITE_INVERSE_SUBTRACT,  i18nc("Blending mode - Inverse Subtract", "Inverse Subtract")));
    
    m_map.insert(m_categories[1], KoID(COMPOSITE_XOR             ,  i18nc("Blending mode - XOR", "XOR")));
    m_map.insert(m_categories[1], KoID(COMPOSITE_OR              ,  i18nc("Blending mode - OR", "OR")));
    m_map.insert(m_categories[1], KoID(COMPOSITE_AND             ,  i18nc("Blending mode - AND", "AND")));
    m_map.insert(m_categories[1], KoID(COMPOSITE_NAND            ,  i18nc("Blending mode - NAND", "NAND")));
    m_map.insert(m_categories[1], KoID(COMPOSITE_NOR             ,  i18nc("Blending mode - NOR", "NOR")));
    m_map.insert(m_categories[1], KoID(COMPOSITE_XNOR            ,  i18nc("Blending mode - XNOR", "XNOR")));
    m_map.insert(m_categories[1], KoID(COMPOSITE_IMPLICATION     ,  i18nc("Blending mode - IMPLICATION", "IMPLICATION")));
    m_map.insert(m_categories[1], KoID(COMPOSITE_NOT_IMPLICATION ,  i18nc("Blending mode - NOT IMPLICATION", "NOT IMPLICATION")));
    m_map.insert(m_categories[1], KoID(COMPOSITE_CONVERSE        ,  i18nc("Blending mode - CONVERSE", "CONVERSE")));
    m_map.insert(m_categories[1], KoID(COMPOSITE_NOT_CONVERSE    ,  i18nc("Blending mode - NOT CONVERSE", "NOT CONVERSE")));

    m_map.insert(m_categories[2], KoID(COMPOSITE_BURN       ,  i18nc("Blending mode - Burn", "Burn")));
    m_map.insert(m_categories[2], KoID(COMPOSITE_LINEAR_BURN,  i18nc("Blending mode - Linear Burn", "Linear Burn")));
    m_map.insert(m_categories[2], KoID(COMPOSITE_DARKEN     ,  i18nc("Blending mode - Darken", "Darken")));
    m_map.insert(m_categories[2], KoID(COMPOSITE_GAMMA_DARK ,  i18nc("Blending mode - Gamma Dark", "Gamma Dark")));
    m_map.insert(m_categories[2], KoID(COMPOSITE_DARKER_COLOR     ,  i18nc("Blending mode - Darker Color", "Darker Color")));
    m_map.insert(m_categories[2], KoID(COMPOSITE_SHADE_IFS_ILLUSIONS,  i18nc("Blending mode - Shade (IFS Illusions)", "Shade (IFS Illusions)")));
    m_map.insert(m_categories[2], KoID(COMPOSITE_FOG_DARKEN_IFS_ILLUSIONS,  i18nc("Blending mode - Fog Darken (IFS Illusions)", "Fog Darken (IFS Illusions)")));
    m_map.insert(m_categories[2], KoID(COMPOSITE_EASY_BURN       ,  i18nc("Blending mode - Easy Burn", "Easy Burn")));

    m_map.insert(m_categories[3], KoID(COMPOSITE_DODGE       ,  i18nc("Blending mode - Color Dodge", "Color Dodge")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_LINEAR_DODGE,  i18nc("Blending mode - Linear Dodge", "Linear Dodge")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_LIGHTEN     ,  i18nc("Blending mode - Lighten", "Lighten")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_LINEAR_LIGHT,  i18nc("Blending mode - Linear Light", "Linear Light")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_SCREEN      ,  i18nc("Blending mode - Screen", "Screen")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_PIN_LIGHT   ,  i18nc("Blending mode - Pin Light", "Pin Light")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_VIVID_LIGHT ,  i18nc("Blending mode - Vivid Light", "Vivid Light")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_FLAT_LIGHT  ,  i18nc("Blending mode - Flat Light", "Flat Light")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_HARD_LIGHT  ,  i18nc("Blending mode - Hard Light", "Hard Light")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_SOFT_LIGHT_IFS_ILLUSIONS,  i18nc("Blending mode - Soft Light (IFS Illusions)", "Soft Light (IFS Illusions)")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_SOFT_LIGHT_PEGTOP_DELPHI,  i18nc("Blending mode - Soft Light (Pegtop-Delphi)", "Soft Light (Pegtop-Delphi)")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_SOFT_LIGHT_PHOTOSHOP,  i18nc("Blending mode - Soft Light (Photoshop)", "Soft Light (Photoshop)")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_SOFT_LIGHT_SVG,  i18nc("Blending mode - Soft Light (SVG)", "Soft Light (SVG)")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_GAMMA_LIGHT ,  i18nc("Blending mode - Gamma Light", "Gamma Light")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_GAMMA_ILLUMINATION ,  i18nc("Blending mode - Gamma Illumination", "Gamma Illumination")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_LIGHTER_COLOR     ,  i18nc("Blending mode - Lighter Color", "Lighter Color")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_PNORM_A           ,  i18nc("Blending mode - P-Norm A", "P-Norm A")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_PNORM_B           ,  i18nc("Blending mode - P-Norm B", "P-Norm B")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_SUPER_LIGHT     ,  i18nc("Blending mode - Super Light", "Super Light")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_TINT_IFS_ILLUSIONS,  i18nc("Blending mode - Tint (IFS Illusions)", "Tint (IFS Illusions)")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_FOG_LIGHTEN_IFS_ILLUSIONS,  i18nc("Blending mode - Fog Lighten (IFS Illusions)", "Fog Lighten (IFS Illusions)")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_EASY_DODGE       ,  i18nc("Blending mode - Easy Dodge", "Easy Dodge")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_LUMINOSITY_SAI       ,  i18nc("Blending mode - Luminosity/Shine (SAI)", "Luminosity/Shine (SAI)")));

    m_map.insert(m_categories[4], KoID(COMPOSITE_MOD              ,  i18nc("Blending mode - Modulo", "Modulo")));
    m_map.insert(m_categories[4], KoID(COMPOSITE_MOD_CON          ,  i18nc("Blending mode - Modulo - Continuous", "Modulo - Continuous")));
    m_map.insert(m_categories[4], KoID(COMPOSITE_DIVISIVE_MOD     ,  i18nc("Blending mode - Divisive Modulo", "Divisive Modulo")));
    m_map.insert(m_categories[4], KoID(COMPOSITE_DIVISIVE_MOD_CON ,  i18nc("Blending mode - Divisive Modulo - Continuous", "Divisive Modulo - Continuous")));
    m_map.insert(m_categories[4], KoID(COMPOSITE_MODULO_SHIFT     ,  i18nc("Blending mode - Modulo Shift", "Modulo Shift")));
    m_map.insert(m_categories[4], KoID(COMPOSITE_MODULO_SHIFT_CON ,  i18nc("Blending mode - Modulo Shift - Continuous", "Modulo Shift - Continuous")));

    m_map.insert(m_categories[5], KoID(COMPOSITE_DIFF                 ,  i18nc("Blending mode - Difference", "Difference")));
    m_map.insert(m_categories[5], KoID(COMPOSITE_EQUIVALENCE          ,  i18nc("Blending mode - Equivalence", "Equivalence")));
    m_map.insert(m_categories[5], KoID(COMPOSITE_ADDITIVE_SUBTRACTIVE ,  i18nc("Blending mode - Additive Subtractive", "Additive Subtractive")));
    m_map.insert(m_categories[5], KoID(COMPOSITE_EXCLUSION            ,  i18nc("Blending mode - Exclusion", "Exclusion")));
    m_map.insert(m_categories[5], KoID(COMPOSITE_ARC_TANGENT          ,  i18nc("Blending mode - Arcus Tangent", "Arcus Tangent")));
    m_map.insert(m_categories[5], KoID(COMPOSITE_NEGATION             ,  i18nc("Blending mode - Negation", "Negation")));

    m_map.insert(m_categories[6], koidCompositeOverStatic());
    m_map.insert(m_categories[6], KoID(COMPOSITE_BEHIND          ,  i18nc("Blending mode - Behind", "Behind")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_GREATER         ,  i18nc("Blending mode - Greater", "Greater")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_OVERLAY         ,  i18nc("Blending mode - Overlay", "Overlay")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_LAMBERT_LIGHTING, i18nc("Blending mode - Lambert Lighting (Linear)", "Lambert Lighting (Linear)")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_LAMBERT_LIGHTING_GAMMA_2_2, i18nc("Blending mode - Lambert Lighting (Gamma 2.2)", "Lambert Lighting (Gamma 2.2)")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_ERASE           ,  i18nc("Blending mode - Erase", "Erase")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_ALPHA_DARKEN    ,  i18nc("Blending mode - Alpha Darken", "Alpha Darken")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_HARD_MIX        ,  i18nc("Blending mode - Hard Mix", "Hard Mix")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_HARD_MIX_PHOTOSHOP,  i18nc("Blending mode - Hard Mix (Photoshop)", "Hard Mix (Photoshop)")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_HARD_MIX_SOFTER_PHOTOSHOP,  i18nc("Blending mode - Hard Mix Softer (Photoshop)", "Hard Mix Softer (Photoshop)")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_GRAIN_MERGE     ,  i18nc("Blending mode - Grain Merge", "Grain Merge")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_GRAIN_EXTRACT   ,  i18nc("Blending mode - Grain Extract", "Grain Extract")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_PARALLEL        ,  i18nc("Blending mode - Parallel", "Parallel")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_ALLANON         ,  i18nc("Blending mode - Allanon", "Allanon")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_GEOMETRIC_MEAN  ,  i18nc("Blending mode - Geometric Mean", "Geometric Mean")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_DESTINATION_ATOP,  i18nc("Blending mode - Destination Atop", "Destination Atop")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_DESTINATION_IN  ,  i18nc("Blending mode - Destination In", "Destination In")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_HARD_OVERLAY    ,  i18nc("Blending mode - Hard Overlay", "Hard Overlay")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_INTERPOLATION   ,  i18nc("Blending mode - Interpolation", "Interpolation")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_INTERPOLATIONB  ,  i18nc("Blending mode - Interpolation - 2X", "Interpolation - 2X")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_PENUMBRAA       ,  i18nc("Blending mode - Penumbra A", "Penumbra A")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_PENUMBRAB       ,  i18nc("Blending mode - Penumbra B", "Penumbra B")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_PENUMBRAC       ,  i18nc("Blending mode - Penumbra C", "Penumbra C")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_PENUMBRAD       ,  i18nc("Blending mode - Penumbra D", "Penumbra D")));

    m_map.insert(m_categories[7], KoID(COMPOSITE_BUMPMAP   ,  i18nc("Blending mode - Bumpmap", "Bumpmap")));
    m_map.insert(m_categories[7], KoID(COMPOSITE_COMBINE_NORMAL,  i18nc("Blending mode - Combine Normal Map", "Combine Normal Map")));
    m_map.insert(m_categories[7], KoID(COMPOSITE_DISSOLVE  ,  i18nc("Blending mode - Dissolve", "Dissolve")));
    m_map.insert(m_categories[7], KoID(COMPOSITE_COPY_RED  ,  i18nc("Blending mode - Copy Red", "Copy Red")));
    m_map.insert(m_categories[7], KoID(COMPOSITE_COPY_GREEN,  i18nc("Blending mode - Copy Green", "Copy Green")));
    m_map.insert(m_categories[7], KoID(COMPOSITE_COPY_BLUE ,  i18nc("Blending mode - Copy Blue", "Copy Blue")));
    m_map.insert(m_categories[7], KoID(COMPOSITE_COPY      ,  i18nc("Blending mode - Copy", "Copy")));
    m_map.insert(m_categories[7], KoID(COMPOSITE_TANGENT_NORMALMAP,  i18nc("Blending mode - Tangent Normalmap", "Tangent Normalmap")));

    m_map.insert(m_categories[8], KoID(COMPOSITE_COLOR         ,  i18nc("Blending mode - Color HSY", "Color")));
    m_map.insert(m_categories[8], KoID(COMPOSITE_HUE           ,  i18nc("Blending mode - Hue HSY", "Hue")));
    m_map.insert(m_categories[8], KoID(COMPOSITE_SATURATION    ,  i18nc("Blending mode - Saturation HSY", "Saturation")));
    m_map.insert(m_categories[8], KoID(COMPOSITE_LUMINIZE      ,  i18nc("Blending mode - Luminosity HSY", "Luminosity")));
    m_map.insert(m_categories[8], KoID(COMPOSITE_DEC_SATURATION,  i18nc("Blending mode - Decrease Saturation HSY", "Decrease Saturation")));
    m_map.insert(m_categories[8], KoID(COMPOSITE_INC_SATURATION,  i18nc("Blending mode - Increase Saturation HSY", "Increase Saturation")));
    m_map.insert(m_categories[8], KoID(COMPOSITE_DEC_LUMINOSITY,  i18nc("Blending mode - Decrease Luminosity HSY", "Decrease Luminosity")));
    m_map.insert(m_categories[8], KoID(COMPOSITE_INC_LUMINOSITY,  i18nc("Blending mode - Increase Luminosity HSY", "Increase Luminosity")));

    m_map.insert(m_categories[9], KoID(COMPOSITE_COLOR_HSI         ,  i18nc("Blending mode - Color HSI", "Color HSI")));
    m_map.insert(m_categories[9], KoID(COMPOSITE_HUE_HSI           ,  i18nc("Blending mode - Hue HSI", "Hue HSI")));
    m_map.insert(m_categories[9], KoID(COMPOSITE_SATURATION_HSI    ,  i18nc("Blending mode - Saturation HSI", "Saturation HSI")));
    m_map.insert(m_categories[9], KoID(COMPOSITE_INTENSITY         ,  i18nc("Blending mode - Intensity HSI", "Intensity")));
    m_map.insert(m_categories[9], KoID(COMPOSITE_DEC_SATURATION_HSI,  i18nc("Blending mode - Decrease Saturation HSI", "Decrease Saturation HSI")));
    m_map.insert(m_categories[9], KoID(COMPOSITE_INC_SATURATION_HSI,  i18nc("Blending mode - Increase Saturation HSI", "Increase Saturation HSI")));
    m_map.insert(m_categories[9], KoID(COMPOSITE_DEC_INTENSITY     ,  i18nc("Blending mode - Decrease Intensity", "Decrease Intensity")));
    m_map.insert(m_categories[9], KoID(COMPOSITE_INC_INTENSITY     ,  i18nc("Blending mode - Increase Intensity", "Increase Intensity")));

    m_map.insert(m_categories[10], KoID(COMPOSITE_COLOR_HSL         ,  i18nc("Blending mode - Color HSL", "Color HSL")));
    m_map.insert(m_categories[10], KoID(COMPOSITE_HUE_HSL           ,  i18nc("Blending mode - Hue HSL", "Hue HSL")));
    m_map.insert(m_categories[10], KoID(COMPOSITE_SATURATION_HSL    ,  i18nc("Blending mode - Saturation HSL", "Saturation HSL")));
    m_map.insert(m_categories[10], KoID(COMPOSITE_LIGHTNESS         ,  i18nc("Blending mode - Lightness HSI", "Lightness")));
    m_map.insert(m_categories[10], KoID(COMPOSITE_DEC_SATURATION_HSL,  i18nc("Blending mode - Decrease Saturation HSL", "Decrease Saturation HSL")));
    m_map.insert(m_categories[10], KoID(COMPOSITE_INC_SATURATION_HSL,  i18nc("Blending mode - Increase Saturation HSL", "Increase Saturation HSL")));
    m_map.insert(m_categories[10], KoID(COMPOSITE_DEC_LIGHTNESS     ,  i18nc("Blending mode - Decrease Lightness", "Decrease Lightness")));
    m_map.insert(m_categories[10], KoID(COMPOSITE_INC_LIGHTNESS     ,  i18nc("Blending mode - Increase Lightness", "Increase Lightness")));

    m_map.insert(m_categories[11], KoID(COMPOSITE_COLOR_HSV         ,  i18nc("Blending mode - Color HSV", "Color HSV")));
    m_map.insert(m_categories[11], KoID(COMPOSITE_HUE_HSV           ,  i18nc("Blending mode - Hue HSV", "Hue HSV")));
    m_map.insert(m_categories[11], KoID(COMPOSITE_SATURATION_HSV    ,  i18nc("Blending mode - Saturation HSV", "Saturation HSV")));
    m_map.insert(m_categories[11], KoID(COMPOSITE_VALUE             ,  i18nc("Blending mode - Value HSV", "Value")));
    m_map.insert(m_categories[11], KoID(COMPOSITE_DEC_SATURATION_HSV,  i18nc("Blending mode - Decrease Saturation HSV", "Decrease Saturation HSV")));
    m_map.insert(m_categories[11], KoID(COMPOSITE_INC_SATURATION_HSV,  i18nc("Blending mode - Increase Saturation HSV", "Increase Saturation HSV")));
    m_map.insert(m_categories[11], KoID(COMPOSITE_DEC_VALUE         ,  i18nc("Blending mode - Decrease Value HSV", "Decrease Value")));
    m_map.insert(m_categories[11], KoID(COMPOSITE_INC_VALUE         ,  i18nc("Blending mode - Increase Value HSV", "Increase Value")));
    
    m_map.insert(m_categories[12], KoID(COMPOSITE_REFLECT          ,  i18nc("Blending mode - Reflect", "Reflect")));
    m_map.insert(m_categories[12], KoID(COMPOSITE_GLOW             ,  i18nc("Blending mode - Glow", "Glow")));
    m_map.insert(m_categories[12], KoID(COMPOSITE_FREEZE           ,  i18nc("Blending mode - Freeze", "Freeze")));
    m_map.insert(m_categories[12], KoID(COMPOSITE_HEAT             ,  i18nc("Blending mode - Heat", "Heat")));
    m_map.insert(m_categories[12], KoID(COMPOSITE_GLEAT            ,  i18nc("Blending mode - Glow-Heat", "Glow-Heat")));
    m_map.insert(m_categories[12], KoID(COMPOSITE_HELOW            ,  i18nc("Blending mode - Heat-Glow", "Heat-Glow")));
    m_map.insert(m_categories[12], KoID(COMPOSITE_REEZE            ,  i18nc("Blending mode - Reflect-Freeze", "Reflect-Freeze")));
    m_map.insert(m_categories[12], KoID(COMPOSITE_FRECT            ,  i18nc("Blending mode - Freeze-Reflect", "Freeze-Reflect")));
    m_map.insert(m_categories[12], KoID(COMPOSITE_FHYRD            ,  i18nc("Blending mode - Heat-Glow & Freeze-Reflect Hybrid", "Heat-Glow & Freeze-Reflect Hybrid")));

    // "Wet" Modes

    m_map.insert(m_categories[0], KoID(COMPOSITE_WET_ADD             ,  i18nc("Blending mode - Wet Addition", "Wet Addition")));
    m_map.insert(m_categories[0], KoID(COMPOSITE_WET_SUBTRACT        ,  i18nc("Blending mode - Wet Subtract", "Wet Subtract")));
    m_map.insert(m_categories[0], KoID(COMPOSITE_WET_MULT            ,  i18nc("Blending mode - Wet Multiply", "Wet Multiply")));
    m_map.insert(m_categories[0], KoID(COMPOSITE_WET_DIVIDE          ,  i18nc("Blending mode - Wet Divide", "Wet Divide")));
    m_map.insert(m_categories[0], KoID(COMPOSITE_WET_INVERSE_SUBTRACT,  i18nc("Blending mode - Wet Inverse Subtract", "Wet Inverse Subtract")));

    m_map.insert(m_categories[1], KoID(COMPOSITE_WET_XOR             ,  i18nc("Blending mode - Wet XOR", "Wet XOR")));
    m_map.insert(m_categories[1], KoID(COMPOSITE_WET_OR              ,  i18nc("Blending mode - Wet OR", "Wet OR")));
    m_map.insert(m_categories[1], KoID(COMPOSITE_WET_AND             ,  i18nc("Blending mode - Wet AND", "Wet AND")));
    m_map.insert(m_categories[1], KoID(COMPOSITE_WET_NAND            ,  i18nc("Blending mode - Wet NAND", "Wet NAND")));
    m_map.insert(m_categories[1], KoID(COMPOSITE_WET_NOR             ,  i18nc("Blending mode - Wet NOR", "Wet NOR")));
    m_map.insert(m_categories[1], KoID(COMPOSITE_WET_XNOR            ,  i18nc("Blending mode - Wet XNOR", "Wet XNOR")));
    m_map.insert(m_categories[1], KoID(COMPOSITE_WET_IMPLICATION     ,  i18nc("Blending mode - Wet IMPLICATION", "Wet IMPLICATION")));
    m_map.insert(m_categories[1], KoID(COMPOSITE_WET_NOT_IMPLICATION ,  i18nc("Blending mode - Wet NOT IMPLICATION", "Wet NOT IMPLICATION")));
    m_map.insert(m_categories[1], KoID(COMPOSITE_WET_CONVERSE        ,  i18nc("Blending mode - Wet CONVERSE", "Wet CONVERSE")));
    m_map.insert(m_categories[1], KoID(COMPOSITE_WET_NOT_CONVERSE    ,  i18nc("Blending mode - Wet NOT CONVERSE", "Wet NOT CONVERSE")));

    m_map.insert(m_categories[2], KoID(COMPOSITE_WET_BURN       ,  i18nc("Blending mode - Wet Burn", "Wet Burn")));
    m_map.insert(m_categories[2], KoID(COMPOSITE_WET_LINEAR_BURN,  i18nc("Blending mode - Wet Linear Burn", "Wet Linear Burn")));
    m_map.insert(m_categories[2], KoID(COMPOSITE_WET_DARKEN     ,  i18nc("Blending mode - Wet Darken", "Wet Darken")));
    m_map.insert(m_categories[2], KoID(COMPOSITE_WET_GAMMA_DARK ,  i18nc("Blending mode - Wet Gamma Dark", "Wet Gamma Dark")));
    m_map.insert(m_categories[2], KoID(COMPOSITE_WET_DARKER_COLOR     ,  i18nc("Blending mode - Wet Darker Color", "Wet Darker Color")));
    m_map.insert(m_categories[2], KoID(COMPOSITE_WET_SHADE_IFS_ILLUSIONS,  i18nc("Blending mode - Wet Shade (IFS Illusions)", "Wet Shade (IFS Illusions)")));
    m_map.insert(m_categories[2], KoID(COMPOSITE_WET_FOG_DARKEN_IFS_ILLUSIONS,  i18nc("Blending mode - Wet Fog Darken (IFS Illusions)", "Wet Fog Darken (IFS Illusions)")));
    m_map.insert(m_categories[2], KoID(COMPOSITE_WET_EASY_BURN       ,  i18nc("Blending mode - Wet Easy Burn", "Wet Easy Burn")));

    m_map.insert(m_categories[3], KoID(COMPOSITE_WET_DODGE       ,  i18nc("Blending mode - Wet Color Dodge", "Wet Color Dodge")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_WET_LINEAR_DODGE,  i18nc("Blending mode - Wet Linear Dodge", "Wet Linear Dodge")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_WET_LIGHTEN     ,  i18nc("Blending mode - Wet Lighten", "Wet Lighten")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_WET_LINEAR_LIGHT,  i18nc("Blending mode - Wet Linear Light", "Wet Linear Light")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_WET_SCREEN      ,  i18nc("Blending mode - Wet Screen", "Wet Screen")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_WET_PIN_LIGHT   ,  i18nc("Blending mode - Wet Pin Light", "Wet Pin Light")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_WET_VIVID_LIGHT ,  i18nc("Blending mode - Wet Vivid Light", "Wet Vivid Light")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_WET_FLAT_LIGHT  ,  i18nc("Blending mode - Wet Flat Light", "Wet Flat Light")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_WET_HARD_LIGHT  ,  i18nc("Blending mode - Wet Hard Light", "Wet Hard Light")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_WET_SOFT_LIGHT_IFS_ILLUSIONS,  i18nc("Blending mode - Wet Soft Light (IFS Illusions)", "Wet Soft Light (IFS Illusions)")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_WET_SOFT_LIGHT_PEGTOP_DELPHI,  i18nc("Blending mode - Wet Soft Light (Pegtop-Delphi)", "Wet Soft Light (Pegtop-Delphi)")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_WET_SOFT_LIGHT_PHOTOSHOP,  i18nc("Blending mode - Wet Soft Light (Photoshop)", "Wet Soft Light (Photoshop)")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_WET_SOFT_LIGHT_SVG,  i18nc("Blending mode - Wet Soft Light (SVG)", "Wet Soft Light (SVG)")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_WET_GAMMA_LIGHT ,  i18nc("Blending mode - Wet Gamma Light", "Wet Gamma Light")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_WET_GAMMA_ILLUMINATION ,  i18nc("Blending mode - Wet Gamma Illumination", "Wet Gamma Illumination")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_WET_LIGHTER_COLOR     ,  i18nc("Blending mode - Wet Lighter Color", "Wet Lighter Color")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_WET_PNORM_A           ,  i18nc("Blending mode - Wet P-Norm A", "Wet P-Norm A")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_WET_PNORM_B           ,  i18nc("Blending mode - Wet P-Norm B", "Wet P-Norm B")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_WET_SUPER_LIGHT     ,  i18nc("Blending mode - Wet Super Light", "Wet Super Light")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_WET_TINT_IFS_ILLUSIONS,  i18nc("Blending mode - Wet Tint (IFS Illusions)", "Wet Tint (IFS Illusions)")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_WET_FOG_LIGHTEN_IFS_ILLUSIONS,  i18nc("Blending mode - Wet Fog Lighten (IFS Illusions)", "Wet Fog Lighten (IFS Illusions)")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_WET_EASY_DODGE       ,  i18nc("Blending mode - Wet Easy Dodge", "Wet Easy Dodge")));
    m_map.insert(m_categories[3], KoID(COMPOSITE_WET_LUMINOSITY_SAI       ,  i18nc("Blending mode - Wet Luminosity/Shine (SAI)", "Wet Luminosity/Shine (SAI)")));

    m_map.insert(m_categories[4], KoID(COMPOSITE_WET_MOD              ,  i18nc("Blending mode - Wet Modulo", "Wet Modulo")));
    m_map.insert(m_categories[4], KoID(COMPOSITE_WET_MOD_CON          ,  i18nc("Blending mode - Wet Modulo - Continuous", "Wet Modulo - Continuous")));
    m_map.insert(m_categories[4], KoID(COMPOSITE_WET_DIVISIVE_MOD     ,  i18nc("Blending mode - Wet Divisive Modulo", "Wet Divisive Modulo")));
    m_map.insert(m_categories[4], KoID(COMPOSITE_WET_DIVISIVE_MOD_CON ,  i18nc("Blending mode - Wet Divisive Modulo - Continuous", "Wet Divisive Modulo - Continuous")));
    m_map.insert(m_categories[4], KoID(COMPOSITE_WET_MODULO_SHIFT     ,  i18nc("Blending mode - Wet Modulo Shift", "Wet Modulo Shift")));
    m_map.insert(m_categories[4], KoID(COMPOSITE_WET_MODULO_SHIFT_CON ,  i18nc("Blending mode - Wet Modulo Shift - Continuous", "Wet Modulo Shift - Continuous")));

    m_map.insert(m_categories[5], KoID(COMPOSITE_WET_DIFF                 ,  i18nc("Blending mode - Wet Difference", "Wet Difference")));
    m_map.insert(m_categories[5], KoID(COMPOSITE_WET_EQUIVALENCE          ,  i18nc("Blending mode - Wet Equivalence", "Wet Equivalence")));
    m_map.insert(m_categories[5], KoID(COMPOSITE_WET_ADDITIVE_SUBTRACTIVE ,  i18nc("Blending mode - Wet Additive Subtractive", "Wet Additive Subtractive")));
    m_map.insert(m_categories[5], KoID(COMPOSITE_WET_EXCLUSION            ,  i18nc("Blending mode - Wet Exclusion", "Wet Exclusion")));
    m_map.insert(m_categories[5], KoID(COMPOSITE_WET_ARC_TANGENT          ,  i18nc("Blending mode - Wet Arcus Tangent", "Wet Arcus Tangent")));
    m_map.insert(m_categories[5], KoID(COMPOSITE_WET_NEGATION             ,  i18nc("Blending mode - Wet Negation", "Wet Negation")));

//    m_map.insert(m_categories[6], koidCompositeOverStatic());
    // TODO implement optimised Wet Normal (Over)
    m_map.insert(m_categories[6], KoID(COMPOSITE_WET_OVER          ,  i18nc("Blending mode - Wet Normal", "Wet Normal")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_WET_OVERLAY         ,  i18nc("Blending mode - Wet Overlay", "Wet Overlay")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_WET_LAMBERT_LIGHTING, i18nc("Blending mode - Wet Lambert Lighting (Linear)", "Wet Lambert Lighting (Linear)")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_WET_LAMBERT_LIGHTING_GAMMA_2_2, i18nc("Blending mode - Wet Lambert Lighting (Gamma 2.2)", "Wet Lambert Lighting (Gamma 2.2)")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_WET_HARD_MIX        ,  i18nc("Blending mode - Wet Hard Mix", "Wet Hard Mix")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_WET_HARD_MIX_PHOTOSHOP,  i18nc("Blending mode - Wet Hard Mix (Photoshop)", "Wet Hard Mix (Photoshop)")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_WET_HARD_MIX_SOFTER_PHOTOSHOP,  i18nc("Blending mode - Wet Hard Mix Softer (Photoshop)", "Wet Hard Mix Softer (Photoshop)")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_WET_GRAIN_MERGE     ,  i18nc("Blending mode - Wet Grain Merge", "Wet Grain Merge")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_WET_GRAIN_EXTRACT   ,  i18nc("Blending mode - Wet Grain Extract", "Wet Grain Extract")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_WET_PARALLEL        ,  i18nc("Blending mode - Wet Parallel", "Wet Parallel")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_WET_ALLANON         ,  i18nc("Blending mode - Wet Allanon", "Wet Allanon")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_WET_GEOMETRIC_MEAN  ,  i18nc("Blending mode - Wet Geometric Mean", "Wet Geometric Mean")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_WET_HARD_OVERLAY    ,  i18nc("Blending mode - Wet Hard Overlay", "Wet Hard Overlay")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_WET_INTERPOLATION   ,  i18nc("Blending mode - Wet Interpolation", "Wet Interpolation")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_WET_INTERPOLATIONB  ,  i18nc("Blending mode - Wet Interpolation - 2X", "Wet Interpolation - 2X")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_WET_PENUMBRAA       ,  i18nc("Blending mode - Wet Penumbra A", "Wet Penumbra A")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_WET_PENUMBRAB       ,  i18nc("Blending mode - Wet Penumbra B", "Wet Penumbra B")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_WET_PENUMBRAC       ,  i18nc("Blending mode - Wet Penumbra C", "Wet Penumbra C")));
    m_map.insert(m_categories[6], KoID(COMPOSITE_WET_PENUMBRAD       ,  i18nc("Blending mode - Wet Penumbra D", "Wet Penumbra D")));

    m_map.insert(m_categories[7], KoID(COMPOSITE_WET_BUMPMAP   ,  i18nc("Blending mode - Wet Bumpmap", "Wet Bumpmap")));
    m_map.insert(m_categories[7], KoID(COMPOSITE_WET_COMBINE_NORMAL,  i18nc("Blending mode - Wet Combine Normal Map", "Wet Combine Normal Map")));
    m_map.insert(m_categories[7], KoID(COMPOSITE_WET_TANGENT_NORMALMAP,  i18nc("Blending mode - Wet Tangent Normalmap", "Wet Tangent Normalmap")));

    m_map.insert(m_categories[8], KoID(COMPOSITE_WET_COLOR         ,  i18nc("Blending mode - Wet Color HSY", "Wet Color")));
    m_map.insert(m_categories[8], KoID(COMPOSITE_WET_HUE           ,  i18nc("Blending mode - Wet Hue HSY", "Wet Hue")));
    m_map.insert(m_categories[8], KoID(COMPOSITE_WET_SATURATION    ,  i18nc("Blending mode - Wet Saturation HSY", "Wet Saturation")));
    m_map.insert(m_categories[8], KoID(COMPOSITE_WET_LUMINIZE      ,  i18nc("Blending mode - Wet Luminosity HSY", "Wet Luminosity")));
    m_map.insert(m_categories[8], KoID(COMPOSITE_WET_DEC_SATURATION,  i18nc("Blending mode - Wet Decrease Saturation HSY", "Wet Decrease Saturation")));
    m_map.insert(m_categories[8], KoID(COMPOSITE_WET_INC_SATURATION,  i18nc("Blending mode - Wet Increase Saturation HSY", "Wet Increase Saturation")));
    m_map.insert(m_categories[8], KoID(COMPOSITE_WET_DEC_LUMINOSITY,  i18nc("Blending mode - Wet Decrease Luminosity HSY", "Wet Decrease Luminosity")));
    m_map.insert(m_categories[8], KoID(COMPOSITE_WET_INC_LUMINOSITY,  i18nc("Blending mode - Wet Increase Luminosity HSY", "Wet Increase Luminosity")));

    m_map.insert(m_categories[9], KoID(COMPOSITE_WET_COLOR_HSI         ,  i18nc("Blending mode - Wet Color HSI", "Wet Color HSI")));
    m_map.insert(m_categories[9], KoID(COMPOSITE_WET_HUE_HSI           ,  i18nc("Blending mode - Wet Hue HSI", "Wet Hue HSI")));
    m_map.insert(m_categories[9], KoID(COMPOSITE_WET_SATURATION_HSI    ,  i18nc("Blending mode - Wet Saturation HSI", "Wet Saturation HSI")));
    m_map.insert(m_categories[9], KoID(COMPOSITE_WET_INTENSITY         ,  i18nc("Blending mode - Wet Intensity HSI", "Wet Intensity")));
    m_map.insert(m_categories[9], KoID(COMPOSITE_WET_DEC_SATURATION_HSI,  i18nc("Blending mode - Wet Decrease Saturation HSI", "Wet Decrease Saturation HSI")));
    m_map.insert(m_categories[9], KoID(COMPOSITE_WET_INC_SATURATION_HSI,  i18nc("Blending mode - Wet Increase Saturation HSI", "Wet Increase Saturation HSI")));
    m_map.insert(m_categories[9], KoID(COMPOSITE_WET_DEC_INTENSITY     ,  i18nc("Blending mode - Wet Decrease Intensity", "Wet Decrease Intensity")));
    m_map.insert(m_categories[9], KoID(COMPOSITE_WET_INC_INTENSITY     ,  i18nc("Blending mode - Wet Increase Intensity", "Wet Increase Intensity")));

    m_map.insert(m_categories[10], KoID(COMPOSITE_WET_COLOR_HSL         ,  i18nc("Blending mode - Wet Color HSL", "Wet Color HSL")));
    m_map.insert(m_categories[10], KoID(COMPOSITE_WET_HUE_HSL           ,  i18nc("Blending mode - Wet Hue HSL", "Wet Hue HSL")));
    m_map.insert(m_categories[10], KoID(COMPOSITE_WET_SATURATION_HSL    ,  i18nc("Blending mode - Wet Saturation HSL", "Wet Saturation HSL")));
    m_map.insert(m_categories[10], KoID(COMPOSITE_WET_LIGHTNESS         ,  i18nc("Blending mode - Wet Lightness HSI", "Wet Lightness")));
    m_map.insert(m_categories[10], KoID(COMPOSITE_WET_DEC_SATURATION_HSL,  i18nc("Blending mode - Wet Decrease Saturation HSL", "Wet Decrease Saturation HSL")));
    m_map.insert(m_categories[10], KoID(COMPOSITE_WET_INC_SATURATION_HSL,  i18nc("Blending mode - Wet Increase Saturation HSL", "Wet Increase Saturation HSL")));
    m_map.insert(m_categories[10], KoID(COMPOSITE_WET_DEC_LIGHTNESS     ,  i18nc("Blending mode - Wet Decrease Lightness", "Wet Decrease Lightness")));
    m_map.insert(m_categories[10], KoID(COMPOSITE_WET_INC_LIGHTNESS     ,  i18nc("Blending mode - Wet Increase Lightness", "Wet Increase Lightness")));

    m_map.insert(m_categories[11], KoID(COMPOSITE_WET_COLOR_HSV         ,  i18nc("Blending mode - Wet Color HSV", "Wet Color HSV")));
    m_map.insert(m_categories[11], KoID(COMPOSITE_WET_HUE_HSV           ,  i18nc("Blending mode - Wet Hue HSV", "Wet Hue HSV")));
    m_map.insert(m_categories[11], KoID(COMPOSITE_WET_SATURATION_HSV    ,  i18nc("Blending mode - Wet Saturation HSV", "Wet Saturation HSV")));
    m_map.insert(m_categories[11], KoID(COMPOSITE_WET_VALUE             ,  i18nc("Blending mode - Wet Value HSV", "Wet Value")));
    m_map.insert(m_categories[11], KoID(COMPOSITE_WET_DEC_SATURATION_HSV,  i18nc("Blending mode - Wet Decrease Saturation HSV", "Wet Decrease Saturation HSV")));
    m_map.insert(m_categories[11], KoID(COMPOSITE_WET_INC_SATURATION_HSV,  i18nc("Blending mode - Wet Increase Saturation HSV", "Wet Increase Saturation HSV")));
    m_map.insert(m_categories[11], KoID(COMPOSITE_WET_DEC_VALUE         ,  i18nc("Blending mode - Wet Decrease Value HSV", "Wet Decrease Value")));
    m_map.insert(m_categories[11], KoID(COMPOSITE_WET_INC_VALUE         ,  i18nc("Blending mode - Wet Increase Value HSV", "Wet Increase Value")));

    m_map.insert(m_categories[12], KoID(COMPOSITE_WET_REFLECT          ,  i18nc("Blending mode - Wet Reflect", "Wet Reflect")));
    m_map.insert(m_categories[12], KoID(COMPOSITE_WET_GLOW             ,  i18nc("Blending mode - Wet Glow", "Wet Glow")));
    m_map.insert(m_categories[12], KoID(COMPOSITE_WET_FREEZE           ,  i18nc("Blending mode - Wet Freeze", "Wet Freeze")));
    m_map.insert(m_categories[12], KoID(COMPOSITE_WET_HEAT             ,  i18nc("Blending mode - Wet Heat", "Wet Heat")));
    m_map.insert(m_categories[12], KoID(COMPOSITE_WET_GLEAT            ,  i18nc("Blending mode - Wet Glow-Heat", "Wet Glow-Heat")));
    m_map.insert(m_categories[12], KoID(COMPOSITE_WET_HELOW            ,  i18nc("Blending mode - Wet Heat-Glow", "Wet Heat-Glow")));
    m_map.insert(m_categories[12], KoID(COMPOSITE_WET_REEZE            ,  i18nc("Blending mode - Wet Reflect-Freeze", "Wet Reflect-Freeze")));
    m_map.insert(m_categories[12], KoID(COMPOSITE_WET_FRECT            ,  i18nc("Blending mode - Wet Freeze-Reflect", "Wet Freeze-Reflect")));
    m_map.insert(m_categories[12], KoID(COMPOSITE_WET_FHYRD            ,  i18nc("Blending mode - Wet Heat-Glow & Freeze-Reflect Hybrid", "Wet Heat-Glow & Freeze-Reflect Hybrid")));
}

const KoCompositeOpRegistry& KoCompositeOpRegistry::instance()
{
    return *registry;
}

KoID KoCompositeOpRegistry::getDefaultCompositeOp() const
{
    return koidCompositeOverStatic();
}

KoID KoCompositeOpRegistry::getKoID(const QString& compositeOpID) const
{
    KoIDMap::const_iterator itr = std::find(m_map.begin(), m_map.end(), KoID(compositeOpID));
    return (itr != m_map.end()) ? *itr : KoID();
}

QString KoCompositeOpRegistry::getCompositeOpDisplayName(const QString& compositeOpID) const
{
    // In and Out are created in lcms2engine but not registered in KoCompositeOpRegistry.
    // FIXME: Change these to use i18nc with context?
    if (compositeOpID == COMPOSITE_IN) {
        return i18n("In");
    } else if (compositeOpID == COMPOSITE_OUT) {
        return i18n("Out");
    }

    const QString name = getKoID(compositeOpID).name();
    if (name.isNull()) {
        warnPigment << "Got null display name for composite op" << compositeOpID;
        return compositeOpID;
    }
    return name;
}

KoCompositeOpRegistry::KoIDMap KoCompositeOpRegistry::getCompositeOps() const
{
    return m_map;
}

KoCompositeOpRegistry::KoIDMap KoCompositeOpRegistry::getLayerStylesCompositeOps() const
{
    QVector<QString> ids;

    // not available via the blending modes list in Krita
    // ids << COMPOSITE_PASS_THROUGH;

    ids << COMPOSITE_OVER;
    ids << COMPOSITE_DISSOLVE;
    ids << COMPOSITE_DARKEN;
    ids << COMPOSITE_MULT;
    ids << COMPOSITE_BURN;
    ids << COMPOSITE_LINEAR_BURN;
    ids << COMPOSITE_DARKER_COLOR;
    ids << COMPOSITE_LIGHTEN;
    ids << COMPOSITE_SCREEN;
    ids << COMPOSITE_DODGE;
    ids << COMPOSITE_LINEAR_DODGE;
    ids << COMPOSITE_LIGHTER_COLOR;
    ids << COMPOSITE_OVERLAY;
    ids << COMPOSITE_SOFT_LIGHT_PHOTOSHOP;
    ids << COMPOSITE_HARD_LIGHT;
    ids << COMPOSITE_VIVID_LIGHT;
    ids << COMPOSITE_LINEAR_LIGHT;
    ids << COMPOSITE_PIN_LIGHT;
    ids << COMPOSITE_HARD_MIX_PHOTOSHOP;
    ids << COMPOSITE_HARD_MIX_SOFTER_PHOTOSHOP;
    ids << COMPOSITE_DIFF;
    ids << COMPOSITE_EXCLUSION;
    ids << COMPOSITE_SUBTRACT;
    ids << COMPOSITE_DIVIDE;
    ids << COMPOSITE_HUE;
    ids << COMPOSITE_SATURATION;
    ids << COMPOSITE_COLOR;
    ids << COMPOSITE_LUMINIZE;

    KoIDMap result;
    Q_FOREACH (const QString &id, ids) {
        KoIDMap::const_iterator iter = std::find(m_map.begin(), m_map.end(), KoID(id));
        KIS_SAFE_ASSERT_RECOVER(iter != m_map.end()) { continue; }

        result.insert(iter.key(), iter.value());
    }

    return result;
}

KoCompositeOpRegistry::KoIDList KoCompositeOpRegistry::getCategories() const
{
    return m_categories;
}

QString  KoCompositeOpRegistry::getCategoryDisplayName(const QString& categoryID) const
{
    KoIDList::const_iterator itr = std::find(m_categories.begin(), m_categories.end(), KoID(categoryID));
    const QString name = (itr != m_categories.end()) ? itr->name() : QString();
    if (name.isNull()) {
        warnPigment << "Got null display name for composite op category" << categoryID;
        return categoryID;
    }
    return name;
}

KoCompositeOpRegistry::KoIDList KoCompositeOpRegistry::getCompositeOps(const KoID& category, const KoColorSpace* colorSpace) const
{
    qint32                  num = m_map.count(category);
    KoIDMap::const_iterator beg = m_map.find(category);
    KoIDMap::const_iterator end = beg + num;

    KoIDList list;
    list.reserve(num);

    if(colorSpace) {
        for(; beg!=end; ++beg){
            if(colorSpace->hasCompositeOp(beg->id()))
                list.push_back(*beg);
        }
    }
    else {
        for(; beg!=end; ++beg)
            list.push_back(*beg);
    }

    return list;
}

KoCompositeOpRegistry::KoIDList KoCompositeOpRegistry::getCompositeOps(const KoColorSpace* colorSpace) const
{
    KoIDMap::const_iterator beg = m_map.begin();
    KoIDMap::const_iterator end = m_map.end();

    KoIDList list;
    list.reserve(m_map.size());

    if(colorSpace) {
        for(; beg!=end; ++beg){
            if(colorSpace->hasCompositeOp(beg->id()))
                list.push_back(*beg);
        }
    }
    else {
        for(; beg!=end; ++beg)
            list.push_back(*beg);
    }

    return list;
}

bool KoCompositeOpRegistry::colorSpaceHasCompositeOp(const KoColorSpace* colorSpace, const KoID& compositeOp) const
{
    return colorSpace ? colorSpace->hasCompositeOp(compositeOp.id()) : false;
}
