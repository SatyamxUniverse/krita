/*
 * SPDX-FileCopyrightText: 2023 Alvin Wong <alvin@alvinhc.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "SvgMoveTextStrategy.h"
#include "SvgMoveTextCommand.h"

#include "KoSvgText.h"
#include "KoSvgTextShape.h"

#include "KoCanvasBase.h"
#include "KoSnapGuide.h"
#include "KoToolBase.h"
#include "kis_algebra_2d.h"
#include <QPainter>
#include <KisHandlePainterHelper.h>
#include <KoViewConverter.h>

SvgMoveTextStrategy::SvgMoveTextStrategy(KoToolBase *tool, KoSvgTextShape *shape, const QPointF &clicked)
    : KoInteractionStrategy(tool)
    , m_shape(shape)
    , m_dragStart(clicked)
    , m_initialPosition(shape->absolutePosition())
    , m_finalPosition(m_initialPosition)
    , m_mouseOffset(m_dragStart - m_initialPosition)
{
    this->tool()->canvas()->snapGuide()->setIgnoredShapes(KoShape::linearizeSubtree({shape}));
    m_bounds = m_shape->absoluteTransformation().map(m_shape->outlineRect());
    m_bounds << m_shape->absoluteTransformation().map(m_shape->initialTextPosition());
}

void SvgMoveTextStrategy::paint(QPainter &painter, const KoViewConverter &converter)
{
    QPolygonF points = m_shape->absoluteTransformation().map(m_shape->outlineRect());
    const QTransform originalPainterTransform = painter.transform();
    painter.setTransform(converter.documentToView(), true);
    KisHandlePainterHelper handlePainter(&painter, originalPainterTransform, 1.0);

    handlePainter.setHandleStyle(KisHandleStyle::primarySelection());
    handlePainter.drawRubberLine(points);
}

void SvgMoveTextStrategy::handleMouseMove(const QPointF &mouseLocation, Qt::KeyboardModifiers modifiers)
{
    const QPointF delta = mouseLocation - m_dragStart;

    if (modifiers & Qt::ShiftModifier) {
        m_finalPosition = m_initialPosition+ snapToClosestAxis(delta);
    } else {
        m_finalPosition =
                tool()->canvas()->snapGuide()->snapWithPolygon(m_initialPosition + m_mouseOffset + delta,
                                                               m_bounds.translated(delta.x(), delta.y()), modifiers) - m_mouseOffset;
    }

    SvgMoveTextCommand(m_shape, m_finalPosition, m_initialPosition).redo();
    tool()->repaintDecorations();
}

KUndo2Command *SvgMoveTextStrategy::createCommand()
{
    tool()->canvas()->snapGuide()->reset();
    if (KisAlgebra2D::fuzzyPointCompare(m_initialPosition, m_finalPosition)) {
        return nullptr;
    }
    return new SvgMoveTextCommand(m_shape, m_finalPosition, m_initialPosition);
}

void SvgMoveTextStrategy::cancelInteraction()
{
    SvgMoveTextCommand(m_shape, m_finalPosition, m_initialPosition).undo();
    tool()->repaintDecorations();
}

void SvgMoveTextStrategy::finishInteraction(Qt::KeyboardModifiers /*modifiers*/)
{
}
