/*
 *  SPDX-FileCopyrightText: 2021 Tanmay Chavan <earendil01tc@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KISBOOLEANOPERATIONS_H
#define KISBOOLEANOPERATIONS_H

//#include "KisIntersectionFinder.h"
//#include "KisPathClipper.h"
#include <QPainterPath>


/*
 * This class contains the functions needed for boolean operations on vector
 * shapes. It mostly is a wrapper class which uses the other functions
 * together to give final clipped shapes.
 *
 * Currently, the clipping algorithm is buggy. It is able to find all the
 * intersection points accurately between two shapes containing curves and also
 * splits the elements about the points of intersection. However clipping using
 * KisPathClipper doesn't work as expected, and gives some extra edges.
 *
 * The current approach is to traverse QPainterPath and substitute the
 * curves we have calculated. This approach, has been implemented in
 * KisIntersectionFinder::resubstituteCurves() works for almost all cases
 * but might generate a few extra lines while carrying subtraction on certain
 * shapes (containing curves.)
 */

class KisBooleanOperations {

public:

    KisBooleanOperations();
    ~KisBooleanOperations();

    /*
     * returns the union of two QPainterPaths
     */
    QPainterPath unite(const QPainterPath &sub, const QPainterPath &clip);


    /*
     * returns the intersection of two QPainterPaths
     */
    QPainterPath intersect(QPainterPath &sub, QPainterPath &clip);


    /*
     * returns the difference of the second QPainterPath from the first.
     */
    QPainterPath subtract(QPainterPath &sub, QPainterPath &clip);


    /*
     * Unites the second QPainterPath with the first, and returns it as well.
     */
    QPainterPath uniteAndAdd(QPainterPath &sub, const QPainterPath &clip);


    /*
     * Intersects the second QPainterPath with the first, and returns it as
     * well.
     */
    QPainterPath intersectAndAdd(QPainterPath &sub, QPainterPath &clip);


    /*
     * Subtracts the second QPainterPath from the first, and then returns it
     * as well.
     */
    QPainterPath subtractAndAdd(QPainterPath &sub, QPainterPath &clip);


    QPainterPath uniteAndAddByProxy(QPainterPath &dstOutline, QVector<QPainterPath> srcOutlines);

    QPainterPath intersectAndAddByProxy(QPainterPath &dstOutline, QVector<QPainterPath> srcOutlines);

    QPainterPath subtractAndAddByProxy(QPainterPath &dstOutline, QVector<QPainterPath> srcOutlines);

    /*
     * test function to test various clipping operations.
     */
    QPainterPath testAdd();


    /*
     * Temp debugging function
     */
    void printElements(const QPainterPath &path);


    /*
     * Temp debugging function
     */
    QPainterPath partialQPainterPath(QPainterPath path);
};

#endif
