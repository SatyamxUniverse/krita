/*
 *  SPDX-FileCopyrightText: 2021 Tanmay Chavan <earendil01tc@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kisbooleanoperationstest.h"
#include "KisBooleanOperations.h"

#include <QPainterPath>
#include <QTest>

KisBooleanOperationsTest::KisBooleanOperationsTest(QObject *parent)
    : QObject(parent) {}


void KisBooleanOperationsTest::curveCurveClippingTest() {

  QPainterPath ellipse1;
  QPainterPath ellipse2;

  ellipse1.addEllipse(QPointF(480, 100), 120, 30);
  ellipse2.addEllipse(QPointF(540, 100), 30, 120);

  KisBooleanOperations booleanOpsHandler;

  QPainterPath res = booleanOpsHandler.unite(ellipse1, ellipse2);

  QCOMPARE(res.elementCount(), 31);
}


void KisBooleanOperationsTest::lineCurveClippingTest() {

  QPainterPath ellipse;
  QPainterPath roundedRect;

  ellipse.addEllipse(QPointF(460, 80), 30, 120);
  roundedRect.addRoundedRect(360, 50, 200, 100, 20, 20);

  KisBooleanOperations booleanOpsHandler;

  QPainterPath res = booleanOpsHandler.uniteAndAdd(ellipse, roundedRect);

  QCOMPARE(res.elementCount(), 31);
}


void KisBooleanOperationsTest::lineLineClippingTest() {

  QPainterPath rect1;
  QPainterPath rect2;

  rect1.addRect(50, 50, 200, 100);
  rect2.addRect(50, 60, 100, 200);

  KisBooleanOperations booleanOpsHandler;

  QPainterPath res = booleanOpsHandler.intersect(rect1, rect2);

  QCOMPARE(res.elementCount(), 5);
}


void KisBooleanOperationsTest::seperatePathsTest() {

  QPainterPath ellipse1;
  QPainterPath ellipse2;

  ellipse1.addEllipse(QPointF(780, 100), 120, 30);
  ellipse2.addEllipse(QPointF(540, 100), 30, 120);

  KisBooleanOperations booleanOpsHandler;

  QPainterPath res = booleanOpsHandler.unite(ellipse1, ellipse2);

  QCOMPARE(res.elementCount(), 26);
}


// WIP: improve below two tests
void KisBooleanOperationsTest::booleanUnionTest() {

  QPainterPath ellipse;
  QPainterPath roundedRect;

  ellipse.addEllipse(QPointF(460, 80), 30, 120);
  roundedRect.addRoundedRect(360, 50, 200, 100, 20, 20);

  KisBooleanOperations booleanOpsHandler;

  QPainterPath res = booleanOpsHandler.uniteAndAdd(ellipse, roundedRect);

  QCOMPARE(res.elementCount(), 31);
}


void KisBooleanOperationsTest::booleanIntersectionTest() {

  QPainterPath rect1;
  QPainterPath rect2;

  rect1.addRect(50, 50, 200, 100);
  rect2.addRect(50, 60, 100, 200);

  KisBooleanOperations booleanOpsHandler;

  QPainterPath res = booleanOpsHandler.intersect(rect1, rect2);

  QCOMPARE(res.elementCount(), 5);
}


// this passes
void KisBooleanOperationsTest::booleanDifferenceTest() {

  QPainterPath rect1;
  QPainterPath rect2;

  rect1.addRect(50, 50, 200, 100);
  rect2.addRect(50, 60, 100, 200);

  KisBooleanOperations booleanOpsHandler;
  QPainterPath res = booleanOpsHandler.subtract(rect1, rect2);

  QCOMPARE(res.elementCount(), 7);
}


// this fails
void KisBooleanOperationsTest::booleanDifferenceTest2() {

    QPainterPath ellipse;
    QPainterPath roundedRect;

    ellipse.addEllipse(QPointF(460, 80), 30, 120);
    roundedRect.addRoundedRect(360, 50, 200, 100, 20, 20);

    KisBooleanOperations booleanOpsHandler;

    QPainterPath res = booleanOpsHandler.subtract(ellipse, roundedRect);

    /*
     * the value is currently 16. It should be 15, as there is one extraneous
     * lineTo element joining the two disjoint QPainterPaths after
     * subtraction.
     */
    QCOMPARE(res.elementCount(), 15);
}

QTEST_APPLESS_MAIN(KisBooleanOperationsTest);
#include "kisbooleanoperationstest.moc"
