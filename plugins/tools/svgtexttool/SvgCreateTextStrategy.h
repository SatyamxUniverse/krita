/*
 * SPDX-FileCopyrightText: 2023 Alvin Wong <alvin@alvinhc.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SVG_CREATE_TEXT_STRATEGY_H
#define SVG_CREATE_TEXT_STRATEGY_H

#include <KoInteractionStrategy.h>

#include <QPointF>
#include <QSizeF>

#include <memory>

class SvgTextTool;

class KoSvgTextShape;

class SvgCreateTextStrategy : public KoInteractionStrategy
{
public:
    SvgCreateTextStrategy(SvgTextTool *tool, const QPointF &clicked);
    ~SvgCreateTextStrategy() override = default;

    void paint(QPainter &painter, const KoViewConverter &converter) override;
    void handleMouseMove(const QPointF &mouseLocation, Qt::KeyboardModifiers modifiers) override;
    KUndo2Command *createCommand() override;
    void cancelInteraction() override;
    void finishInteraction(Qt::KeyboardModifiers modifiers) override;

    bool draggingInlineSize();

private:
    KoSvgTextShape *createTextShape();

private:
    QPointF m_dragStart;
    QPointF m_dragEnd;
    QSizeF m_minSizeInline;
    Qt::KeyboardModifiers m_modifiers;
    std::unique_ptr<KoSvgTextShape> m_previewTextShape;
    double m_ascender;
    double m_lineHeight;
};

#endif /* SVG_CREATE_TEXT_STRATEGY_H */
