/*
 *  SPDX-FileCopyrightText: 2017 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoSvgTextProperties.h"

#include <QMap>
#include "KoSvgText.h"
#include <SvgUtil.h>
#include <SvgLoadingContext.h>
#include <SvgGraphicContext.h>
#include <QFontMetrics>
#include <QGlobalStatic>
#include "kis_global.h"
#include "kis_dom_utils.h"


#include <fontconfig/fontconfig.h>

struct KoSvgTextProperties::Private
{
    QMap<PropertyId, QVariant> properties;

    static bool isInheritable(PropertyId id);
};

KoSvgTextProperties::KoSvgTextProperties()
    : m_d(new Private)
{
}

KoSvgTextProperties::~KoSvgTextProperties()
{
}

KoSvgTextProperties::KoSvgTextProperties(const KoSvgTextProperties &rhs)
    : m_d(new Private)
{
    m_d->properties = rhs.m_d->properties;
}

KoSvgTextProperties &KoSvgTextProperties::operator=(const KoSvgTextProperties &rhs)
{
    if (&rhs != this) {
        m_d->properties = rhs.m_d->properties;
    }
    return *this;
}

void KoSvgTextProperties::setProperty(KoSvgTextProperties::PropertyId id, const QVariant &value)
{
    m_d->properties.insert(id, value);
}

bool KoSvgTextProperties::hasProperty(KoSvgTextProperties::PropertyId id) const
{
    return m_d->properties.contains(id);
}

QVariant KoSvgTextProperties::property(KoSvgTextProperties::PropertyId id, const QVariant &defaultValue) const
{
    return m_d->properties.value(id, defaultValue);
}

void KoSvgTextProperties::removeProperty(KoSvgTextProperties::PropertyId id)
{
    m_d->properties.remove(id);
}

QVariant KoSvgTextProperties::propertyOrDefault(KoSvgTextProperties::PropertyId id) const
{
    QVariant value = m_d->properties.value(id);
    if (value.isNull()) {
        value = defaultProperties().property(id);
    }
    return value;
}

QList<KoSvgTextProperties::PropertyId> KoSvgTextProperties::properties() const
{
    return m_d->properties.keys();
}

bool KoSvgTextProperties::isEmpty() const
{
    return m_d->properties.isEmpty();
}


bool KoSvgTextProperties::Private::isInheritable(PropertyId id) {
    return
            id != UnicodeBidiId &&
            id != AlignmentBaselineId &&
            id != BaselineShiftModeId &&
            id != BaselineShiftValueId &&
            id != FontFeatureSettingsId &&
            id != TextDecorationLineId &&
            id != TextDecorationColorId &&
            id != TextDecorationStyleId &&
            id != InlineSizeId &&
            id != TextTrimId;
}

void KoSvgTextProperties::resetNonInheritableToDefault()
{
    auto it = m_d->properties.begin();
    for (; it != m_d->properties.end(); ++it) {
        if (!m_d->isInheritable(it.key())) {
            it.value() = defaultProperties().property(it.key());
        }
    }
}

void KoSvgTextProperties::inheritFrom(const KoSvgTextProperties &parentProperties)
{
    auto it = parentProperties.m_d->properties.constBegin();
    for (; it != parentProperties.m_d->properties.constEnd(); ++it) {
        if (!hasProperty(it.key()) && m_d->isInheritable(it.key())) {
            setProperty(it.key(), it.value());
        }
    }
}

bool KoSvgTextProperties::inheritsProperty(KoSvgTextProperties::PropertyId id, const KoSvgTextProperties &parentProperties) const
{
    return !hasProperty(id) || parentProperties.property(id) == property(id);
}

KoSvgTextProperties KoSvgTextProperties::ownProperties(const KoSvgTextProperties &parentProperties) const
{
    KoSvgTextProperties result;

    auto it = m_d->properties.constBegin();
    for (; it != m_d->properties.constEnd(); ++it) {
        if (!parentProperties.hasProperty(it.key()) || parentProperties.property(it.key()) != it.value()) {
            result.setProperty(it.key(), it.value());
        }
    }

    return result;
}

inline qreal roundToStraightAngle(qreal value)
{
    return normalizeAngle(int((value + M_PI_4) / M_PI_2) * M_PI_2);
}

void KoSvgTextProperties::parseSvgTextAttribute(const SvgLoadingContext &context, const QString &command, const QString &value)
{
    const QMap <QString, KoSvgText::FontVariantFeature> featureMap = KoSvgText::fontVariantStrings();
    if (command == "writing-mode") {
        setProperty(WritingModeId, KoSvgText::parseWritingMode(value));
    } else if (command == "glyph-orientation-vertical") {
        KoSvgText::AutoValue autoValue = KoSvgText::parseAutoValueAngular(value, context);

        if (!autoValue.isAuto) {
            autoValue.customValue = roundToStraightAngle(autoValue.customValue);
        }

        setProperty(TextOrientationId, KoSvgText::parseTextOrientationFromGlyphOrientation(autoValue));
    } else if (command == "text-orientation") {
        setProperty(TextOrientationId, KoSvgText::parseTextOrientation(value));
    } else if (command == "direction") {
        setProperty(DirectionId, KoSvgText::parseDirection(value));
    } else if (command == "unicode-bidi") {
        setProperty(UnicodeBidiId, KoSvgText::parseUnicodeBidi(value));
    } else if (command == "text-anchor") {
        setProperty(TextAnchorId, KoSvgText::parseTextAnchor(value));
    } else if (command == "dominant-baseline") {
        setProperty(DominantBaselineId, KoSvgText::parseBaseline(value));
    } else if (command == "alignment-baseline") {
        setProperty(AlignmentBaselineId, KoSvgText::parseBaseline(value));
    } else if (command == "baseline-shift") {
        KoSvgText::BaselineShiftMode mode = KoSvgText::parseBaselineShiftMode(value);
        setProperty(BaselineShiftModeId, mode);
        if (mode == KoSvgText::ShiftPercentage) {
            if (value.endsWith("%")) {
                setProperty(BaselineShiftValueId, SvgUtil::fromPercentage(value));
            } else {
                const qreal parsedValue = SvgUtil::parseUnitXY(context.currentGC(), value);
                const qreal lineHeight = propertyOrDefault(FontSizeId).toReal();

                if (lineHeight != 0.0) {
                    setProperty(BaselineShiftValueId, parsedValue / lineHeight);
                }
            }
        }
    } else if (command == "kerning") {
        setProperty(KerningId, KoSvgText::fromAutoValue(KoSvgText::parseAutoValueXY(value, context)));
    } else if (command == "letter-spacing") {
        setProperty(LetterSpacingId, KoSvgText::fromAutoValue(KoSvgText::parseAutoValueXY(value, context, "normal")));
    } else if (command == "word-spacing") {
        setProperty(WordSpacingId, KoSvgText::fromAutoValue(KoSvgText::parseAutoValueXY(value, context, "normal")));
    } else if (command == "font-family") {
        QStringList familiesList = value.split(',', QString::SkipEmptyParts);
        for (QString &family : familiesList) {
            family = family.trimmed();
            if ((family.startsWith('\"') && family.endsWith('\"')) ||
                (family.startsWith('\'') && family.endsWith('\''))) {

                family = family.mid(1, family.size() - 2);
            }
        }
        setProperty(FontFamiliesId, familiesList);

    } else if (command == "font-style") {
        const QFont::Style style =
            value == "italic" ? QFont::StyleItalic :
            value == "oblique" ? QFont::StyleOblique :
            QFont::StyleNormal;

        setProperty(FontStyleId, style);
    } else if (command == "font-variant" ||
               command == "font-variant-ligatures" ||
               command == "font-variant-position" ||
               command == "font-variant-caps" ||
               command == "font-variant-numeric" ||
               command == "font-variant-east-asian" ||
               command == "font-variant-alternates") {

        const QStringList features = value.split(" ");
        for (QString f: features) {

            KoSvgText::FontVariantFeature feature = featureMap.value(f.split("(").first());

            if (feature == KoSvgText::CommonLigatures || feature == KoSvgText::NoCommonLigatures) {
                setProperty(FontVariantCommonLigId, feature);
            } else if (feature == KoSvgText::DiscretionaryLigatures || feature == KoSvgText::NoDiscretionaryLigatures) {
                setProperty(FontVariantDiscretionaryLigId, feature);
            } else if (feature == KoSvgText::HistoricalLigatures || feature == KoSvgText::NoHistoricalLigatures) {
                setProperty(FontVariantHistoricalLigId, feature);
            } else if (feature == KoSvgText::ContextualAlternates || feature == KoSvgText::NoContextualAlternates) {
                setProperty(FontVariantContextualAltId, feature);
            }

            if (feature == KoSvgText::PositionSub || feature == KoSvgText::PositionSuper) {
                setProperty(FontVariantPositionId, feature);
            }

            if (feature >= KoSvgText::SmallCaps && feature <= KoSvgText::TitlingCaps) {
                setProperty(FontVariantCapsId, feature);
            }

            if (feature == KoSvgText::LiningNums || feature == KoSvgText::OldStyleNums) {
                setProperty(FontVariantNumFigureId, feature);
            }
            if (feature == KoSvgText::ProportionalNums || feature == KoSvgText::TabularNums) {
                setProperty(FontVariantNumSpacingId, feature);
            }
            if (feature == KoSvgText::DiagonalFractions || feature == KoSvgText::StackedFractions) {
                setProperty(FontVariantNumFractId, feature);
            }
            if (feature == KoSvgText::Ordinal) {
                setProperty(FontVariantNumOrdinalId, feature);
            }
            if (feature == KoSvgText::SlashedZero) {
                setProperty(FontVariantNumSlashedZeroId, feature);
            }

            if (feature >= KoSvgText::EastAsianJis78 && feature <= KoSvgText::EastAsianTraditional) {
                setProperty(FontVariantEastAsianVarId, feature);
            }
            if (feature == KoSvgText::EastAsianFullWidth || feature == KoSvgText::EastAsianProportionalWidth) {
                setProperty(FontVariantEastAsianWidthId, feature);
            }
            if (feature == KoSvgText::EastAsianRuby) {
                setProperty(FontVariantRubyId, feature);
            }

            if (feature == KoSvgText::HistoricalForms) {
                setProperty(FontVariantHistoricalFormsId, feature);
            }

            if (feature == KoSvgText::FontVariantNone || feature == KoSvgText::FontVariantNormal) {
                if (command == "font-variant" || command == "font-variant-ligatures") {
                    removeProperty(FontVariantCommonLigId);
                    removeProperty(FontVariantDiscretionaryLigId);
                    removeProperty(FontVariantHistoricalLigId);
                    removeProperty(FontVariantContextualAltId);
                }
                if (command == "font-variant" || command == "font-variant-position") {
                    removeProperty(FontVariantPositionId);
                }
                if (command == "font-variant" || command == "font-variant-caps") {
                    removeProperty(FontVariantCapsId);
                }
                if (command == "font-variant" || command == "font-variant-numeric") {
                    removeProperty(FontVariantNumFigureId);
                    removeProperty(FontVariantNumSpacingId);
                    removeProperty(FontVariantNumFractId);
                    removeProperty(FontVariantNumSlashedZeroId);
                    removeProperty(FontVariantNumOrdinalId);
                }
                if (command == "font-variant" || command == "font-variant-east-asian") {
                    removeProperty(FontVariantEastAsianVarId);
                    removeProperty(FontVariantEastAsianWidthId);
                    removeProperty(FontVariantRubyId);
                }
                if (command == "font-variant" || command == "font-variant-alternates") {
                    removeProperty(FontVariantHistoricalFormsId);
                }
            }

        }

    } else if (command == "font-feature-settings"){
        setProperty(FontFeatureSettingsId, value.split(","));
    } else if (command == "font-stretch") {
        int newStretch = 100;

        static const std::vector<int> fontStretches = {50, 62, 75, 87, 100, 112, 125, 150, 200};
        static const std::vector<QString> fontStretchNames =
            {"ultra-condensed","extra-condensed","condensed","semi-condensed",
             "normal",
             "semi-expanded","expanded","extra-expanded","ultra-expanded"};

        if (value == "wider") {
            auto it = std::upper_bound(fontStretches.begin(),
                                       fontStretches.end(),
                                       propertyOrDefault(KoSvgTextProperties::FontStretchId).toInt());

            newStretch = it != fontStretches.end() ? *it : fontStretches.back();
        } else if (value == "narrower") {
            auto it = std::upper_bound(fontStretches.rbegin(),
                                       fontStretches.rend(),
                                       propertyOrDefault(KoSvgTextProperties::FontStretchId).toInt(),
                                       std::greater<int>());

            newStretch = it != fontStretches.rend() ? *it : fontStretches.front();

        } else {
            // try to read numerical stretch value
            bool ok = false;
            newStretch = value.toInt(&ok, 10);

            if (!ok) {
                auto it = std::find(fontStretchNames.begin(), fontStretchNames.end(), value);
                if (it != fontStretchNames.end()) {
                    newStretch = fontStretches[it - fontStretchNames.begin()];
                }
            }
        }

        setProperty(FontStretchId, newStretch);

    } else if (command == "font-weight") {
        int weight = 400;

        // map svg weight to qt weight
        // svg value        qt value
        // 100,200,300      1, 17, 33
        // 400              50          (normal)
        // 500,600          58,66
        // 700              75          (bold)
        // 800,900          87,99
        static const std::vector<int> svgFontWeights = {100,200,300,400,500,600,700,800,900};

        if (value == "bold")
            weight = 700;
        else if (value == "bolder") {
            auto it = std::upper_bound(svgFontWeights.begin(),
                                       svgFontWeights.end(),
                                       propertyOrDefault(FontWeightId).toInt());

            weight = it != svgFontWeights.end() ? *it : svgFontWeights.back();
        } else if (value == "lighter") {
            auto it = std::upper_bound(svgFontWeights.rbegin(),
                                       svgFontWeights.rend(),
                                       propertyOrDefault(FontWeightId).toInt(),
                                       std::greater<int>());

            weight = it != svgFontWeights.rend() ? *it : svgFontWeights.front();
        } else {
            bool ok = false;

            // try to read numerical weight value
            int parsed = value.toInt(&ok, 10);
            if (ok) {
                weight = qBound(0, parsed, 1000);
            }
        }

        setProperty(FontWeightId, weight);

    } else if (command == "font-size") {
        const qreal pointSize = SvgUtil::parseUnitY(context.currentGC(), value);
        if (pointSize > 0.0) {
            setProperty(FontSizeId, pointSize);
        }
    } else if (command == "font-size-adjust") {
        setProperty(FontSizeAdjustId, KoSvgText::fromAutoValue(KoSvgText::parseAutoValueY(value, context, "none")));

    } else if (command == "text-decoration"
               || command == "text-decoration-line"
               || command == "text-decoration-style"
               || command == "text-decoration-color"
               || command == "text-decoration-position") {
        using namespace KoSvgText;
        TextDecorations deco = propertyOrDefault(TextDecorationLineId).value<KoSvgText::TextDecorations>();
        TextDecorationStyle style = TextDecorationStyle(propertyOrDefault(TextDecorationStyleId).toInt());
        TextDecorationUnderlinePosition underlinePosH = TextDecorationUnderlinePosition(propertyOrDefault(TextDecorationPositionHorizontalId).toInt());;
        TextDecorationUnderlinePosition underlinePosV = TextDecorationUnderlinePosition(propertyOrDefault(TextDecorationPositionVerticalId).toInt());;
        QColor textDecorationColor = propertyOrDefault(TextDecorationStyleId).value<QColor>();
        qDebug() << "default color" << textDecorationColor;


        Q_FOREACH (const QString &param, value.split(' ', QString::SkipEmptyParts)) {
            if (param == "line-through") {
                deco |= DecorationLineThrough;
            } else if (param == "underline") {
                deco |= DecorationUnderline;
            } else if (param == "overline") {
                deco |= DecorationOverline;
            } else if (param == "solid") {
                style = Solid;
            } else if (param == "double") {
                style = Double;
            } else if (param == "dotted") {
                style = Dotted;
            } else if (param == "dashed") {
                style = Dashed;
            } else if (param == "wavy") {
                style = Wavy;
            } else if (param == "auto") {
                underlinePosH = UnderlineAuto;
            } else if (param == "under") {
                underlinePosH = UnderlineUnder;
            } else if (param == "left") {
                underlinePosV = UnderlineLeft;
            } else if (param == "auto") {
                underlinePosV = UnderlineRight;
            } else if (QColor::isValidColor(param)) {
                // TODO: Convert to KoColor::fromSvg11.
                textDecorationColor = QColor(param);
            }
        }

        if (command == "text-decoration" || command == "text-decoration-line") {
            setProperty(TextDecorationLineId, QVariant::fromValue(deco));
        }
        if (command == "text-decoration" || command == "text-decoration-style") {
            setProperty(TextDecorationStyleId, style);
        }
        if (command == "text-decoration" || command == "text-decoration-color") {
            setProperty(TextDecorationColorId, QVariant::fromValue(textDecorationColor));
        }
        if (command == "text-decoration" || command == "text-decoration-position") {
            setProperty(TextDecorationPositionHorizontalId, underlinePosH);
            setProperty(TextDecorationPositionVerticalId, underlinePosV);
        }

    } else if (command == "xml:lang") {
        setProperty(TextLanguage, value);
    } else if (command == "text-transform") {
        setProperty(TextTransformId, QVariant::fromValue(KoSvgText::parseTextTransform(value)));
    } else if (command == "white-space") {
        KoSvgText::TextSpaceTrims trims = propertyOrDefault(TextTrimId).value<KoSvgText::TextSpaceTrims>();
        KoSvgText::TextWrap wrap = KoSvgText::TextWrap(propertyOrDefault(TextWrapId).toInt());
        KoSvgText::TextSpaceCollapse collapse = KoSvgText::TextSpaceCollapse(propertyOrDefault(TextCollapseId).toInt());

        KoSvgText::whiteSpaceValueToLongHands(value, collapse, wrap, trims);

        setProperty(TextTrimId, QVariant::fromValue(trims));
        setProperty(TextWrapId, wrap);
        setProperty(TextCollapseId, collapse);

    } else if (command == "xml:space") {
        KoSvgText::TextSpaceCollapse collapse = KoSvgText::TextSpaceCollapse(propertyOrDefault(TextCollapseId).toInt());
        KoSvgText::xmlSpaceToLongHands(value, collapse);
        setProperty(TextCollapseId, collapse);
    } else if (command == "word-break") {
        setProperty(WordBreakId, KoSvgText::parseWordBreak(value));
    } else if (command == "line-break") {
        setProperty(LineBreakId, KoSvgText::parseLineBreak(value));
    } else if (command == "text-align" || command == "text-align-all" || command == "text-align-last") {
        if (command == "text-align" || command == "text-align-all") {
            setProperty(TextAlignAllId, KoSvgText::parseTextAlign(value));
        }
        if (command == "text-align" || command == "text-align-last") {
            setProperty(TextAlignLastId, KoSvgText::parseTextAlign(value));
        }
    } else if (command == "line-height") {

        bool ok = false;
        qreal parsed = value.toDouble(&ok);
        qreal fontSize = propertyOrDefault(FontSizeId).toReal();
        KoSvgText::AutoValue lineheightVal;
        // Normal: use font metrics, ratio/percentage: multiply with fontsize, other: is absolute length.
        if (ok) {
            lineheightVal.customValue = fontSize * parsed;
            lineheightVal.isAuto = false;
        } else if (value.endsWith("%")) {
            lineheightVal.isAuto = false;
            lineheightVal.customValue = fontSize * SvgUtil::fromPercentage(value);
        } else {
            lineheightVal = KoSvgText::parseAutoValueXY(value, context, "normal");
        }
        setProperty(LineHeightId, KoSvgText::fromAutoValue(lineheightVal));
    } else if (command == "text-indent") {
        setProperty(TextIndentId, QVariant::fromValue(KoSvgText::parseTextIndent(value, context)));
    } else if (command == "hanging-punctuation") {
        KoSvgText::HangingPunctuations hang;
        Q_FOREACH (const QString &param, value.split(' ', QString::SkipEmptyParts)) {
            if (param == "first") {
                hang.setFlag(KoSvgText::HangFirst, true);
            } else if (param == "last") {
                hang.setFlag(KoSvgText::HangLast, true);
            } else if (param == "allow-end") {
                hang.setFlag(KoSvgText::HangEnd, true);
                hang.setFlag(KoSvgText::HangForce, false);
            } else if (param == "force-end") {
                hang.setFlag(KoSvgText::HangEnd, true);
                hang.setFlag(KoSvgText::HangForce, true);
            }
        }
        setProperty(HangingPunctuationId, QVariant::fromValue(hang));
    } else if (command == "inline-size") {
        setProperty(InlineSizeId, KoSvgText::fromAutoValue(KoSvgText::parseAutoValueXY(value, context, "auto")));
    } else if (command == "overflow") {
        setProperty(TextOverFlowId, value == "visible"? KoSvgText::OverFlowVisible: KoSvgText::OverFlowClip);
    } else if (command == "text-overflow") {
        setProperty(TextOverFlowId, value == "ellipse"? KoSvgText::OverFlowEllipse: KoSvgText::OverFlowClip);
    } else if (command == "tab-size") {
        setProperty(TabSizeId, QVariant::fromValue(KoSvgText::parseTabSize(value, context)));
    }else {
        qFatal("FATAL: Unknown SVG property: %s = %s", command.toUtf8().data(), value.toUtf8().data());
    }
}

QMap<QString, QString> KoSvgTextProperties::convertToSvgTextAttributes() const
{
    using namespace KoSvgText;

    QMap<QString, QString> result;

    bool svg1_1 = false;

    if (hasProperty(WritingModeId)) {
        result.insert("writing-mode", writeWritingMode(WritingMode(property(WritingModeId).toInt()), svg1_1));
    }

    if (hasProperty(TextOrientationId)) {
        if (svg1_1) {
            TextOrientation orientation = TextOrientation(property(TextOrientationId).toInt());
            QString value = "auto";
            if (orientation == OrientationUpright){
                value = "0";
            } else if (orientation == OrientationSideWays) {
                value = "90";
            }
            result.insert("glyph-orientation-vertical", value);
        } else {
            result.insert("text-orientation", writeTextOrientation(TextOrientation(property(TextOrientationId).toInt())));
        }
    }

    if (hasProperty(DirectionId)) {
        result.insert("direction", writeDirection(Direction(property(DirectionId).toInt())));
    }

    if (hasProperty(UnicodeBidiId)) {
        result.insert("unicode-bidi", writeUnicodeBidi(UnicodeBidi(property(UnicodeBidiId).toInt())));
    }

    if (hasProperty(TextAnchorId)) {
        result.insert("text-anchor", writeTextAnchor(TextAnchor(property(TextAnchorId).toInt())));
    }

    if (hasProperty(DominantBaselineId)) {
        result.insert("dominant-baseline", writeDominantBaseline(Baseline(property(DominantBaselineId).toInt())));
    }

    if (hasProperty(AlignmentBaselineId)) {
        result.insert("alignment-baseline", writeAlignmentBaseline(Baseline(property(AlignmentBaselineId).toInt())));
    }

    if (hasProperty(BaselineShiftModeId)) {
        result.insert("baseline-shift", writeBaselineShiftMode(
                      BaselineShiftMode(property(BaselineShiftModeId).toInt()),
                      property(BaselineShiftValueId).toReal()));
    }

    if (hasProperty(KerningId)) {
        result.insert("kerning", writeAutoValue(property(KerningId).value<AutoValue>()));
    }

    if (hasProperty(LetterSpacingId)) {
        result.insert("letter-spacing", writeAutoValue(property(LetterSpacingId).value<AutoValue>(), "normal"));
    }

    if (hasProperty(WordSpacingId)) {
        result.insert("word-spacing", writeAutoValue(property(WordSpacingId).value<AutoValue>(), "normal"));
    }

    if (hasProperty(FontFamiliesId)) {
        result.insert("font-family", property(FontFamiliesId).toStringList().join(','));
    }

    if (hasProperty(FontStyleId)) {
        const QFont::Style style = QFont::Style(property(FontStyleId).toInt());

        const QString value =
            style == QFont::StyleItalic ? "italic" :
            style == QFont::StyleOblique ? "oblique" : "normal";

        result.insert("font-style", value);
    }

    QStringList features;
    const QMap <QString, KoSvgText::FontVariantFeature> featureMap = KoSvgText::fontVariantStrings();

    QVector<PropertyId> liga = {FontVariantCommonLigId,
                                FontVariantDiscretionaryLigId,
                                FontVariantHistoricalLigId,
                                FontVariantContextualAltId,
                                FontVariantNumFigureId,
                                FontVariantNumSpacingId,
                                FontVariantNumFractId,
                                FontVariantNumSlashedZeroId,
                                FontVariantNumOrdinalId,
                                FontVariantEastAsianVarId,
                                FontVariantEastAsianWidthId,
                                FontVariantRubyId,
                                FontVariantHistoricalFormsId,
                                FontVariantPositionId,
                                FontVariantCapsId};
    for (PropertyId id: liga) {
        if (hasProperty(id)) {
            features.append(featureMap.key(KoSvgText::FontVariantFeature(property(id).toInt())));
        }
    }
    if (!features.isEmpty()) {
        result.insert("font-variant", features.join(" "));
    }
    if (hasProperty(FontFeatureSettingsId)) {
        result.insert("font-feature-settings", property(FontFeatureSettingsId).toStringList().join(", "));
    }

    if (hasProperty(FontStretchId)) {
        const int stretch = property(FontStretchId).toInt();

        static const std::vector<int> fontStretches = {50, 62, 75, 87, 100, 112, 125, 150, 200};
        static const std::vector<QString> fontStretchNames =
            {"ultra-condensed","extra-condensed","condensed","semi-condensed",
             "normal",
             "semi-expanded","expanded","extra-expanded","ultra-expanded"};

        auto it = std::lower_bound(fontStretches.begin(), fontStretches.end(), stretch);
        if (it != fontStretches.end()) {
            result.insert("font-stretch", fontStretchNames[it - fontStretches.begin()]);
        }
    }

    if (hasProperty(FontWeightId)) {

        result.insert("font-weight", KisDomUtils::toString(property(FontWeightId).toInt()));
    }

    if (hasProperty(FontSizeId)) {
        const qreal size = property(FontSizeId).toReal();
        result.insert("font-size", KisDomUtils::toString(size));
    }

    if (hasProperty(FontSizeAdjustId)) {
        result.insert("font-size-adjust", writeAutoValue(property(FontSizeAdjustId).value<AutoValue>()));
    }

    QStringList decoStrings;
    if (hasProperty(TextDecorationLineId)) {
        TextDecorations deco = property(TextDecorationLineId).value<TextDecorations>();



        if (deco & DecorationUnderline) {
            decoStrings.append("underline");
        }

        if (deco & DecorationOverline) {
            decoStrings.append("overline");
        }

        if (deco & DecorationLineThrough) {
            decoStrings.append("line-through");
        }

        if (deco != DecorationNone) {
            if (hasProperty(TextDecorationStyleId)) {
                TextDecorationStyle style = TextDecorationStyle(property(TextDecorationStyleId).toInt());

                if (style == Solid) {
                    decoStrings.append("solid");
                } else if (style == Double) {
                    decoStrings.append("double");
                } else if (style == Dotted) {
                    decoStrings.append("dotted");
                } else if (style == Dashed) {
                    decoStrings.append("dashed");
                } else if (style == Wavy) {
                    decoStrings.append("wavy");
                }
            }
            if (hasProperty(TextDecorationColorId)) {
                QColor color = property(TextDecorationColorId).value<QColor>();
                if (color.isValid()) {
                    decoStrings.append(color.name());
                }
            }
        }
        if (!decoStrings.isEmpty()) {
            result.insert("text-decoration", decoStrings.join(' '));
        }
    }

    QStringList decoPositionStrings;
    if (hasProperty(TextDecorationPositionHorizontalId)) {
        TextDecorationUnderlinePosition pos = TextDecorationUnderlinePosition(property(TextDecorationPositionHorizontalId).toInt());
        if (pos == UnderlineAuto) {
            decoPositionStrings.append("auto");
        } else if (pos == UnderlineUnder) {
            decoPositionStrings.append("under");
        } else if (pos == UnderlineLeft) {
            decoPositionStrings.append("left");
        } else if (pos == UnderlineRight) {
            decoPositionStrings.append("right");
        }
    }
    if (hasProperty(TextDecorationPositionVerticalId)) {
        TextDecorationUnderlinePosition pos = TextDecorationUnderlinePosition(property(TextDecorationPositionVerticalId).toInt());
        if (pos == UnderlineAuto) {
            decoPositionStrings.append("auto");
        } else if (pos == UnderlineUnder) {
            decoPositionStrings.append("under");
        } else if (pos == UnderlineLeft) {
            decoPositionStrings.append("left");
        } else if (pos == UnderlineRight) {
            decoPositionStrings.append("right");
        }
    }
    if (!decoPositionStrings.isEmpty()) {
        result.insert("text-decoration-position", decoPositionStrings.join(' '));
    }
    if (hasProperty(TextLanguage)) {
        result.insert("xml:lang", property(TextLanguage).toString());
    }

    if (hasProperty(TextTransformId)) {
        result.insert("text-transform", writeTextTransform(property(TextTransformId).value<TextTransformInfo>()));
    }
    if (hasProperty(WordBreakId)) {
        result.insert("word-break", writeWordBreak(WordBreak(property(WordBreakId).toInt())));
    }
    if (hasProperty(LineBreakId)) {
        result.insert("line-break", writeLineBreak(LineBreak(property(LineBreakId).toInt())));
    }
    if (hasProperty(TextAlignAllId)) {
        TextAlign all = TextAlign(property(TextAlignAllId).toInt());
        result.insert("text-align", writeTextAlign(all));
        TextAlign last = TextAlign(property(TextAlignLastId).toInt());
        if (last != all || last != AlignLastAuto) {
            result.insert("text-align-last", writeTextAlign(last));
        }
    }
    if (hasProperty(TextCollapseId) || hasProperty(TextWrapId)) {
        KoSvgText::TextSpaceTrims trims = propertyOrDefault(TextTrimId).value<KoSvgText::TextSpaceTrims>();
        KoSvgText::TextWrap wrap = KoSvgText::TextWrap(propertyOrDefault(TextWrapId).toInt());
        KoSvgText::TextSpaceCollapse collapse = KoSvgText::TextSpaceCollapse(propertyOrDefault(TextCollapseId).toInt());
        if (collapse == KoSvgText::PreserveSpaces || svg1_1) {
            result.insert("xml:space", writeXmlSpace(collapse));
        } else {
            result.insert("white-space", writeWhiteSpaceValue(collapse, wrap, trims));
        }
    }
    if (hasProperty(LineHeightId)) {
        KoSvgText::AutoValue lineHeight = property(LineHeightId).value<AutoValue>();
        if (lineHeight.isAuto) {
            result.insert("line-height", "normal");
        } else {
            // We always compute the percentage because it's the least ambiguous with regard to SVG units.
            qreal fontSize = propertyOrDefault(FontSizeId).toReal();
            result.insert("line-height", QString::number((lineHeight.customValue/fontSize)*100)+"%");
        }
    }
    if (hasProperty(InlineSizeId)) {
        result.insert("inline-size", writeAutoValue(property(InlineSizeId).value<AutoValue>(), "auto"));
    }
    if (hasProperty(TextIndentId)) {
        result.insert("text-indent", writeTextIndent(propertyOrDefault(TextIndentId).value<TextIndentInfo>()));
    }
    if (hasProperty(TabSizeId)) {
        result.insert("tab-size", writeTabSize(propertyOrDefault(TabSizeId).value<TabSizeInfo>()));
    }
    if (hasProperty(HangingPunctuationId)) {
        HangingPunctuations hang = property(HangingPunctuationId).value<HangingPunctuations>();
        QStringList value;

        if (hang.testFlag(HangFirst)) {
            value.append("first");
        }
        if (hang.testFlag(HangLast)) {
            value.append("last");
        }
        if (hang.testFlag(HangEnd)) {
            if (hang.testFlag(HangForce)) {
                value.append("force-end");
            } else {
                value.append("allow-end");
            }
        }

        if (!value.isEmpty()) {
            result.insert("hanging-punctuation", value.join(" "));
        }
    }

    return result;
}

QFont KoSvgTextProperties::generateFont() const
{
    QString fontFamily;

    QStringList familiesList =
        propertyOrDefault(KoSvgTextProperties::FontFamiliesId).toStringList();
    if (!familiesList.isEmpty()) {
        fontFamily = familiesList.first();
    }
    const QFont::Style style =
        QFont::Style(propertyOrDefault(KoSvgTextProperties::FontStyleId).toInt());

    // for rounding see a comment below!
    QFont font(fontFamily
               , qRound(propertyOrDefault(KoSvgTextProperties::FontSizeId).toReal())
               , propertyOrDefault(KoSvgTextProperties::FontWeightId).toInt()
               , style != QFont::StyleNormal);
    font.setStyle(style);

    /**
     * The constructor of QFont cannot accept fractional font size, so we pass
     * a rounded one to it and set the correct one later on
     */
    font.setPointSizeF(propertyOrDefault(KoSvgTextProperties::FontSizeId).toReal());

    font.setStretch(propertyOrDefault(KoSvgTextProperties::FontStretchId).toInt());

    using namespace KoSvgText;

    TextDecorations deco =
        propertyOrDefault(KoSvgTextProperties::TextDecorationLineId)
            .value<KoSvgText::TextDecorations>();

    font.setStrikeOut(deco & DecorationLineThrough);
    font.setUnderline(deco & DecorationUnderline);
    font.setOverline(deco & DecorationOverline);

    struct FakePaintDevice : public QPaintDevice
    {
        QPaintEngine *paintEngine() const override {
            return nullptr;
        }

        int metric(QPaintDevice::PaintDeviceMetric metric) const override {

            if (metric == QPaintDevice::PdmDpiX || metric == QPaintDevice::PdmDpiY) {
                return 72;
            }


            return QPaintDevice::metric(metric);
        }
    };

    // paint device is used only to initialize DPI, so
    // we can delete it right after creation of the font
    FakePaintDevice fake72DpiPaintDevice;
    return QFont(font, &fake72DpiPaintDevice);
}

QStringList KoSvgTextProperties::fontFileNameForText(QString text, QVector<int> &lengths) const
{
    if (text.isEmpty()) {
        return QStringList();
    }

    //FcObjectSet *objectSet = FcObjectSetBuild(FC_FAMILY, FC_FILE, FC_WIDTH, FC_WEIGHT, FC_SLANT, nullptr);
    FcCharSet *charSet = FcCharSetNew();
    for (int i = 0; i < text.size(); ++i) {
        FcCharSetAddChar(charSet, text.at(i).unicode());
    }
    FcPattern *p = FcPatternCreate();
    QStringList familiesList =
        propertyOrDefault(KoSvgTextProperties::FontFamiliesId).toStringList();
    for (QString family: familiesList) {
        const FcChar8 *vals = reinterpret_cast<FcChar8*>(family.toUtf8().data());
        FcPatternAddString(p, FC_FAMILY, vals);
    }
    const QFont::Style style =
        QFont::Style(propertyOrDefault(KoSvgTextProperties::FontStyleId).toInt());
    if (style == QFont::StyleItalic) {
        FcPatternAddInteger(p, FC_SLANT, FC_SLANT_ITALIC);
    } else if (style == QFont::StyleOblique) {
        FcPatternAddInteger(p, FC_SLANT, FC_SLANT_ITALIC);
    } else {
        FcPatternAddInteger(p, FC_SLANT, FC_SLANT_ROMAN);
    }
    FcPatternAddInteger(p, FC_WEIGHT, FcWeightFromOpenType(propertyOrDefault(KoSvgTextProperties::FontWeightId).toInt()));
    FcPatternAddInteger(p, FC_WIDTH, propertyOrDefault(KoSvgTextProperties::FontStretchId).toInt());
    FcPatternAddBool(p, FC_OUTLINE, true);

    FcResult result;
    FcFontSet *fontSet = FcFontSort(FcConfigGetCurrent(), p, FcTrue, &charSet, &result);

    QString fontFileName;
    FcChar8 *fileValue = 0;
    FcCharSet *set = 0;
    QVector<int> familyValues(text.size());
    familyValues.fill(-1);

    for (int i=0; i<fontSet->nfont; i++) {
        if (FcPatternGetCharSet(fontSet->fonts[i], FC_CHARSET, 0, &set) == FcResultMatch) {
            for (int j = 0; j < text.size(); ++j) {
                if (familyValues.at(j) == -1) {
                    QString unicode = text.at(j);
                    if (text.at(j).isHighSurrogate()) {
                        unicode += text.at(j+1);
                    }
                    if (text.at(j).isLowSurrogate()) {
                        familyValues[j] = familyValues[j-1];
                        continue;
                    }
                    if (FcCharSetHasChar(set, unicode.toUcs4().first())) {
                        familyValues[j] = i;
                    }
                }
            }
            if (!familyValues.contains(-1)) {
                break;
            }
        }
    }
    int length = 0;
    int startIndex = 0;
    int lastIndex = familyValues.at(0);
    if (FcPatternGetString(fontSet->fonts[lastIndex], FC_FILE, 0, &fileValue) == FcResultMatch) {
        fontFileName = QString(reinterpret_cast<char*>(fileValue));
    }
    QStringList fileNames;
    for (int i = 0; i< familyValues.size(); i++) {
        if (lastIndex != familyValues.at(i)) {
            lengths.append(text.mid(startIndex, length).size());
            fileNames.append(fontFileName);
            startIndex = i;
            length = 0;
            lastIndex = familyValues.at(i);
            if (lastIndex != -1) {
                if (FcPatternGetString(fontSet->fonts[lastIndex], FC_FILE, 0, &fileValue) == FcResultMatch) {
                    fontFileName = QString(reinterpret_cast<char*>(fileValue));
                }
            }
        }
        length +=1;
    }
    if (length > 0) {
        lengths.append(text.mid(startIndex, length).size());
        fileNames.append(fontFileName);
    }

    return fileNames;
}

QStringList KoSvgTextProperties::fontFeaturesForText(int start, int length) const
{
    using namespace KoSvgText;
    QStringList fontFeatures;
    QVector<PropertyId> list = {
        FontVariantCommonLigId,
        FontVariantDiscretionaryLigId,
        FontVariantHistoricalLigId,
        FontVariantContextualAltId,
        FontVariantHistoricalFormsId,
        FontVariantPositionId,
        FontVariantCapsId,
        FontVariantNumFractId,
        FontVariantNumFigureId,
        FontVariantNumOrdinalId,
        FontVariantNumSpacingId,
        FontVariantNumSlashedZeroId,
        FontVariantEastAsianVarId,
        FontVariantEastAsianWidthId,
        FontVariantRubyId
    };

    for (PropertyId id: list) {
        if (hasProperty(id)) {
            FontVariantFeature feature = FontVariantFeature(property(id).toInt());
            if (feature != FontVariantNormal) {
                QStringList openTypeTags = fontVariantOpentypeTags(feature);
                for (QString tag: openTypeTags) {
                    QString openTypeTag = tag;
                    openTypeTag += QString("[%1:%2]").arg(start).arg(start+length);
                    if (feature == NoCommonLigatures ||
                            feature == NoDiscretionaryLigatures||
                            feature == NoHistoricalLigatures ||
                            feature == NoContextualAlternates) {
                        openTypeTag += "=0";
                    } else {
                        openTypeTag += "=1";
                    }
                    fontFeatures.append(openTypeTag);
                }
            }
        }
    }

    if (!property(KerningId).value<AutoValue>().isAuto) {
        QString openTypeTag = "kern";
        openTypeTag += QString("[%1:%2]").arg(start).arg(start+length);
        openTypeTag += "=0";
        fontFeatures.append(openTypeTag);
        openTypeTag = "vkrn";
        openTypeTag += QString("[%1:%2]").arg(start).arg(start+length);
        openTypeTag += "=0";
        fontFeatures.append(openTypeTag);
    }

    if (hasProperty(FontFeatureSettingsId)) {
        QStringList features = property(FontFeatureSettingsId).toStringList();
        for (int i=0; i < features.size(); i++) {
            QString feature = features.at(i).trimmed();
            if ((!feature.startsWith('\'') && !feature.startsWith('\"')) || feature.isEmpty()) {
                continue;
            }
            QString openTypeTag = feature.mid(1, 4);
            if (feature.size() == 6) {
                openTypeTag += QString("[%1:%2]=1").arg(start).arg(start+length);
            } else {
                feature = feature.remove(0, 6).trimmed();
                bool ok = false;
                int featureVal = feature.toInt(&ok);
                if (feature == "on") {
                    openTypeTag += QString("[%1:%2]=1").arg(start).arg(start+length);
                } else if (feature == "off") {
                    openTypeTag += QString("[%1:%2]=0").arg(start).arg(start+length);
                } else if (ok) {
                    openTypeTag += QString("[%1:%2]=%3").arg(start).arg(start+length).arg(featureVal);
                } else {
                    continue;
                }
            }
            fontFeatures.append(openTypeTag);
        }
    }

    return fontFeatures;
}

QMap<QString, qreal> KoSvgTextProperties::fontAxisSettings() const
{
    QMap<QString, qreal> settings;
    settings.insert("wght", propertyOrDefault(KoSvgTextProperties::FontWeightId).toInt());
    settings.insert("wdth", propertyOrDefault(KoSvgTextProperties::FontStretchId).toInt());
    settings.insert("opsz", propertyOrDefault(KoSvgTextProperties::FontSizeId).toReal());
    const QFont::Style style =
        QFont::Style(propertyOrDefault(KoSvgTextProperties::FontStyleId).toInt());
    if (style == QFont::StyleItalic) {
        settings.insert("ital", 1);
    } else if (style == QFont::StyleOblique) {
        settings.insert("ital", 1);
        settings.insert("slnt", 11);
    } else {
        settings.insert("ital", 0);
    }

    return settings;
}

QStringList KoSvgTextProperties::supportedXmlAttributes()
{
    QStringList attributes;
    attributes << "writing-mode" << "glyph-orientation-vertical" << "glyph-orientation-horizontal"
               << "direction" << "unicode-bidi" << "text-anchor"
               << "dominant-baseline" << "alignment-baseline" << "baseline-shift"
               << "kerning" << "letter-spacing" << "word-spacing" << "xml:lang";
    return attributes;
}

namespace {
Q_GLOBAL_STATIC(KoSvgTextProperties, s_defaultProperties)
}

const KoSvgTextProperties &KoSvgTextProperties::defaultProperties()
{
    if (!s_defaultProperties.exists()) {
        using namespace KoSvgText;

        s_defaultProperties->setProperty(WritingModeId, HorizontalTB);
        s_defaultProperties->setProperty(DirectionId, DirectionLeftToRight);
        s_defaultProperties->setProperty(UnicodeBidiId, BidiNormal);
        s_defaultProperties->setProperty(TextAnchorId, AnchorStart);
        s_defaultProperties->setProperty(DominantBaselineId, BaselineAuto);
        s_defaultProperties->setProperty(AlignmentBaselineId, BaselineAuto);
        s_defaultProperties->setProperty(BaselineShiftModeId, ShiftNone);
        s_defaultProperties->setProperty(BaselineShiftValueId, 0.0);
        s_defaultProperties->setProperty(KerningId, fromAutoValue(AutoValue()));
        s_defaultProperties->setProperty(TextOrientationId, OrientationMixed);
        s_defaultProperties->setProperty(LetterSpacingId, fromAutoValue(AutoValue()));
        s_defaultProperties->setProperty(WordSpacingId, fromAutoValue(AutoValue()));

        QFont font;

        s_defaultProperties->setProperty(FontFamiliesId, font.family());
        s_defaultProperties->setProperty(FontStyleId, font.style());
        s_defaultProperties->setProperty(FontStretchId, 100);
        s_defaultProperties->setProperty(FontWeightId, 400);
        s_defaultProperties->setProperty(FontSizeId, font.pointSizeF());
        s_defaultProperties->setProperty(FontSizeAdjustId, fromAutoValue(AutoValue()));

        {
            using namespace KoSvgText;
            TextDecorations deco = DecorationNone;

            s_defaultProperties->setProperty(TextDecorationLineId, QVariant::fromValue(deco));
            s_defaultProperties->setProperty(TextDecorationPositionHorizontalId, UnderlineAuto);
            s_defaultProperties->setProperty(TextDecorationPositionVerticalId, UnderlineAuto);
            s_defaultProperties->setProperty(TextDecorationColorId, QVariant::fromValue(Qt::transparent));
            s_defaultProperties->setProperty(TextDecorationStyleId, Solid);

            s_defaultProperties->setProperty(TextCollapseId, Collapse);
            s_defaultProperties->setProperty(TextWrapId, Wrap);
            TextSpaceTrims trim = TrimNone;
            s_defaultProperties->setProperty(TextTrimId, QVariant::fromValue(trim));
            s_defaultProperties->setProperty(LineBreakId, LineBreakAuto);
            s_defaultProperties->setProperty(WordBreakId, WordBreakNormal);
            s_defaultProperties->setProperty(TextAlignAllId, AlignStart);
            s_defaultProperties->setProperty(TextAlignLastId, AlignLastAuto);
            s_defaultProperties->setProperty(TextTransformId, TextTransformNone);
            s_defaultProperties->setProperty(LineHeightId, fromAutoValue(AutoValue()));
            s_defaultProperties->setProperty(TabSizeId, 8);
            HangingPunctuations hang = HangNone;
            s_defaultProperties->setProperty(HangingPunctuationId, QVariant::fromValue(hang));
        }
    }
    return *s_defaultProperties;
}

