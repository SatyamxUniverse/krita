/*
 *  SPDX-FileCopyrightText: 2022 Deif Lou <ginoba@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef FILLCOLORBENCHMARK_H
#define FILLCOLORBENCHMARK_H

#include <QObject>

class FillColorBenchmark : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void benchmarkFillColor();
};

#endif
