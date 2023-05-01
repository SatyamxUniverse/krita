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
#include <unordered_map>

#include <KoColor.h>
#include <kis_paint_device.h>
#include <kis_pixel_selection.h>
#include <KisRunnableStrokeJobsInterface.h>
#include <KisRunnableStrokeJobUtils.h>

#include "KisMultiThreadedScanlineFill_TilePolicies.h"


inline uint qHash(const QPoint &key)
{
    return qHash((static_cast<quint64>(key.x()) << 32) | static_cast<quint64>(key.y()));
}

template<>
struct std::hash<QPoint>
{
    std::size_t operator()(const QPoint& pt) const noexcept
    {
        return qHash(pt);
    }
};

namespace KisMultiThreadedScanlineFillNS
{

struct Span
{
    qint32 x1;
    qint32 x2;
    qint32 y;
    qint32 dy;
};

using SeedSpanList = QVector<Span>;
using TilePropagationInfo = std::unordered_map<TileId, SeedSpanList>;

template <typename TilePolicyFactory>
class Filler;

using TilePropagationInfoSP = QSharedPointer<TilePropagationInfo>;

template<typename TilePolicyFactory>
struct FillSharedData
{
    FillSharedData(KisPaintDeviceSP _referenceDevice,
                   KisPaintDeviceSP _externalDevice,
                   KisPixelSelectionSP _maskDevice,
                   KisPixelSelectionSP _selectionDevice,
                   const QRect &_workingRect,
                   const TilePolicyFactory &_tilePolicyFactory,
                   KisRunnableStrokeJobsInterface *_jobsInterface)
        : referenceDevice(_referenceDevice)
        , externalDevice(_externalDevice)
        , maskDevice(_maskDevice)
        , selectionDevice(_selectionDevice)
        , workingRect(_workingRect)
        , tilePolicyFactory(_tilePolicyFactory)
        , jobsInterface(_jobsInterface)
    {
    }

    KisPaintDeviceSP referenceDevice;
    KisPaintDeviceSP externalDevice;
    KisPixelSelectionSP maskDevice;
    KisPixelSelectionSP selectionDevice;
    QRect workingRect;
    std::remove_reference_t<TilePolicyFactory> tilePolicyFactory;
    KisRunnableStrokeJobsInterface *jobsInterface = 0;
};

template<typename TilePolicyFactory>
using FillSharedDataSP = QSharedPointer<FillSharedData<TilePolicyFactory>>;


template <typename TilePolicyFactory>
struct PopulateFillTasks : KisRunnableStrokeJobData
{
    PopulateFillTasks() : KisRunnableStrokeJobData(nullptr, KisStrokeJobData::SEQUENTIAL) {}

    PopulateFillTasks(FillSharedDataSP<TilePolicyFactory> d,
                      QVector<TilePropagationInfoSP> _results)
        : KisRunnableStrokeJobData(nullptr, KisStrokeJobData::SEQUENTIAL)
        , m_d(d)
        , results(_results)
    {}

    void run() override;

    FillSharedDataSP<TilePolicyFactory> m_d;
    QVector<TilePropagationInfoSP> results;
};

template <typename TilePolicyFactory>
class Filler
{
    using TilePolicyType = typename TilePolicyFactory::TilePolicyType;

public:
    Filler(FillSharedDataSP<TilePolicyFactory> sharedData)
        : m_d(sharedData)
    {
    }

private:
    friend void fillContiguousGroup(KisPaintDeviceSP, KisPaintDeviceSP, qint32, quint8, qint32, const QRect&, const QPoint&);

    template <typename T>
    friend struct PopulateFillTasks;

    FillSharedDataSP<TilePolicyFactory> m_d;

    TilePropagationInfo processTile(const TileId &tileId,
                                    const SeedSpanList &seedSpans,
                                    TilePolicyType &tilePolicy)
    {
        TilePropagationInfo tilePropagationInfo;

        tilePolicy.beginProcessing(m_d->referenceDevice, m_d->externalDevice, m_d->maskDevice, m_d->selectionDevice, tileId, m_d->workingRect);

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
                            if (x < m_d->workingRect.left()) {
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
                        if (x2 > m_d->workingRect.right()) {
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
                        if (spanY1 >= m_d->workingRect.top() && spanY1 <= m_d->workingRect.bottom()) {
                            if (spanY1 < tilePolicy.tileSubRect().top()) {
                                tilePropagationInfo[{tileId.x(), tileId.y() - 1}].append({x1, x2 - 1, spanY1, 1});
                            } else if (spanY1 > tilePolicy.tileSubRect().bottom()) {
                                tilePropagationInfo[{tileId.x(), tileId.y() + 1}].append({x1, x2 - 1, spanY1, -1});
                            } else {
                                spans.push({x1, x2 - 1, spanY1, -span.dy});
                            }
                        }
                        if (spanY2 >= m_d->workingRect.top() && spanY2 <= m_d->workingRect.bottom()) {
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

template <typename TilePolicyFactory>
void PopulateFillTasks<TilePolicyFactory>::run()
{
    using TilePolicyType = typename TilePolicyFactory::TilePolicyType;

    TilePropagationInfo tilePropagationInfo;

    for (auto it = results.cbegin(); it != results.cend(); ++it) {
        for (auto infoIt = (*it)->begin(); infoIt != (*it)->end(); ++infoIt) {
            auto dstIt = tilePropagationInfo.find(infoIt->first);

            if (dstIt != tilePropagationInfo.end()) {
                dstIt->second.append(infoIt->second);
            } else {
                tilePropagationInfo.insert(*infoIt);
            }
        }
    }

    results.clear();

    QVector<KisRunnableStrokeJobDataBase*> jobs;

    for (auto it = tilePropagationInfo.begin(); it !=  tilePropagationInfo.end(); ++it) {
        std::pair<QPoint, SeedSpanList> value = *it;

        TilePropagationInfoSP info(new TilePropagationInfo);
        results.append(info);

        KritaUtils::addJobConcurrent(jobs,
            [info, value, d = m_d] () {
                TilePolicyType tilePolicy = d->tilePolicyFactory();
                Filler<TilePolicyFactory> filler(d);
                TilePropagationInfo result = filler.processTile(value.first, value.second, tilePolicy);
                *info = result;
            });
    }

    if (!jobs.isEmpty()) {
        jobs.append(new PopulateFillTasks<TilePolicyFactory>(*this));
        m_d->jobsInterface->addRunnableJobs(jobs);
    }
}

template <typename TilePolicyFactory>
void startFillProcess(const QPoint &startPoint,
                      KisPaintDeviceSP referenceDevice,
                      KisPaintDeviceSP externalDevice,
                      KisPixelSelectionSP maskDevice,
                      KisPixelSelectionSP selectionDevice,
                      const QRect &workingRect,
                      const TilePolicyFactory &tilePolicyFactory,
                      KisRunnableStrokeJobsInterface *jobsInterface)
{
    FillSharedDataSP<TilePolicyFactory> sharedData(
        new FillSharedData<TilePolicyFactory>(referenceDevice, externalDevice,
                                              maskDevice, selectionDevice,
                                              workingRect,
                                              tilePolicyFactory,
                                              jobsInterface));

    const TileId seedPointTileId(
        static_cast<qint32>(std::floor((startPoint.x() - referenceDevice->x()) / static_cast<qreal>(tileSize.width()))),
        static_cast<qint32>(std::floor((startPoint.y() - referenceDevice->y()) / static_cast<qreal>(tileSize.height())))
        );

    QVector<TilePropagationInfoSP> results;

    results.append(toQShared(
        new TilePropagationInfo(
            {{seedPointTileId, {Span{startPoint.x(), startPoint.x(), startPoint.y(), 1}}}})));


    jobsInterface->addRunnableJob(new PopulateFillTasks<TilePolicyFactory>(sharedData, results));
}


}

#endif
