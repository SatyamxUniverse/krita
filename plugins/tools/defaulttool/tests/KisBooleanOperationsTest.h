/*
 *  SPDX-FileCopyrightText: 2021 Tanmay Chavan <earendil01tc@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KISBOOLEANOPERATIONSTEST_H
#define KISBOOLEANOPERATIONSTEST_H

#include <QObject>
#include <QtTest/QtTest>

class KisBooleanOperationsTest : public QObject
{
    Q_OBJECT
public:
    explicit KisBooleanOperationsTest(QObject *parent = nullptr);

private Q_SLOTS:

    void curveCurveClippingTest();
    void lineCurveClippingTest();
    void lineLineClippingTest();
    void seperatePathsTest();

    void booleanUnionTest();
    void booleanIntersectionTest();
    void booleanDifferenceTest();

    void booleanDifferenceTest2();

};

#endif // KISBOOLEANOPERATIONSTEST_H
