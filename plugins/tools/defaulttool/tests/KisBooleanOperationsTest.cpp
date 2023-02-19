/*
 *  SPDX-FileCopyrightText: 2021 Tanmay Chavan <earendil01tc@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisBooleanOperationsTest.h"
#include "defaulttool/KisBooleanOperations.h"

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

  QVector<int> elementTypes{0, 2, 3, 3, 2, 3, 3, 2, 3, 3, 2, 3, 3, 2, 3, 3, 2, 3, 3, 2, 3, 3, 2, 3, 3, 2, 3, 3, 2, 3, 3};

  for (int i = 0; i < res.elementCount(); i++) {

      Q_ASSERT(res.elementAt(i).type == elementTypes.at(i));
  }
}


void KisBooleanOperationsTest::lineCurveClippingTest() {

  QPainterPath ellipse;
  QPainterPath roundedRect;

  ellipse.addEllipse(QPointF(460, 80), 30, 120);
  roundedRect.addRoundedRect(360, 50, 200, 100, 20, 20);

  KisBooleanOperations booleanOpsHandler;

  QPainterPath res = booleanOpsHandler.intersectAndAdd(ellipse, roundedRect);

  QCOMPARE(res.elementCount(), 15);

  QVector<int> elementTypes{0, 2, 3, 3, 1, 2, 3, 3, 2, 3, 3, 1, 2, 3, 3};

  for (int i = 0; i < res.elementCount(); i++) {

      Q_ASSERT(res.elementAt(i).type == elementTypes.at(i));
  }
}


void KisBooleanOperationsTest::lineLineClippingTest() {

  QPainterPath rect1;
  QPainterPath rect2;

  rect1.addRect(50, 50, 200, 100);
  rect2.addRect(50, 60, 100, 200);

  KisBooleanOperations booleanOpsHandler;

  QPainterPath res = booleanOpsHandler.intersect(rect1, rect2);

  QCOMPARE(res.elementCount(), 5);

  QVector<int> elementTypes{0, 1, 1, 1, 1};

  for (int i = 0; i < res.elementCount(); i++) {

      Q_ASSERT(res.elementAt(i).type == elementTypes.at(i));
  }
}


void KisBooleanOperationsTest::seperatePathsTest() {

  QPainterPath ellipse1;
  QPainterPath ellipse2;

  ellipse1.addEllipse(QPointF(780, 100), 120, 30);
  ellipse2.addEllipse(QPointF(540, 100), 30, 120);

  KisBooleanOperations booleanOpsHandler;

  QPainterPath res = booleanOpsHandler.unite(ellipse1, ellipse2);

  QCOMPARE(res.elementCount(), 26);

  QVector<int> elementTypes{0, 2, 3, 3, 2, 3, 3, 2, 3, 3, 2, 3, 3, 0, 2, 3, 3, 2, 3, 3, 2, 3, 3, 2, 3, 3};

  for (int i = 0; i < res.elementCount(); i++) {

      Q_ASSERT(res.elementAt(i).type == elementTypes.at(i));
  }
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

  QVector<int> elementTypes{0, 2, 3, 3, 1, 2, 3, 3, 2, 3, 3, 1, 2, 3, 3, 1, 2, 3, 3, 1, 2, 3, 3, 2, 3, 3, 1, 2, 3, 3, 1};

  for (int i = 0; i < res.elementCount(); i++) {

      Q_ASSERT(res.elementAt(i).type == elementTypes.at(i));
  }
}


void KisBooleanOperationsTest::booleanIntersectionTest() {

  QPainterPath rect1;
  QPainterPath rect2;

  rect1.addRoundedRect(50, 50, 200, 100,20,20);
  rect2.addRoundedRect(50, 60, 100, 200,20,20);

  KisBooleanOperations booleanOpsHandler;

  QPainterPath res = booleanOpsHandler.intersect(rect2, rect1);

  QVector<int> elementTypes{0, 2, 3, 3, 1, 2, 3, 3, 1, 1, 2, 3, 3, 1};

  QCOMPARE(res.elementCount(), 14);

  for (int i = 0; i < res.elementCount(); i++) {

      Q_ASSERT(res.elementAt(i).type == elementTypes.at(i));
  }
}


void KisBooleanOperationsTest::booleanDifferenceTest() {

  QPainterPath rect1;
  QPainterPath rect2;

  rect1.addRect(50, 50, 200, 100);
  rect2.addRect(50, 60, 100, 200);

  KisBooleanOperations booleanOpsHandler;
  QPainterPath res = booleanOpsHandler.subtract(rect1, rect2);

  QVector<int> elementTypes{0, 1, 1, 1, 1, 1, 1};

  QCOMPARE(res.elementCount(), 7);

  for (int i = 0; i < res.elementCount(); i++) {

      Q_ASSERT(res.elementAt(i).type == elementTypes.at(i));
  }
}

void KisBooleanOperationsTest::booleanDifferenceTest2() {

    QPainterPath ellipse;
    QPainterPath roundedRect;

    ellipse.addEllipse(QPointF(460, 80), 30, 120);
    roundedRect.addRoundedRect(360, 50, 200, 100, 20, 20);

    KisBooleanOperations booleanOpsHandler;

    QPainterPath res = booleanOpsHandler.subtract(ellipse, roundedRect);

    QVector<int> elementTypes{0, 2, 3, 3, 2, 3, 3, 1, 2, 3, 3, 2, 3, 3, 1};
    QCOMPARE(res.elementCount(), 14);

    for (int i = 0; i < res.elementCount(); i++) {

        Q_ASSERT(res.elementAt(i).type == elementTypes.at(i));
    }

    /*
     * the last curve is wrongly placed.
     */
    QCOMPARE(res.elementCount(), 16);
}

SIMPLE_TEST_MAIN(KisBooleanOperationsTest, defaulttool)
