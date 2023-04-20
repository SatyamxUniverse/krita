/*
 *  SPDX-FileCopyrightText: 2022 Deif Lou <ginoba@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef FILLCOLOREXTERNALBENCHMARK_H
#define FILLCOLOREXTERNALBENCHMARK_H

#include <QObject>

class FillColorExternalBenchmark : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void benchmarkFillColorExternal_Aligned();
    void benchmarkFillColorExternal_Unaligned();
};

#endif
