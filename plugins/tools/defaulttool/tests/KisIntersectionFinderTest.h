/*
 *  SPDX-FileCopyrightText: 2021 Tanmay Chavan <earendil01tc@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KISINTERSECTIONFINDERTEST_H
#define KISINTERSECTIONFINDERTEST_H

#include <QObject>

class KisIntersectionFinderTest : public QObject
{
    Q_OBJECT
public:
    explicit KisIntersectionFinderTest(QObject *parent = nullptr);

private Q_SLOTS:

    void lineLineIntersectionTest();
    void lineCurveIntersectionTest();
    void curveCurveIntersectionTest();
    void curveSelfIntersectionTest();

    void QPainterPathCompatibilityTest();

    void allIntersectionPointsTest();

};

#endif // KISINTERSECTIONFINDERTEST_H
