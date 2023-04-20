/*
 *  SPDX-FileCopyrightText: 2022 Deif Lou <ginoba@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KISMULTITHREADEDSCANLINEFILL_FILLER_H
#define KISMULTITHREADEDSCANLINEFILL_FILLER_H

#include <QStack>
#include <QPoint>
#include <QVector>
#include <QHash>
#include <QSize>
#include <QtConcurrent>
#include <QFuture>
#include <QThread>

#include <vector>

#include <KoColor.h>
#include <kis_paint_device.h>
#include <kis_pixel_selection.h>

#include "KisMultiThreadedScanlineFill_TilePolicies.h"

inline uint qHash(const QPoint &key)
{
    return qHash((static_cast<quint64>(key.x()) << 32) | static_cast<quint64>(key.y()));
}

namespace KisMultiThreadedScanlineFillNS
{

template <typename SelectionPolicyType_, typename TilePolicyType_>
class Filler
{
public:
    static_assert(std::is_same<SelectionPolicyType_, typename TilePolicyType_::SelectionPolicyType>::value,
                  "Selection policies don't match.");

    using SelectionPolicyType = SelectionPolicyType_;
    using TilePolicyType = TilePolicyType_;

    Filler(KisPaintDeviceSP referenceDevice,
           KisPaintDeviceSP externalDevice,
           KisPixelSelectionSP maskDevice,
           KisPixelSelectionSP selectionDevice,
           const KoColor &fillColor,
           const QRect &workingRect,
           const SelectionPolicyType &masterSelectionPolicy)
        : m_referenceDevice(referenceDevice)
        , m_externalDevice(externalDevice)
        , m_maskDevice(maskDevice)
        , m_selectionDevice(selectionDevice)
        , m_poolSize(static_cast<quint32>(QThread::idealThreadCount()))
        , m_workingRect(workingRect)
    {
        m_pool.reserve(m_poolSize);
        for (quint32 i = 0; i < m_poolSize; ++i) {
            m_pool.push_back({QPair<bool, QFuture<TilePropagationInfo>>(), TilePolicyType(fillColor, masterSelectionPolicy)});
        }
    }

    void fill(const QPoint &startPoint)
    {
        const TileId seedPointTileId(
            static_cast<qint32>(std::floor((startPoint.x() - m_referenceDevice->x()) / static_cast<qreal>(tileSize.width()))),
            static_cast<qint32>(std::floor((startPoint.y() - m_referenceDevice->y()) / static_cast<qreal>(tileSize.height())))
        );

        TilePropagationInfo tilePropagationInfo;
        tilePropagationInfo.insert(seedPointTileId, {Span{startPoint.x(), startPoint.x(), startPoint.y(), 1}});

        while (!tilePropagationInfo.isEmpty()) {
            qint32 jobsRemainingToBeAdded = tilePropagationInfo.size();
            qint32 numberOfCompletedJobs = 0;
            TilePropagationInfo tilePropagationInfoResults;
            QHashIterator<TileId, SeedSpanList> tilePropagationInfoIt(tilePropagationInfo);

            // Initialize the "job running" flag
            for (quint32 i = 0; i < m_poolSize; ++i) {
                m_pool[i].job.first = false;
            }
            // Add initial jobs
            for (quint32 i = 0; i < m_poolSize; ++i) {
                if (tilePropagationInfoIt.hasNext()) {
                    tilePropagationInfoIt.next();
                    m_pool[i].job.first = true;
                    m_pool[i].job.second =
                        QtConcurrent::run(
                            [tilePropagationInfoIt, i, this]
                            () -> TilePropagationInfo
                            {
                                return processTile(tilePropagationInfoIt.key(), tilePropagationInfoIt.value(), m_pool[i].tilePolicy);
                            }
                        );
                    --jobsRemainingToBeAdded;
                } else {
                    break;
                }
            }
            // Add remaining jobs
            while (jobsRemainingToBeAdded > 0) {
                for (quint32 i = 0; i < m_poolSize; ++i) {
                    if (m_pool[i].job.first == true && m_pool[i].job.second.isFinished()) {
                        QHashIterator<TileId, SeedSpanList> futureTilePropagationInfoIt(m_pool[i].job.second.result());
                        while (futureTilePropagationInfoIt.hasNext()) {
                            futureTilePropagationInfoIt.next();
                            const TileId &tileId = futureTilePropagationInfoIt.key();
                            const SeedSpanList &seedSpanList = futureTilePropagationInfoIt.value();
                            tilePropagationInfoResults[tileId].append(seedSpanList);
                        }
                        tilePropagationInfoIt.next();
                        m_pool[i].job.second =
                            QtConcurrent::run(
                                [tilePropagationInfoIt, i, this]
                                () -> TilePropagationInfo
                                {
                                    return processTile(tilePropagationInfoIt.key(), tilePropagationInfoIt.value(), m_pool[i].tilePolicy);
                                }
                            );
                        ++numberOfCompletedJobs;
                        --jobsRemainingToBeAdded;
                        if (jobsRemainingToBeAdded == 0) {
                            break;
                        }
                    }
                }
            }
            // Wait for the remaining jobs to complete
            while (numberOfCompletedJobs != tilePropagationInfo.size()) {
                for (quint32 i = 0; i < m_poolSize; ++i) {
                    if (m_pool[i].job.first == true && m_pool[i].job.second.isFinished()) {
                        QHashIterator<TileId, SeedSpanList> futureTilePropagationInfoIt(m_pool[i].job.second.result());
                        while (futureTilePropagationInfoIt.hasNext()) {
                            futureTilePropagationInfoIt.next();
                            const TileId &tileId = futureTilePropagationInfoIt.key();
                            const SeedSpanList &seedSpanList = futureTilePropagationInfoIt.value();
                            tilePropagationInfoResults[tileId].append(seedSpanList);
                        }
                        m_pool[i].job.first = false;
                        ++numberOfCompletedJobs;
                        if (numberOfCompletedJobs == tilePropagationInfo.size()) {
                            break;
                        }
                    }
                }
            }
            // Propagate the results
            tilePropagationInfo = tilePropagationInfoResults;
        }
    }

private:
    friend void fillContiguousGroup(KisPaintDeviceSP, KisPaintDeviceSP, qint32, quint8, qint32, const QRect&, const QPoint&);

    struct Span
    {
        qint32 x1;
        qint32 x2;
        qint32 y;
        qint32 dy;
    };

    using SeedSpanList = QVector<Span>;
    using TilePropagationInfo = QHash<TileId, SeedSpanList>;

    struct PoolEntry
    {
        QPair<bool, QFuture<TilePropagationInfo>> job;
        TilePolicyType tilePolicy;
    };

    KisPaintDeviceSP m_referenceDevice;
    KisPaintDeviceSP m_externalDevice;
    KisPixelSelectionSP m_maskDevice;
    KisPixelSelectionSP m_selectionDevice;
    const quint32 m_poolSize;
    const QRect m_workingRect;
    // Using std::vector since QVector does not allow deleted default constructor
    // when using the reserve method (at least on 5.12, Sin 5.15 it does)
    std::vector<PoolEntry> m_pool;

    TilePropagationInfo processTile(const TileId &tileId,
                                    const SeedSpanList &seedSpans,
                                    TilePolicyType &tilePolicy)
    {
        TilePropagationInfo tilePropagationInfo;

        tilePolicy.beginProcessing(m_referenceDevice, m_externalDevice, m_maskDevice, m_selectionDevice, tileId, m_workingRect);

        // Tile based scanline fill
        {
            QStack<Span> spans;

            for (const Span &seedSpan : seedSpans) {
                spans.push(seedSpan);
            }

            while(!spans.isEmpty()) {
                Span span = spans.pop();

                KIS_ASSERT(span.x1 >= tilePolicy.tileSubRect().left());
                KIS_ASSERT(span.x2 <= tilePolicy.tileSubRect().right());
                KIS_ASSERT(span.y >= tilePolicy.tileSubRect().top());
                KIS_ASSERT(span.y <= tilePolicy.tileSubRect().bottom());

                tilePolicy.setWorkingRow(span.y);

                qint32 x1 = span.x1;
                qint32 x2 = span.x1;
                // Expand to the left if needed. This will find the left extreme of
                // the first subspan and fill on the way
                if (!tilePolicy.isAlreadySet(span.x1) && tilePolicy.isInsideBoundarySelection(span.x1)) {
                    quint8 opacity = tilePolicy.calculateOpacity(span.x1);
                    if (opacity) {
                        ++x2;
                        tilePolicy.setValue(span.x1, opacity);
                        while (true) {
                            const qint32 x = x1 - 1;
                            if (x < m_workingRect.left()) {
                                break;
                            }
                            if (x < tilePolicy.tileSubRect().left()) {
                                tilePropagationInfo[{tileId.x() - 1, tileId.y()}].append({x, x, span.y, 1});
                                break;
                            }
                            if (tilePolicy.isAlreadySet(x) || !tilePolicy.isInsideBoundarySelection(x)) {
                                break;
                            }
                            opacity = tilePolicy.calculateOpacity(x);
                            if (opacity == OPACITY_TRANSPARENT_U8) {
                                break;
                            }
                            tilePolicy.setValue(x, opacity);
                            --x1;
                        }
                    }
                }
                // Now we go to the right finding the fillable subspans and propagate
                while (true) {
                    // Find the right extreme of the current subspan and fill on the way
                    while (true) {
                        if (x2 > m_workingRect.right()) {
                            break;
                        }
                        if (x2 > tilePolicy.tileSubRect().right()) {
                            tilePropagationInfo[{tileId.x() + 1, tileId.y()}].append({x2, x2, span.y, 1});
                            break;
                        }
                        if (tilePolicy.isAlreadySet(x2) || !tilePolicy.isInsideBoundarySelection(x2)) {
                            break;
                        }
                        quint8 opacity = tilePolicy.calculateOpacity(x2);
                        if (opacity == OPACITY_TRANSPARENT_U8) {
                            break;
                        }
                        tilePolicy.setValue(x2, opacity);
                        ++x2;
                    }
                    // Propagate the subspan to top and bottom neighbor tiles
                    if (x2 > x1) {
                        const qint32 spanY1 = span.y - span.dy;
                        const qint32 spanY2 = span.y + span.dy;
                        if (spanY1 >= m_workingRect.top() && spanY1 <= m_workingRect.bottom()) {
                            if (spanY1 < tilePolicy.tileSubRect().top()) {
                                tilePropagationInfo[{tileId.x(), tileId.y() - 1}].append({x1, x2 - 1, spanY1, 1});
                            } else if (spanY1 > tilePolicy.tileSubRect().bottom()) {
                                tilePropagationInfo[{tileId.x(), tileId.y() + 1}].append({x1, x2 - 1, spanY1, -1});
                            } else {
                                spans.push({x1, x2 - 1, spanY1, -span.dy});
                            }
                        }
                        if (spanY2 >= m_workingRect.top() && spanY2 <= m_workingRect.bottom()) {
                            if (spanY2 < tilePolicy.tileSubRect().top()) {
                                tilePropagationInfo[{tileId.x(), tileId.y() - 1}].append({x1, x2 - 1, spanY2, 1});
                            } else if (spanY2 > tilePolicy.tileSubRect().bottom()) {
                                tilePropagationInfo[{tileId.x(), tileId.y() + 1}].append({x1, x2 - 1, spanY2, -1});
                            } else {
                                spans.push({x1, x2 - 1, spanY2, span.dy});
                            }
                        }
                    }
                    ++x2;
                    // Skip non-selectable pixels
                    while (x2 <= span.x2) {
                        if (!tilePolicy.isAlreadySet(x2) || !tilePolicy.isInsideBoundarySelection(x2)) {
                            break;
                        }
                        if (tilePolicy.calculateOpacity(x2) > OPACITY_TRANSPARENT_U8) {
                            break;
                        }
                        ++x2;
                    }
                    x1 = x2;
                    if (x2 > span.x2) {
                        break;
                    }
                }
            }
        }

        tilePolicy.endProcessing();

        return tilePropagationInfo;
    }
};

}

#endif
