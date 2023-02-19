/*
 *  SPDX-FileCopyrightText: 2021 Tanmay Chavan <earendil01tc@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisIntersectionFinderTest.h"
#include "defaulttool/KisIntersectionFinder.h"
#include "defaulttool/KisNumericalEngine.h"
//#include "painterpath.h"
//#include "painterpath_p.h"
#include <QPainterPath>
#include <QTest>

bool fuzzyIsNull(qreal d)
{
    if (sizeof(qreal) == sizeof(double))
        return qAbs(d) <= 1e-12;
    else
        return qAbs(d) <= 1e-5f;
}

bool comparePoints(const QPointF &a, const QPointF &b)
{
    // the epsilon in fuzzyIsNull is far too small for our use

//    return fuzzyIsNull(a.x() - b.x())
//           && fuzzyIsNull(a.y() - b.y());

    return qAbs(a.x() - b.x()) < 1e-6 && qAbs(a.y() - b.y()) < 1e-6;
}

KisIntersectionFinderTest::KisIntersectionFinderTest(QObject *parent) : QObject(parent)
{

}

void KisIntersectionFinderTest::lineLineIntersectionTest() {


    Line line1(QPointF(20,20), QPointF(60, 80));
    Line line2(QPointF(50,20), QPointF(50,100));


    KisIntersectionFinder KIF;
    QVector<QPointF> result = KIF.intersectionPoint(line1, line2);

    QPointF expectedResult(50, 65);

    QVERIFY(expectedResult == result.at(0));



}
void KisIntersectionFinderTest::lineCurveIntersectionTest() {

//    line curve: QPointF( 92.5507, 45.5 ),
    QPointF cp0(45,100); //(4, 1);
    QPointF cp1(45, 69.62); //(8, 4);
    QPointF cp2(69.62,45); //(6, 7);
    QPointF cp3(100, 45); //(2, 6);

    CubicBezier cb1 {cp3, cp2, cp1, cp0};

    qreal yC = 45.5;

    QLineF qL(QPointF(50,yC), QPointF(150, yC));
    Line l1(qL);

    KisIntersectionFinder KIF;

    QVector<QPointF> result = KIF.intersectionPoint(l1, cb1) ;

    QVector<QPointF> expectedResult{QPointF( 92.55069287, 45.5 )};

    QVERIFY(comparePoints(result.at(0), expectedResult.at(0)));

}


void KisIntersectionFinderTest::curveCurveIntersectionTest(){

    // curve curve: QPointF( 23.4613, 55.1994 ), curve curve: QPointF( 24.7472, 65.2252 ), curve curve: QPointF( 46.0768, 67.2854 ), curve curve: QPointF( 49.9148, 50.3836 ), curve curve: QPointF( 76.7174, 46.3192 ), curve curve: QPointF( 79.0565, 69.1624 ),
    QPointF cp0(20, 20); //(4, 1);
    QPointF cp1(40, 260); //(8, 4);
    QPointF cp2(60, -160); //(6, 7);
    QPointF cp3(80,80); //(2, 6);


    QPointF cpa(90, 25);
    QPointF cpb(300, 35);
    QPointF cpc(-180, 60);
    QPointF cpd(100, 70);

    CubicBezier cb1(cp0, cp1, cp2, cp3);
    CubicBezier cb2(cpa, cpb, cpc, cpd);

    KisIntersectionFinder KIF;

    QVector<QPointF> resultPoints = KIF.intersectionPoint(cb1, cb2);
    QVector<QPointF> expectedResult{QPointF( 23.46126967, 55.19943443 ), QPointF( 24.74716557, 65.2251848 ), QPointF( 46.07679144, 67.28542324 ), QPointF( 49.91476333, 50.38356123 ), QPointF( 76.71740217, 46.31916376 ), QPointF( 79.05649411, 69.16240838 )};


}
void KisIntersectionFinderTest::curveSelfIntersectionTest() {


}

// was used for MyPainterPath: obsolete for now.
void KisIntersectionFinderTest::QPainterPathCompatibilityTest() {

    QPainterPath QPath;
    QPath.addRoundedRect(50,50,100,100, 20, 20);

    QPainterPath KisPath;
    KisPath.addRoundedRect(50,50,100,100, 20, 20);

    //rounded-rect elements: 0, 2, 3, 3, 1, 2, 3, 3, 1, 2, 3, 3, 1, 2, 3, 3, 1
    QVERIFY(QPath.elementCount() == KisPath.elementCount());

    bool result = true;

    for (int i = 0; i < QPath.elementCount(); i++) {
        if (QPath.elementAt(i) != KisPath.elementAt(i)) {
            result = false;
            break;
        }
    }

    QVERIFY(result);
}

void KisIntersectionFinderTest::allIntersectionPointsTest() {

    QPainterPath horiRect;
    horiRect.addRoundedRect(50,50,100,100, 20, 20);

    QPainterPath ellipse;
    ellipse.addEllipse(QPointF(110, 100),79.79, 55);

    KisIntersectionFinder KIF(horiRect, ellipse);

    QVector<QPointF> result = KIF.findAllIntersections();
    QVector<QPointF> expectedResult{QPointF( 51.37900113, 62.68749982 ), QPointF( 76.71327585, 50 ), QPointF( 76.71327585, 150 ), QPointF( 51.37900113, 137.3125002 )};

    QVERIFY(result.size() == expectedResult.size());

    bool isEqual = true;

    for (int i = 0; i < result.size(); i++) {
        if(!comparePoints(result.at(i), expectedResult.at(i))) {
            isEqual = false;
            break;
        }
    }

    QVERIFY(isEqual);
}

SIMPLE_TEST_MAIN(KisIntersectionFinderTest, defaulttool)

