/*
 *  SPDX-FileCopyrightText: 2021 Tanmay Chavan <earendil01tc@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisBooleanOperations.h"
#include "KisIntersectionFinder.h"
#include "KisPathClipper.h"
#include <QPainterPath>

#include "kis_algebra_2d.h"
#include <iostream>
#include <iomanip>

KisBooleanOperations::KisBooleanOperations()
{
}
KisBooleanOperations::~KisBooleanOperations()
{
}

QPainterPath KisBooleanOperations::unite(const QPainterPath &sub, const QPainterPath &clip)
{
    if (sub.isEmpty()) {
        return clip;
    }

    if (clip.isEmpty()) {
        return sub;
    }

    if (sub.isEmpty() && clip.isEmpty()) {
        return QPainterPath();
    }

    KisIntersectionFinder KIF(sub, clip);
    KIF.findAllIntersections();

    KIF.processShapes();

    QPainterPath splittedSub = KIF.subjectShapeToPath();
    QPainterPath splittedClip = KIF.clipShapeToPath();

    QPainterPath res = splittedSub | splittedClip;
    QPainterPath res1 = splittedClip | splittedSub;

    QPainterPath processedRes = KIF.resubstituteCurves(res);
    QPainterPath processedRes1 = KIF.resubstituteCurves(res1);

    QPainterPath::Element ele;

//  the incorrect path has some un-substituted elements
//    QPainterPath trueRes = (processedRes.elementCount() < processedRes1.elementCount()) ? processedRes : processedRes1;

//    GreinerHormannClipping gcv(splittedSub, splittedClip, intPoints);
//    trueRes = gcv.unite();

//    auto subjectList = gcv.getSubList();
//    auto clipList = gcv.getClipList();

    for (int i = 0; i < processedRes.elementCount(); i++) {

        ele = processedRes.elementAt(i);

        if (!splittedSub.contains(ele) && !splittedClip.contains(ele)) {

            return processedRes1;
        }
    }


    return processedRes;
}

QPainterPath KisBooleanOperations::intersect(QPainterPath &sub, QPainterPath &clip)
{

    if (sub.isEmpty() || clip.isEmpty()) {
        return QPainterPath();
    }

    KisIntersectionFinder KIF(sub, clip);
    QVector<KisClippingVertex> intPoints = KIF.findAllIntersections();
    KIF.processShapes();

    QPainterPath splittedSub = KIF.subjectShapeToPath();
    QPainterPath splittedClip = KIF.clipShapeToPath();

    QPainterPath res = splittedSub & splittedClip;
    QPainterPath res1 = splittedClip & splittedSub;

    QPainterPath processedRes = KIF.resubstituteCurves(res);
    QPainterPath processedRes1 = KIF.resubstituteCurves(res1);

    QPainterPath::Element ele;

//  the incorrect path has some un-substituted elements
    for (int i = 0; i < processedRes.elementCount(); i++) {

        ele = processedRes.elementAt(i);

        if (!splittedSub.contains(ele) && !splittedClip.contains(ele)) {

            return processedRes1;
        }
    }

//    QPainterPath trueRes = (processedRes.elementCount() < processedRes1.elementCount()) ? processedRes : processedRes1;

    GreinerHormannClipping ghc(splittedSub, splittedClip, intPoints);

    return ghc.unite();
}

QPainterPath KisBooleanOperations::subtract(QPainterPath &sub, QPainterPath &clip)
{
    if (sub.isEmpty()) {
        return QPainterPath();
    }

    if (clip.isEmpty()) {
        return sub;
    }


    KisIntersectionFinder KIF(sub, clip);
    KIF.findAllIntersections();
    KIF.processShapes();

    QPainterPath splittedSub = KIF.subjectShapeToPath();
    QPainterPath splittedClip = KIF.clipShapeToPath();

    QPainterPath res = splittedSub - splittedClip;

    bool subtractionOp = true;

    QPainterPath processedRes = KIF.resubstituteCurves(res, subtractionOp);

    return processedRes;
}

QPainterPath KisBooleanOperations::uniteAndAdd(QPainterPath &sub, const QPainterPath &clip)
{
    if (sub.isEmpty()) {
        sub = clip;
        return sub;
    }

    QPainterPath res = unite(sub, clip);
    sub = res;

    return sub;
}

QPainterPath KisBooleanOperations::intersectAndAdd(QPainterPath &sub, QPainterPath &clip)
{
    if (sub.isEmpty() || clip.isEmpty()) {
        return QPainterPath();
    }

    QPainterPath res = intersect(sub, clip);
    sub = res;

    return sub;
}

QPainterPath KisBooleanOperations::subtractAndAdd(QPainterPath &sub, QPainterPath &clip)
{
    if (clip.isEmpty()) {
        return sub;
    }

    QPainterPath res = subtract(sub, clip);
    sub = res;

    return sub;
}

QPainterPath KisBooleanOperations::testAdd()
{
    //        QPainterPath sample1;
    //        QPainterPath sample2;
    //        QPainterPath sample3;

    //        sample1.addRoundedRect(-500,-500,2000,1000,100,100);
    //        sample2.addEllipse(QPointF(1500, 1000), 500, 1200);

    //        std::cout << "\n\n-----" << std::endl;

    //        for (int i = 0; i < sample1.elementCount(); i++) {
    //            QPainterPath::Element ele = sample1.elementAt(i);
    //            std::cout << ele.type << " " << ele.x << " " << ele.y << std::endl;
    //        }

    //        std::cout << "-----" << std::endl;
    //        for (int i = 0; i < sample2.elementCount(); i++) {
    //            QPainterPath::Element ele = sample2.elementAt(i);
    //            std::cout << ele.type << " " << ele.x << " " << ele.y << std::endl;
    //        }

    //        dstOutline = booleanOpsHandler.unite(sample1, sample2);
    //        sample3.addRoundedRect(980, 100, 300,100,20,20);
    //        QVector<QPainterPath> testPaths{sample1, sample2, sample3}; //, sample3

    //        for (int i = 0; i < testPaths.size(); i++) {

    //            booleanOpsHandler.uniteAndAdd(dstOutline, testPaths.at(i));
    //        }

    // return dstOutline;

    QPainterPath ellipse;
    QPainterPath roundedRect;

    ellipse.addEllipse(QPointF(460, 80), 30, 120);
    roundedRect.addRoundedRect(360, 50, 200, 100, 20, 20);

    KisBooleanOperations booleanOpsHandler;

    QPainterPath res = booleanOpsHandler.subtract(ellipse, roundedRect);


    std::cout << res.elementCount() << std::endl;
    std::cout << "[";
    for (int i = 0; i < res.elementCount(); i++) {
        QPainterPath::Element e = res.elementAt(i);
        std::cout << e.type << ", ";
    }

    std::cout << " ]";
    return res;
}

void KisBooleanOperations::printElements(const QPainterPath &path)
{
    std::cout << "Krita QPP elements:" << std::endl;


    for (int i = 0; i < path.elementCount(); i++) {
        QPainterPath::Element element = path.elementAt(i);
        if (element.type == 3) {
            continue;
        }

        std::cout << element.type << " " << element.x << " " << element.y << std::endl;
    }
}

QPainterPath KisBooleanOperations::partialQPainterPath(QPainterPath path)
{
    QPainterPath result;

    for (int i = 0; i < path.elementCount(); i++) {
        QPainterPath::Element element = path.elementAt(i);
        switch (element.type) {
        case QPainterPath::MoveToElement:
            result.moveTo(QPointF(element.x, element.y));
            break;

        case QPainterPath::LineToElement:
            result.lineTo(QPointF(element.x, element.y));
            break;

        case QPainterPath::CurveToElement:
            result.cubicTo(QPointF(element.x, element.y),
                           QPointF(path.elementAt(i + 1).x, path.elementAt(i + 1).y),
                           QPointF(path.elementAt(i + 2).x, path.elementAt(i + 2).y));
            break;

        default:
            continue;
        }
    }

    return result;
}
