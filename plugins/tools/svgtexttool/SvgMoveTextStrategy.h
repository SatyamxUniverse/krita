/*
 * SPDX-FileCopyrightText: 2023 Alvin Wong <alvin@alvinhc.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SVG_MOVE_TEXT_STRATEGY_H
#define SVG_MOVE_TEXT_STRATEGY_H

#include <KoInteractionStrategy.h>

#include <QPointF>
#include <QPolygonF>

class KoSvgTextShape;

class SvgMoveTextStrategy : public KoInteractionStrategy
{
public:
    SvgMoveTextStrategy(KoToolBase *tool, KoSvgTextShape *shape, const QPointF &clicked);
    ~SvgMoveTextStrategy() override = default;

    void paint(QPainter &painter, const KoViewConverter &converter) override;

    void handleMouseMove(const QPointF &mouseLocation, Qt::KeyboardModifiers modifiers) override;
    KUndo2Command *createCommand() override;
    void cancelInteraction() override;
    void finishInteraction(Qt::KeyboardModifiers modifiers) override;

private:
    KoSvgTextShape *m_shape;
    QPointF m_dragStart;
    QPointF m_initialPosition;
    QPointF m_finalPosition;
    QPointF m_mouseOffset;
    QPolygonF m_bounds;
};

#endif /* SVG_MOVE_TEXT_STRATEGY_H */
