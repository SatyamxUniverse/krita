/*
 *  SPDX-FileCopyrightText: 2024 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <QMap>
#include <QDebug>

#include "KoOpenTypeFeatureInfoFactory.h"
#include <klocalizedstring.h>

struct Q_DECL_HIDDEN KoOpenTypeFeatureInfoFactory::Private
{
    QMap<QString, KoOpenTypeFeatureInfo> infoMap;
};

KoOpenTypeFeatureInfoFactory::KoOpenTypeFeatureInfoFactory()
    : d(new Private)
{
    QVector<KoOpenTypeFeatureInfo> initialMap;

    // General discretionary features
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("aalt"),
                      i18nc("@title", "Access All Alternates"),
                      i18nc("@tooltip", "Access any possible substitutions."),
                      {KoOpenTypeFeatureInfo::GSUB1, KoOpenTypeFeatureInfo::GSUB3},
                      true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("calt"),
                      i18nc("@title", "Contextual Alternates"),
                      i18nc("@tooltip", "Replaces glyphs depending on their context within a text."),
                      {KoOpenTypeFeatureInfo::GSUB6}, false));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("clig"),
                      i18nc("@title", "Contextual Ligatures"),
                      i18nc("@tooltip", "Replaces sequences of glyphs with ligatures depending on their context within a text."),
                      {KoOpenTypeFeatureInfo::GSUB8}, false));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("cswh"),
                      i18nc("@title", "Contextual Swash"),
                      i18nc("@tooltip", "Replaces glyphs with swashed glyphs depending on their context within a text."),
                      {KoOpenTypeFeatureInfo::GSUB8}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("dlig"),
                      i18nc("@title", "Discretionary Ligatures"),
                      i18nc("@tooltip", "Replaces sequences of glyphs with ligatures intended for typographic effect."),
                      {KoOpenTypeFeatureInfo::GSUB4}, false));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("falt"),
                      i18nc("@title", "Final Glyph on Line Alternates"),
                      i18nc("@tooltip", "Replaces glyphs with their line-end forms."),
                      {KoOpenTypeFeatureInfo::GSUB1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("hist"),
                      i18nc("@title", "Historical Forms"),
                      i18nc("@tooltip", "Replaces glyphs with their historical forms."),
                      {KoOpenTypeFeatureInfo::GSUB1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("hlig"),
                      i18nc("@title", "Historical Forms"),
                      i18nc("@tooltip", "Replaces sequences glyphs with ligatures that were in use in the past, but rare today."),
                      {KoOpenTypeFeatureInfo::GSUB4}, false));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("jalt"),
                      i18nc("@title", "Justification Alternates"),
                      i18nc("@tooltip", "Replaces glyphs with their alternates meant for justification."),
                      {KoOpenTypeFeatureInfo::GSUB1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("liga"),
                      i18nc("@title", "Standard Ligatures"),
                      i18nc("@tooltip", "Replaces sequences glyphs with common ligatures."),
                      {KoOpenTypeFeatureInfo::GSUB4}, false));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("lfbd"),
                      i18nc("@title", "Left Bounds"),
                      i18nc("@tooltip", "This adjusts glyphs so there is no space at the left side."),
                      {KoOpenTypeFeatureInfo::GPOS1}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("locl"),
                      i18nc("@title", "Localized Forms"),
                      i18nc("@tooltip", "This replaces glyphs with language specific versions."),
                      {KoOpenTypeFeatureInfo::GSUB1}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("opbd"),
                      i18nc("@title", "Optical Bounds"),
                      i18nc("@tooltip", "Adjusts glyphs so they align by their optical bounds. Doesn't do anything in Krita."),
                      {KoOpenTypeFeatureInfo::GPOS1}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("ornm"),
                      i18nc("@title", "Ornaments"),
                      i18nc("@tooltip", "Replaces glyphs with ornaments for decorative purposes."),
                      {KoOpenTypeFeatureInfo::GSUB1, KoOpenTypeFeatureInfo::GSUB3}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("rand"),
                      i18nc("@title", "Randomize"),
                      i18nc("@tooltip", "Replaces glyphs with random alternates."),
                      {KoOpenTypeFeatureInfo::GSUB3}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("rtbd"),
                      i18nc("@title", "Right Bounds"),
                      i18nc("@tooltip", "This adjusts glyphs so there is no space at the right side."),
                      {KoOpenTypeFeatureInfo::GPOS1}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("salt"),
                      i18nc("@title", "Stylistic Alternates"),
                      i18nc("@tooltip", "Replaces glyphs with stylistic alternates that do not fit in other categories."),
                      {KoOpenTypeFeatureInfo::GSUB1, KoOpenTypeFeatureInfo::GSUB3}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("swsh"),
                      i18nc("@title", "Swash"),
                      i18nc("@tooltip", "Replaces glyphs with swashed alternates."),
                      {KoOpenTypeFeatureInfo::GSUB1, KoOpenTypeFeatureInfo::GSUB3}, true));

    // General required features.
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("ccmp"),
                      i18nc("@title", "Glyph Composition/Decomposition"),
                      i18nc("@tooltip", "This composes or decomposes graphemes so that the resulting glyphs can later be recomposed. Commonly used to handle diacritics."),
                      {KoOpenTypeFeatureInfo::GSUB2}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("kern"),
                      i18nc("@title", "Kerning"),
                      i18nc("@tooltip", "This controls kerning on the font."),
                      {KoOpenTypeFeatureInfo::GPOS2}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("mark"),
                      i18nc("@title", "Mark Positioning"),
                      i18nc("@tooltip", "This controls the positioning of mark glyphs on base glyphs. Commonly used for diacritics."),
                      {KoOpenTypeFeatureInfo::GPOS4, KoOpenTypeFeatureInfo::GPOS5}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("mkmk"),
                      i18nc("@title", "Mark Positioning"),
                      i18nc("@tooltip", "This controls the positioning of mark glyphs onto other mark glyphs. Commonly used for diacritics."),
                      {KoOpenTypeFeatureInfo::GPOS6}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("rclt"),
                      i18nc("@title", "Required Contextual Alternates"),
                      i18nc("@tooltip", "Replaces glyphs contextually wth alternates, required to make the font work."),
                      {KoOpenTypeFeatureInfo::GSUB6}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("rlig"),
                      i18nc("@title", "Required Ligatures"),
                      i18nc("@tooltip", "Replaces sequences of glyphs with required ligatures."),
                      {KoOpenTypeFeatureInfo::GSUB4}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("rvrn"),
                      i18nc("@title", "Required Variation Alternates"),
                      i18nc("@tooltip", "Replaces glyphs in a variable font with ones suited for the current variation."),
                      {KoOpenTypeFeatureInfo::GSUB1}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("size"),
                      i18nc("@title", "Optical size"),
                      i18nc("@tooltip", "Indicates how much the font is suitable for its current size. Does nothing in Krita."),
                      {KoOpenTypeFeatureInfo::GPOS1}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("vkrn"),
                      i18nc("@title", "Vertical Kerning"),
                      i18nc("@tooltip", "This controls vertical kerning on the font."),
                      {KoOpenTypeFeatureInfo::GPOS2, KoOpenTypeFeatureInfo::GPOS8}));

    // Number and math features.
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("afrc"),
                      i18nc("@title", "Alternative Fractions"),
                      i18nc("@tooltip", "Replaces figures separated by a slash with a nut fraction form."),
                      {KoOpenTypeFeatureInfo::GSUB4}, false));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("dnom"),
                      i18nc("@title", "Denominators"),
                      i18nc("@tooltip", "Replaces figures with forms that are suited to represent the denominator in a fraction."),
                      {KoOpenTypeFeatureInfo::GSUB1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("dtls"),
                      i18nc("@title", "Dotless Forms"),
                      i18nc("@tooltip", "Replaces dotted letters such as i and j with dotless forms, for use in mathematical formula."),
                      {KoOpenTypeFeatureInfo::GSUB1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("flac"),
                      i18nc("@title", "Flattened accent forms"),
                      i18nc("@tooltip", "Replaces accents on capital letters with flattened forms. Does nothing in Krita, as this is not a math engine."),
                      {KoOpenTypeFeatureInfo::GSUB1}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("frac"),
                      i18nc("@title", "Fractions"),
                      i18nc("@tooltip", "Replaces figures separated by a slash with a proper diagonal fraction form. If a font has the numerator and denominator features, and the numbers are separated by a 'fraction slash' (U+2044), then this will replace the figures with numerators before the slash and denominators after the slash."),
                      {KoOpenTypeFeatureInfo::GSUB1, KoOpenTypeFeatureInfo::GSUB4}, false));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("lnum"),
                      i18nc("@title", "Lining Figures"),
                      i18nc("@tooltip", "Replaces oldstyle figures with lining figures, which harmonize well with uppercase letters."),
                      {KoOpenTypeFeatureInfo::GSUB1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("mgrk"),
                      i18nc("@title", "Mathematical Greek"),
                      i18nc("@tooltip", "Replaces normal Greek script with glyphs intended for mathematical usage."),
                      {KoOpenTypeFeatureInfo::GSUB1}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("nalt"),
                      i18nc("@title", "Alternate Annotation Forms"),
                      i18nc("@tooltip", "Replaces figures and letters with notational forms, like encircled."),
                      {KoOpenTypeFeatureInfo::GSUB1, KoOpenTypeFeatureInfo::GSUB3}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("numr"),
                      i18nc("@title", "Numerators"),
                      i18nc("@tooltip", "Replaces figures with forms that are suited to represent the numerator in a fraction."),
                      {KoOpenTypeFeatureInfo::GSUB1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("onum"),
                      i18nc("@title", "Oldstyle Figures"),
                      i18nc("@tooltip", "Replaces lining figures with oldstyle figures, which harmonize well with lowercase letters."),
                      {KoOpenTypeFeatureInfo::GSUB1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("ordn"),
                      i18nc("@title", "Ordinals"),
                      i18nc("@tooltip", "Replaces letters that follow figures with their ordinal forms."),
                      {KoOpenTypeFeatureInfo::GSUB6, KoOpenTypeFeatureInfo::GSUB4}, false));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("pnum"),
                      i18nc("@title", "Proportional Figures"),
                      i18nc("@tooltip", "Replaces tabular figures with proportional figures."),
                      {KoOpenTypeFeatureInfo::GSUB1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("sinf"),
                      i18nc("@title", "Scientific Inferiors"),
                      i18nc("@tooltip", "Replaces glyphs with forms that are suited for displaying the number in chemical formulas."),
                      {KoOpenTypeFeatureInfo::GSUB1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("ssty"),
                      i18nc("@title", "Math script style alternates"),
                      i18nc("@tooltip", "Replaces glyphs with ones more suited for super- and subscripts. Does nothing in Krita."),
                      {KoOpenTypeFeatureInfo::GSUB3}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("subs"),
                      i18nc("@title", "Subscript"),
                      i18nc("@tooltip", "Replaces glyphs with ones more suited for subscript."),
                      {KoOpenTypeFeatureInfo::GSUB1, KoOpenTypeFeatureInfo::GPOS1, KoOpenTypeFeatureInfo::GPOS2}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("sups"),
                      i18nc("@title", "Superscript"),
                      i18nc("@tooltip", "Replaces glyphs with ones more suited for superscript."),
                      {KoOpenTypeFeatureInfo::GSUB1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("tnum"),
                      i18nc("@title", "Tabular Figures"),
                      i18nc("@tooltip", "Replaces proportional figures with tabular figures."),
                      {KoOpenTypeFeatureInfo::GSUB1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("zero"),
                      i18nc("@title", "Slashed Zero"),
                      i18nc("@tooltip", "Replaces the number zero with one that has a slash in the middle, which can help prevent confusion with similar glyphs like the letter 'O'."),
                      {KoOpenTypeFeatureInfo::GSUB1}, true));

    // LGC and other bicarmel features.
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("case"),
                      i18nc("@title", "Case-Sensitive Forms"),
                      i18nc("@tooltip", "Adjusts glyphs to work beter with text that consists of only capitals or lining figures."),
                      {KoOpenTypeFeatureInfo::GSUB1, KoOpenTypeFeatureInfo::GPOS1}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("cpsp"),
                      i18nc("@title", "Capital Spacing"),
                      i18nc("@tooltip", "Adjusts inter-glyph spacing for all-capital text."),
                      {KoOpenTypeFeatureInfo::GPOS1}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("c2pc"),
                      i18nc("@title", "Petite Capitals From Capitals"),
                      i18nc("@tooltip", "Replaces uppercase letters with petite capital forms, which are closer to the x-height of a font than small capital forms."),
                      {KoOpenTypeFeatureInfo::GSUB1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("c2sc"),
                      i18nc("@title", "Small Capitals From Capitals"),
                      i18nc("@tooltip", "Replaces uppercase letters with small capital forms."),
                      {KoOpenTypeFeatureInfo::GSUB1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("pcap"),
                      i18nc("@title", "Petite Capitals"),
                      i18nc("@tooltip", "Replaces lowercase letters with petite capital forms, which are closer to the x-height of a font than small capital forms."),
                      {KoOpenTypeFeatureInfo::GSUB1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("smcp"),
                      i18nc("@title", "Small Capitals"),
                      i18nc("@tooltip", "Replaces lowercase letters with small capital forms."),
                      {KoOpenTypeFeatureInfo::GSUB1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("titl"),
                      i18nc("@title", "Titling"),
                      i18nc("@tooltip", "Replaces glyphs with alternate forms suited for header text."),
                      {KoOpenTypeFeatureInfo::GSUB1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("unic"),
                      i18nc("@title", "Unicase"),
                      i18nc("@tooltip", "Replaces both upper and lowercase glyphs with unicase glyphs."),
                      {KoOpenTypeFeatureInfo::GSUB1}, true));

    // Indic and other brahmic-derived script features.
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("abvf"),
                      i18nc("@title", "Above-base Forms"),
                      i18nc("@tooltip", "Controls substitution for above base forms in Khmer and other Indic scripts"),
                      {KoOpenTypeFeatureInfo::GSUB1}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("abvm"),
                      i18nc("@title", "Above-base Mark Positioning"),
                      i18nc("@tooltip", "Controls above-base mark positioning for Indic scripts."),
                      {KoOpenTypeFeatureInfo::GPOS4, KoOpenTypeFeatureInfo::GPOS5}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("abvs"),
                      i18nc("@title", "Above-base Substitutions"),
                      i18nc("@tooltip", "Controls above-base mark ligatures for Indic scripts."),
                      {KoOpenTypeFeatureInfo::GSUB4}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("akhn"),
                      i18nc("@title", "Akhand"),
                      i18nc("@tooltip", "Controls Akhand ligatures for Indic scripts."),
                      {KoOpenTypeFeatureInfo::GSUB4}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("blwf"),
                      i18nc("@title", "Below-base Forms"),
                      i18nc("@tooltip", "Controls substitution for below base forms in Indic scripts."),
                      {KoOpenTypeFeatureInfo::GSUB4}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("blwm"),
                      i18nc("@title", "Below-base Mark Positioning"),
                      i18nc("@tooltip", "Controls below-base mark positioning for Indic scripts."),
                      {KoOpenTypeFeatureInfo::GPOS4, KoOpenTypeFeatureInfo::GPOS5}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("blws"),
                      i18nc("@title", "Below-base Substitutions"),
                      i18nc("@tooltip", "Controls below-base mark ligatures for Indic scripts."),
                      {KoOpenTypeFeatureInfo::GSUB4}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("cfar"),
                      i18nc("@title", "Conjunct Form After Ro"),
                      i18nc("@tooltip", "Controls glyphs following conjoined Ro in Khmer script."),
                      {KoOpenTypeFeatureInfo::GSUB1}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("cjct"),
                      i18nc("@title", "Conjunct Forms"),
                      i18nc("@tooltip", "Controls conjunct forms in Indic scripts."),
                      {KoOpenTypeFeatureInfo::GSUB4}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("dist"),
                      i18nc("@title", "Distances"),
                      i18nc("@tooltip", "Controls distances in Indic scripts, to avoid collisions."),
                      {KoOpenTypeFeatureInfo::GPOS1, KoOpenTypeFeatureInfo::GPOS2}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("half"),
                      i18nc("@title", "Half Forms"),
                      i18nc("@tooltip", "Replaces consonants in Indic scripts with their half forms."),
                      {KoOpenTypeFeatureInfo::GSUB4}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("haln"),
                      i18nc("@title", "Halant Forms"),
                      i18nc("@tooltip", "Replaces consonants in Indic scripts with their halant forms."),
                      {KoOpenTypeFeatureInfo::GSUB4}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("nukt"),
                      i18nc("@title", "Nukta Forms"),
                      i18nc("@tooltip", "Replaces consonants combined with Nukta in Indic scripts with their Nukta forms."),
                      {KoOpenTypeFeatureInfo::GSUB4}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("pref"),
                      i18nc("@title", "Pre-base Forms"),
                      i18nc("@tooltip", "Replaces the pre-base form of a consonant in Khmer script, when a 'Coeng Ra' situation occurs."),
                      {KoOpenTypeFeatureInfo::GSUB4}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("pres"),
                      i18nc("@title", "Pre-base Substitutions"),
                      i18nc("@tooltip", "Replaces consonants with their pre-base forms in Indic scripts."),
                      {KoOpenTypeFeatureInfo::GSUB4, KoOpenTypeFeatureInfo::GSUB5}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("pstf"),
                      i18nc("@title", "Post-base Forms"),
                      i18nc("@tooltip", "Replaces consonants with their post-base forms in Gurmukhi, Malayalam, and Khmer."),
                      {KoOpenTypeFeatureInfo::GSUB4}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("psts"),
                      i18nc("@title", "Post-base Substitutions"),
                      i18nc("@tooltip", "Replaces base glyphs and post-base glyphs with a ligature in Indic scripts."),
                      {KoOpenTypeFeatureInfo::GSUB4}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("rkrf"),
                      i18nc("@title", "Rakar Forms"),
                      i18nc("@tooltip", "Replaces sequences of glyphs in Indic scripts with their Rakar forms."),
                      {KoOpenTypeFeatureInfo::GSUB4}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("rphf"),
                      i18nc("@title", "Reph Form"),
                      i18nc("@tooltip", "Replaces Reph forms in Indic scripts with a consonant and Halant sequence."),
                      {KoOpenTypeFeatureInfo::GSUB4}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("vatu"),
                      i18nc("@title", "Vattu Variants"),
                      i18nc("@tooltip", "Replaces ligatures in Indic scripts with a base consonant and Vattu form."),
                      {KoOpenTypeFeatureInfo::GSUB4}));

    // CJK features.
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("chws"),
                      i18nc("@title", "Contextual Half-width Spacing"),
                      i18nc("@tooltip", "Respaces full-width glyphs to be half-width depending on the context, common examples include CJK full-width brackets and other punctuation marks."),
                      {KoOpenTypeFeatureInfo::GPOS2, KoOpenTypeFeatureInfo::GPOS8}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("cpct"),
                      i18nc("@title", "Centered CJK Punctuation"),
                      i18nc("@tooltip", "Adjusts non-centered punctuation to be centered in CJK scripts."),
                      {KoOpenTypeFeatureInfo::GPOS1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("expt"),
                      i18nc("@title", "Expert Forms"),
                      i18nc("@tooltip", "Replaces standard forms in Japanese fonts with expert forms."),
                      {KoOpenTypeFeatureInfo::GSUB1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("fwid"),
                      i18nc("@title", "Full Widths"),
                      i18nc("@tooltip", "Replaces or respaces proportional glyphs with full-width glyphs."),
                      {KoOpenTypeFeatureInfo::GSUB1, KoOpenTypeFeatureInfo::GPOS1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("chws"),
                      i18nc("@title", "Contextual Half-width Spacing"),
                      i18nc("@tooltip", "Respaces full-width glyphs to be half-width, common examples include CJK full-width brackets and other punctuation marks."),
                      {KoOpenTypeFeatureInfo::GPOS1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("hkna"),
                      i18nc("@title", "Horizontal Kana Alternates"),
                      i18nc("@tooltip", "Replaces standard kana in Japanese fonts with ones suited for horizontal layout."),
                      {KoOpenTypeFeatureInfo::GSUB1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("hngl"),
                      i18nc("@title", "Hangul"),
                      i18nc("@tooltip", "Replaces hanja in Korean with the corresponding hangul. Deprecated."),
                      {KoOpenTypeFeatureInfo::GSUB1, KoOpenTypeFeatureInfo::GSUB3}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("hojo"),
                      i18nc("@title", "Hojo Kanji Forms"),
                      i18nc("@tooltip", "Replaces 'JIS X 0213:2004' form kanji in Japanese fonts with the Hojo ('JIS X 0212-1990') forms."),
                      {KoOpenTypeFeatureInfo::GSUB1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("hwid"),
                      i18nc("@title", "Half Widths"),
                      i18nc("@tooltip", "Replaces or respaces proportional or full-width glyphs with half-width glyphs."),
                      {KoOpenTypeFeatureInfo::GSUB1, KoOpenTypeFeatureInfo::GPOS1}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("ital"),
                      i18nc("@title", "Italics"),
                      i18nc("@tooltip", "Replaces Latin glyphs in a CJK font with their italic forms."),
                      {KoOpenTypeFeatureInfo::GSUB1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("jp78"),
                      i18nc("@title", "JIS78 Forms"),
                      i18nc("@tooltip", "Replaces standard form kanji in Japanese fonts with the JIS78 forms."),
                      {KoOpenTypeFeatureInfo::GSUB1, KoOpenTypeFeatureInfo::GSUB3}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("jp83"),
                      i18nc("@title", "JIS83 Forms"),
                      i18nc("@tooltip", "Replaces standard form kanji in Japanese fonts with the JIS83 forms."),
                      {KoOpenTypeFeatureInfo::GSUB1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("jp90"),
                      i18nc("@title", "JIS90 Forms"),
                      i18nc("@tooltip", "Replaces standard form kanji in Japanese fonts with the JIS90 forms."),
                      {KoOpenTypeFeatureInfo::GSUB1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("jp04"),
                      i18nc("@title", "JIS2004 Forms"),
                      i18nc("@tooltip", "Replaces standard form kanji in Japanese fonts with the JIS2004 forms."),
                      {KoOpenTypeFeatureInfo::GSUB1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("ljmo"),
                      i18nc("@title", "Leading Jamo Forms"),
                      i18nc("@tooltip", "Replaces sequences of leading class Hangul Jamo with a combined leading form."),
                      {KoOpenTypeFeatureInfo::GSUB1}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("nlck"),
                      i18nc("@title", "NLC Kanji Forms"),
                      i18nc("@tooltip", "Replaces standard form kanji in Japanese fonts with the NLC forms."),
                      {KoOpenTypeFeatureInfo::GSUB1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("palt"),
                      i18nc("@title", "Proportional Alternate Widths"),
                      i18nc("@tooltip", "Respaces full-width glyphs to fit in a proportional context."),
                      {KoOpenTypeFeatureInfo::GPOS1}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("pkna"),
                      i18nc("@title", "Proportional Kana"),
                      i18nc("@tooltip", "Replaces full-width kana in Japanese fonts with proportional kana."),
                      {KoOpenTypeFeatureInfo::GSUB1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("pwid"),
                      i18nc("@title", "Proportional Widths"),
                      i18nc("@tooltip", "Replaces uniform-width glyphs with proportional glyphs."),
                      {KoOpenTypeFeatureInfo::GSUB1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("qwid"),
                      i18nc("@title", "Quarter Widths"),
                      i18nc("@tooltip", "Replaces glyphs with quarter-width glyphs."),
                      {KoOpenTypeFeatureInfo::GSUB1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("ruby"),
                      i18nc("@title", "Ruby Notation Forms"),
                      i18nc("@tooltip", "Replaces glyphs in CJK fonts with ones suited for the small text in ruby annotations."),
                      {KoOpenTypeFeatureInfo::GSUB1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("smpl"),
                      i18nc("@title", "Simplified Forms"),
                      i18nc("@tooltip", "Replaces glyphs in CJK fonts with 'simplified' forms."),
                      {KoOpenTypeFeatureInfo::GSUB1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("tjmo"),
                      i18nc("@title", "Trailing Jamo Forms"),
                      i18nc("@tooltip", "Replaces sequences of trailing class Hangul Jamo with a combined trailing form."),
                      {KoOpenTypeFeatureInfo::GSUB1}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("tnam"),
                      i18nc("@title", "Traditional Name Forms"),
                      i18nc("@tooltip", "Replaces standard form Kanji in Japanese fonts with the ones traditionally used in names."),
                      {KoOpenTypeFeatureInfo::GSUB1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("trad"),
                      i18nc("@title", "Traditional Forms"),
                      i18nc("@tooltip", "Replaces glyphs in CJK fonts with 'traditional' forms."),
                      {KoOpenTypeFeatureInfo::GSUB1, KoOpenTypeFeatureInfo::GSUB3}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("twid"),
                      i18nc("@title", "Third Widths"),
                      i18nc("@tooltip", "Replaces glyphs with third-width glyphs."),
                      {KoOpenTypeFeatureInfo::GSUB1, KoOpenTypeFeatureInfo::GPOS1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("valt"),
                      i18nc("@title", "Alternate Vertical Metrics"),
                      i18nc("@tooltip", "Repositions glyphs so they look more appropriate in vertical layout."),
                      {KoOpenTypeFeatureInfo::GPOS1}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("halt"),
                      i18nc("@title", "Alternate Half Widths"),
                      i18nc("@tooltip", "Repositions glyphs so they look more appropriate in horizontal layout."),
                      {KoOpenTypeFeatureInfo::GPOS1}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("vchw"),
                      i18nc("@title", "Vertical Contextual Half-width Spacing"),
                      i18nc("@tooltip", "Respaces full-width glyphs to be half-width depending on the context, common examples include CJK full-width brackets and other punctuation marks."),
                      {KoOpenTypeFeatureInfo::GPOS2}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("vert"),
                      i18nc("@title", "Vertical Alternates"),
                      i18nc("@tooltip", "Replaces glyphs in CJK fonts with ones suited for vertical writing."),
                      {KoOpenTypeFeatureInfo::GSUB1}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("vhal"),
                      i18nc("@title", "Alternate Vertical Half Metrics"),
                      i18nc("@tooltip", "Repositions glyphs so they are half-width in a vertical writing context."),
                      {KoOpenTypeFeatureInfo::GPOS1}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("vjmo"),
                      i18nc("@title", "Vowel Jamo Forms"),
                      i18nc("@tooltip", "Replaces sequences of vowel class Hangul Jamo with a combined vowel form."),
                      {KoOpenTypeFeatureInfo::GSUB1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("vkna"),
                      i18nc("@title", "Vertical Kana Alternates"),
                      i18nc("@tooltip", "Replaces standard kana in Japanese fonts with ones suited for vertical layout."),
                      {KoOpenTypeFeatureInfo::GSUB1}, true));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("vpal"),
                      i18nc("@title", "Proportional Alternate Vertical Metrics"),
                      i18nc("@tooltip", "Repositions glyphs so they are proportional in a vertical writing context."),
                      {KoOpenTypeFeatureInfo::GPOS1}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("vrtr"),
                      i18nc("@title", "Vertical Alternates for Rotation"),
                      i18nc("@tooltip", "Replaces a subset of glyphs with rotated variants for vertical layout."),
                      {KoOpenTypeFeatureInfo::GSUB1}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("vrt2"),
                      i18nc("@title", "Vertical Alternates and Rotation"),
                      i18nc("@tooltip", "Replaces all horizontal-script glyphs with rotated variants for vertical layout."),
                      {KoOpenTypeFeatureInfo::GSUB1}));

    // Arabic and other RTL joined script features
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("curs"),
                      i18nc("@title", "Cursive Positioning"),
                      i18nc("@tooltip", "Adjusts the position of glyphs so they cursively connect."),
                      {KoOpenTypeFeatureInfo::GPOS3}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("fina"),
                      i18nc("@title", "Terminal Forms"),
                      i18nc("@tooltip", "Replaces the final glyph in a sequence of glyphs in a joined script with its end form."),
                      {KoOpenTypeFeatureInfo::GSUB1, KoOpenTypeFeatureInfo::GSUB5, KoOpenTypeFeatureInfo::GSUB6, KoOpenTypeFeatureInfo::GSUB8}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("fin2"),
                      i18nc("@title", "Terminal Form #2"),
                      i18nc("@tooltip", "Replaces the Alaph glyph at the end of Syriac words with its appropriate form."),
                      {KoOpenTypeFeatureInfo::GSUB1}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("fin3"),
                      i18nc("@title", "Terminal Form #3"),
                      i18nc("@tooltip", "Replaces the Alaph glyph at the end of Syriac words with its appropriate form, when neither Terminal forms or Terminal forms 2 are applicable."),
                      {KoOpenTypeFeatureInfo::GSUB5}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("init"),
                      i18nc("@title", "Initial Forms"),
                      i18nc("@tooltip", "Replaces the first glyph in a sequence of glyphs in a joined script with its start form."),
                      {KoOpenTypeFeatureInfo::GSUB1, KoOpenTypeFeatureInfo::GSUB5, KoOpenTypeFeatureInfo::GSUB6, KoOpenTypeFeatureInfo::GSUB8}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("isol"),
                      i18nc("@title", "Initial Forms"),
                      i18nc("@tooltip", "Replaces a glyph in a sequence of glyphs in a joined script with its isolated form."),
                      {KoOpenTypeFeatureInfo::GSUB1, KoOpenTypeFeatureInfo::GSUB5, KoOpenTypeFeatureInfo::GSUB6, KoOpenTypeFeatureInfo::GSUB8}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("ltra"),
                      i18nc("@title", "Left-to-right glyph alternates"),
                      i18nc("@tooltip", "Replaces glyphs with their left-to-right alternates."),
                      {KoOpenTypeFeatureInfo::GSUB1}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("ltrm"),
                      i18nc("@title", "Left-to-right mirrored forms"),
                      i18nc("@tooltip", "Replaces glyphs with their left-to-right mirrored forms."),
                      {KoOpenTypeFeatureInfo::GSUB1}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("medi"),
                      i18nc("@title", "Medial Forms"),
                      i18nc("@tooltip", "Replaces glyphs in the middle of a sequence of glyphs in a joined script with their medial forms."),
                      {KoOpenTypeFeatureInfo::GSUB1, KoOpenTypeFeatureInfo::GSUB5, KoOpenTypeFeatureInfo::GSUB6, KoOpenTypeFeatureInfo::GSUB8}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("med2"),
                      i18nc("@title", "Medial Forms #2"),
                      i18nc("@tooltip", "Replaces the Alaph glyph in the middle of Syriac words with its appropriate form."),
                      {KoOpenTypeFeatureInfo::GSUB1}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("mset"),
                      i18nc("@title", "mset"),
                      i18nc("@tooltip", "Positions Arabic combining marks in fonts for Windows 95 using glyph substitution."),
                      {KoOpenTypeFeatureInfo::GSUB5}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("rtla"),
                      i18nc("@title", "Right-to-left glyph alternates"),
                      i18nc("@tooltip", "Replaces glyphs with their right-to-left alternates."),
                      {KoOpenTypeFeatureInfo::GSUB1}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("rtlm"),
                      i18nc("@title", "Right-to-left mirrored forms"),
                      i18nc("@tooltip", "Replaces glyphs with their right-to-left mirrored forms."),
                      {KoOpenTypeFeatureInfo::GSUB1}));
    initialMap.append(KoOpenTypeFeatureInfo(QLatin1String("stch"),
                      i18nc("@title", "Stretching Glyph Decomposition"),
                      i18nc("@tooltip", "Replaces glyphs that need to stretch with stretchable glyphs."),
                      {KoOpenTypeFeatureInfo::GSUB2}));

    //stylistic sets
    for (int i = 1; i <= 20; i++) {
        char *tag{new char[4]{'s', 's', '0', '0'}};
        tag[2] = (i/10) + '0';
        tag[3] = (i%10) + '0';

        initialMap.append(KoOpenTypeFeatureInfo(QLatin1String(tag, 4),
                          i18nc("@title", "Stylistic Set %1", i),
                          i18nc("@tooltip", "Replaces glyphs with alternates."),
                          {KoOpenTypeFeatureInfo::GSUB1}, true));
    }


    //character variants
    for (int i = 1; i <= 20; i++) {
        char *tag{new char[4]{'c', 'v', '0', '0'}};
        tag[2] = (i/10) + '0';
        tag[3] = (i%10) + '0';

        initialMap.append(KoOpenTypeFeatureInfo(QLatin1String(tag, 4),
                          i18nc("@title", "Character Variant %1", i),
                          i18nc("@tooltip", "Replaces glyphs with alternates."),
                          {KoOpenTypeFeatureInfo::GSUB1}, true));
    }

    Q_FOREACH(KoOpenTypeFeatureInfo feature, initialMap) {
        d->infoMap.insert(feature.tag, feature);
    }
}

KoOpenTypeFeatureInfoFactory::~KoOpenTypeFeatureInfoFactory()
{
}

KoOpenTypeFeatureInfo KoOpenTypeFeatureInfoFactory::infoByTag(const QLatin1String &tag)
{
    KoOpenTypeFeatureInfo def(tag, QString(), QString(), {});
    return d->infoMap.value(tag, def);
}
