/*
 *  SPDX-FileCopyrightText: 2022 Deif Lou <ginoba@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef FILLWITHBOUNDARYBENCHMARK_H
#define FILLWITHBOUNDARYBENCHMARK_H

#include <QObject>

class FillSelectionWithBoundaryBenchmark : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void benchmarkFillSelectionWithBoundary_Aligned_Aligned();
    void benchmarkFillSelectionWithBoundary_Aligned_Unaligned();
    void benchmarkFillSelectionWithBoundary_Unaligned_Aligned();
    void benchmarkFillSelectionWithBoundary_Unaligned_Unaligned();
};

#endif
