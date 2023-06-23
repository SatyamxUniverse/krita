/*
 *  SPDX-FileCopyrightText: 2023 Agata Cacko <cacko.azh@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "TestEllipseInPolygon.h"


#include <testui.h>
#include <testutil.h>


#include <kis_painting_assistant.h>
#include <PerspectiveBasedAssistantHelper.h>
#include <kis_algebra_2d.h>
#include <kis_global.h>

#include <ConcentricEllipseAssistant.h>
#include "EllipseInPolygon.h"
#include "PerspectiveBasedAssistantHelper.h"
#include <kis_painting_assistant.h>


void TestEllipseInPolygon::testRandomPointsOnEllipse()
{
    // better option: if it wasn't necessary to just guess the x range, but calculate it
    QRandomGenerator random(1234);
    int n = 100;
    int xmin = -1000;
    int xmax = 1000;
    auto rand = [random] () mutable {return random.generateDouble();};
    double eps = 1e-7;
    for(int i = 0; i < n; i++) {
        ConicFormula c;
        c.setFormulaActual(rand(), rand(), rand(), rand(), rand(), rand());
        QList<QPointF> points = c.getRandomPoints(random, n, xmin, xmax);
        //ENTER_FUNCTION() << "Number of points: " << points.count();
        for(int j = 0; j < points.count(); j++) {
            //if (qAbs(c.calculateFormulaForPoint(points[j])) >= eps) {
            //    ENTER_FUNCTION() << "value is: " << qAbs(c.calculateFormulaForPoint(points[j]));
            //}
            QVERIFY(qAbs(c.calculateFormulaForPoint(points[j])) < eps);
        }
    }
}

void TestEllipseInPolygon::testConicCalculationsPhaseAB()
{
    // DONE
    QRandomGenerator random(1234);
    int n = 100;
    auto rand = [random] () mutable {return random.generateDouble();};
    double eps = 1e-10;
    for(int i = 0; i < n; i++) {
        ConicFormula c;
        c.setFormulaActual(rand(), rand(), rand(), rand(), rand(), rand());
        QPointF point(rand(), rand());

        auto result = ConicCalculations::normalizeFormula(c, point);
        ConicFormula res = std::get<0>(result);
        QPointF resPoint = std::get<1>(result);
        double normalizeBy = std::get<2>(result);

        QCOMPARE(c.calculateFormulaForPoint(point), res.calculateFormulaForPoint(resPoint));

        if (qAbs(normalizeBy) > eps) {
            QCOMPARE(point, resPoint*normalizeBy);
            for(int i = 0; i < N; i++) {
                if (i >= 0 && i <= 2) {
                    QCOMPARE(c.getFormulaActual()[i], res.getFormulaActual()[i]/normalizeBy/normalizeBy);
                } else if (i == 3 || i == 4) {
                    QCOMPARE(c.getFormulaActual()[i], res.getFormulaActual()[i]/normalizeBy);
                } else {
                    QCOMPARE(c.getFormulaActual()[i], res.getFormulaActual()[i]);
                }
            }
        }
    }

}

void TestEllipseInPolygon::testConicCalculationsPhaseBC()
{
    // PASSES
    QRandomGenerator random(1234);
    int n = 100;
    auto rand = [random] () mutable {return random.generateDouble();};
    auto sq = [] (qreal a) {return a*a;};

    for(int i = 0; i < n; i++) {
        ConicFormula c;
        c.setFormulaActual(rand(), rand(), rand(), rand(), rand(), rand());
        QPointF point(rand(), rand());

        ConicFormula result = ConicCalculations::canonizeFormula(c);

        // TODO: find a few random points on the ellipse and check to make sure they are still there
        //

        qreal checkIfReallyCanonized = sq(result.A) + sq(result.B) + sq(result.C)
                + sq(result.D) + sq(result.E) + sq(result.F);

        QCOMPARE(checkIfReallyCanonized, 1.0);
        QVERIFY(result.isSpecial());
        QVERIFY(!c.isSpecial());

        double canonizingNumber = result.getFormulaActual()[0]/c.getFormulaActual()[0];
        for (int i = 0; i < N; i++) {
            QCOMPARE(result.getFormulaActual()[i], c.getFormulaActual()[i]*canonizingNumber);
        }

        QCOMPARE(c.calculateFormulaForPoint(point)*canonizingNumber, result.calculateFormulaForPoint(point));

    }

}

void TestEllipseInPolygon::testConicCalculationsPhaseCD()
{
    // NOT DONE
    // make sure the formula after rerotating is correct - the same points

    QRandomGenerator random(1234);
    int n = 100;
    auto rand = [random] () mutable {return random.generateDouble();};
    double eps = 1e-15;
    double bigEps = 1e-4;
    double xmin = -1000;
    double xmax = 1000;

    for(int i = 0; i < n; i++) {
        ConicFormula c = randomFormula(random, ConicFormula::SPECIAL);
        //QPointF point = randomPoint(random);
        QList<QPointF> pointsOnEllipse = c.getRandomPoints(random, 30, -1000, 1000);
        if (pointsOnEllipse.count() <= 0) continue;
        QPointF point = pointsOnEllipse[0];

        auto result = ConicCalculations::rotateFormula(c, point);
        ConicFormula rotatedFormula = std::get<0>(result);
        double K = std::get<1>(result);
        double L = std::get<2>(result);
        QPointF rotatedPoint = std::get<3>(result);

        // check if the point still solves the formula the same way
        //QCOMPARE(c.calculateFormulaForPoint(point), resFormula.calculateFormulaForPoint(resPoint));

        // check if derotated point is the same as rotated
        QPointF derotatedPoint = EllipseInPolygon::getRotatedPoint(rotatedPoint, K, L, true);
        QCOMPARE(point, derotatedPoint);

        // check if derotated formula is the same as rotated
        QVector<double> derotatedFormulaDoubles = EllipseInPolygon::getRotatedFormula(rotatedFormula.getFormulaActual(), K, L, true);
        //ENTER_FUNCTION() << ppVar(K) << ppVar(L);
        for(int i = 0; i < N; i++) {
            //QCOMPARE(resFormula2[i], c.getFormulaActual()[i]);
            //ENTER_FUNCTION() << ppVar(c.getFormulaActual()[i]) << ppVar(rotatedFormula.getFormulaActual()[i]) << ppVar(derotatedFormulaDoubles[i]);
        }



        ConicFormula derotatedFormula(derotatedFormulaDoubles, "", ConicFormula::ACTUAL);
        derotatedFormula.convertTo(ConicFormula::SPECIAL);

        c.Name = "c";
        //derotatedFormula.Name = "derotatedFormula";
        c.printOutInAllForms();
        //derotatedFormula.printOutInAllForms();



        // TODO: add checking for actual rotation as well
        // - check if the axis are horizontal and vertical

        double angleBefore = c.getAxisAngle();
        double angleAfter = rotatedFormula.getAxisAngle();
        double angleAfterRerotating = c.getAxisAngle();

        QCOMPARE(angleBefore, angleAfterRerotating);
        //QCOMPARE(angleAfter, 0);
        QVERIFY(qAbs(angleAfter) < eps);


        QList<QPointF> points = c.getRandomPoints(random, n, xmin, xmax);

        //c.printOutInAllForms();
        //derotatedFormula.printOutInAllForms();

        for (int j = 0; j < points.count(); j++) {

            QTransform t;
            t.rotate(kisRadiansToDegrees(-angleBefore));

            if (!(c.calculateFormulaForPoint(points[j]) < bigEps)
                || !(derotatedFormula.calculateFormulaForPoint(points[j]) < bigEps)
                || !(rotatedFormula.calculateFormulaForPoint(t.map(points[j])) < bigEps)
                || !(rotatedFormula.calculateFormulaForPoint(EllipseInPolygon::getRotatedPoint(points[j], K, L)) < bigEps)) {


                ENTER_FUNCTION() << ppVar(c.calculateFormulaForPoint(points[j]));

                //QPointF rotatedPointLocal = EllipseInPolygon::getRotatedPoint(points[j], K, L);

                ENTER_FUNCTION() << ppVar(derotatedFormula.calculateFormulaForPoint(points[j]));

                ENTER_FUNCTION() << ppVar(angleBefore) << ppVar(K) << ppVar(L);
                ENTER_FUNCTION() << ppVar(points[j]) << ppVar(t.map(points[j])) << ppVar(EllipseInPolygon::getRotatedPoint(points[j], K, L));
                ENTER_FUNCTION() << ppVar(rotatedFormula.calculateFormulaForPoint(t.map(points[j])));

                ENTER_FUNCTION() << ppVar(rotatedFormula.calculateFormulaForPoint(EllipseInPolygon::getRotatedPoint(points[j], K, L)));
            }

            QVERIFY(c.calculateFormulaForPoint(points[j]) < bigEps);
            QVERIFY(derotatedFormula.calculateFormulaForPoint(points[j]) < bigEps);
            QVERIFY(rotatedFormula.calculateFormulaForPoint(t.map(points[j])) < bigEps);
            QVERIFY(rotatedFormula.calculateFormulaForPoint(EllipseInPolygon::getRotatedPoint(points[j], K, L)) < bigEps);

        }




    }
}

void TestEllipseInPolygon::testConicCalculationsPhaseDE()
{
    QRandomGenerator random(1234);
    int n = 100;
    auto rand = [random] () mutable {return random.generateDouble();};
    auto sq = [] (qreal a) {return a*a;};

    int xmin = -1000;
    int xmax = 1000;

    double bigEps = 1e-12;




    // first manual test
    ConicFormula f;
    f.setFormulaActual(1, 2, 3, 4, 5, -1);
    f.convertTo(ConicFormula::SPECIAL);
    ConicFormula r = ConicCalculations::moveToOrigin(f, 10, 20);
    f.printOutInAllForms();
    r.printOutInAllForms();

    ENTER_FUNCTION() << f.calculateFormulaForPoint(QPointF(0, 1)) << r.calculateFormulaForPoint(QPointF(-10, -19));
    QTransform tr;
    tr.translate(-10, -20);
    ENTER_FUNCTION() << "trans " << tr.map(QPointF(0, 1));


    QList<QPointF> pointsOnCircle = f.getRandomPoints(random, n, -2, 2);
    for (int i = 0; i < pointsOnCircle.count(); i++) {
        QPointF p = tr.map(pointsOnCircle[i]);
        ENTER_FUNCTION() << ppVar(pointsOnCircle[i]) << ppVar(f.calculateFormulaForPoint(pointsOnCircle[i])) << ppVar(p) << ppVar(r.calculateFormulaForPoint(p));
        QVERIFY(qAbs(r.calculateFormulaForPoint(p)) < bigEps);
    }






// static ConicFormula moveToOrigin(ConicFormula formula, double u, double v); // D => E


    for(int i = 0; i < n; i++) {
        ConicFormula c = randomFormula(random, ConicFormula::SPECIAL);
        QPointF point = randomPoint(random);

        ConicFormula result = ConicCalculations::moveToOrigin(c, point.x(), point.y());

        //result.
        //QCOMPARE(c.calculateFormulaForPoint(point), result.calculateFormulaForPoint(QPointF(0, 0)));

        QList<QPointF> points = c.getRandomPoints(random, n, xmin, xmax);
        QTransform t;
        t.translate(-point.x(), -point.y());

        for (int j = 0; j < points.count(); j++) {
            QCOMPARE(c.calculateFormulaForPoint(points[j]), result.calculateFormulaForPoint(t.map(points[j])));
        }


    }

}

void TestEllipseInPolygon::testConicCalculationsPhaseEF()
{
    // DONE
    QRandomGenerator random(1234);
    int n = 100;

    for(int i = 0; i < n; i++) {
        ConicFormula c = randomFormula(random, ConicFormula::ACTUAL);
        QPointF point = randomPoint(random);
        QPointF pointSwapped = point;

        std::tuple<ConicFormula, bool> r = ConicCalculations::swapXY(c);

        if (std::get<1>(r)) {
            pointSwapped = QPointF(point.y(), point.x());
        }

        QCOMPARE(c.calculateFormulaForPoint(point), std::get<0>(r).calculateFormulaForPoint(pointSwapped));
    }

}

void TestEllipseInPolygon::testConicCalculationsPhaseFG()
{
    // DONE
    QRandomGenerator random(1234);
    int n = 100;

    for(int i = 0; i < n; i++) {
        ConicFormula c = randomFormula(random, ConicFormula::ACTUAL);
        QPointF point = randomPoint(random);
        QPointF pointSwapped = point;

        ConicFormula r = ConicCalculations::negateAllSigns(c);

        QCOMPARE(qAbs(c.calculateFormulaForPoint(point)), qAbs(r.calculateFormulaForPoint(pointSwapped)));
        QVERIFY(r.C >= 0);

    }
}

void TestEllipseInPolygon::testConicCalculationsPhaseGH()
{
    // DONE
    QRandomGenerator random(1234);
    int n = 100;

    for(int i = 0; i < n; i++) {
        ConicFormula c = randomFormula(random, ConicFormula::ACTUAL);
        QPointF point = randomPoint(random);
        QPointF pointSwapped = point;

        std::tuple<ConicFormula, bool> r = ConicCalculations::negateX(c);

        if (std::get<1>(r)) {
            pointSwapped = QPointF(-point.x(), point.y());
        }

        QCOMPARE(c.calculateFormulaForPoint(point), std::get<0>(r).calculateFormulaForPoint(pointSwapped));
    }
}

void TestEllipseInPolygon::testConicCalculationsPhaseHI()
{
    // DONE
    QRandomGenerator random(1234);
    int n = 100;

    for(int i = 0; i < n; i++) {
        ConicFormula c = randomFormula(random, ConicFormula::ACTUAL);
        QPointF point = randomPoint(random);
        QPointF pointSwapped = point;

        std::tuple<ConicFormula, bool> r = ConicCalculations::negateY(c);

        if (std::get<1>(r)) {
            pointSwapped = QPointF(point.x(), -point.y());
        }

        QCOMPARE(c.calculateFormulaForPoint(point), std::get<0>(r).calculateFormulaForPoint(pointSwapped));
    }
}

void TestEllipseInPolygon::testProjectInEllipseInPolygonHelper(EllipseInPolygon &eip, QPointF &original)
{
    // - project using EiP code
    // - project using our own test code, make sure it's the same
    // - transform the original point using the transformations
    // - check the point belongs to the ellipse
    // - check it's distance to focal points is not infinite (I guess I'll have to calculate focal points, then?)
    // - check the distance of the original point to the nearest point is sane
    // - check it's in the same quadrant of the ellipse (again, calculating :/ )

    QPointF projected = eip.project(original, &original);
    QPointF center = QPointF(eip.finalEllipseCenter[0], eip.finalEllipseCenter[1]);
    ENTER_FUNCTION() << kisDistance(projected, original) + kisDistance(projected, center) << kisDistance(original, center);
    // now, maybe angle between ori - center and ori-proj
    QPointF vector1 = center - projected;
    QPointF vector2 = center - original;
    //qreal cosinus = KisAlgebra2D::dotProduct(vec1, vec2)/(kisDistance(vec1, QPointF()), kisDistance(vec2, QPointF()));
    //ENTER_FUNCTION() << "cosinus is = " << cosinus;

    qreal angle = qAtan2(vector2.y(), vector2.x()) - atan2(vector1.y(), vector1.x());

    if (angle > M_PI) {
        angle -= 2 * M_PI;
    } else if (angle <= -M_PI) {
        angle += 2 * M_PI;
    }


    ENTER_FUNCTION() << "angle is: " << angle << kisRadiansToDegrees(angle);
    //maxAbsAngle = qMax(maxAbsAngle, qAbs(angle));

    QVERIFY(qAbs(angle) < M_PI_2);
    QVERIFY(eip.calculateFormula(projected) < 1e-13);
}

void TestEllipseInPolygon::testProjectInEllipseInPolygon()
{
    // test plan:
    // - get random points

    QRandomGenerator random(1234);

    int n = 100;
    //qreal maxAbsAngle = 0;
    for (int i = 0; i < n; i++) {
        EllipseInPolygon eip = randomEllipseInPolygon(random);
        QPointF original = randomPoint(random);
        testProjectInEllipseInPolygonHelper(eip, original);

    }

    //ENTER_FUNCTION() << "angles: " << ppVar(kisRadiansToDegrees(maxAbsAngle));

}

void TestEllipseInPolygon::testProjectInEllipseInPolygonManual()
{

}

void TestEllipseInPolygon::testDifferentCodes()
{
    QRandomGenerator random(1234);

    int n = 100;
    double eps = 1e-12;
    for (int i = 0; i < n; i++) {
        EllipseInPolygon eip = randomEllipseInPolygon(random);
        QPointF original = randomPoint(random);
        QPointF projectedSecond = eip.projectModifiedEberlySecond(original);
        QPointF projectedThird = eip.projectModifiedEberlyThird(original, &original);
        QPointF projectedFourth = eip.projectModifiedEberlyFourthNoDebug(original, &original);


        if (kisDistance(projectedSecond, projectedThird) >= eps || kisDistance(projectedSecond, projectedFourth) >= eps) {
            ENTER_FUNCTION() << ppVar(projectedSecond) << ppVar(projectedThird) << ppVar(projectedFourth)
                             << kisDistance(projectedSecond, projectedThird) << kisDistance(projectedSecond, projectedFourth);
            ENTER_FUNCTION() << "results from the conic: " << eip.calculateFormula(projectedSecond) << eip.calculateFormula(projectedThird) << eip.calculateFormula(projectedFourth);
            if (qAbs(eip.calculateFormula(projectedThird)) < qAbs(eip.calculateFormula(projectedSecond))) {
                ENTER_FUNCTION() << "SUCCESS: better precision!";
            }
        }
        //QVERIFY(kisDistance(projectedSecond, projectedThird) < eps);
        //QVERIFY(kisDistance(projectedSecond, projectedFourth)  < eps);
        QVERIFY(kisDistance(projectedThird, projectedFourth)  < eps);


    }
}

void TestEllipseInPolygon::testMirroredOrNot()
{
    // <handles><handle id="0" x="633.960" y="753.840"/><handle id="1" x="758.165" y="753.840"/><handle id="2" x="875.880" y="900.720"/><handle id="3" x="521.640" y="898.560"/></handles>

    EllipseInPolygon eip;
    QPolygonF poly;
    poly << QPointF(633.960,753.840) << QPointF(758.165, 753.840) << QPointF(875.880, 900.720) << QPointF(521.640, 898.560);
    PerspectiveBasedAssistantHelper::CacheData cache;
    PerspectiveBasedAssistantHelper::updateCacheData(cache, poly);
    eip.updateToPolygon(poly, cache.horizon);

    EllipseInPolygon eipConcentric;
    EllipseInPolygon eipConcentricMirrored;

    QPointF point(0,0);

    eipConcentric.updateToPointOnConcentricEllipse(eip.originalTransform, point, cache.horizon, false);
    eipConcentricMirrored.updateToPointOnConcentricEllipse(eip.originalTransform, point, cache.horizon, true);

    ENTER_FUNCTION() << eipConcentric.onTheCorrectSideOfHorizon(point) << eipConcentric.horizonLineSign(point);
    ENTER_FUNCTION() << eipConcentricMirrored.onTheCorrectSideOfHorizon(point) << eipConcentricMirrored.horizonLineSign(point);

    ENTER_FUNCTION() << eipConcentric.horizon << eipConcentricMirrored.horizon;
    ENTER_FUNCTION() << eipConcentric.horizonFormula[0] << eipConcentric.horizonFormula[1] << eipConcentric.horizonFormula[2];
    ENTER_FUNCTION() << eipConcentricMirrored.horizonFormula[0] << eipConcentricMirrored.horizonFormula[1] << eipConcentricMirrored.horizonFormula[2];
    ENTER_FUNCTION() << eipConcentric.horizonLineSign(eipConcentric.horizon.p1()) << eipConcentric.horizonLineSign(eipConcentric.horizon.p2());
    ENTER_FUNCTION() << eipConcentric.horizonLineSign(eipConcentricMirrored.horizon.p1()) << eipConcentric.horizonLineSign(eipConcentricMirrored.horizon.p2());


    auto calculateHorizonResult = [] (EllipseInPolygon& eip, QPointF point) {
        return eip.horizonFormula[0]*point.x() + eip.horizonFormula[1]*point.y() + eip.horizonFormula[2];
    };
    ENTER_FUNCTION() << calculateHorizonResult(eipConcentric, eipConcentric.horizon.p1()) << calculateHorizonResult(eipConcentric, eipConcentric.horizon.p2());
    ENTER_FUNCTION() << calculateHorizonResult(eipConcentricMirrored, eipConcentricMirrored.horizon.p1()) << calculateHorizonResult(eipConcentricMirrored, eipConcentricMirrored.horizon.p2());


}

void TestEllipseInPolygon::testDiagonalHorizon()
{
    EllipseInPolygon eip;
    QPolygonF poly;

    // for it to be diagonal, we gotta find the

    poly << QPointF(633.960,753.840) << QPointF(758.165, 753.840) << QPointF(875.880, 900.720) << QPointF(521.640, 898.560);
    PerspectiveBasedAssistantHelper::CacheData cache;
    PerspectiveBasedAssistantHelper::updateCacheData(cache, poly);
    eip.updateToPolygon(poly, cache.horizon);

    EllipseInPolygon eipConcentric;
    EllipseInPolygon eipConcentricMirrored;

    QPointF point(0,0);

    eipConcentric.updateToPointOnConcentricEllipse(eip.originalTransform, point, cache.horizon, false);
    eipConcentricMirrored.updateToPointOnConcentricEllipse(eip.originalTransform, point, cache.horizon, true);

    ENTER_FUNCTION() << eipConcentric.onTheCorrectSideOfHorizon(point) << eipConcentric.horizonLineSign(point);
    ENTER_FUNCTION() << eipConcentricMirrored.onTheCorrectSideOfHorizon(point) << eipConcentricMirrored.horizonLineSign(point);

    ENTER_FUNCTION() << eipConcentric.horizon << eipConcentricMirrored.horizon;
    ENTER_FUNCTION() << eipConcentric.horizonFormula[0] << eipConcentric.horizonFormula[1] << eipConcentric.horizonFormula[2];
    ENTER_FUNCTION() << eipConcentricMirrored.horizonFormula[0] << eipConcentricMirrored.horizonFormula[1] << eipConcentricMirrored.horizonFormula[2];
    ENTER_FUNCTION() << eipConcentric.horizonLineSign(eipConcentric.horizon.p1()) << eipConcentric.horizonLineSign(eipConcentric.horizon.p2());
    ENTER_FUNCTION() << eipConcentric.horizonLineSign(eipConcentricMirrored.horizon.p1()) << eipConcentric.horizonLineSign(eipConcentricMirrored.horizon.p2());


    auto calculateHorizonResult = [] (EllipseInPolygon& eip, QPointF point) {
        return eip.horizonFormula[0]*point.x() + eip.horizonFormula[1]*point.y() + eip.horizonFormula[2];
    };
    ENTER_FUNCTION() << calculateHorizonResult(eipConcentric, eipConcentric.horizon.p1()) << calculateHorizonResult(eipConcentric, eipConcentric.horizon.p2());
    ENTER_FUNCTION() << calculateHorizonResult(eipConcentricMirrored, eipConcentricMirrored.horizon.p1()) << calculateHorizonResult(eipConcentricMirrored, eipConcentricMirrored.horizon.p2());
}

ConicFormula TestEllipseInPolygon::randomFormula(QRandomGenerator &random, ConicFormula::TYPE type)
{
    auto rand = [random] () mutable {return random.generateDouble();};
    ConicFormula c;
    c.setFormulaActual(rand(), rand(), rand(), rand(), rand(), rand());
    if (type == ConicFormula::SPECIAL) {
        c.convertTo(type);
    }
    return c;
}

QPointF TestEllipseInPolygon::randomPoint(QRandomGenerator &random)
{
    auto rand = [random] () mutable {return random.generateDouble();};
    return QPointF(rand(), rand());
}

QPolygonF TestEllipseInPolygon::randomTetragon(QRandomGenerator &random) {
    // TODO: it doesn't actually give a proper convex tetragon
    // but it's a good enough guess for tests for now
    QPointF current = TestEllipseInPolygon::randomPoint(random);

    QPolygonF poly;
    poly << current;

    qreal angleSum = 0;


    // just like in the algorithm to find whether the polygon is convex or not
    for (int i = 0; i < 3; i++) {
        qreal distance = random.generateDouble();
        qreal angle = angleSum + random.bounded(0, M_PI);

        //
        QPointF next = current + distance*QPointF(qCos(angle), qSin(angle));

        poly << next;
        current = next;
        angleSum = angle;
    }
    return poly;

}

EllipseInPolygon TestEllipseInPolygon::randomEllipseInPolygon(QRandomGenerator &random)
{
    int attempts = 100; // usually 0-5 attempts needed
    for (int i = 0; i < attempts; i++) {

        QPolygonF randTetragon = randomTetragon(random);
        QList<KisPaintingAssistantHandleSP> handles;
        Q_FOREACH(QPointF p, randTetragon) {
            handles << KisPaintingAssistantHandleSP(new KisPaintingAssistantHandle(p));
        }
        QPolygonF polygon;

        bool r = PerspectiveBasedAssistantHelper::getTetragon(handles, true, polygon);
        if (r) {
            // good polygon!
            PerspectiveBasedAssistantHelper::CacheData cache;
            PerspectiveBasedAssistantHelper::updateCacheData(cache, polygon);

            EllipseInPolygon eip;
            r = eip.updateToPolygon(polygon, cache.horizon);

            if (r) {
                //ENTER_FUNCTION() << "Success at " << i;
                return eip;
            } else {
                //ENTER_FUNCTION() << "Failure " << ppVar(i) << " because of no square to quad" << polygon;
            }

        } else {
            //ENTER_FUNCTION() << "Failure  " << ppVar(i) << ", apparently it's not convex?" << polygon;
        }

    }
    KIS_ASSERT(false && "No valid ellipses in N attempts");
    return EllipseInPolygon();
}

KISTEST_MAIN(TestEllipseInPolygon)
