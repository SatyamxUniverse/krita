/*
 *  SPDX-FileCopyrightText: 2021 Tanmay Chavan <earendil01tc@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef NUMERICALENGINETEST_H
#define NUMERICALENGINETEST_H

#include <QObject>
#include <QTest>
//#include "defaulttool/KisNumericalEngine.h"

class NumericalEngineTest : public QObject
{
    Q_OBJECT
public:
    explicit NumericalEngineTest(QObject *parent = nullptr);

private Q_SLOTS:
    
    void leadingCoefficientTest();
    void expressionEvaluateTest();
//    void additionTest();
    void multiplicationTest();

    void gslRootFindingTest();


};

#endif // NUMERICALENGINETEST_H


