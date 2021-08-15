/*
 *  SPDX-FileCopyrightText: 2021 Tanmay Chavan <earendil01tc@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */


#include "kisbooleanoperations.h"
#include "kisintersectionfinder.h"
#include "kispathclipper.h"
#include <QPainterPath>

KisBooleanOperations::KisBooleanOperations(){

}
KisBooleanOperations::~KisBooleanOperations(){

}


QPainterPath KisBooleanOperations::unite(QPainterPath &sub, QPainterPath &clip) {

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
    QVector<KisClippingVertex> intPoints = KIF.findAllIntersections();

    KIF.processShapes();

    QPainterPath splittedSub = KIF.subjectShapeToPath();
    QPainterPath splittedClip = KIF.clipShapeToPath();

    KisPathClipper clipper(splittedSub, splittedClip);

    QPainterPath res = splittedSub | splittedClip;

    QPainterPath processedRes = KIF.resubstituteCurves(res);

    return processedRes;
}


QPainterPath KisBooleanOperations::intersect(QPainterPath &sub, QPainterPath &clip) {

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
    QVector<KisClippingVertex> intPoints = KIF.findAllIntersections();
    KIF.processShapes();

    QPainterPath splittedSub = KIF.subjectShapeToPath();
    QPainterPath splittedClip = KIF.clipShapeToPath();

//    KisPathClipper clipper(splittedSub, splittedClip);

    QPainterPath res = splittedSub & splittedClip;

    QPainterPath processedRes = KIF.resubstituteCurves(res);

    return processedRes;
}


QPainterPath KisBooleanOperations::subtract(QPainterPath &sub, QPainterPath &clip) {

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
    QVector<KisClippingVertex> intPoints = KIF.findAllIntersections();
    KIF.processShapes();

    QPainterPath splittedSub = KIF.subjectShapeToPath();
    QPainterPath splittedClip = KIF.clipShapeToPath();

//    KisPathClipper clipper(splittedSub, splittedClip);

    QPainterPath res = splittedSub - splittedClip;

    QPainterPath processedRes = KIF.resubstituteCurves(res);

    return processedRes;
}


QPainterPath KisBooleanOperations::uniteAndAdd( QPainterPath &sub, QPainterPath &clip ) {

    QPainterPath res = unite(sub, clip);
    sub = res;

    return (sub);

}


QPainterPath KisBooleanOperations::intersectAndAdd( QPainterPath &sub, QPainterPath &clip ) {

    QPainterPath res = intersect(sub, clip);
    sub = res;

    return (sub);

}


QPainterPath KisBooleanOperations::subtractAndAdd( QPainterPath &sub, QPainterPath &clip ) {

    QPainterPath res = subtract(sub, clip);
    sub = res;

    return (sub);

}


QPainterPath KisBooleanOperations::testAdd() {

    QPainterPath ellipse;
    QPainterPath roundedRect;

    ellipse.addEllipse(QPointF(460, 80), 30, 120);
    roundedRect.addRoundedRect(360, 50, 200, 100, 20, 20);

    KisBooleanOperations booleanOpsHandler;

    QPainterPath res = booleanOpsHandler.intersect(ellipse, roundedRect);

    return res;
}

