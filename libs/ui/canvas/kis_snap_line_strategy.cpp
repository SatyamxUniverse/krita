/*
 *  SPDX-FileCopyrightText: 2016 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_snap_line_strategy.h"

#include <QPainterPath>
#include "kis_global.h"

struct KisSnapLineStrategy::Private
{
    QList<qreal> horizontalLines;
    QList<qreal> verticalLines;
};

KisSnapLineStrategy::KisSnapLineStrategy(KoSnapGuide::Strategy type)
    : KoSnapStrategy(type),
      m_d(new Private)
{
}

KisSnapLineStrategy::~KisSnapLineStrategy()
{
}

bool KisSnapLineStrategy::snap(const QPointF &mousePosition, KoSnapProxy *proxy, qreal maxSnapDistance)
{
    Q_UNUSED(proxy);

    QPointF snappedPoint = mousePosition;
    qreal minXDistance = std::numeric_limits<qreal>::max();
    qreal minYDistance = std::numeric_limits<qreal>::max();

    Q_FOREACH (qreal line, m_d->horizontalLines) {
        const qreal dist = qAbs(mousePosition.y() - line);

        if (dist < maxSnapDistance && dist < minYDistance) {
            minYDistance = dist;
            snappedPoint.ry() = line;
        }
    }

    Q_FOREACH (qreal line, m_d->verticalLines) {
        const qreal dist = qAbs(mousePosition.x() - line);

        if (dist < maxSnapDistance && dist < minXDistance) {
            minXDistance = dist;
            snappedPoint.rx() = line;
        }
    }

    if (kisDistance(snappedPoint, mousePosition) > maxSnapDistance) {
        if (minXDistance < minYDistance) {
            snappedPoint.ry() = mousePosition.y();
        } else {
            snappedPoint.rx() = mousePosition.x();
        }
    }

    setSnappedPosition(snappedPoint);
    return
        minXDistance < std::numeric_limits<qreal>::max() ||
            minYDistance < std::numeric_limits<qreal>::max();
}

bool KisSnapLineStrategy::snapWithLine(const QPointF &mousePosition, const bool &p1, const QLineF &line, KoSnapProxy *proxy, qreal maxSnapDistance)
{
    Q_UNUSED(proxy);

    QPointF snappedPoint = mousePosition;
    qreal horzSnap, vertSnap;
    QPointF horzInter, vertInter;
    bool snap = false;
    qreal minXDistance = std::numeric_limits<qreal>::max();
    qreal minYDistance = std::numeric_limits<qreal>::max();

    Q_FOREACH (qreal guideLine, m_d->horizontalLines) {
        QPointF intersect;
        if (QLineF::NoIntersection != QLineF(-1, guideLine, 1, guideLine).intersects(line, &intersect)) {
            qreal dist = p1? QLineF(intersect, line.p1()).length(): QLineF(intersect, line.p2()).length();
            if (dist < minYDistance) {
                minYDistance = dist;
                horzSnap = guideLine;
                horzInter = intersect;
            }
        }
    }

    Q_FOREACH (qreal guideLine, m_d->verticalLines) {
        QPointF intersect;
        if (QLineF::NoIntersection != QLineF(guideLine, -1, guideLine, 1).intersects(line, &intersect)) {
            qreal dist = p1? QLineF(intersect, line.p1()).length(): QLineF(intersect, line.p2()).length();
            if (dist < minXDistance) {
                minXDistance = dist;
                vertSnap = guideLine;
                vertInter = intersect;
            }
        }
    }

    snap = minXDistance < std::numeric_limits<qreal>::max() ||
            minYDistance < std::numeric_limits<qreal>::max();

    if (snap) {
        snap = minYDistance < maxSnapDistance || minXDistance < maxSnapDistance;

        if (minYDistance < minXDistance) {
            snappedPoint = horzInter;
        } else {
            snappedPoint = vertInter;
        }
    }
    setSnappedPosition(snappedPoint);

    return snap;
}

QPainterPath KisSnapLineStrategy::decoration(const KoViewConverter &converter) const
{
    Q_UNUSED(converter);
    return QPainterPath();
}

void KisSnapLineStrategy::addLine(Qt::Orientation orientation, qreal pos)
{
    if (orientation == Qt::Horizontal) {
        m_d->horizontalLines << pos;
    } else {
        m_d->verticalLines << pos;
    }
}

void KisSnapLineStrategy::setHorizontalLines(const QList<qreal> &lines)
{
    m_d->horizontalLines = lines;
}

void KisSnapLineStrategy::setVerticalLines(const QList<qreal> &lines)
{
    m_d->verticalLines = lines;
}

