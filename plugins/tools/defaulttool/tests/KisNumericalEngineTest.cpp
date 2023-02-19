/*
 *  SPDX-FileCopyrightText: 2021 Tanmay Chavan <earendil01tc@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisNumericalEngineTest.h"
#include <QTest>
#include <iostream>

bool compareApprox(qreal num1, qreal num2) {
    return (qAbs(num1 - num2) < 1e-8);
}

NumericalEngineTest::NumericalEngineTest(QObject *parent) : QObject(parent)
{

}

void NumericalEngineTest::leadingCoefficientTest() {
    Expression ex1a;
    ex1a.add(Term(5, 2, 0));
    ex1a.add(Term(2, 1, 0));
    ex1a.add(Term(1, 0, 0));
    ex1a.subtract(Term(5,2,0));
    ex1a.sortExpress(Expression::descending);
    ex1a.validateLeadingCoefficient();

    Expression ex1b;
    ex1b.add(Term(2,1,0));
    ex1b.add(Term(1,0,0));
    ex1b.sortExpress(Expression::descending);
    ex1b.validateLeadingCoefficient();


    QVERIFY(ex1a == ex1b);

    Expression ex2a;

    ex2a.add(Term(0,3,0));
    ex2a.add(Term(-4,2,0));
    ex2a.add(Term(0,1,0));

    ex2a.validateLeadingCoefficient();

    Expression ex2b;

    ex2b.add(Term(56,3,0));
    ex2b.add(Term(-4, 2, 0));
    ex2b.add(Term(0,1,0));

    ex2b.validateLeadingCoefficient();

    QVERIFY(!(ex2a == ex2b));



}

void NumericalEngineTest::expressionEvaluateTest() {

    // x = 0, single variable
    Expression ex1a;
    ex1a.add(Term(90, 2, 0));
    ex1a.add(Term(-20, 1, 0));
    ex1a.add(Term(1, 0, 0));
    ex1a.subtract(Term(5,2,0));

    qreal sum = ex1a.evaluate(0);

    QVERIFY(sum == 1);

    // x = 2, single variable
    Expression ex2a;
    ex2a.add(Term(5,2,0));
    ex2a.add(Term(0,1,0));
    ex2a.add(Term(-10,0,0));

    qreal sum2 = ex2a.evaluate(2);

    QVERIFY( sum2 == 10);


    // x = 2, single variable but with non-zero y index
    // should give a warning
    Expression ex3a;
    ex3a.add(Term(5,2,0));
    ex3a.add(Term(0,1,0));
    ex3a.add(Term(-10,0,2));

    qreal sum3 = ex3a.evaluate(2);

    QVERIFY( sum3 == 10);

    // x = 2, single variable but with non-zero y index
    // should give a warning
    Expression ex4a;
    ex4a.add(Term(5,2,0));
    ex4a.add(Term(0,1,0));
    ex4a.add(Term(-10,0,2));

    qreal sum4 = ex4a.evaluate2d(2,5);

    QVERIFY( sum4 == -230);

}
//void NumericalEngineTest::additionTest() {
//    Expression ex1a;
//    ex1a.add(Term(5, 0, 2));
//    ex1a.add(Term(-1, 0, 2));

//}
void NumericalEngineTest::multiplicationTest() {

    Expression ex1a;

    ex1a.add(Term(90, 2, 0));
    ex1a.add(Term(-20, 1, 0));
    ex1a.add(Term(1, 0, 0));


    Expression ex1b;

    ex1b.add(Term(5,2,2));
//    ex1b.add(Term(0,1,0));
    ex1b.add(Term(-10,0,0));


    Expression result = ex1a * ex1b;

    Expression expectedResult;
    expectedResult.add(Term(450,4,2));
    expectedResult.add(Term(-900, 2, 0));
    expectedResult.add(Term(-100,3,2));
    expectedResult.add(Term(200,1,0));
    expectedResult.add(Term(5, 2, 2));
    expectedResult.add(Term(-10, 0, 0));

    QVERIFY(result == expectedResult);


}

void NumericalEngineTest::gslRootFindingTest() {

    qreal arr[10] = { 10, -200, 20,0,32.88,4,-8,6,5,10.00001 };
    qreal z[2 * 10];

    gsl_poly_complex_workspace* w = gsl_poly_complex_workspace_alloc (10);

    gsl_poly_complex_solve (arr, 10, w, z);

    gsl_poly_complex_workspace_free (w);

    QVector<qreal> realRootComponent;
    QVector<qreal> imaginaryRootComponent;

    for ( int i = 0; i < 10; i++ ) {
        realRootComponent.push_back (z[2 * i]);
        imaginaryRootComponent.push_back(z[2 * i + 1]);
        }

    QVector<qreal> expectedRealResult = {1.264781613,1.030798058,1.030798058,-0.2017115816,-0.2017115816,-0.9762505033,-0.9762505033,-1.520706656,0.05025359669,0};
    QVector<qreal> expectedImaginaryResult = {0,1.005489164,-1.005489164,1.44938063,-1.44938063,1.173438352,-1.173438352,0,0,0};

    std::sort(realRootComponent.begin(), realRootComponent.end());
    std::sort(imaginaryRootComponent.begin(), imaginaryRootComponent.end());
    std::sort(expectedRealResult.begin(), expectedRealResult.end());
    std::sort(expectedImaginaryResult.begin(), expectedImaginaryResult.end());

    bool isEqual = true;

    for (int i = 0; i < 10; i++) {
        if (!compareApprox(realRootComponent.at(i), expectedRealResult.at(i)) ||
                !compareApprox(imaginaryRootComponent.at(i), expectedImaginaryResult.at(i))) {
            isEqual = false;
            break;
        }
    }

    QVERIFY(isEqual);
}

SIMPLE_TEST_MAIN(KisIntersectionFinderTest, defaulttool)
