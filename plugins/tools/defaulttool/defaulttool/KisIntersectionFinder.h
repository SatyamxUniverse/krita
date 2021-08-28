/*
 *  SPDX-FileCopyrightText: 2021 Tanmay Chavan <earendil01tc@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KISINTERSECTIONFINDER_H
#define KISINTERSECTIONFINDER_H

#include <QPointF>
#include "KisNumericalEngine.h"
//#include "KisPathClipper.h"


/*
 * enum buildingBlockElementType:
 * This enum is a copy of QPainterPath::ElementType.
 */
enum BuildingBlockElementType {
    MoveToElement,
    LineToElement,
    CurveToElement,
//        CurveToDataElement
};



struct KisIntersectionPoint {

    qreal parameter;
    QPointF point;

    bool operator<(const KisIntersectionPoint& p2);

    bool operator==(const KisIntersectionPoint& p2);
};

/*
 * class BuildingBlock:
 *
 * A copy of QPainterPath::Element. Contains three types of building blocks:
 * Line, Curve, and Move. For finding intersections, only Line and curve are
 * considered, with moveTo elements being treated as 'gaps'. Each element has
 * an ID, a type, and a Line or a CubicBezier member as per the type.
 */
class BuildingBlock {

public:

    friend class KisIntersectionFinder;

    BuildingBlock();
    BuildingBlock(int id, BuildingBlockElementType buildingblocktype, Line l1);
    BuildingBlock(int id, BuildingBlockElementType buildingblocktype, CubicBezier c1);
    BuildingBlock(int id, BuildingBlockElementType buildingblocktype, QPointF p1);


    bool isMoveTo();
    bool isLineTo();
    bool isCurveTo();


    QPointF getMoveTo();
    Line getLineTo();
    CubicBezier getCurveTo();


private:

    int ID;
    BuildingBlockElementType type;
    Line l;
    CubicBezier cb;
    QPointF moveToPoint;

};



/*
 * class KisClippingVertex:
 *
 * Used for intersection points and regular vertices, storing their
 * co-ordinates, the elements which intersect at that point, and the type of
 * the point (regular vertex, regular intersection point, or a degenerate case
 * where the intersection point lies on a vertex).
 *
 */
class KisClippingVertex {

public:

    // This enum is useful for the Greiner-Hormann approach. Is (currently)
    // obsolete as Qt's clipping approach is used.
    enum VertexType{
        regularVertex,
        regularIntersection,
        degenerateIntersection
    };

    enum flag{
        en,
        ex
    };

    KisClippingVertex() = default;
    KisClippingVertex(QPointF pt, VertexType type, int idFirst= -1, int idSecond = -1, qreal para1 = -1, qreal para2 = -1);

public:

    QPointF point;
    VertexType type;
    QPointF pseudoPoint;  // could be useful for Greiner-Hormann algorithm
    int idFirstBuildingBlock;
    int idSecondBuildingBlock;
    qreal parameterFirstBuildingBlock;
    qreal parameterSecondBuildingBlock;

//public:


    inline void setIdFirstBuildingBlock(int id) {

        this->idFirstBuildingBlock = id;
    }

    inline void setIdSecondBuildingBlock(int id) {

        this->idSecondBuildingBlock = id;
    }

};

/*
 * Data classes:
 * As our regular classes for curves and lines have many computations and are
 * heavy, these classes only contain the basic data to uniquely represent a
 * line or a curve.
 */

struct CubicBezierData{
    QPointF cp0;
    QPointF cp1;
    QPointF cp2;
    QPointF cp3;

    CubicBezierData() {

    }

    CubicBezierData(QPointF p1, QPointF p2, QPointF p3, QPointF p4) :
    cp0(p1),
    cp1(p2),
    cp2(p3),
    cp3(p4)
    {

    }
};

class LineData;

class MoveData;

/*
 * class KisIntersectionFinder:
 * This class is responsible to find all of the intersection points between
 * two QPainterPaths.
 *
 * The algorithm is roughly as follows:
 *
 * It breaks down the given QPainterPath into several BuildingBlock elements,
 * which can be either Line or CubicBezier objects.
 * These elements are inserted linearly in two different QVectors,subjectShape
 * and clipShape for the first shape elements and second shape elements
 * respectively.
 *
 * Then, it proceeds to pairwise check intersections for every element in both
 * paths (including self-intersections of a shape), giving us a complexity of
 * O(n^2) where n is the total number of elements. The intersection ponits are
 * then recorded (TODO: record them in a vector wrapped inside another class
 * which also contains references to the elements involved as well as the
 * parameters for both the elements.)
 *
 */
class KisIntersectionFinder {

public:

    KisIntersectionFinder() = default;
    KisIntersectionFinder(QPainterPath subject, QPainterPath clip);
    ~KisIntersectionFinder() = default;


    friend class CubicBezier;
    friend class BuildingBlock;

    /*
     * Returns a vector of all the intersection points between two Bezier
     * curves. Calls findRoots() and evaluates the parameter to obtain the
     * co-ordinates of the intersection point. Further, it checks whether the
     * point actually lies on the curve.
     *
     */
    QVector<KisClippingVertex> intersectionPoint(CubicBezier c1, CubicBezier c2);


    /*
     * Returns a vector of all the intersection points between a cubic Bezier
     * curve and a line. Calls findRoots() and evaluates the parameter to
     * obtain the co-ordinates of the intersection point. Further, it checks
     * whether the point actually lies on the line (we know it lies on the
     * curve as we are using the parametric equations of the curve.)
     *
     */
    QVector<KisClippingVertex> intersectionPoint(Line l1, CubicBezier c2);


    /*
     * Convenience function, works same as the above function.
     */
    QVector<KisClippingVertex> intersectionPoint(CubicBezier c1, Line l2);


    /*
     *  Returns a vector containing the intersection point between two lines.
     *  Uses a QVector of QPointF instead of just QPointF for compatibility.
     */
    QVector<QPointF> intersectionPoint(Line l1,Line l2);


    /*
     * Checks whether two lines intersect or not. However, is pretty similar
     * to intersectionPoint() and requires that much time to run, hence avoid
     * if possible.
     *
     */
    bool linesIntersect(const QLineF &a, const QLineF &b) const;



    /*
     * Finds the intersection points between two BuildingBlocks by calling the
     * appropriate function as per the type of the BuildingBlock.
     *
     */
    QVector<KisClippingVertex> findIntersectionPoints(BuildingBlock e1, BuildingBlock e2);


    /*
     * Finds all the intersection points between the two QPainterPaths given
     * in the constructor, including self-intersections. Returns them in a
     * QVector.
     *
     */
    QVector<KisClippingVertex> findAllIntersections();

    /*
     * Splits the components about the intersection points, thus creating
     * a new structure with no interseection points and only common vertices.
     *
     */
    void processShapes();


    /*
     * Returns all of the intersection points wrapped in KisClippingVertex.
     *
     */
    inline QVector<QVector<KisClippingVertex>> getSubToClipIntersectionVertices(){

        return subToClipIntersectionVertices;
    }


    /*
     * Converts the splitted subject shape to a QPainterPath and returns it.
     */
    QPainterPath subjectShapeToPath();


    /*
     * Converts the splitted clip shape to a QPainterPath and returns it.
     */
    QPainterPath clipShapeToPath();


    /*
     * Returns all vertices, including intersections
     */
    QVector<BuildingBlock> getSubjectShape() {
        return subjectShape;
    }


    /*
     * Returns all vertices, including intersections
     */
    QVector<BuildingBlock> getNetShape();


    /*
     * Returns all clipped curves.
     */
    QVector<CubicBezier> getClippedCurves();


    /*
     * Returns all clipped curves' data.
     */
    QVector<CubicBezierData> getClippedCurvesData();


    /*
     * Returns the end points of all clipped curves in the clipped
     * shape.
     */
    QVector<QPointF> getAllCurveEndPoints();


    /*
     * This function mainly handles the clipping. It goes through the given
     * QPainterPath and checks for probable curves. It then replaces them with
     * the original curves.
     *
     * This approach doesn't consider the case of exactly parallel curves, and
     * might produce slightly wrong results during the subtraction/difference
     * operation.
     */
    QPainterPath resubstituteCurves(QPainterPath path, bool differenceOp = false);



    /*
     *  The following functions are not directly used, however can be useful
     *  for Greiner-Hormann approach for clipping the shapes entirely.
     */

    /*
     * Returns all vertices, including intersections
     */
    QVector<QPointF> getRegularVertices();


private:

    /*
     * The following two vectors are used to contain and map intersection
     * points. Each row contains a vector containing all the intersection
     * points for the following segment.
     *
     * subToClipIntersectionVertices contains all the intersection points
     * between the subject shape to the clip shape.
     */
    QVector<QVector<KisClippingVertex>> subToClipIntersectionVertices;


    /*
     * clipToSubIntersectionVertices contains all the intersection points
     * between the subject shape to the clip shape.
     */
    QVector<QVector<KisClippingVertex>> clipToSubIntersectionVertices;


    /*
     * subToSubIntersectionVertices contains all the self-intersection points
     * between of the subject shape.
     */
    QVector<QVector<KisClippingVertex>> subToSubIntersectionVertices;


    /*
     * clipToClipIntersectionVertices contains all the self-intersection points
     * between of the clip shape.
     */
    QVector<QVector<KisClippingVertex>> clipToClipIntersectionVertices;


    QVector<BuildingBlock> subjectShape;
    QVector<BuildingBlock> subjectShapeProcessed;

    QVector<BuildingBlock> clipShape;
    QVector<BuildingBlock> clipShapeProcessed;

    QVector<BuildingBlock> netShape;
    QVector<BuildingBlock> netShapeProcessed;

    /*
     * The following containers are used for the function resubstituteCurves().
     */
    QVector<CubicBezier> clippedCurves;
    QVector<CubicBezierData> clippedCurvesData;
    QVector<QPointF> allCurveEndPoints;


    /*
     * The following containers are used to record the vertices in the shapes.
     * They are necessary for Greiner-Hormann clipping algorithm, and these
     * can be used readily for that purpose if chosen to implement it.
     */
    QVector<KisClippingVertex> subjectVertices;
    QVector<KisClippingVertex> subjectVerticesWithIntersections;
    QVector<KisClippingVertex> clipVertices;
    QVector<KisClippingVertex> clipVerticesWithIntersections;
    QVector<QPointF> netRegularVertices;


//    KoRTree<CubicBezier> tree;

};


class GreinerClippingVertex{
public:





    enum ElementType {
        MoveToElement,
        LineToElement,
        CurveToElement,
        CurveToDataElement
    };

    enum VertexType {
        RegularVertex,
        RegularIntersection,
        DegenrateIntersection
    };

    enum Flag{
        en,
        ex,
        null
    };

    QPointF point;
    ElementType elementType;
    VertexType vertexType;
    Flag flag;
    int referenceIndex;
    GreinerClippingVertex* reference = 0;


    GreinerClippingVertex() = default;
    GreinerClippingVertex(QPainterPath::Element element, VertexType type = RegularVertex, Flag flag = null, int referenceIdx = -1);

private:


};

class GreinerHormannClipping {
public:

    QPainterPath subjectPath;
    QPainterPath clipPath;
    QVector<GreinerClippingVertex> subjectList;
    QVector<GreinerClippingVertex> clipList;
    QVector<KisClippingVertex> intersections;

    QVector<GreinerClippingVertex> getSubList();
    QVector<GreinerClippingVertex> getClipList();

    GreinerHormannClipping(QPainterPath sub, QPainterPath clip, QVector<KisClippingVertex> intersectionPoints);

    void generateSubjectList(QPainterPath subject, QVector<KisClippingVertex> intersectionPoints);
    void generateClipList(QPainterPath clip, QVector<KisClippingVertex> intersectionPoints);

    void linkIntersectionPoints(QVector<KisClippingVertex> intersectionPoints);
    void markIntersectionPoints(QPainterPath subPath, QPainterPath clipPath);

    QPainterPath unite();
};


#endif // KISINTERSECTIONFINDER_H
