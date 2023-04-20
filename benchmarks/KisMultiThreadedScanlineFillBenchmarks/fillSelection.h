/*
 *  SPDX-FileCopyrightText: 2022 Deif Lou <ginoba@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef FILLSELECTIONBENCHMARK_H
#define FILLSELECTIONBENCHMARK_H

#include <QObject>

class FillSelectionBenchmark : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void benchmarkFillSelection_Aligned();
    void benchmarkFillSelection_Unaligned();
};

#endif
