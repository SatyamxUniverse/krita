/*
 *  SPDX-FileCopyrightText: 2017 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoSvgTextShape.h"

#include <QTextLayout>

#include <raqm.h>
#include <fontconfig/fontconfig.h>
#include FT_COLOR_H

#include <klocalizedstring.h>

#include "KoSvgText.h"
#include "KoSvgTextProperties.h"
#include <KoShapeContainer_p.h>
#include <text/KoSvgTextChunkShape_p.h>
#include <text/KoSvgTextShapeMarkupConverter.h>
#include <text/KoFontRegistery.h>
#include <KoDocumentResourceManager.h>
#include <KoShapeController.h>

#include "kis_debug.h"

#include <KoXmlReader.h>
#include <KoXmlNS.h>
#include <KoShapeLoadingContext.h>
#include <KoIcon.h>
#include <KoProperties.h>
#include <KoColorBackground.h>
#include <KoPathShape.h>
#include <KoClipMaskPainter.h>

#include <SvgLoadingContext.h>
#include <SvgGraphicContext.h>
#include <SvgUtil.h>

#include <QApplication>
#include <QThread>
#include <vector>
#include <memory>
#include <QPainter>
#include <QPainterPath>
#include <QFileInfo>
#include <QtMath>
#include <QLineF>

#include <boost/optional.hpp>

#include <text/KoSvgTextChunkShapeLayoutInterface.h>

#include <FlakeDebug.h>

#include <QSharedData>

struct CharacterResult {
    QPointF finalPosition;
    qreal rotate = 0.0;
    bool hidden = false; // whether the character will be drawn.
    // we can't access characters that aren't part of a typographic character
    // so we're setting 'middle' to true and addressable to 'false'.
    // The original svg specs' notion of addressable character relies on utf16,
    // and it's suggested to have it per-typographic character.
    // https://github.com/w3c/svgwg/issues/537
    bool addressable = false; // whether the character is not discarded for various reasons.
    bool middle = true; // whether the character is the second of last of a typographic character.
    bool anchored_chunk = false; // whether this is the start of a new chunk.

    QPainterPath path;
    QImage image{0};

    QVector<QPainterPath> colorLayers;
    QVector<QBrush> colorLayerColors;
    QVector<bool> replaceWithForeGroundColor;

    QRectF boundingBox;
    int typographic_index = -1;
    QPointF cssPosition = QPointF();
    QPointF advance;
    qreal textLengthApplied = false;

    KoSvgText::TextAnchor anchor = KoSvgText::AnchorStart;
    KoSvgText::Direction direction = KoSvgText::DirectionLeftToRight;
    bool textOnPath = false;

};

class KoSvgTextShape::Private
{
public:

    // NOTE: the cache data is shared between all the instances of
    //       the shape, though it will be reset locally if the
    //       accessing thread changes

    Private()
    {}

    Private(const Private &rhs)
    , textRendering(rhs.textRendering)
    , xRes(rhs.xRes)
    , yRes(rhs.yRes)
    , result(rhs.result)
    {
    }

    TextRendering textRendering = Auto;
    int xRes = 72;
    int yRes = 72;

    QVector<CharacterResult> result;


    void clearAssociatedOutlines(const KoShape *rootShape);
    QPainterPath convertFromFreeTypeOutline(FT_GlyphSlotRec *glyphSlot);
    QImage convertFromFreeTypeBitmap(FT_GlyphSlotRec *glyphSlot);
    void applyTextLength(const KoShape *rootShape,
                         QVector<CharacterResult> &result,
                         int &currentIndex,
                         int &resolvedDescendentNodes,
                         bool isHorizontal);
    void getAnchors(const KoShape *rootShape,
                         QVector<CharacterResult> &result,
                         int &currentIndex);
    void applyAnchoring(QVector<CharacterResult> &result,
                        bool isHorizontal);
    void applyTextPath(const KoShape *rootShape,
                         QVector<CharacterResult> &result, bool isHorizontal);
    void paintPaths(QPainter &painter, KoShapePaintingContext &paintContext,
                    QPainterPath outlineRect,
                    const KoShape *rootShape, QVector<CharacterResult> &result, QPainterPath &chunk, int &currentIndex);
};

KoSvgTextShape::KoSvgTextShape()
    : KoSvgTextChunkShape()
    , d(new Private)
{
    setShapeId(KoSvgTextShape_SHAPEID);
}

KoSvgTextShape::KoSvgTextShape(const KoSvgTextShape &rhs)
    : KoSvgTextChunkShape(rhs)
    , d(new Private(*rhs.d))
{
    setShapeId(KoSvgTextShape_SHAPEID);
}

KoSvgTextShape::~KoSvgTextShape()
{
}

KoShape *KoSvgTextShape::cloneShape() const
{
    return new KoSvgTextShape(*this);
}

void KoSvgTextShape::shapeChanged(ChangeType type, KoShape *shape)
{
    KoSvgTextChunkShape::shapeChanged(type, shape);

    if (type == StrokeChanged || type == BackgroundChanged || type == ContentChanged) {
        relayout();
    }
}

void KoSvgTextShape::paintComponent(QPainter &painter, KoShapePaintingContext &paintContext) const
{

    Q_UNUSED(paintContext);

    if (d->textRendering == OptimizeLegibility) {
        /**
         * HACK ALERT:
         *
         * For hinting and bitmaps, we need to get the hinting metrics from freetype,
         * but those need the DPI. We can't get the DPI normally, however, neither rotate
         * and shear change the length of a line, and it may not be that bad if freetype
         * receives a scaled value for the DPI.
         */
        int xRes = qRound(painter.transform().map(QLineF(QPointF(), QPointF(72, 0))).length());
        int yRes = qRound(painter.transform().map(QLineF(QPointF(), QPointF(0, 72))).length());
        if (xRes != d->xRes || yRes != d->yRes) {
            d->xRes = xRes;
            d->yRes = yRes;
            relayout();
        }
    } else {
        if (72 != d->xRes || 72 != d->yRes) {
            d->xRes = 72;
            d->yRes = 72;
            relayout();
        }
    }
    painter.save();
    if (d->textRendering == OptimizeSpeed) {
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    }

    QPainterPath chunk;
    int currentIndex = 0;
    if (d->result.size()>0) {
        d->paintPaths(painter, paintContext, this->outline(), this, d->result, chunk, currentIndex);
    }
    /* Debug
    Q_FOREACH (KoShape *child, this->shapes()) {
        const KoSvgTextChunkShape *textPathChunk = dynamic_cast<const KoSvgTextChunkShape*>(child);
        if (textPathChunk) {
            painter.save();
            painter.setPen(Qt::magenta);
            painter.setOpacity(0.5);
            if (textPathChunk->layoutInterface()->textPath()) {
                QPainterPath p = textPathChunk->layoutInterface()->textPath()->outline();
                p = textPathChunk->layoutInterface()->textPath()->transformation().map(p);
                painter.strokePath(p, QPen(Qt::green));
                painter.drawPoint(p.pointAtPercent(0));
                painter.drawPoint(p.pointAtPercent(p.percentAtLength(p.length()*0.5)));
                painter.drawPoint(p.pointAtPercent(1.0));
            }
            painter.restore();
        }
    }
    */
    painter.restore();

}

void KoSvgTextShape::paintStroke(QPainter &painter, KoShapePaintingContext &paintContext) const
{
    Q_UNUSED(painter);
    Q_UNUSED(paintContext);

    // do nothing! everything is painted in paintComponent()
}

QPainterPath KoSvgTextShape::textOutline() const
{

    /*QPainterPath result;
    result.setFillRule(Qt::WindingFill);
    qDebug() << "starting creation textoutlne";

    for (int layoutIndex = 0; layoutIndex < (int)d->cachedLayouts.size(); layoutIndex++) {

        for (int j = 0; j < layout->lineCount(); j++) {
            QTextLine line = layout->lineAt(j);

            Q_FOREACH (const QGlyphRun &run, line.glyphRuns()) {
                const QVector<quint32> indexes = run.glyphIndexes();
                const QVector<QPointF> positions = run.positions();
                const QRawFont font = run.rawFont();

                KIS_SAFE_ASSERT_RECOVER(indexes.size() == positions.size()) { continue; }

                for (int k = 0; k < indexes.size(); k++) {
                    QPainterPath glyph = font.pathForGlyph(indexes[k]);
                    glyph.translate(positions[k] + layoutOffset);
                    result += glyph;
                }

                const qreal thickness = font.lineThickness();
                const QRectF runBounds = run.boundingRect();

                if (run.overline()) {
                    // the offset is calculated to be consistent with the way how Qt renders the text
                    const qreal y = line.y();
                    QRectF overlineBlob(runBounds.x(), y, runBounds.width(), thickness);
                    overlineBlob.translate(layoutOffset);

                    QPainterPath path;
                    path.addRect(overlineBlob);

                    // don't use direct addRect, because it doesn't care about Qt::WindingFill
                    result += path;
                }

                if (run.strikeOut()) {
                    // the offset is calculated to be consistent with the way how Qt renders the text
                    const qreal y = line.y() + 0.5 * line.height();
                    QRectF strikeThroughBlob(runBounds.x(), y, runBounds.width(), thickness);
                    strikeThroughBlob.translate(layoutOffset);

                    QPainterPath path;
                    path.addRect(strikeThroughBlob);

                    // don't use direct addRect, because it doesn't care about Qt::WindingFill
                    result += path;
                }

                if (run.underline()) {
                    const qreal y = line.y() + line.ascent() + font.underlinePosition();
                    QRectF underlineBlob(runBounds.x(), y, runBounds.width(), thickness);
                    underlineBlob.translate(layoutOffset);

                    QPainterPath path;
                    path.addRect(underlineBlob);

                    // don't use direct addRect, because it doesn't care about Qt::WindingFill
                    result += path;
                }
            }
        }
    }*/

    return QPainterPath();
}

void KoSvgTextShape::setTextRenderingFromString(QString textRendering)
{
    if (textRendering == "optimizeSpeed") {
        d->textRendering = OptimizeSpeed;
    } else if (textRendering == "optimizeLegibility") {
        d->textRendering = OptimizeLegibility;
    } else if (textRendering == "geometricPrecision") {
        d->textRendering = GeometricPrecision;
    } else{
        d->textRendering = Auto;
    }
}

QString KoSvgTextShape::textRenderingString() const
{
    if (d->textRendering == OptimizeSpeed) {
        return "optimizeSpeed";
    } else if (d->textRendering == OptimizeLegibility) {
        return "optimizeLegibility";
    } else if (d->textRendering == GeometricPrecision) {
        return "geometricPrecision";
    } else {
        return "auto";
    }
}

void KoSvgTextShape::resetTextShape()
{
    KoSvgTextChunkShape::resetTextShape();
    relayout();
}

void KoSvgTextShape::relayout() const
{
    d->clearAssociatedOutlines(this);

    // The following is based on the text-layout algorithm in SVG 2.
    KoSvgText::WritingMode writingMode = KoSvgText::WritingMode(
                this->textProperties().propertyOrDefault(KoSvgTextProperties::WritingModeId).toInt());
    KoSvgText::Direction direction = KoSvgText::Direction(
                this->textProperties().propertyOrDefault(KoSvgTextProperties::DirectionId).toInt());

    bool isHorizontal = true;
    if (writingMode == KoSvgText::TopToBottom) {
        isHorizontal = false;
    }
    FT_Int32 loadFlags = FT_LOAD_DEFAULT;
    if (d->textRendering == GeometricPrecision && d->textRendering == Auto) {
        // without load_no_hinting, the advance and offset will be rounded
        // to nearest pixel, which we don't want as we're using the vector outline.
        loadFlags |= FT_LOAD_NO_HINTING;
        loadFlags |= FT_LOAD_NO_BITMAP;
    } else {
        loadFlags |= FT_LOAD_RENDER;
    }

    // Whenever the freetype docs talk about a 26.6 floating point unit, they mean a 1/64 value.
    const qreal ftFontUnit = 64.0;
    const qreal ftFontUnitFactor = 1/ftFontUnit;
    QTransform ftTF = QTransform::fromScale(ftFontUnitFactor, -ftFontUnitFactor);
    qreal finalRes = qMin(d->xRes, d->yRes);
    qreal scaleToPT = float(72./finalRes);
    qreal scaleToPixel = float(finalRes/72.);
    QTransform dpiScale = QTransform::fromScale(scaleToPT, scaleToPT);
    ftTF *= dpiScale;

    // First, get text. We use the subChunks because that handles bidi for us.
    // SVG 1.1 suggests that each time the xy position of a piece of text changes,
    // that this should be seperately shaped, but according to SVGWG issues 631 and 635
    // noone who actually uses bidi likes this, and it also complicates the algorithm,
    // so we're not doing that. Anchored Chunks will get generated later.
    // https://github.com/w3c/svgwg/issues/631
    // https://github.com/w3c/svgwg/issues/635

    QVector<KoSvgTextChunkShapeLayoutInterface::SubChunk> textChunks = layoutInterface()->collectSubChunks();
    QString text;
    for (KoSvgTextChunkShapeLayoutInterface::SubChunk chunk : textChunks) {
        text.append(chunk.text);
    }

    // Then, pass everything to a css-compatible text-layout algortihm.
    raqm_t *layout(raqm_create());

    if (raqm_set_text_utf16(layout, text.utf16(), text.size())) {
        if (writingMode == KoSvgText::TopToBottom) {
            raqm_set_par_direction(layout, raqm_direction_t::RAQM_DIRECTION_TTB);
        } else if (direction == KoSvgText::DirectionRightToLeft) {
            raqm_set_par_direction(layout, raqm_direction_t::RAQM_DIRECTION_RTL);
        } else {
            raqm_set_par_direction(layout, raqm_direction_t::RAQM_DIRECTION_LTR);
        }

        int start = 0;
        int length = 0;
        FT_Face face = NULL;
        for (KoSvgTextChunkShapeLayoutInterface::SubChunk chunk : textChunks) {
            length = chunk.text.size();
            QVector<int> lengths;
            KoSvgTextProperties properties = chunk.format.associatedShapeWrapper().shape()->textProperties();
            QStringList fontFeatures = properties.fontFeaturesForText(start, length);

            qreal fontSize = properties.property(KoSvgTextProperties::FontSizeId).toReal();
            const QFont::Style style =
                QFont::Style(properties.propertyOrDefault(KoSvgTextProperties::FontStyleId).toInt());
            QVector<FT_Face> faces = KoFontRegistery::instance()->facesForCSSValues(properties.property(KoSvgTextProperties::FontFamiliesId).toStringList(),
                                                                                    lengths,
                                                                                    chunk.text,
                                                                                    fontSize,
                                                                                    properties.propertyOrDefault(KoSvgTextProperties::FontWeightId).toInt(),
                                                                                    properties.propertyOrDefault(KoSvgTextProperties::FontStretchId).toInt(),
                                                                                    style != QFont::StyleNormal);
            KoFontRegistery::instance()->configureFaces(faces, fontSize, finalRes, finalRes, properties.fontAxisSettings());
            if (properties.hasProperty(KoSvgTextProperties::TextLanguage)) {
                raqm_set_language(layout,
                                  properties.property(KoSvgTextProperties::TextLanguage).toString().toUtf8(),
                                  start, length);
            }
            for (QString feature: fontFeatures) {
                qDebug() << "adding feature" << feature;
                raqm_add_font_feature(layout, feature.toUtf8(), feature.toUtf8().size());
            }
            KoSvgText::AutoValue letterSpacing = properties.propertyOrDefault(KoSvgTextProperties::LetterSpacingId).value<KoSvgText::AutoValue>();
            if (!letterSpacing.isAuto) {
                raqm_set_letter_spacing_range(layout, letterSpacing.customValue * ftFontUnit * scaleToPixel, false, start, length);
            }
            KoSvgText::AutoValue wordSpacing = properties.propertyOrDefault(KoSvgTextProperties::WordSpacingId).value<KoSvgText::AutoValue>();
            if (!wordSpacing.isAuto) {
                raqm_set_word_spacing_range(layout, wordSpacing.customValue * ftFontUnit * scaleToPixel, false, start, length);
            }

            QVector<int> familyValues(text.size());
            familyValues.fill(-1);

            for (int i = 0; i < lengths.size(); i++ )  {
                length = lengths.at(i);
                FT_Int32 faceLoadFlags = loadFlags;
                FT_Face face = faces.at(i);
                if (FT_HAS_COLOR(face)) {
                    loadFlags |= FT_LOAD_COLOR;
                }
                if (FT_HAS_VERTICAL(face)) {
                    loadFlags |= FT_LOAD_VERTICAL_LAYOUT;
                }
                if (start == 0) {
                    raqm_set_freetype_face(layout, face);
                    raqm_set_freetype_load_flags(layout, faceLoadFlags);
                }
                if (length > 0) {
                    raqm_set_freetype_face_range(layout, face, start, length);
                    raqm_set_freetype_load_flags_range(layout, faceLoadFlags, start, length);
                }
                start += length;
            }

        }
        FT_Done_Face(face);
        qDebug() << "text-length:" << text.size();
    }

    if (raqm_layout(layout)) {
        qDebug() << "layout succeeded";
    }

    // 1. Setup.
    qDebug() << "1. Setup";

    QVector<CharacterResult> result(text.size());

    // 2. Set flags and assign initial positions
    // We also retreive a path here.
    qDebug() << "2. Set flags and assign initial positions";
    size_t count;
    raqm_glyph_t *glyphs = raqm_get_glyphs (layout, &count);
    if (!glyphs) {
        return;
    }
    QVector<int> addressableIndices;

    QPointF totalAdvanceFTFontCoordinates;

    for (int g=0; g < int(count); g++) {
        FT_Int32 faceLoadFlags = loadFlags;
        if (!isHorizontal && FT_HAS_VERTICAL(glyphs[g].ftface)) {
            faceLoadFlags |= FT_LOAD_VERTICAL_LAYOUT;
        }
        if (FT_HAS_COLOR(glyphs[g].ftface)) {
            faceLoadFlags |= FT_LOAD_COLOR;
        }
        int error = FT_Load_Glyph(glyphs[g].ftface, glyphs[g].index, faceLoadFlags);
        if (error != 0) {
            continue;
        }

        //qDebug() << "glyph" << g << "cluster" << glyphs[g].cluster << glyphs[g].index;

        QPainterPath glyph = d->convertFromFreeTypeOutline(glyphs[g].ftface->glyph);

        glyph.translate(glyphs[g].x_offset, glyphs[g].y_offset);
        glyph = ftTF.map(glyph);

        CharacterResult charResult = result[glyphs[g].cluster];

        if (!charResult.path.isEmpty()) {
            // this is for glyph clusters, unicode combining marks are always added.
            // we could have these as seperate paths, but there's no real purpose,
            // and the svg standard prefers 'ligatures' to be treated as a single glyph.
            // It simplifies things for us in any case.
            charResult.path.addPath(glyph.translated(charResult.advance));
        } else {
            charResult.path = glyph;
        }
        // TODO: Handle glyph clusters better...
        charResult.image = d->convertFromFreeTypeBitmap(glyphs[g].ftface->glyph);

        if (glyph.isEmpty()) {
            bool usePixmap = !charResult.image.isNull();

            if (usePixmap) {
                QPointF topLeft(glyphs[g].ftface->glyph->bitmap_left*64,
                                (glyphs[g].ftface->glyph->bitmap_top - charResult.image.size().height())*64);
                charResult.boundingBox = QRectF(topLeft, charResult.image.size()*64);
            } else if (isHorizontal) {
                charResult.boundingBox = QRectF(0,
                                                glyphs[g].ftface->size->metrics.descender,
                                                glyphs[g].x_advance,
                                                (glyphs[g].ftface->size->metrics.ascender
                                                 - glyphs[g].ftface->size->metrics.descender));
            } else {
                charResult.boundingBox = QRectF(-(glyphs[g].ftface->size->metrics.height *0.5),
                                                0,
                                                glyphs[g].ftface->size->metrics.height,
                                                glyphs[g].y_advance);
            }
            charResult.boundingBox = ftTF.mapRect(charResult.boundingBox);
        } else {
            charResult.boundingBox = charResult.path.boundingRect();
        }

        // Retreive CPAL/COLR V0 color layers, directly based off the sample code in the freetype docs.
        FT_UInt layerGlyphIndex = 0;
        FT_UInt layerColorIndex = 0;
        FT_LayerIterator  iterator;
        FT_Color*         palette;
        int paletteIndex = 0;
        error = FT_Palette_Select( glyphs[g].ftface, paletteIndex, &palette );
        if ( error ) {
            palette = NULL;
        }
        iterator.p = NULL;
        bool haveLayers = FT_Get_Color_Glyph_Layer( glyphs[g].ftface,
                                                  glyphs[g].index,
                                                  &layerGlyphIndex,
                                                  &layerColorIndex,
                                                  &iterator );
        if (haveLayers && palette) {
            do {
                QBrush layerColor;
                bool isForeGroundColor = false;

                if ( layerColorIndex == 0xFFFF ) {
                    layerColor = Qt::black;
                    isForeGroundColor = true;
                } else {
                    FT_Color color = palette[layerColorIndex];
                    layerColor = QColor(color.red, color.green, color.blue, color.alpha);
                }
                FT_Load_Glyph(glyphs[g].ftface, layerGlyphIndex, faceLoadFlags);
                QPainterPath p = d->convertFromFreeTypeOutline(glyphs[g].ftface->glyph);
                p.translate(glyphs[g].x_offset, glyphs[g].y_offset);
                charResult.colorLayers.append(ftTF.map(p));
                charResult.colorLayerColors.append(layerColor);
                charResult.replaceWithForeGroundColor.append(isForeGroundColor);

            } while (FT_Get_Color_Glyph_Layer( glyphs[g].ftface,
                                               glyphs[g].index,
                                               &layerGlyphIndex,
                                               &layerColorIndex,
                                               &iterator ));
        }


        charResult.typographic_index = g;
        charResult.addressable = true;
        addressableIndices.append(glyphs[g].cluster);
        // if character in middle of line, this doesn't mean much rght now,
        // because we don't do linebreaking yet, but once we do, we should set these
        // appropriately.
        if (glyphs[g].cluster == 0) {
            charResult.anchored_chunk = true;
        }
        charResult.middle = false;
        QPointF advance(glyphs[g].x_advance, glyphs[g].y_advance);
        charResult.advance += ftTF.map(advance);
        totalAdvanceFTFontCoordinates += advance;
        charResult.cssPosition = ftTF.map(totalAdvanceFTFontCoordinates) - charResult.advance;

        result[glyphs[g].cluster] = charResult;
    }
    // we're done with raqm for now.
    raqm_destroy(layout);

    // This is the best point to start applying linebreaking and text-wrapping.
    // If we're doing text-wrapping we should skip the other positioning steps of the algorithm.

    // 3. Resolve character positioning
    qDebug() << "3. Resolve character positioning";
    QVector<KoSvgText::CharTransformation> resolvedTransforms(text.size());
    bool textInPath = false;
    int globalIndex = 0;
    this->layoutInterface()->resolveCharacterPositioning(addressableIndices,
                                                         resolvedTransforms,
                                                         textInPath,
                                                         globalIndex,
                                                         isHorizontal);
    // 4. Adjust positions: dx, dy
    qDebug() << "4. Adjust positions: dx, dy";

    QPointF shift = QPointF();
    for (int i = 0; i < result.size(); i++) {
        if (addressableIndices.contains(i)) {
            KoSvgText::CharTransformation transform = resolvedTransforms[i];
            if (transform.hasRelativeOffset()) {
                shift += transform.relativeOffset();
            }
            CharacterResult charResult = result[i];
            if (transform.rotate) {
                charResult.rotate = *transform.rotate;
            }
            charResult.finalPosition = charResult.cssPosition + shift;
            if (transform.startsNewChunk()) {
                charResult.anchored_chunk = true;
            }
            result[i] = charResult;
        }
    }

    // 5. Apply ‘textLength’ attribute
    qDebug() << "5. Apply ‘textLength’ attribute";
    globalIndex = 0;
    int resolved = 0;
    d->applyTextLength(this, result, globalIndex, resolved, isHorizontal);

    // 6. Adjust positions: x, y
    qDebug() << "6. Adjust positions: x, y";

    // https://github.com/w3c/svgwg/issues/617
    shift = QPointF();
    //bool setNextAnchor = false;
    for (int i = 0; i < result.size(); i++) {
        if (addressableIndices.contains(i)) {
            KoSvgText::CharTransformation transform = resolvedTransforms[i];
            CharacterResult charResult = result[i];
            if (transform.xPos) {
                qreal d = transform.dxPos? *transform.dxPos : 0.0;
                shift.setX(*transform.xPos + (d - charResult.finalPosition.x()));
            }
            if (transform.yPos) {
                qreal d = transform.dyPos? *transform.dyPos : 0.0;
                shift.setY(*transform.yPos + (d - charResult.finalPosition.y()));
            }
            charResult.finalPosition += shift;

            /*
            if (setNextAnchor) {
                charResult.anchored_chunk = true;
            }

            if (charResult.middle && charResult.anchored_chunk) {
                charResult.anchored_chunk = false;
                if (i+1 < result.size()) {
                    setNextAnchor = true;
                } else {
                    setNextAnchor = false;
                }
            }
            */

            result[i] = charResult;
        }
    }

    // 7. Apply anchoring
    qDebug() << "7. Apply anchoring";
    globalIndex = 0;
    d->getAnchors(this, result, globalIndex);

    d->applyAnchoring(result, isHorizontal);

    // 8. Position on path
    qDebug() << "8. Position on path";

    d->applyTextPath(this, result, isHorizontal);

    // 9. return result.
    d->result = result;
    globalIndex = 0;
    QTransform tf;
    for (KoSvgTextChunkShapeLayoutInterface::SubChunk chunk : textChunks) {
        KoSvgText::AssociatedShapeWrapper wrapper = chunk.format.associatedShapeWrapper();
        int j = chunk.text.size();
        for (int i = globalIndex; i< globalIndex + j; i++) {
            if (result.at(i).addressable && result.at(i).hidden == false) {
                tf.reset();
                tf.translate(result.at(i).finalPosition.x(), result.at(i).finalPosition.y());
                tf.rotateRadians(result.at(i).rotate);
                wrapper.addCharacterRect(tf.mapRect(result.at(i).boundingBox));
            }
        }
        globalIndex += j;
    }
}

void KoSvgTextShape::Private::clearAssociatedOutlines(const KoShape *rootShape)
{
    const KoSvgTextChunkShape *chunkShape = dynamic_cast<const KoSvgTextChunkShape*>(rootShape);
    KIS_SAFE_ASSERT_RECOVER_RETURN(chunkShape);

    chunkShape->layoutInterface()->clearAssociatedOutline();

    Q_FOREACH (KoShape *child, chunkShape->shapes()) {
        clearAssociatedOutlines(child);
    }
}

QPainterPath KoSvgTextShape::Private::convertFromFreeTypeOutline(FT_GlyphSlotRec *glyphSlot) {
    QPointF cp = QPointF();
    // convert the outline to a painter path
    // This is taken from qfontengine_ft.cpp.
    QPainterPath glyph;
    glyph.setFillRule(Qt::WindingFill);
    int i = 0;
    for (int j = 0; j < glyphSlot->outline.n_contours; ++j) {
        int last_point = glyphSlot->outline.contours[j];
        // qDebug() << "contour:" << i << "to" << last_point;
        QPointF start = QPointF(glyphSlot->outline.points[i].x, glyphSlot->outline.points[i].y);
        if (!(glyphSlot->outline.tags[i] & 1)) {               // start point is not on curve:
            if (!(glyphSlot->outline.tags[last_point] & 1)) {  // end point is not on curve:
                //qDebug() << "  start and end point are not on curve";
                start = (QPointF(glyphSlot->outline.points[last_point].x,
                                 glyphSlot->outline.points[last_point].y) + start) / 2.0;
            } else {
                //qDebug() << "  end point is on curve, start is not";
                start = QPointF(glyphSlot->outline.points[last_point].x,
                                glyphSlot->outline.points[last_point].y);
            }
            --i;   // to use original start point as control point below
        }
        start += cp;
        //qDebug() << "  start at" << start;
        glyph.moveTo(start);
        QPointF c[4];
        c[0] = start;
        int n = 1;
        while (i < last_point) {
            ++i;
            c[n] = cp + QPointF(glyphSlot->outline.points[i].x, glyphSlot->outline.points[i].y);
            //qDebug() << "    " << i << c[n] << "tag =" << (int)g->outline.tags[i]
            //                   << ": on curve =" << (bool)(g->outline.tags[i] & 1);
            ++n;
            switch (glyphSlot->outline.tags[i] & 3) {
            case 2:
                // cubic bezier element
                if (n < 4)
                    continue;
                c[3] = (c[3] + c[2])/2;
                --i;
                break;
            case 0:
                // quadratic bezier element
                if (n < 3)
                    continue;
                c[3] = (c[1] + c[2])/2;
                c[2] = (2*c[1] + c[3])/3;
                c[1] = (2*c[1] + c[0])/3;
                --i;
                break;
            case 1:
            case 3:
                if (n == 2) {
                    //qDebug() << "  lineTo" << c[1];
                    glyph.lineTo(c[1]);
                    c[0] = c[1];
                    n = 1;
                    continue;
                } else if (n == 3) {
                    c[3] = c[2];
                    c[2] = (2*c[1] + c[3])/3;
                    c[1] = (2*c[1] + c[0])/3;
                }
                break;
            }
            //qDebug() << "  cubicTo" << c[1] << c[2] << c[3];
            glyph.cubicTo(c[1], c[2], c[3]);
            c[0] = c[3];
            n = 1;
        }
        if (n == 1) {
            //qDebug() << "  closeSubpath";
            glyph.closeSubpath();
        } else {
            c[3] = start;
            if (n == 2) {
                c[2] = (2*c[1] + c[3])/3;
                c[1] = (2*c[1] + c[0])/3;
            }
            //qDebug() << "  close cubicTo" << c[1] << c[2] << c[3];
            glyph.cubicTo(c[1], c[2], c[3]);
        }
        ++i;
    }
    return glyph;
}

QImage KoSvgTextShape::Private::convertFromFreeTypeBitmap(FT_GlyphSlotRec *glyphSlot)
{
   QImage img;
   QSize size(glyphSlot->bitmap.width, glyphSlot->bitmap.rows);

   if (glyphSlot->bitmap.pixel_mode == FT_PIXEL_MODE_MONO) {
       img = QImage(size, QImage::Format_Mono);
       uchar *src = glyphSlot->bitmap.buffer;
       for (uint y = 0; y < glyphSlot->bitmap.rows; y++) {
           memcpy(img.scanLine(y), src, glyphSlot->bitmap.pitch);
           src += glyphSlot->bitmap.pitch;
       }
   } else if (glyphSlot->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY) {
       img = QImage(size, QImage::Format_Grayscale8);
       uchar *src = glyphSlot->bitmap.buffer;
       for (uint y = 0; y < glyphSlot->bitmap.rows; y++) {
           memcpy(img.scanLine(y), src, glyphSlot->bitmap.pitch);
           src += glyphSlot->bitmap.pitch;
       }
   } else if (glyphSlot->bitmap.pixel_mode == FT_PIXEL_MODE_BGRA) {
       img = QImage(size, QImage::Format_ARGB32_Premultiplied);
       uchar *src = glyphSlot->bitmap.buffer;
       for (uint y = 0; y < glyphSlot->bitmap.rows; y++) {
           if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
               // TODO: actually test this... We need to go from BGRA to ARGB.
               for (int x = 0; x < static_cast<int>(size.width()); x++) {
                   int p = x*4;
                   img.scanLine(y)[p]   = src[p+3];
                   img.scanLine(y)[p+1] = src[p+2];
                   img.scanLine(y)[p+2] = src[p+1];
                   img.scanLine(y)[p+3] = src[p];
               }
           } else {
               memcpy(img.scanLine(y), src, glyphSlot->bitmap.pitch);
           }
           src += glyphSlot->bitmap.pitch;
       }
   }

   return img;
}

void KoSvgTextShape::Private::applyTextLength(const KoShape *rootShape,
                                              QVector<CharacterResult> &result,
                                              int &currentIndex,
                                              int &resolvedDescendentNodes,
                                              bool isHorizontal)
{
    const KoSvgTextChunkShape *chunkShape = dynamic_cast<const KoSvgTextChunkShape*>(rootShape);
    KIS_SAFE_ASSERT_RECOVER_RETURN(chunkShape);

    int i = currentIndex;
    int j = i + chunkShape->layoutInterface()->numChars();
    int resolvedChildren = 0;

    Q_FOREACH (KoShape *child, chunkShape->shapes()) {
        applyTextLength(child, result, currentIndex, resolvedChildren, isHorizontal);
    }
    // Raqm handles bidi reordering for us, but this algorithm does not anticipate
    // that, so we need to keep track of which typographic item belongs where.
    QMap<int, int> typographicToIndex;
    if (!chunkShape->layoutInterface()->textLength().isAuto) {
        qreal a = 0.0;
        qreal b = 0.0;
        int n = 0;
        for (int k = i; k < j; k++) {
            if (result.at(k).addressable) {
                if (result.at(k).typographic_index > -1) {
                    typographicToIndex.insert(result.at(k).typographic_index, k);
                }
                // if character is linebreak, return;

                qreal pos = result.at(k).finalPosition.x();
                qreal advance = result.at(k).advance.x();
                if (!isHorizontal) {
                    pos = result.at(k).finalPosition.y();
                    advance = result.at(k).advance.y();
                }
                if (k==i) {
                    a = qMin(pos, pos+advance);
                    b = qMax(pos, pos+advance);
                } else {
                    a = qMin(a, qMin(pos, pos+advance));
                    b = qMax(b, qMax(pos, pos+advance));
                }
                if (!result.at(k).textLengthApplied) {
                    n +=1;
                }
            }
        }
        n += resolvedChildren;
        n -= 1;
        qreal delta = chunkShape->layoutInterface()->textLength().customValue - (b-a);
        QPointF d (delta/n, 0);
        if (!isHorizontal) {
            d = QPointF(0, delta/n);
        }
        QPointF shift;
        for (int k : typographicToIndex.keys()) {
            CharacterResult cr = result[typographicToIndex.value(k)];
            if (cr.addressable) {
                cr.finalPosition += shift;
                if (!cr.textLengthApplied) {
                    shift += d;
                }
                cr.textLengthApplied = true;
            }
            result[typographicToIndex.value(k)] = cr;
        }
        resolvedDescendentNodes += 1;

        // apply the shift to all consequetive chars as long as they don't start a new chunk.
        for (int k = j; k < result.size(); k++) {
            if (result.at(k).anchored_chunk) {
                break;
            }
            CharacterResult cr = result[k];
            cr.finalPosition += shift;
            result[k] = cr;
        }
    }

    currentIndex = j;
}

void KoSvgTextShape::Private::getAnchors(const KoShape *rootShape,
                         QVector<CharacterResult> &result,
                         int &currentIndex)
{
    const KoSvgTextChunkShape *chunkShape = dynamic_cast<const KoSvgTextChunkShape*>(rootShape);
    KIS_SAFE_ASSERT_RECOVER_RETURN(chunkShape);

    if (chunkShape->isTextNode()) {
        int length = chunkShape->layoutInterface()->numChars();
        for (int i = 0; i < length; i++) {
            CharacterResult cr = result[currentIndex + i];
            cr.anchor = KoSvgText::TextAnchor(
                chunkShape->textProperties().propertyOrDefault(KoSvgTextProperties::TextAnchorId).toInt());
            cr.direction = KoSvgText::Direction(
                chunkShape->textProperties().propertyOrDefault(KoSvgTextProperties::DirectionId).toInt());
            if (chunkShape->layoutInterface()->textPath() && i == 0) {
                cr.anchored_chunk = true;
            }
            result[currentIndex + i] = cr;
        }
        currentIndex += length;
    } else {
        Q_FOREACH (KoShape *child, chunkShape->shapes()) {
            getAnchors(child, result, currentIndex);
        }
    }
}

void KoSvgTextShape::Private::applyAnchoring(QVector<CharacterResult> &result, bool isHorizontal)
{
    QMap<int, int> typographicToIndex;
    int i = 0;
    int start = 0;
    while (start < result.size()) {
        int lowestTypographicalIndex = result.size();
        qreal a = 0;
        qreal b = 0;
        for (i = start; i < result.size(); i++) {
            if (result.at(i).anchored_chunk && i > start) {
                break;
            }
            if (result.at(i).typographic_index > -1) {
                typographicToIndex.insert(result.at(i).typographic_index, i);
                lowestTypographicalIndex = qMin(lowestTypographicalIndex, result.at(i).typographic_index);
            }
            qreal pos = result.at(i).finalPosition.x();
            qreal advance = result.at(i).advance.x();
            if (!isHorizontal) {
                pos = result.at(i).finalPosition.y();
                advance = result.at(i).advance.y();
            }
            if (result.at(i).anchored_chunk) {
                a = qMin(pos, pos+advance);
                b = qMax(pos, pos+advance);
            } else {
                a = qMin(a, qMin(pos, pos+advance));
                b = qMax(b, qMax(pos, pos+advance));
            }
        }
        qreal shift = 0;
        int typo = typographicToIndex.value(lowestTypographicalIndex);
        if (isHorizontal) {
            shift = result.at(typo).finalPosition.x();
        }else  {
            shift = result.at(typo).finalPosition.y();
        }

        bool rtl = result.at(start).direction == KoSvgText::DirectionRightToLeft;
        if ((result.at(start).anchor == KoSvgText::AnchorStart && !rtl)
         || (result.at(start).anchor == KoSvgText::AnchorEnd    && rtl)) {

            shift -= a;

        } else if ((result.at(start).anchor == KoSvgText::AnchorEnd  && !rtl)
                || (result.at(start).anchor == KoSvgText::AnchorStart && rtl)) {

            shift -= b;

        } else {
            shift -= ((a + b) * 0.5);

        }
        QPointF shiftP(shift, 0);
        if (!isHorizontal) {
            shiftP = QPointF(0, shift);
        }

        for (int j = start; j < i; j++) {
            CharacterResult cr = result[j];
            cr.finalPosition += shiftP;
            result[j] = cr;
        }
        start = i;
    }
}

void KoSvgTextShape::Private::applyTextPath(const KoShape *rootShape,
                                            QVector<CharacterResult> &result,
                                            bool isHorizontal)
{
    // Unlike all the other applying functions, this one only iterrates over the top-level.
    // SVG is not designed to have nested textPaths.
    // Source: https://github.com/w3c/svgwg/issues/580
    const KoSvgTextChunkShape *chunkShape = dynamic_cast<const KoSvgTextChunkShape*>(rootShape);
    KIS_SAFE_ASSERT_RECOVER_RETURN(chunkShape);
    bool inPath = false;
    bool afterPath = false;
    int currentIndex = 0;
    QPointF pathEnd;
    Q_FOREACH (KoShape *child, chunkShape->shapes()) {
        const KoSvgTextChunkShape *textPathChunk = dynamic_cast<const KoSvgTextChunkShape*>(child);
        KIS_SAFE_ASSERT_RECOVER_RETURN(textPathChunk);
        int endIndex = currentIndex + textPathChunk->layoutInterface()->numChars();

        KoPathShape *shape = dynamic_cast<KoPathShape*>(textPathChunk->layoutInterface()->textPath());
        if (shape) {
            QPainterPath path = shape->outline();
            path = shape->transformation().map(path);
            inPath = true;
            if(textPathChunk->layoutInterface()->textOnPathInfo().side == KoSvgText::TextPathSideRight) {
                path = path.toReversed();
            }
            qreal length = path.length();
            qreal offset = 0.0;
            bool isClosed = (shape->isClosedSubpath(0) && shape->subpathCount() == 1);
            if (textPathChunk->layoutInterface()->textOnPathInfo().startOffsetIsPercentage) {
                offset = length * (0.01 * textPathChunk->layoutInterface()->textOnPathInfo().startOffset);
            } else {
                offset = textPathChunk->layoutInterface()->textOnPathInfo().startOffset;
            }

            for (int i = currentIndex; i < endIndex; i++) {
                qreal startpointOnThePath = /*-a +*/ offset;
                CharacterResult cr = result[i];
                bool rtl = (cr.direction == KoSvgText::DirectionRightToLeft);

                if (cr.middle == false) {
                    qreal mid = cr.finalPosition.x() + (cr.advance.x() * 0.5) + startpointOnThePath;
                    if (!isHorizontal) {
                        mid = cr.finalPosition.y() + (cr.advance.y() * 0.5) + startpointOnThePath;
                    }
                    if (isClosed) {

                        if ((cr.anchor == KoSvgText::AnchorStart && !rtl)
                         || (cr.anchor == KoSvgText::AnchorEnd    && rtl)) {
                            if (mid - offset < 0 || mid - offset > length) {
                                cr.hidden = true;
                            }
                        } else if ((cr.anchor == KoSvgText::AnchorEnd  && !rtl)
                                || (cr.anchor == KoSvgText::AnchorStart && rtl)) {
                            if (mid - offset < -length || mid - offset > 0) {
                                cr.hidden = true;
                            }
                        } else {
                            if (mid - offset < -(length*0.5) || mid - offset > (length*0.5)) {
                                cr.hidden = true;
                            }
                        }
                        if (mid < 0) { mid += length;}
                        mid = fmod(mid, length);
                    } else {
                        if(mid < 0 || mid > length){
                            cr.hidden = true;
                        }
                    }
                    if (!cr.hidden) {
                        qreal percent = path.percentAtLength(mid);
                        QPointF pos = path.pointAtPercent(percent);
                        qreal tAngle = path.angleAtPercent(percent);
                        if (tAngle>180) {
                            
                            tAngle = 0 - (360 - tAngle);
                        }
                        QPointF vectorT(qCos(qDegreesToRadians(tAngle)), -qSin(qDegreesToRadians(tAngle)));
                        if (isHorizontal) {
                            cr.rotate -= qDegreesToRadians(tAngle);
                            QPointF vectorN(-vectorT.y(), vectorT.x());
                            qreal o = (cr.advance.x()*0.5);
                            cr.finalPosition = pos - (o*vectorT) + (cr.finalPosition.y()*vectorN);
                        } else {
                            cr.rotate -= qDegreesToRadians(tAngle+90);
                            QPointF vectorN(vectorT.y(), -vectorT.x());
                            qreal o = (cr.advance.y()*0.5);
                            cr.finalPosition = pos - (o*vectorT) + (cr.finalPosition.x()*vectorN);
                        }
                    }
                }
                result[i] = cr;
            }
            pathEnd = path.pointAtPercent(1.0);
        } else {
            if (inPath) {
                inPath = false;
                afterPath = true;
                pathEnd -= result.at(currentIndex).finalPosition;
            }
            if (afterPath) {
                for (int i = currentIndex; i < endIndex; i++) {
                    CharacterResult cr = result[i];
                    if (cr.anchored_chunk) {
                        afterPath = false;
                    } else {
                        cr.finalPosition += pathEnd;
                        result[i] = cr;
                    }
                }
            }
        }
        currentIndex = endIndex;
    }
}

void KoSvgTextShape::Private::paintPaths(QPainter &painter, KoShapePaintingContext &paintContext,
                                         QPainterPath outlineRect, const KoShape *rootShape,
                                         QVector<CharacterResult> &result, QPainterPath &chunk,
                                         int &currentIndex)
{
    const KoSvgTextChunkShape *chunkShape = dynamic_cast<const KoSvgTextChunkShape*>(rootShape);
    KIS_SAFE_ASSERT_RECOVER_RETURN(chunkShape);
    if (chunkShape->isTextNode()) {
        QTransform tf;
        int j = currentIndex + chunkShape->layoutInterface()->numChars();
        KoClipMaskPainter fillPainter(&painter,
                                      painter.transform().mapRect(outlineRect.boundingRect()));
        if (chunkShape->background()) {
            if (dynamic_cast<KoColorBackground*>(chunkShape->background().data())) {
                chunkShape->background()->paint(*fillPainter.shapePainter(), paintContext, chunkShape->outline());
            } else {
                chunkShape->background()->paint(*fillPainter.shapePainter(), paintContext, outlineRect);
            }
            fillPainter.maskPainter()->fillPath(outlineRect, Qt::black);
            if (textRendering != OptimizeSpeed) {
                fillPainter.maskPainter()->setRenderHint(QPainter::Antialiasing, true);
                fillPainter.maskPainter()->setRenderHint(QPainter::SmoothPixmapTransform, true);
            } else {
                fillPainter.maskPainter()->setRenderHint(QPainter::Antialiasing, false);
                fillPainter.maskPainter()->setRenderHint(QPainter::SmoothPixmapTransform, false);
            }
        }

        for (int i = currentIndex; i< j; i++) {
            if (result.at(i).addressable && result.at(i).hidden == false) {
                tf.reset();
                tf.translate(result.at(i).finalPosition.x(), result.at(i).finalPosition.y());
                tf.rotateRadians(result.at(i).rotate);
                /* Debug
                painter.save();
                painter.setBrush(Qt::transparent);
                QPen pen(Qt::cyan);
                pen.setWidthF(0.1);
                painter.setPen(pen);
                painter.drawPolygon(tf.map(result.at(i).boundingBox));
                painter.setPen(Qt::red);
                painter.drawPoint(result.at(i).finalPosition);
                painter.restore();
                */
                /**
                 * There's an annoying problem here that officially speaking
                 * the chunks need to be unified into one single path before
                 * drawing, so there's no weirdness with the stroke, but
                 * QPainterPath's union function will frequently lead to
                 * reduced quality of the paths because of 'numerical
                 * instability'.
                 */
                QPainterPath p = tf.map(result.at(i).path);
                //if (chunk.intersects(p)) {
                //    chunk |= tf.map(result.at(i).path);
                //} else {
                if (result.at(i).colorLayers.size()) {
                    for (int c = 0; c < result.at(i).colorLayers.size(); c++) {
                        QBrush color = result.at(i).colorLayerColors.at(c);
                        bool replace = result.at(i).replaceWithForeGroundColor.at(c);
                        // In theory we can use the pattern or gradient as well for ColorV0 fonts, but ColorV1
                        // fonts can have gradients, so I am hesitant.
                        KoColorBackground *b = dynamic_cast<KoColorBackground*>(chunkShape->background().data());
                        if (b && replace) {
                            color = b->brush();
                        }
                        painter.fillPath(tf.map(result.at(i).colorLayers.at(c)), color);
                    }
                } else {
                    chunk.addPath(p);
                }
                //}
                if (p.isEmpty() && !result.at(i).image.isNull()) {
                    if (result.at(i).image.isGrayscale() || result.at(i).image.format() == QImage::Format_Mono) {
                        fillPainter.maskPainter()->save();
                        fillPainter.maskPainter()->translate(result.at(i).finalPosition.x(), result.at(i).finalPosition.y());
                        fillPainter.maskPainter()->rotate(qRadiansToDegrees(result.at(i).rotate));
                        fillPainter.maskPainter()->setCompositionMode(QPainter::CompositionMode_Plus);
                        fillPainter.maskPainter()->drawImage(result.at(i).boundingBox, result.at(i).image);
                        fillPainter.maskPainter()->restore();
                    } else {
                        painter.save();
                        painter.translate(result.at(i).finalPosition.x(), result.at(i).finalPosition.y());
                        painter.rotate(qRadiansToDegrees(result.at(i).rotate));
                        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
                        painter.drawImage(result.at(i).boundingBox, result.at(i).image);
                        painter.restore();
                    }
                }
            }
        }
        if (chunkShape->background()) {
            chunk.setFillRule(Qt::WindingFill);
            fillPainter.maskPainter()->fillPath(chunk, Qt::white);
        }
        fillPainter.renderOnGlobalPainter();
        KoShapeStrokeSP maskStroke;
        if (chunkShape->stroke()) {

            KoShapeStrokeSP stroke = qSharedPointerDynamicCast<KoShapeStroke>(chunkShape->stroke());

            if (stroke) {
                if (stroke->lineBrush().gradient()) {
                    KoClipMaskPainter strokePainter(&painter,
                                                    painter.transform().mapRect(outlineRect.boundingRect()));
                    strokePainter.shapePainter()->fillRect(outlineRect.boundingRect(), stroke->lineBrush());
                    maskStroke = KoShapeStrokeSP(new KoShapeStroke(*stroke.data()));
                    maskStroke->setColor(Qt::white);
                    maskStroke->setLineBrush(Qt::white);
                    strokePainter.maskPainter()->fillPath(outlineRect, Qt::black);
                    if (textRendering != OptimizeSpeed) {
                        strokePainter.maskPainter()->setRenderHint(QPainter::Antialiasing, true);
                    } else {
                        qDebug() << "turning off antialiasing for strokes";
                        strokePainter.maskPainter()->setRenderHint(QPainter::Antialiasing, false); 
                    }
                    maskStroke->paint(KoPathShape::createShapeFromPainterPath(chunk), *strokePainter.maskPainter());
                    strokePainter.renderOnGlobalPainter();
                } else {
                    stroke->paint(KoPathShape::createShapeFromPainterPath(chunk), painter);
                }
            }

        }
        chunk = QPainterPath();
        currentIndex = j;

    } else {
        Q_FOREACH (KoShape *child, chunkShape->shapes()) {
            paintPaths(painter, paintContext, outlineRect, child, result, chunk, currentIndex);
        }
    }
}

bool KoSvgTextShape::isRootTextNode() const
{
    return true;
}

KoSvgTextShapeFactory::KoSvgTextShapeFactory()
    : KoShapeFactoryBase(KoSvgTextShape_SHAPEID, i18nc("Text label in SVG Text Tool", "Text"))
{
    setToolTip(i18n("SVG Text Shape"));
    setIconName(koIconNameCStr("x-shape-text"));
    setLoadingPriority(5);
    setXmlElementNames(KoXmlNS::svg, QStringList("text"));

    KoShapeTemplate t;
    t.name = i18n("SVG Text");
    t.iconName = koIconName("x-shape-text");
    t.toolTip = i18n("SVG Text Shape");
    addTemplate(t);
}

KoShape *KoSvgTextShapeFactory::createDefaultShape(KoDocumentResourceManager *documentResources) const
{
    debugFlake << "Create default svg text shape";

    KoSvgTextShape *shape = new KoSvgTextShape();
    shape->setShapeId(KoSvgTextShape_SHAPEID);

    KoSvgTextShapeMarkupConverter converter(shape);
    converter.convertFromSvg(i18nc("Default text for the text shape", "<text>Placeholder Text</text>"),
                             "<defs/>",
                             QRectF(0, 0, 200, 60),
                             documentResources->documentResolution());

    debugFlake << converter.errors() << converter.warnings();

    return shape;
}

KoShape *KoSvgTextShapeFactory::createShape(const KoProperties *params, KoDocumentResourceManager *documentResources) const
{
    KoSvgTextShape *shape = new KoSvgTextShape();
    shape->setShapeId(KoSvgTextShape_SHAPEID);

    QString svgText = params->stringProperty("svgText", i18nc("Default text for the text shape",
                                                              "<text>Placeholder Text</text>"));
    QString defs = params->stringProperty("defs" , "<defs/>");
    QRectF shapeRect = QRectF(0, 0, 200, 60);
    QVariant rect = params->property("shapeRect");

    if (rect.type()==QVariant::RectF) {
        shapeRect = rect.toRectF();
    }

    KoSvgTextShapeMarkupConverter converter(shape);
    converter.convertFromSvg(svgText,
                             defs,
                             shapeRect,
                             documentResources->documentResolution());

    shape->setPosition(shapeRect.topLeft());

    return shape;
}

bool KoSvgTextShapeFactory::supports(const QDomElement &/*e*/, KoShapeLoadingContext &/*context*/) const
{
    return false;
}
