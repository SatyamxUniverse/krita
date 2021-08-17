/*
 *  SPDX-FileCopyrightText: 2021 Tanmay Chavan <earendil01tc@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kisintersectionfinder.h"
#include "bezier.h"
#include "databuffer.h"
#include "kisnumericalengine.h"
#include "kispathclipper.h"
#include "vectorpath.h"

#include <chrono>

// helper functions

static inline bool fuzzyIsNull(qreal d)
{
    if (sizeof(qreal) == sizeof(double)) {
        return qAbs(d) <= 1e-12;
    }

    else {
        return qAbs(d) <= 1e-5f;
    }

}

static inline bool comparePoints(const QPointF &a, const QPointF &b)
{
    // the epsilon in fuzzyIsNull is far too small for our use

    return qAbs(a.x() - b.x()) < 1e-6 && qAbs(a.y() - b.y()) < 1e-6;
}

// returns dot product of two point vectors
static qreal dot(const QPointF &a, const QPointF &b)
{
    return a.x() * b.x() + a.y() * b.y();
}




BuildingBlock::BuildingBlock() {}

BuildingBlock::BuildingBlock(int id, BuildingBlockElementType buildingblocktype, QPointF p1) : ID(id), type(buildingblocktype), moveToPoint(p1){

}

BuildingBlock::BuildingBlock(int id, BuildingBlockElementType buildingblocktype, Line l1) : ID(id), type(buildingblocktype), l(l1) {

}

BuildingBlock::BuildingBlock(int id, BuildingBlockElementType buildingblocktype, CubicBezier c1) : ID(id), type(buildingblocktype) {

    cb = c1;
}

bool BuildingBlock::isMoveTo() { return type == MoveToElement; }
bool BuildingBlock::isLineTo() { return type == LineToElement; }
bool BuildingBlock::isCurveTo() { return type == CurveToElement; }

QPointF BuildingBlock::getMoveTo() {

        Q_ASSERT(type == MoveToElement);
        return moveToPoint;

}

Line BuildingBlock::getLineTo() {

        Q_ASSERT(type == LineToElement);
        return l;

}

CubicBezier BuildingBlock::getCurveTo() {

        Q_ASSERT(type == CurveToElement);
        return cb;

}





KisClippingVertex::KisClippingVertex(QPointF pt, VertexType type, int idFirst, int idSecond, qreal para1, qreal para2) :
    point(pt), type(type), idFirstBuildingBlock(idFirst), idSecondBuildingBlock(idSecond),
    parameterFirstBuildingBlock(para1), parameterSecondBuildingBlock(para2)
{

}


/*
 * Class KisIntersectionFinder:
 * Finds all the intersection points between two QPainterPaths. Then it
 * processes them so that the elements in the shapes are splitted about every
 * intersection point.
 */

/*
 * The constructor converts the paths to SubjectShape and ClipShape, QVectors
 * containing BuildingBlock, which are elements of the shapes. BuildingBlock
 * also contains Line and CubicBezier objects.
 */
KisIntersectionFinder::KisIntersectionFinder(QPainterPath subject, QPainterPath clip) {
    // converts QPainterPath (same as QPainterPath) to a sequence of Line and CubicBezier objects

    int segId = 0;
    int netSegId = 0;
    QPointF currPoint(0,0);
    QPointF lastPoint(0,0);

    subToClipIntersectionVertices.reserve(subject.elementCount());

    // The first element should be moveTo. If not, the shape will start from
    // the origin (0,0)
    if ( !subject.elementAt(0).isMoveTo() ) {

        subjectVertices << KisClippingVertex(QPointF(0,0), KisClippingVertex::VertexType::regularVertex);
    }

    for (int i = 0; i < subject.elementCount(); i++) {
        QPainterPath::Element e = subject.elementAt(i);

        // ignore moveTo elements as there is no segment for intersection
        if (e.isMoveTo()) {
            currPoint = e;

            BuildingBlock moveEle(segId,MoveToElement,currPoint);
            subjectShape.push_back(moveEle);

            netShape.push_back(moveEle);

            subjectVertices << KisClippingVertex(currPoint, KisClippingVertex::VertexType::regularVertex);

            lastPoint = currPoint;

        }

        else if (e.isLineTo()) {
            currPoint = QPointF(e.x, e.y);
            BuildingBlock lineEle(segId,LineToElement,Line(lastPoint, currPoint));
            subjectShape.push_back(lineEle);

            netShape.push_back(lineEle);

            subjectVertices << KisClippingVertex(currPoint, KisClippingVertex::VertexType::regularVertex);

            lastPoint = currPoint;
        }

        // Qt elavates quadratic Bezier curves to cubic curves inside QPainterPath, hence only cubic curves with four control points are expected
        else if (e.isCurveTo()) {

            QPointF cp2(e);
            QPointF cp3(subject.elementAt(i+1));
            QPointF cp4(subject.elementAt(i+2));
            CubicBezier cubic(lastPoint, cp2, cp3, cp4);

            BuildingBlock curveEle(segId,CurveToElement,cubic);
            subjectShape.push_back(curveEle);
            netShape.push_back(curveEle);
            subjectVertices << KisClippingVertex(cp4, KisClippingVertex::VertexType::regularVertex);

            lastPoint = cp4;
            i += 2;
        }


        segId++;
        netSegId++;
    }

//    subjectShape << BuildingBlock(subjectVertices.size(), MoveToElement, subjectVertices.first().point);
    subjectVertices << KisClippingVertex(subjectVertices.first().point, KisClippingVertex::VertexType::regularVertex);

    // reset counter variables and repeat procedure for clip shape
    lastPoint = QPointF(0, 0);
    currPoint = QPointF(0, 0);
    segId = 0;

    if ( !clip.elementAt(0).isMoveTo() ) {

        clipVertices << KisClippingVertex(QPointF(0,0), KisClippingVertex::VertexType::regularVertex);
    }

    for (int i = 0; i < clip.elementCount(); i++) {
        QPainterPath::Element e = clip.elementAt(i);


        if (e.isMoveTo()) {
            currPoint = e;

            BuildingBlock moveEle(segId,MoveToElement,currPoint);
            clipShape.push_back(moveEle);

            netShape.push_back(moveEle);

            clipVertices << KisClippingVertex(currPoint, KisClippingVertex::VertexType::regularVertex);

            lastPoint = currPoint;
        }

        else if (e.isLineTo()) {
            currPoint = QPointF(e.x, e.y);
            BuildingBlock lineEle(segId,LineToElement,Line(lastPoint, currPoint));
            clipShape.push_back(lineEle);

            netShape.push_back(lineEle);

            clipVertices << KisClippingVertex(currPoint, KisClippingVertex::VertexType::regularVertex);

            lastPoint = currPoint;
        }

        else if (e.isCurveTo()) {

            QPointF cp2(e);
            QPointF cp3(clip.elementAt(i+1));
            QPointF cp4(clip.elementAt(i+2));
            CubicBezier cubic(lastPoint, cp2, cp3, cp4);
            i += 2;
            BuildingBlock curveEle(segId,CurveToElement,cubic);
            clipShape.push_back(curveEle);

            netShape.push_back(curveEle);

            clipVertices << KisClippingVertex(cp4, KisClippingVertex::VertexType::regularVertex);

            lastPoint = cp4;

        }

        segId++;
        netSegId++;
    }

//    clipShape << BuildingBlock(clipVertices.size(), MoveToElement, clipVertices.first().point);
    clipVertices << KisClippingVertex(clipVertices.first().point, KisClippingVertex::VertexType::regularVertex);

    for( int i = 0; i < subject.elementCount() + clip.elementCount(); i++ ) {
        QVector<KisClippingVertex> v;
        subToClipIntersectionVertices.push_back(v);
        clipToSubIntersectionVertices.push_back(v);
    }

}


// Checks if two line segments intersect. Same as QPathClipper::linesIntersect()
bool KisIntersectionFinder::linesIntersect(const QLineF &a, const QLineF &b) const
{

    const QPointF p1 = a.p1();
    const QPointF p2 = a.p2();

    const QPointF q1 = b.p1();
    const QPointF q2 = b.p2();

    if (comparePoints(p1, p2) || comparePoints(q1, q2))
        return false;

    const bool p1_equals_q1 = comparePoints(p1, q1);
    const bool p2_equals_q2 = comparePoints(p2, q2);

    if (p1_equals_q1 && p2_equals_q2)
        return true;

    const bool p1_equals_q2 = comparePoints(p1, q2);
    const bool p2_equals_q1 = comparePoints(p2, q1);

    if (p1_equals_q2 && p2_equals_q1)
        return true;

    const QPointF pDelta = p2 - p1;
    const QPointF qDelta = q2 - q1;

    const qreal par = pDelta.x() * qDelta.y() - pDelta.y() * qDelta.x();

    if (qFuzzyIsNull(par)) {
        const QPointF normal(-pDelta.y(), pDelta.x());

        // coinciding?
        if (qFuzzyIsNull(dot(normal, q1 - p1))) {
            const qreal dp = dot(pDelta, pDelta);

            const qreal tq1 = dot(pDelta, q1 - p1);
            const qreal tq2 = dot(pDelta, q2 - p1);

            if ((tq1 > 0 && tq1 < dp) || (tq2 > 0 && tq2 < dp))
                return true;

            const qreal dq = dot(qDelta, qDelta);

            const qreal tp1 = dot(qDelta, p1 - q1);
            const qreal tp2 = dot(qDelta, p2 - q1);

            if ((tp1 > 0 && tp1 < dq) || (tp2 > 0 && tp2 < dq))
                return true;
        }

        return false;
    }

    const qreal invPar = 1 / par;

    const qreal tp = (qDelta.y() * (q1.x() - p1.x()) -
                      qDelta.x() * (q1.y() - p1.y())) * invPar;

    if (tp < 0 || tp > 1)
        return false;

    const qreal tq = (pDelta.y() * (q1.x() - p1.x()) -
                      pDelta.x() * (q1.y() - p1.y())) * invPar;

    return tq >= 0 && tq <= 1;
}



// Finds intersection points between two cubic Bezier curves. returns a QVector of the intersection points represented by QPointF class.
QVector<KisClippingVertex> KisIntersectionFinder::intersectionPoint(CubicBezier c1, CubicBezier c2){

    QVector<KisClippingVertex> res;
//    auto t0 = high_resolution_clock::now ();
    QVector<QPointF> result;

    if (!c1.boundingBox().intersects(c2.boundingBox())) {
        // if the bounding boxes don't intersect, the curves surely will not intersect
        if (!c1.boundingBox().contains(c2.boundingBox()) && !c2.boundingBox().contains(c1.boundingBox())){
            return res; //ult;

        }
    }
//    auto t2 = high_resolution_clock::now();

//    auto duration = duration_cast<microseconds>( t2 - t0 );
//    std::cout << "box check prelim: " << duration.count() << " microsecs" << std::endl;

    QVector<qreal> roots = c1.findRoots(c2);

//    auto t3 = high_resolution_clock::now();

//    duration = duration_cast<microseconds>( t3 - t2 );
//    std::cout << "find roots: " << duration.count() << " microsecs" << std::endl;

    QPointF ptOfIntersection;

    Q_FOREACH(qreal root, roots) {
        ptOfIntersection.setX(c1.parametric_x.evaluate(root));
        ptOfIntersection.setY(c1.parametric_y.evaluate(root));

        qreal supposedParameter = c2.inversionEquationEvaluated(ptOfIntersection);
        if (supposedParameter <= 1 && supposedParameter >= 0) {
            // the parameter lies within the range, hence it lies on the curve bounded by the first and fourth end points

            if (!comparePoints(c1.cp0, ptOfIntersection) && !comparePoints(c1.cp3, ptOfIntersection)
                    && !comparePoints(ptOfIntersection, c2.cp0) && !comparePoints(ptOfIntersection, c2.cp3)) {
                // if the intersection point lies too close to the end points of both curves, reject it
                result.push_back(ptOfIntersection);

                res.push_back(KisClippingVertex(ptOfIntersection,KisClippingVertex::regularIntersection,-1,-1,root, supposedParameter));
            }
        }
    }

//    auto t4 = high_resolution_clock::now ();

//    duration = duration_cast<microseconds>( t4 - t3 );
//    std::cout << "point validity: " << duration.count() << " microsecs" << std::endl;

    return res;

}


QVector<KisClippingVertex> KisIntersectionFinder::intersectionPoint(Line l1, CubicBezier c2) {
    QVector<QPointF> result;
    QVector<KisClippingVertex> res;

    QRectF curveBoundingBox = c2.boundingBox();
    QLineF qLine = l1.getQLine();

    QVector<qreal> roots;

    if (curveBoundingBox.contains(qLine.p1()) || curveBoundingBox.contains(qLine.p2())) {
        // at least one of the end points of the line lies in the bounding box, hence proceed to calculate intersection
        roots = l1.findRoots(c2);
    }



    else {
        QLineF topEdge{curveBoundingBox.topLeft(), curveBoundingBox.topRight()};
        QLineF rightEdge{curveBoundingBox.topRight(), curveBoundingBox.bottomRight()};
        QLineF bottomEdge{curveBoundingBox.bottomRight(), curveBoundingBox.bottomLeft()};
        QLineF leftEdge{curveBoundingBox.bottomLeft(), curveBoundingBox.topLeft()};

            if (linesIntersect(qLine,topEdge) || linesIntersect(qLine,rightEdge) || linesIntersect(qLine,bottomEdge) || linesIntersect(qLine,leftEdge)) {

            // line intersects at least one side of the curve, proceed to calculate intersection
            roots = l1.findRoots(c2);
        }
        else {
            // line does not intersect bounding box nor lies within it, so it surely doesn't intersect the curve
            return res;
        }
    }

    if(roots.isEmpty()) {
        return res;
    }

    Q_FOREACH (qreal root, roots) {
        QPointF currIntersectionPoint{c2.getParametricX().evaluate(root) ,  c2.getParametricY().evaluate(root)};

        if (l1.checkIntersection(currIntersectionPoint)) {

            if (!comparePoints(currIntersectionPoint, c2.cp0) && !comparePoints(currIntersectionPoint, c2.cp3)
                && !comparePoints(currIntersectionPoint, qLine.p1()) && !comparePoints(currIntersectionPoint, qLine.p2())) {

                // if the intersection point lies too close to the end points of both curves, reject it
                result.push_back(currIntersectionPoint);

                res.push_back(KisClippingVertex(currIntersectionPoint,KisClippingVertex::regularIntersection,-1,-1,root, root));
            }
        }

    }

    return res;
}

struct QIntersection
{
    qreal alphaA;
    qreal alphaB;

    QPointF pos;
};


QVector<QPointF> KisIntersectionFinder::intersectionPoint(Line l1,Line l2) {
    QVector<QPointF> intersections;

    QLineF a = l1.getQLine();
    QLineF b = l2.getQLine();

    const QPointF p1 = a.p1();
    const QPointF p2 = a.p2();

    const QPointF q1 = b.p1();
    const QPointF q2 = b.p2();

    if (comparePoints(p1, p2) || comparePoints(q1, q2))
        return intersections;

    const bool p1_equals_q1 = comparePoints(p1, q1);
    const bool p2_equals_q2 = comparePoints(p2, q2);

    if (p1_equals_q1 && p2_equals_q2)
        return intersections;

    const bool p1_equals_q2 = comparePoints(p1, q2);
    const bool p2_equals_q1 = comparePoints(p2, q1);

    if (p1_equals_q2 && p2_equals_q1)
        return intersections;

    const QPointF pDelta = p2 - p1;
    const QPointF qDelta = q2 - q1;

    const qreal par = pDelta.x() * qDelta.y() - pDelta.y() * qDelta.x();

    if (qFuzzyIsNull(par)) {
        const QPointF normal(-pDelta.y(), pDelta.x());

        // coinciding?
        if (qFuzzyIsNull(dot(normal, q1 - p1))) {
            const qreal invDp = 1 / dot(pDelta, pDelta);

            const qreal tq1 = dot(pDelta, q1 - p1) * invDp;
            const qreal tq2 = dot(pDelta, q2 - p1) * invDp;

            if (tq1 > 0 && tq1 < 1) {
                QIntersection intersection;
                intersection.alphaA = tq1;
                intersection.alphaB = 0;
                intersection.pos = q1;

                intersections.push_back(q1);
            }

            if (tq2 > 0 && tq2 < 1) {
                QIntersection intersection;
                intersection.alphaA = tq2;
                intersection.alphaB = 1;
                intersection.pos = q2;

                intersections.push_back(q2);
            }

            const qreal invDq = 1 / dot(qDelta, qDelta);

            const qreal tp1 = dot(qDelta, p1 - q1) * invDq;
            const qreal tp2 = dot(qDelta, p2 - q1) * invDq;

            if (tp1 > 0 && tp1 < 1) {
                QIntersection intersection;
                intersection.alphaA = 0;
                intersection.alphaB = tp1;
                intersection.pos = p1;

                intersections.push_back(p1);
            }

            if (tp2 > 0 && tp2 < 1) {
                QIntersection intersection;
                intersection.alphaA = 1;
                intersection.alphaB = tp2;
                intersection.pos = p2;
                intersections.push_back(p2);
            }
        }

        return intersections;
    }

    // if the lines are not parallel and share a common end point, then they
    // don't intersect
    if (p1_equals_q1 || p1_equals_q2 || p2_equals_q1 || p2_equals_q2)
        return intersections;


    const qreal tp = (qDelta.y() * (q1.x() - p1.x()) -
                      qDelta.x() * (q1.y() - p1.y())) / par;
    const qreal tq = (pDelta.y() * (q1.x() - p1.x()) -
                      pDelta.x() * (q1.y() - p1.y())) / par;

    if (tp<0 || tp>1 || tq<0 || tq>1)
        return intersections;

    const bool p_zero = qFuzzyIsNull(tp);
    const bool p_one = qFuzzyIsNull(tp - 1);

    const bool q_zero = qFuzzyIsNull(tq);
    const bool q_one = qFuzzyIsNull(tq - 1);

    if ((q_zero || q_one) && (p_zero || p_one))
        return intersections;

    QPointF pt;
    if (p_zero) {
        pt = p1;
    } else if (p_one) {
        pt = p2;
    } else if (q_zero) {
        pt = q1;
    } else if (q_one) {
        pt = q2;
    } else {
        pt = q1 + (q2 - q1) * tq;  // intersection = p1 + (p2 - p1) * t
    }
    intersections.push_back(pt);

    return intersections;
}

QVector<KisClippingVertex> KisIntersectionFinder::intersectionPoint(CubicBezier c1, Line l2) {

    return intersectionPoint(l2, c1);
}

// function to find intersection points for two BuildingBlocks
QVector<KisClippingVertex> KisIntersectionFinder::findIntersectionPoints(BuildingBlock e1, BuildingBlock e2) {
    QVector<QPointF> intersectionPoints;
    QVector<KisClippingVertex> res;

    if (e1.isCurveTo()) {

        if (e2.isCurveTo()) {
            res = intersectionPoint(e1.getCurveTo(), e2.getCurveTo());
        }
        else {
            res = intersectionPoint(e2.getLineTo(), e1.getCurveTo());
        }
    }

    else {
        if (e2.isCurveTo()) {
            res = intersectionPoint(e1.getLineTo(), e2.getCurveTo());
        }
        else {
            intersectionPoints = intersectionPoint(e1.getLineTo(), e2.getLineTo());

            Q_FOREACH(QPointF p, intersectionPoints) {
                res.append(KisClippingVertex(p, KisClippingVertex::regularIntersection));
            }
        }
    }

    QVector<KisClippingVertex> res2;
    Q_FOREACH(KisClippingVertex v, res) {
        KisClippingVertex v2(v);

        v2.setIdFirstBuildingBlock(e1.ID);
        v2.setIdSecondBuildingBlock(e2.ID);
        res2.push_back(v2);
    }

    return res2;
}



// finds all the intersection points between two PainterPaths
QVector<KisClippingVertex> KisIntersectionFinder::findAllIntersections() {

    QVector<KisClippingVertex> allIntersectionPoints;


    for (int i = 0; i < subjectShape.size(); i++) {

        BuildingBlock b1 = subjectShape.at(i);

        if (b1.isMoveTo()) {
            continue;
        }

        // for self intersections of first shape
        for (int j = i + 1; j < subjectShape.size(); j++) {

            BuildingBlock b2 = subjectShape.at(j);

            if (b2.isMoveTo()) {
                continue;
            }
            QVector<KisClippingVertex> rnd = findIntersectionPoints(b1, b2);
            subToClipIntersectionVertices[b1.ID].append(rnd); // b1.ID
            subToClipIntersectionVertices[b2.ID].append(rnd); // b2.ID
            allIntersectionPoints.append(rnd);
        }

        // for intersections with second shape
        for (int k = 0; k < clipShape.size(); k++) {

            BuildingBlock b2 = clipShape.at(k);

            if (b2.isMoveTo()) {
                continue;
            }

            QVector<KisClippingVertex> rnd = findIntersectionPoints(b1, b2);
            QVector<KisClippingVertex> rnd2 = findIntersectionPoints(b2, b1);
            subToClipIntersectionVertices[b1.ID].append(rnd);
            clipToSubIntersectionVertices[b2.ID].append(rnd2);
            allIntersectionPoints.append(rnd);

        }
    }

    // for self intersections of second shape
    for (int i = 0; i < clipShape.size(); i++) {

        BuildingBlock b1 = clipShape.at(i);

        if (b1.isMoveTo()) {
            continue;
        }

        for (int j = i + 1; j < clipShape.size(); j++) {

            BuildingBlock b2 = clipShape.at(j);

            if (b2.isMoveTo()) {
                continue;
            }


            QVector<KisClippingVertex> rnd = findIntersectionPoints(b1, b2);
            allIntersectionPoints.append(rnd);

            subToClipIntersectionVertices[b1.ID].append(rnd);
            subToClipIntersectionVertices[b2.ID].append(rnd);
        }
    }

    return allIntersectionPoints;
}

void KisIntersectionFinder::processShapes() {

    int newId = 0;
    QPointF lastPt(0,0);

    if (subjectShape.first().isLineTo()) {

    }

    Q_FOREACH(BuildingBlock b, subjectShape ) {
        QVector<KisClippingVertex> intersections =  subToClipIntersectionVertices.at(b.ID);

            if (b.isMoveTo()) {
                BuildingBlock nb(newId++, BuildingBlockElementType::MoveToElement, b.getMoveTo());
                subjectShapeProcessed.push_back(nb);
                lastPt = b.getMoveTo();

                // is ignored for greiner hormann approach

            }

            else if (b.isCurveTo()) {

//                subjectVerticesWithIntersections << b.getCurveTo().cp0;

                QVector<KisIntersectionPoint> points;
                if (b.getCurveTo().cp0 != lastPt) {
                    BuildingBlock nb(newId++, BuildingBlockElementType::MoveToElement, b.getCurveTo().cp0);

                    subjectVerticesWithIntersections << KisClippingVertex(b.getCurveTo().cp0, KisClippingVertex::regularVertex);
                }

                Q_FOREACH( KisClippingVertex v, intersections) {

                    KisIntersectionPoint pt;
                    pt.parameter = v.parameterFirstBuildingBlock;
                    pt.point = v.point;
                    points.push_back(pt);
                }

                std::sort(points.begin(), points.end());
                auto uq = std::unique(points.begin(), points.begin() + points.count());
                points.resize(std::distance(points.begin(), uq));


                QVector<CubicBezier> res{ b.getCurveTo() };
                KisIntersectionPoint currIntPoint;



                while (points.size()) {

                    currIntPoint = points.takeFirst();

                    QVector<CubicBezier> tempRes = res.takeLast().splitCurve(currIntPoint.parameter, currIntPoint.point);
                    res.append(tempRes);
                }

                Q_FOREACH(CubicBezier cb, res) {

                    subjectShapeProcessed.append(BuildingBlock(newId++,BuildingBlockElementType::CurveToElement, cb));
                    clippedCurves.append(cb);
                }

                lastPt = b.getCurveTo().cp3;
                subjectVerticesWithIntersections << KisClippingVertex(lastPt, KisClippingVertex::regularVertex);



            }

            else if (b.isLineTo()) {

                if (b.getLineTo().getQLine().p1() != lastPt) {
                    BuildingBlock nb(newId++, BuildingBlockElementType::MoveToElement, b.getLineTo().getQLine().p1());
                }

                QVector<QPointF> intPoints;

                Q_FOREACH( KisClippingVertex v, intersections) {

                    intPoints.push_back(v.point);
                }
                QVector<Line> res = b.getLineTo().splitLine(intPoints);

                Q_FOREACH(Line ln, res) {

                    subjectShapeProcessed.append(BuildingBlock(newId++,BuildingBlockElementType::LineToElement, ln));
                }
                lastPt = b.getLineTo().getQLine().p1();
            }
    }




    newId = 0;
    lastPt.setX(0);
    lastPt.setY(0);

    Q_FOREACH(BuildingBlock b, clipShape ) {
        QVector<KisClippingVertex> intersections =  clipToSubIntersectionVertices.at(b.ID);
        if (3000000) {

            if (b.isMoveTo()) {
                BuildingBlock nb(newId++, BuildingBlockElementType::MoveToElement, b.getMoveTo());
                clipShapeProcessed.push_back(nb);
                lastPt = b.getMoveTo();

            }

            else if (b.isCurveTo()) {
                if (b.getCurveTo().cp0 != lastPt) {
                    BuildingBlock nb(newId++, BuildingBlockElementType::MoveToElement, b.getCurveTo().cp0);
                }

                QVector<KisIntersectionPoint> points;

                Q_FOREACH( KisClippingVertex v, intersections) {

                    KisIntersectionPoint pt;
                    pt.parameter = v.parameterFirstBuildingBlock;
                    pt.point = v.point;
                    points.push_back(pt);

                }

                std::sort(points.begin(), points.end());
                auto uq = std::unique(points.begin(), points.begin() + points.count());
                points.resize(std::distance(points.begin(), uq));

                QVector<CubicBezier> res{ b.getCurveTo() };

                KisIntersectionPoint currPoint;



                while (points.size()) {


                    currPoint = points.takeFirst();


                    QVector<CubicBezier> tempRes = res.takeLast().splitCurve(currPoint.parameter, currPoint.point);
                    res.append(tempRes);
                }


                Q_FOREACH(CubicBezier cb, res) {

                    clipShapeProcessed.append(BuildingBlock(newId++,BuildingBlockElementType::CurveToElement, cb));
                    clippedCurves.append(cb);
                }
                lastPt = b.getCurveTo().cp3;
            }

            else if (b.isLineTo()) {

                if (b.getLineTo().getQLine().p1() != lastPt) {
                    BuildingBlock nb(newId++, BuildingBlockElementType::MoveToElement, b.getLineTo().getQLine().p1());
                }

                QVector<QPointF> intPoints;

                Q_FOREACH( KisClippingVertex v, intersections) {

                    intPoints.push_back(v.point);
                }
                QVector<Line> res = b.getLineTo().splitLine(intPoints);

                Q_FOREACH(Line ln, res) {

                    clipShapeProcessed.append(BuildingBlock(newId++,BuildingBlockElementType::LineToElement, ln));
                }
                lastPt = b.getLineTo().getQLine().p1();
            }
        }

        else {
        }
    }


    netShape = subjectShape + clipShape;
}



static bool isLine(const QBezier &bezier)
{
    const bool equal_1_2 = comparePoints(bezier.pt1(), bezier.pt2());
    const bool equal_2_3 = comparePoints(bezier.pt2(), bezier.pt3());
    const bool equal_3_4 = comparePoints(bezier.pt3(), bezier.pt4());

    // point?
    if (equal_1_2 && equal_2_3 && equal_3_4)
        return true;

    if (comparePoints(bezier.pt1(), bezier.pt4()))
        return equal_1_2 || equal_3_4;

    return (equal_1_2 && equal_3_4) || (equal_1_2 && equal_2_3) || (equal_2_3 && equal_3_4);
}

/*
 * following functions convert our shapes to PainterPaths
 *
 */


QPainterPath KisIntersectionFinder::subjectShapeToPath(){

    QPainterPath subjectPath;

    if (subjectShapeProcessed.isEmpty()) {
        return subjectPath;
    }

//    std::cout << "firstBlock:  ";

    BuildingBlock firstBuildingBlock = subjectShapeProcessed.first();

    if (firstBuildingBlock.isMoveTo()) {

        subjectPath.moveTo(firstBuildingBlock.getMoveTo());
        netRegularVertices << firstBuildingBlock.getMoveTo();

//        std::cout << "moveTo  " << firstBuildingBlock.getMoveTo().x() << " " << firstBuildingBlock.getMoveTo().y() << std::endl;
    }

    else if (subjectShapeProcessed.first().isLineTo()) {

        subjectPath.moveTo(firstBuildingBlock.getLineTo().getQLine().p1());
        subjectPath.lineTo(firstBuildingBlock.getLineTo().getQLine().p2());

//        std::cout << "moveTo  " << firstBuildingBlock.getLineTo().getQLine().x1() << " " << firstBuildingBlock.getLineTo().getQLine().y1()
//                  << "lineTo  " << firstBuildingBlock.getLineTo().getQLine().x2() << " " << firstBuildingBlock.getLineTo().getQLine().y2()  << std::endl;

        netRegularVertices << firstBuildingBlock.getLineTo().getQLine().p1() << firstBuildingBlock.getLineTo().getQLine().p2();
    }

    else {

        CubicBezier cbFirst = firstBuildingBlock.getCurveTo();

        QBezier bezier = QBezier::fromPoints(cbFirst.cp0 , cbFirst.cp1, cbFirst.cp2, cbFirst.cp3);
        subjectPath.moveTo(cbFirst.cp0);
        netRegularVertices << cbFirst.cp0;

//        std::cout << "lineTo  " << cbFirst.cp0.x() << " " << cbFirst.cp0.y()
//                  << "curveTo  " << cbFirst.cp3.x() << " " << cbFirst.cp3.y() << std::endl;

        if (isLine(bezier)) {

            subjectPath.lineTo(cbFirst.cp3);
        }

        else {

            subjectPath.cubicTo(cbFirst.cp1, cbFirst.cp2, cbFirst.cp3);
    }

        netRegularVertices << cbFirst.cp3;
    }


    for (int i = 1; i < subjectShapeProcessed.size(); i++ ) {

        if (subjectShapeProcessed[i].isMoveTo()) {

            subjectPath.moveTo(subjectShapeProcessed[i].getMoveTo());
            netRegularVertices << subjectShapeProcessed[i].getMoveTo();

//            std::cout << "moveTo  " << subjectShapeProcessed[i].getMoveTo().x() << " " << subjectShapeProcessed[i].getMoveTo().y() << std::endl;
        }

        else if (subjectShapeProcessed[i].isLineTo()) {

            subjectPath.lineTo(subjectShapeProcessed[i].getLineTo().getQLine().p2());
            netRegularVertices << subjectShapeProcessed[i].getLineTo().getQLine().p2();

//            std::cout << "moveTo  " << subjectShapeProcessed[i].getLineTo().getQLine().x1() << " " << subjectShapeProcessed[i].getLineTo().getQLine().y1()
//                      << "lineTo  " << subjectShapeProcessed[i].getLineTo().getQLine().x2() << " " << subjectShapeProcessed[i].getLineTo().getQLine().y2()  << std::endl;
        }

        else {

            CubicBezier cb = subjectShapeProcessed[i].getCurveTo();
            QBezier bezier = QBezier::fromPoints(cb.cp0 , cb.cp1, cb.cp2, cb.cp3);

//            std::cout << "lineTo  " << cb.cp0.x() << " " << cb.cp0.y()
//                      << "curveTo  " << cb.cp3.x() << " " << cb.cp3.y() << std::endl;

            if (isLine(bezier)) {

                subjectPath.lineTo(cb.cp3);
                netRegularVertices  << cb.cp3;
            }

            else {

                subjectPath.cubicTo(cb.cp1, cb.cp2, cb.cp3);
                netRegularVertices  << cb.cp3;

            }
        }

    }

    subjectPath.closeSubpath();
    return subjectPath;
}




QPainterPath KisIntersectionFinder::clipShapeToPath(){

    QPainterPath clipPath;

    if (clipShapeProcessed.isEmpty()) {
        return clipPath;
    }

    BuildingBlock firstBuildingBlock = clipShapeProcessed.first();

    if (firstBuildingBlock.isMoveTo()) {

        clipPath.moveTo(firstBuildingBlock.getMoveTo());
        netRegularVertices << firstBuildingBlock.getMoveTo();
    }

    else if (clipShapeProcessed.first().isLineTo()) {

        clipPath.moveTo(firstBuildingBlock.getLineTo().getQLine().p1());
        clipPath.lineTo(firstBuildingBlock.getLineTo().getQLine().p2());

        netRegularVertices << firstBuildingBlock.getLineTo().getQLine().p1() << firstBuildingBlock.getLineTo().getQLine().p2();
    }

    else {

        CubicBezier cbFirst = firstBuildingBlock.getCurveTo();
        QBezier bezier = QBezier::fromPoints(cbFirst.cp0 , cbFirst.cp1, cbFirst.cp2, cbFirst.cp3);

        clipPath.moveTo(cbFirst.cp0);
        netRegularVertices << cbFirst.cp0;

        if (isLine(bezier)) {

            clipPath.lineTo(cbFirst.cp3);
            netRegularVertices << cbFirst.cp3;
        }

        else {

            clipPath.cubicTo(cbFirst.cp1, cbFirst.cp2, cbFirst.cp3);
            netRegularVertices << cbFirst.cp3;


        }
    }




    for (int i = 1; i < clipShapeProcessed.size(); i++ ) {

        if (clipShapeProcessed[i].isMoveTo()) {

            clipPath.moveTo(clipShapeProcessed[i].getMoveTo());
            netRegularVertices << clipShapeProcessed[i].getMoveTo();
        }

        else if (clipShapeProcessed[i].isLineTo()) {

            clipPath.lineTo(clipShapeProcessed[i].getLineTo().getQLine().p2());
            netRegularVertices << clipShapeProcessed[i].getLineTo().getQLine().p2();
        }

        else {

            CubicBezier cb = clipShapeProcessed[i].getCurveTo();

            QBezier bezier = QBezier::fromPoints(cb.cp0 , cb.cp1, cb.cp2, cb.cp3);


            if (isLine(bezier)) {

                clipPath.lineTo(cb.cp3);
                netRegularVertices  << cb.cp3;
            }

            else {

                QPointF currentPoint, lastPoint;

                clipPath.cubicTo(cb.cp1, cb.cp2, cb.cp3);
                netRegularVertices << cb.cp3;

            }
        }
    }

    clipPath.closeSubpath();
    return clipPath;

}

QVector<QPointF> KisIntersectionFinder::getRegularVertices() {

    return netRegularVertices;
}

// could potentially fail for parallel curves
QVector<CubicBezier> KisIntersectionFinder::getClippedCurves() {

    return clippedCurves;
}

// could potentially fail for parallel curves
QVector<CubicBezierData> KisIntersectionFinder::getClippedCurvesData() {

    Q_FOREACH(CubicBezier cb, clippedCurves) {

        CubicBezierData cbd(cb.cp0, cb.cp1, cb.cp2, cb.cp3);
        clippedCurvesData << cbd;

        allCurveEndPoints << cb.cp0 << cb.cp3;
    }

    return clippedCurvesData;
}

QVector<QPointF> KisIntersectionFinder::getAllCurveEndPoints() {

    auto ip = std::unique(allCurveEndPoints.begin(), allCurveEndPoints.begin() + allCurveEndPoints.size());

    allCurveEndPoints.resize(std::distance(allCurveEndPoints.begin(), ip));

    return allCurveEndPoints;
}


/*
 * WIP: now works for Krita's generated QPainterPaths. Still needs some
 * improvements, especially for clipping shapes with consecutive lineTo and
 * curveTo elements.
 * Have kept most of the raw debugging messages for ease of understanding,
 * I will remove them later.
 *
 * This is the final clipping function. The motivation is to parse the give
 * QPainterPath and find all edges generated after flattening. If a match is
 * found, it should be replaced by the respective curve. All of the curves are
 * found in the initial processShapes() function, and are stored inside
 * clippedCurvesData.
 *
 * However, the algorithm is still buggy. It fails mainly for the difference
 * operation. I guess it is not able to process moveTo elements properly. Some
 * Improvements in it or an altogether new function could solve the issue.
 */
QPainterPath KisIntersectionFinder::resubstituteCurves(QPainterPath path) {

    if (!path.elementCount()) {
        return path;
    }

    QPointF currPoint;
    QPointF lastPoint;
    QPainterPath bufferPath;

    bool firstFound = false;

    QVector<CubicBezierData> allCurveData = getClippedCurvesData();
    QVector<QPointF> curveEndPts = getAllCurveEndPoints();

    QVector<QPointF> endPts;

    QPainterPath substitutedRes;

    for (int j = 0; j < path.elementCount(); j++) {


        QPainterPath::Element ele = path.elementAt(j);
        currPoint = ele;

        /*
         * if a curve is present, the clipping has been trivially handled by
         * Qt. This happens in the case where the two paths don't intersect.
         * Thus, return the path as it is.
         */
        if (ele.isCurveTo() || ele.type == QPainterPath::CurveToDataElement) {

            return path;
        }

        if (ele.isMoveTo()) {

            if (endPts.size() == 1) { //special case for the entry condition

                bufferPath.moveTo(ele);
                bufferPath.closeSubpath();
                substitutedRes.addPath(bufferPath);
                bufferPath.clear();
            }

            else { // endPts.size should be 0

                substitutedRes.moveTo(ele);
                substitutedRes.closeSubpath();
                //substitutedRes.addPath(bufferPath);
                bufferPath.clear();
            }
            endPts.clear();

            if (curveEndPts.contains(currPoint)) {
                endPts << currPoint;
            }
            lastPoint = currPoint;
            continue;
        }


        if (curveEndPts.contains(currPoint)) {

            endPts << currPoint;
//            std::cout << "currPoint inside curveEndPts: " << currPoint.x() << " " << currPoint.y() << std::endl;

        }

        if (endPts.size() == 2) {

            bool curveFound = false;
//            std::cout << "second point done: " << currPoint.x() << " " << currPoint.y() << std::endl;

            if (!firstFound) {

                endPts.replace(0, path.elementAt(0));
                endPts.replace(1, currPoint);
//                std::cout << "curve under focus: " << endPts.first().x() << " " << endPts.first().y()
//                          << "  " << endPts.last().x() << " " << endPts.last().y() << std::endl;
                firstFound = true;
            }

            Q_FOREACH( CubicBezierData cbd, allCurveData) {

////                std::cout << "curve" << cbd.cp0.x() << "," << cbd.cp0.y() << " & " << cbd.cp3.x() << "," << cbd.cp3.y() << std::endl;
                bool curveForward = (cbd.cp0 == endPts.at(1)) && (cbd.cp3 == endPts.at(0));
                bool curveBackward =  (cbd.cp3 == endPts.at(1)) && (cbd.cp0 == endPts.at(0));

                if (curveForward||curveBackward) {

                    if (curveBackward) {
                        substitutedRes.moveTo(cbd.cp0);
                        substitutedRes.cubicTo(cbd.cp1, cbd.cp2, cbd.cp3);
                    }
                    else {
                        substitutedRes.moveTo(cbd.cp3);
                        substitutedRes.cubicTo(cbd.cp2, cbd.cp1, cbd.cp0);
                    }

                    QPointF newPt = endPts.last();
                    endPts.clear();
                    endPts << newPt;
                    bufferPath.clear();

                    lastPoint = currPoint;
                    curveFound = true;
                    break;

                }

                else {
//                    std::cout << "these points were not a match : " << endPts.at(0).x() << " " << endPts.at(0).y()
//                              << "  " << endPts.at(1).x() << " " <<endPts.at(1).y() << " with:\n" << cbd.cp0.x() <<" " << cbd.cp0.y()
//                              << ",  " << cbd.cp3.x() << " " << cbd.cp3.y() << std::endl;
                }
            }

            if (curveFound == false) {
                substitutedRes.addPath(bufferPath);
                substitutedRes.lineTo(endPts.last());
                QPointF newPt = endPts.last();
                endPts.clear();
                endPts << newPt;
                bufferPath.clear();
                lastPoint = currPoint;

//                std::cout << "curve rejected :" << endPts.first().x() << " " << endPts.first().y()
//                          << "  " << endPts.last().x() << " " << endPts.last().y() << std::endl;
            }
        }

        else if (endPts.size() == 1) {

            if (ele.isMoveTo()) {

                bufferPath.moveTo(currPoint);
                lastPoint = currPoint;
                continue;
            }

            bufferPath.lineTo(currPoint);
            lastPoint = currPoint;
        }

        else {

            if (ele.isMoveTo()) {
                substitutedRes.moveTo(currPoint);
                lastPoint = currPoint;
                continue;
            }
            substitutedRes.lineTo(currPoint);
            lastPoint = currPoint;
        }
    }

    /*
     * The algorithm still has a bug, which inserts moveTo(0,0) randomly,
     * generally between consecutive curveTo and lineTo elements. This routine
     * removes them.
     */

    QPainterPath processedRes;

    if (substitutedRes.elementAt(0).type == QPainterPath::MoveToElement && substitutedRes.elementAt(0) == QPointF(0,0)) {
        processedRes.moveTo(path.elementAt(0));

        for (int j = 1; j < substitutedRes.elementCount(); j++) {

            QPainterPath::Element ele = substitutedRes.elementAt(j);

            if (ele.isMoveTo()) {
                if (ele == QPointF(0, 0)){
                    continue; // hack alert!
             }
                processedRes.moveTo(ele);
            }

            else if (ele.isLineTo()) {
                processedRes.lineTo(ele);
            }

            else if (ele.isCurveTo()) {
                processedRes.cubicTo(ele, substitutedRes.elementAt(j+1), substitutedRes.elementAt(j+2));
                j+=2;
            }
        }
    }

    else {
        processedRes = substitutedRes;
    }

    return processedRes;
}


