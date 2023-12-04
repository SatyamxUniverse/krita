/*
 * SPDX-FileCopyrightText: 2023 Alvin Wong <alvin@alvinhc.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "SvgCreateTextStrategy.h"
#include "SvgTextTool.h"

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
#include "kis_global.h"
#include "kundo2command.h"

SvgCreateTextStrategy::SvgCreateTextStrategy(SvgTextTool *tool, const QPointF &clicked)
    : KoInteractionStrategy(tool)
    , m_dragStart(clicked)
    , m_dragEnd(clicked)
    , m_previewTextShape(createTextShape())
{
    // FIXME: This may be incorrect if the first character in the preview text
    //        is using a fallback font.
    double ascender{};
    double descender{};
    double halfLeading{};
    std::tie(ascender, descender, halfLeading) = m_previewTextShape->lineMetricsAtPos(0);
    m_ascender = -ascender;
    m_lineHeight = qAbs(descender - ascender);

    m_minSizeInline = {m_lineHeight, m_lineHeight};

    m_previewTextShape->setPosition(m_dragStart);
}

void SvgCreateTextStrategy::paint(QPainter &painter, const KoViewConverter &converter)
{
    const QTransform originalPainterTransform = painter.transform();

    // Paint the preview shape directly on the view.
    painter.setTransform(m_previewTextShape->absoluteTransformation() * converter.documentToView(), true);
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
    return textShape;
}

KUndo2Command *SvgCreateTextStrategy::createCommand()
{
    SvgTextTool *const tool = qobject_cast<SvgTextTool *>(this->tool());

    // We just reuse the preview shape directly.

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
    const QRectF updateRect = QRectF(m_dragStart, m_dragEnd).normalized();
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
