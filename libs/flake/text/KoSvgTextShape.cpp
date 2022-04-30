/*
 *  SPDX-FileCopyrightText: 2017 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoSvgTextShape.h"

#include <QTextLayout>

#include <raqm.h>
#include <fontconfig/fontconfig.h>
#include <klocalizedstring.h>

#include "KoSvgText.h"
#include "KoSvgTextProperties.h"
#include <KoShapeContainer_p.h>
#include <text/KoSvgTextChunkShape_p.h>
#include <text/KoSvgTextShapeMarkupConverter.h>
#include <KoDocumentResourceManager.h>
#include <KoShapeController.h>

#include "kis_debug.h"

#include <KoXmlReader.h>
#include <KoXmlNS.h>
#include <KoShapeLoadingContext.h>
#include <KoIcon.h>
#include <KoProperties.h>
#include <KoColorBackground.h>

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
#include <boost/optional.hpp>

#include <text/KoSvgTextChunkShapeLayoutInterface.h>

#include <FlakeDebug.h>

#include <QSharedData>


class KoSvgTextShape::Private
{
public:

    // NOTE: the cache data is shared between all the instances of
    //       the shape, though it will be reset locally if the
    //       accessing thread changes
    std::vector<raqm_t*> cachedLayouts;
    std::vector<QPointF> cachedLayoutsOffsets;
    QThread *cachedLayoutsWorkingThread = 0;
    FT_Library library = NULL;

    QPainterPath path;


    void clearAssociatedOutlines(const KoShape *rootShape);

};

KoSvgTextShape::KoSvgTextShape()
    : KoSvgTextChunkShape()
    , d(new Private)
{
    setShapeId(KoSvgTextShape_SHAPEID);
}

KoSvgTextShape::KoSvgTextShape(const KoSvgTextShape &rhs)
    : KoSvgTextChunkShape(rhs)
    , d(new Private)
{
    setShapeId(KoSvgTextShape_SHAPEID);
    // QTextLayout has no copy-ctor, so just relayout everything!
    relayout();
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

    /**
     * HACK ALERT:
     * QTextLayout should only be accessed from the thread it has been created in.
     * If the cached layout has been created in a different thread, we should just
     * recreate the layouts in the current thread to be able to render them.
     */
    qDebug() << "paint component code";
    if (QThread::currentThread() != d->cachedLayoutsWorkingThread) {
        relayout();
    }

    /*for (int i = 0; i < (int)d->cachedLayouts.size(); i++) {
        //d->cachedLayouts[i]->draw(&painter, d->cachedLayoutsOffsets[i]);
    }*/
    qDebug() << "drawing...";

    background()->paint(painter, paintContext, d->path);
    /**
     * HACK ALERT:
     * The layouts of non-gui threads must be destroyed in the same thread
     * they have been created. Because the thread might be restarted in the
     * meantime or just destroyed, meaning that the per-thread freetype data
     * will not be available.
     */
    if (QThread::currentThread() != qApp->thread()) {
        d->cachedLayouts.clear();
        d->cachedLayoutsOffsets.clear();
        d->cachedLayoutsWorkingThread = 0;
    }
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

    return d->path;
}

void KoSvgTextShape::resetTextShape()
{
    KoSvgTextChunkShape::resetTextShape();
    relayout();
}

struct TextChunk {
    QString text;
    QVector<QTextLayout::FormatRange> formats;
    Qt::LayoutDirection direction = Qt::LeftToRight;
    Qt::Alignment alignment = Qt::AlignLeading;

    struct SubChunkOffset {
        QPointF offset;
        int start = 0;
    };

    QVector<SubChunkOffset> offsets;

    boost::optional<qreal> xStartPos;
    boost::optional<qreal> yStartPos;

    QPointF applyStartPosOverride(const QPointF &pos) const {
        QPointF result = pos;

        if (xStartPos) {
            result.rx() = *xStartPos;
        }

        if (yStartPos) {
            result.ry() = *yStartPos;
        }

        return result;
    }
};

QVector<TextChunk> mergeIntoChunks(const QVector<KoSvgTextChunkShapeLayoutInterface::SubChunk> &subChunks)
{
    QVector<TextChunk> chunks;

    for (auto it = subChunks.begin(); it != subChunks.end(); ++it) {
        if (it->transformation.startsNewChunk() || it == subChunks.begin()) {
            TextChunk newChunk = TextChunk();
            newChunk.direction = it->format.layoutDirection();
            newChunk.alignment = it->format.calculateAlignment();
            newChunk.xStartPos = it->transformation.xPos;
            newChunk.yStartPos = it->transformation.yPos;
            chunks.append(newChunk);
        }

        TextChunk &currentChunk = chunks.last();

        if (it->transformation.hasRelativeOffset()) {
            TextChunk::SubChunkOffset o;
            o.start = currentChunk.text.size();
            o.offset = it->transformation.relativeOffset();

            KIS_SAFE_ASSERT_RECOVER_NOOP(!o.offset.isNull());
            currentChunk.offsets.append(o);
        }

        QTextLayout::FormatRange formatRange;
        formatRange.start = currentChunk.text.size();
        formatRange.length = it->text.size();
        formatRange.format = it->format;

        currentChunk.formats.append(formatRange);

        currentChunk.text += it->text;
    }

    return chunks;
}

/**
 * Qt's QTextLayout has a weird trait, it doesn't count space characters as
 * distinct characters in QTextLayout::setNumColumns(), that is, if we want to
 * position a block of text that starts with a space character in a specific
 * position, QTextLayout will drop this space and will move the text to the left.
 *
 * That is why we have a special wrapper object that ensures that no spaces are
 * dropped and their horizontal advance parameter is taken into account.
 */
struct LayoutChunkWrapper
{
    LayoutChunkWrapper(QTextLayout *layout)
        : m_layout(layout)
    {
    }

    QPointF addTextChunk(int startPos, int length, const QPointF &textChunkStartPos)
    {
        QPointF currentTextPos = textChunkStartPos;

        const int lastPos = startPos + length - 1;

        KIS_SAFE_ASSERT_RECOVER_RETURN_VALUE(startPos == m_addedChars, currentTextPos);
        KIS_SAFE_ASSERT_RECOVER_RETURN_VALUE(lastPos < m_layout->text().size(), currentTextPos);

        //        qDebug() << m_layout->text();

        QTextLine line;
        std::swap(line, m_danglingLine);

        if (!line.isValid()) {
            line = m_layout->createLine();
        }

        // skip all the space characters that were not included into the Qt's text line
        const int currentLineStart = line.isValid() ? line.textStart() : startPos + length;
        while (startPos < currentLineStart && startPos <= lastPos) {
            currentTextPos.rx() += skipSpaceCharacter(startPos);
            startPos++;
        }

        if (startPos <= lastPos) {
            // defines the number of columns to look for glyphs
            const int numChars = lastPos - startPos + 1;
            // Tabs break the normal column flow
            // grow to avoid missing glyphs

            int charOffset = 0;
            int noChangeCount = 0;
            while (line.textLength() < numChars) {
                int tl = line.textLength();
                line.setNumColumns(numChars + charOffset);
                if (tl == line.textLength()) {
                    noChangeCount++;
                    // 5 columns max are needed to discover tab char. Set to 10 to be safe.
                    if (noChangeCount > 10) break;
                } else {
                    noChangeCount = 0;
                }
                charOffset++;
            }

            line.setPosition(currentTextPos - QPointF(0, line.ascent()));
            currentTextPos.rx() += line.horizontalAdvance();

            // skip all the space characters that were not included into the Qt's text line
            for (int i = line.textStart() + line.textLength(); i < lastPos; i++) {
                currentTextPos.rx() += skipSpaceCharacter(i);
            }

        } else {
            // keep the created but unused line for future use
            std::swap(line, m_danglingLine);
        }
        m_addedChars += length;

        return currentTextPos;
    }

private:
    qreal skipSpaceCharacter(int pos) {
        const QTextCharFormat format =
                formatForPos(pos, m_layout->formats());

        const QChar skippedChar = m_layout->text()[pos];
        KIS_SAFE_ASSERT_RECOVER_NOOP(skippedChar.isSpace() || !skippedChar.isPrint());

        QFontMetrics metrics(format.font());
#if QT_VERSION >= QT_VERSION_CHECK(5,11,0)
        return metrics.horizontalAdvance(skippedChar);
#else
        return metrics.width(skippedChar);
#endif
    }

    static QTextCharFormat formatForPos(int pos, const QVector<QTextLayout::FormatRange> &formats)
    {
        Q_FOREACH (const QTextLayout::FormatRange &range, formats) {
            if (pos >= range.start && pos < range.start + range.length) {
                return range.format;
            }
        }

        KIS_SAFE_ASSERT_RECOVER_NOOP(0 && "pos should be within the bounds of the layouted text");

        return QTextCharFormat();
    }

private:
    int m_addedChars = 0;
    QTextLayout *m_layout;
    QTextLine m_danglingLine;
};

void KoSvgTextShape::relayout() const
{
    d->cachedLayouts.clear();
    d->cachedLayoutsOffsets.clear();
    d->cachedLayoutsWorkingThread = QThread::currentThread();
    d->path.clear();
    d->path.setFillRule(Qt::WindingFill);
    // Calculate the associated outline for a text chunk.
    d->clearAssociatedOutlines(this);

    QPointF currentTextPos;

    QVector<TextChunk> textChunks = mergeIntoChunks(layoutInterface()->collectSubChunks());

    Q_FOREACH (const TextChunk &chunk, textChunks) {
        raqm_t *layout(raqm_create());

        // QTextOption option;

        // WARNING: never activate this option! It breaks the RTL text layout!
        //option.setFlags(QTextOption::ShowTabsAndSpaces);

        //option.setWrapMode(QTextOption::WrapAnywhere);
        //option.setUseDesignMetrics(true); // TODO: investigate if it is needed?
        //option.setTextDirection(chunk.direction);

        if (!d->library) {
            FT_Init_FreeType(&d->library);
        }
        FcConfig *config = FcConfigGetCurrent();
        FcObjectSet *objectSet = FcObjectSetBuild(FC_FAMILY,FC_FILE, nullptr);

        QMap<QString,FT_Face> faces;

        if (raqm_set_text_utf8(layout, chunk.text.toUtf8(), chunk.text.toUtf8().size())) {
            if (chunk.direction == Qt::RightToLeft) {
                raqm_set_par_direction(layout, raqm_direction_t::RAQM_DIRECTION_RTL);
            } else {
                raqm_set_par_direction(layout, raqm_direction_t::RAQM_DIRECTION_LTR);
            }

            for (QTextLayout::FormatRange range : chunk.formats) {
                FT_Face face = NULL;

                FcPattern *p = FcPatternCreate();
                const FcChar8 *vals = reinterpret_cast<FcChar8*>(range.format.font().family().toUtf8().data());
                qreal fontSize = range.format.font().pointSizeF();
                FcPatternAddString(p, FC_FAMILY, vals);
                /* this is too strict, and will sometimes prevent
                 * fonts to be found, so we'll need to handle this differently...
                if (range.format.font().italic()) {
                    FcPatternAddInteger(p, FC_SLANT, FC_SLANT_ITALIC);
                }
                //The following is wrong, but it'll have to do for now...
                FcPatternAddInteger(p, FC_WEIGHT, range.format.font().weight()*2);
                if (range.format.font().stretch() >= 50) {
                    FcPatternAddInteger(p, FC_WIDTH, range.format.font().stretch());
                }*/
                FcFontSet *fontSet = FcFontList(config, p, objectSet);
                if (fontSet->nfont == 0) {
                    qWarning() << "No fonts found for family name" << range.format.font().family();
                    continue;
                }
                QString fontFileName;
                QStringList fontProperties = QString(reinterpret_cast<char*>(FcNameUnparse(fontSet->fonts[0]))).split(':');
                for (QString value : fontProperties) {
                    if (value.startsWith("file")) {
                        fontFileName = value.split("=").last();
                        fontFileName.remove("\\");
                    }
                }

                int errorCode = FT_New_Face(d->library, fontFileName.toUtf8().data(), 0, &face);
                if (errorCode == 0) {
                    qDebug() << "face loaded" << fontFileName;
                    errorCode = FT_Set_Char_Size(face, fontSize*64.0, 0, 0, 0);
                    if (errorCode == 0) {
                        if (range.start == 0) {
                            raqm_set_freetype_face(layout, face);
                        }
                        if (range.length > 0) {
                            int start = chunk.text.leftRef(range.start).toUtf8().size();
                            int length = chunk.text.midRef(range.start, range.length).toUtf8().size();
                            raqm_set_freetype_face_range(layout, face, start, length);
                        }
                    }
                } else {
                    qDebug() << "Face did not load, FreeType Error: " << errorCode << "Filename:" << fontFileName;
                }
                FT_Done_Face(face);
            }
            qDebug() << "text-length:" << chunk.text.toUtf8().size();
        }

        if (raqm_layout(layout)) {
            qDebug() << "layout succeeded";
        }

        currentTextPos = chunk.applyStartPosOverride(currentTextPos);
        const QPointF anchorPointPos = currentTextPos;


        int lastSubChunkStart = 0;
        QPointF lastSubChunkOffset;

        for (int i = 0; i <= chunk.offsets.size(); i++) {
            const bool isFinalPass = i == chunk.offsets.size();

            const int length =
                    !isFinalPass ?
                        chunk.offsets[i].start - lastSubChunkStart :
                        chunk.text.size() - lastSubChunkStart;

            if (length > 0) {
                currentTextPos += lastSubChunkOffset;
            }

            if (!isFinalPass) {
                lastSubChunkOffset = chunk.offsets[i].offset;
                lastSubChunkStart = chunk.offsets[i].start;
            }
        }


        QPointF diff;

        if (chunk.alignment & Qt::AlignTrailing || chunk.alignment & Qt::AlignHCenter) {
            if (chunk.alignment & Qt::AlignTrailing) {
                diff = currentTextPos - anchorPointPos;
            } else if (chunk.alignment & Qt::AlignHCenter) {
                diff = 0.5 * (currentTextPos - anchorPointPos);
            }

            // TODO: fix after t2b text implemented
            diff.ry() = 0;
        }

        size_t count;
        raqm_glyph_t *glyphs = raqm_get_glyphs (layout, &count);
        if (!glyphs) {
            continue;
        }

        QTransform ftTF;
        const qreal factor = 1/64.;
        // This is dependant on the writing mode, so it needs to be different for ttb.
        ftTF.scale(factor, -factor);

        int formatCount = 0;
        using namespace KoSvgText;
        QTextLayout::FormatRange range = chunk.formats.at(formatCount);

        QPointF cursorPos = currentTextPos;

        for (int g=0; g < int(count); g++) {
            /*
            uint32_t start = chunk.text.leftRef(range.start).toUtf8().size();
            uint32_t length = chunk.text.midRef(range.start, range.length).toUtf8().size();
            while (glyphs[i].cluster >= start+length) {
                formatCount += 1;
                if (formatCount < chunk.formats.size()) {
                    range = chunk.formats.at(formatCount);
                }
            }*/
            const KoSvgCharChunkFormat &format = static_cast<const KoSvgCharChunkFormat&>(range.format);

            int error = FT_Load_Glyph(glyphs[g].ftface, glyphs[g].index, 0);
            if (error != 0) {
                continue;
            }
            FT_GlyphSlotRec* glyphSlot = glyphs[g].ftface->glyph;

            qDebug() << "glyph" << g << "cluster" << glyphs[g].cluster;
            //FT_Glyph_Get_CBox( glyphs[i].ftface->glyph, 0, &bbox );

            QPointF cp = QPointF();
            // convert the outline to a painter path
            QPainterPath glyph;
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
            glyph = ftTF.map(glyph);
            QPointF advance(glyphs[g].x_advance, glyphs[g].y_advance);
            advance = ftTF.map(advance);
            QPointF offset(glyphs[g].x_offset, glyphs[g].y_offset);
            offset = ftTF.map(offset);
            QPointF totalOffset = cursorPos + offset - diff;
            glyph.translate(totalOffset);
            QRectF boundingRect(QPointF(), advance);
            boundingRect.moveTo(totalOffset);

            if (glyph.isEmpty()) {
                format.associatedShapeWrapper().addCharacterRect(boundingRect);
            } else {
                d->path.addPath(glyph);
                format.associatedShapeWrapper().addCharacterRect(glyph.boundingRect());
            }
            qDebug() << glyph.boundingRect() << boundingRect;

            cursorPos += advance;
        }

        d->cachedLayouts.push_back(layout);
        d->cachedLayoutsOffsets.push_back(-diff);
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

    QString svgText = params->stringProperty("svgText", i18nc("Default text for the text shape", "<text>Placeholder Text</text>"));
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
