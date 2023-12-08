/*
 * SPDX-FileCopyrightText: 2023 Alvin Wong <alvin@alvinhc.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "SvgCreateTextStrategy.h"
#include "SvgTextTool.h"

#include <KLocalizedString>
#include <QRectF>
#include <QTimer>

#include <memory>

#include "KisHandlePainterHelper.h"
#include "KoCanvasBase.h"
#include "KoProperties.h"
#include "KoSelection.h"
#include "KoShapeController.h"
#include "KoShapeFactoryBase.h"
#include "KoShapeRegistry.h"
#include "KoSvgTextProperties.h"
#include "KoToolBase.h"
#include "KoViewConverter.h"
#include "KoSnapGuide.h"
#include "commands/KoKeepShapesSelectedCommand.h"
#include "kis_canvas2.h"
#include "kis_global.h"
#include "kundo2command.h"

SvgCreateTextStrategy::SvgCreateTextStrategy(SvgTextTool *tool, const QPointF &clicked)
    : KoInteractionStrategy(tool)
    , m_dragStart(clicked)
    , m_dragEnd(clicked)
    , m_previewTextShape(createTextShape())
{
    const bool isHorizontal = tool->writingMode() == KoSvgText::HorizontalTB;
    setPreviewText(QLatin1String(" "));
    const double spaceSize =
        isHorizontal ? m_previewTextShape->outlineRect().width() : m_previewTextShape->outlineRect().height();

    // Check the ascender and line height with a space, which should always work.
    double ascender{};
    double descender{};
    double halfLeading{};
    std::tie(ascender, descender, halfLeading) = m_previewTextShape->lineMetricsAtPos(0);
    m_ascender = -ascender;
    m_lineHeight = qAbs(descender - ascender);

    m_minSizeInline = {m_lineHeight, m_lineHeight};

    m_previewTextShape->setPosition(m_dragStart);
    initPreviewText(spaceSize, isHorizontal);
}

void SvgCreateTextStrategy::paint(QPainter &painter, const KoViewConverter &converter)
{
    const QTransform originalPainterTransform = painter.transform();

    // Paint the preview shape directly on the view.
    painter.setTransform(m_previewTextShape->absoluteTransformation() * converter.documentToView(), true);
    painter.setPen(QPen(Qt::NoPen));
    painter.setBrush(Qt::SolidPattern);
    painter.setOpacity(1.0);
    m_previewTextShape->paint(painter);
    painter.setTransform(originalPainterTransform, false);

    // Paint a rubberband rect for the drag outline:

    painter.setTransform(converter.documentToView(), true);
    KisHandlePainterHelper handlePainter(&painter, originalPainterTransform, 0.0);

    const QPolygonF poly(QRectF(m_dragStart, m_dragEnd));
    handlePainter.setHandleStyle(KisHandleStyle::primarySelection());
    handlePainter.drawRubberLine(poly);
}

void SvgCreateTextStrategy::handleMouseMove(const QPointF &mouseLocation, Qt::KeyboardModifiers modifiers)
{
    m_dragEnd = this->tool()->canvas()->snapGuide()->snap(mouseLocation, modifiers);
    m_modifiers = modifiers;
    QRectF updateRect = QRectF(m_dragStart, m_dragEnd).normalized();

    // Resize and reposition the preview text shape according to the drag area:
    // TODO: Consider compressing updates?

    updateRect |= m_previewTextShape->boundingRect();

    SvgTextTool *const tool = qobject_cast<SvgTextTool *>(this->tool());

    QRectF rectangle = QRectF(m_dragStart, m_dragEnd).normalized();

    const KoSvgText::WritingMode writingMode = KoSvgText::WritingMode(tool->writingMode());

    bool unwrappedText = m_modifiers.testFlag(Qt::ControlModifier);
    if (rectangle.width() < m_minSizeInline.width() && rectangle.height() < m_minSizeInline.height()) {
        unwrappedText = true;
    }
    KoSvgText::AutoValue newInlineSize; // default is auto
    if (!unwrappedText) {
        double inlineSize{};
        if (writingMode == KoSvgText::HorizontalTB) {
            inlineSize = rectangle.width();
        } else {
            inlineSize = rectangle.height();
        }
        newInlineSize = qRound(inlineSize * 100.) / 100.;
    }
    // Set inline-size:
    {
        KoSvgTextProperties properties = m_previewTextShape->propertiesForPos(-1);
        properties.setProperty(KoSvgTextProperties::InlineSizeId, KoSvgText::fromAutoValue(newInlineSize));
        m_previewTextShape->setPropertiesAtPos(-1, properties);
    }

    generatePreviewText(newInlineSize.isAuto ? -1 : newInlineSize.customValue);

    QPointF origin = rectangle.topLeft();

    {
        const Qt::Alignment halign = tool->horizontalAlign();
        const bool isRtl = tool->isRtl();

        if (writingMode == KoSvgText::HorizontalTB) {
            origin.setY(rectangle.top() + m_ascender);
            if (halign & Qt::AlignCenter) {
                origin.setX(rectangle.center().x());
            } else if ((halign & Qt::AlignRight && !isRtl) || (halign & Qt::AlignLeft && isRtl)) {
                origin.setX(rectangle.right());
            }
        } else {
            if (writingMode == KoSvgText::VerticalRL) {
                origin.setX(rectangle.right() - (m_lineHeight * 0.5));
            } else {
                origin.setX(rectangle.left() + (m_lineHeight * 0.5));
            }

            if (halign & Qt::AlignCenter) {
                origin.setY(rectangle.center().y());
            } else if (halign & Qt::AlignRight) {
                origin.setY(rectangle.bottom());
            }
        }
    }
    if (!rectangle.contains(origin) && unwrappedText) {
        origin = writingMode == KoSvgText::HorizontalTB? QPointF(origin.x(), rectangle.bottom()): QPointF(rectangle.center().x(), origin.y());
    }

    m_previewTextShape->setPosition(origin);
    updateRect |= m_previewTextShape->boundingRect();

    tool->canvas()->updateCanvas(kisGrowRect(updateRect, 100));
}

KoSvgTextShape *SvgCreateTextStrategy::createTextShape()
{
    SvgTextTool *const tool = qobject_cast<SvgTextTool *>(this->tool());

    KoShapeFactoryBase *factory = KoShapeRegistry::instance()->value("KoSvgTextShapeID");
    KoProperties *params = new KoProperties();
    params->setProperty("defs", QVariant(tool->generateDefs()));
    // No need to set "shapeRect" and "origin" because we will reposition the
    // shape in the interaction strategy.

    std::unique_ptr<KoShape> shape{factory->createShape( params, tool->canvas()->shapeController()->resourceManager())};
    KoSvgTextShape *textShape = dynamic_cast<KoSvgTextShape *>(shape.get());
    KIS_ASSERT_RECOVER_RETURN_VALUE(textShape, nullptr);
    (void)shape.release();
    if (KisCanvas2 *canvas = qobject_cast<KisCanvas2 *>(tool->canvas())) {
        textShape->setResolution(canvas->image()->xRes() * 72.0, canvas->image()->yRes() * 72.0);
    }
    return textShape;
}

KUndo2Command *SvgCreateTextStrategy::createCommand()
{
    SvgTextTool *const tool = qobject_cast<SvgTextTool *>(this->tool());

    QRectF updateRect = QRectF(m_dragStart, m_dragEnd).normalized();
    updateRect |= m_previewTextShape->boundingRect();

    // We just reuse the preview shape directly.
    setPreviewText(KoSvgTextShape::defaultPlaceholderText());
    updateRect |= m_previewTextShape->boundingRect();
    tool->canvas()->updateCanvas(kisGrowRect(updateRect, 100));

    KUndo2Command *parentCommand = new KUndo2Command();

    new KoKeepShapesSelectedCommand(tool->koSelection()->selectedShapes(), {}, tool->canvas()->selectedShapesProxy(), false, parentCommand);

    KoShape *textShape = m_previewTextShape.release();
    KUndo2Command *cmd = tool->canvas()->shapeController()->addShape(textShape, 0, parentCommand);
    parentCommand->setText(cmd->text());

    new KoKeepShapesSelectedCommand({}, {textShape}, tool->canvas()->selectedShapesProxy(), true, parentCommand);
    tool->canvas()->snapGuide()->reset();

    return parentCommand;
}

void SvgCreateTextStrategy::cancelInteraction()
{
    tool()->canvas()->snapGuide()->reset();
    QRectF updateRect = QRectF(m_dragStart, m_dragEnd).normalized();
    updateRect |= m_previewTextShape->boundingRect();
    tool()->canvas()->updateCanvas(updateRect);
}

void SvgCreateTextStrategy::finishInteraction(Qt::KeyboardModifiers modifiers)
{
    m_modifiers = modifiers;
}

bool SvgCreateTextStrategy::draggingInlineSize()
{
    QRectF rectangle = QRectF(m_dragStart, m_dragEnd).normalized();
    return (rectangle.width() >= m_minSizeInline.width() || rectangle.height() >= m_minSizeInline.height()) && !m_modifiers.testFlag(Qt::ControlModifier);
}

void SvgCreateTextStrategy::initPreviewText(const double spaceSize, const bool isHorizontal)
{
    static const KLocalizedString s_previewTextI18n =
        ki18nc("text tool: preview text segments (see comment)",
               // i18n: This message is sample sentence split into short
               // segments. It is for showing the text alignment and word
               // wrapping as well as previewing the font settings when adding
               // text. Tips to localizing this:
               // (1.) Do not translate word to word. Use a typography sample, a
               //      line from a famous poem, or something that makes sense.
               // (2.) Try to use short words in the sentence.
               // (3.) Split into segments by a pipe (U+007C `|`) character.
               //  (*) If words are separated by spaces, put a pipe after the
               //      space and end the message with a trailing space.
               //  (*) There are no spaces, split the words logically, for
               //      example: `AB|CD,|EF|GH.`
               "The |quick |brown |fox |jumps |over |the |lazy |dog. ");

    // TODO: Detect language
    m_previewTextSegments = s_previewTextI18n.toString().split('|');

    QString checkStr;
    Q_FOREACH (const QString &seg, m_previewTextSegments) {
        // Surround the string with spaces to try to account for glyphs poking out of the box.
        checkStr = " ";
        checkStr.append(seg);
        checkStr.append(" ");
        setPreviewText(checkStr);
        const double size =
            isHorizontal ? m_previewTextShape->outlineRect().width() : m_previewTextShape->outlineRect().height();
        m_previewTextLengths.append(size - spaceSize * 2.);
    }
    generatePreviewText(-1);
    tool()->canvas()->updateCanvas(kisGrowRect(m_previewTextShape->boundingRect(), 100));
}

void SvgCreateTextStrategy::generatePreviewText(double inlineSize)
{
    if (inlineSize <= 0) {
        setPreviewText(m_previewTextSegments.join(QString()));
        return;
    }
    QString text;
    while (true) {
        for (int i = 0; i < m_previewTextSegments.length(); i++) {
            text.append(m_previewTextSegments.at(i));
            if (inlineSize < 0.) {
                setPreviewText(text);
                return;
            }
            inlineSize -= m_previewTextLengths.at(i);
        }
    }
}

void SvgCreateTextStrategy::setPreviewText(QString text)
{
    if (m_previewTextShape->plainText() == text) {
        return;
    }
    m_previewTextShape->removeText(0, m_previewTextShape->plainText().length());
    m_previewTextShape->insertText(0, std::move(text));
}
