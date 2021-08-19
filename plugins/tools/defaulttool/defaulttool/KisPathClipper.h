/*
 *  SPDX-FileCopyrightText: 2021 Tanmay Chavan <earendil01tc@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */


/*
 * The files pathclipper.cpp and pathclipper.h are mainly copies of the files
 * qpathclipper.cpp, qpathclipper.h, qpathclipper_p.h from Qt codebase. As we
 * needed to modify some parts of the code and they were behind the Qt API,
 * the code had to be forked inside Krita codebase.
 *
 * Most of the code remains the same except some classes. The classes with
 * modifications are renamed to their Kis versions (e.g KisPathclipper),
 * while the classes which are the same retain thei Qt names (QPathVertex).
 *
 *
 *
 *
 * This file contains a modified clipping algorithm. It's basic working is
 * as follows:
 * It recieves processed QPainterPaths which are already splitted about the
 * intersection points. Then while flattening it in KisWingedEdge::addPath(),
 * it adds an extra data member 'curveID' so as to mark it for future
 * substitution. Then, the same curveID is also passed to the KisPathEdge in
 * KisWingedEdge::intersectAndAdd(). Then Qt proceeds to clip it by it's usual
 * method in doClip(). After that, it proceeds to get the QPainterPath from
 * the WingedEdge by using the function KisWingedEdge::toPath(). Here, the
 * function void add() has some extra code to substitute the curves in the
 * correct places.
 *
 * However it seems to fail while tackling curve-curve clipping. It is able to
 * find the intersection  points and then split it, but fails to clip it
 * properly. It is sure that it works correctly till KisWingedEdge::addPath()
 * as it processes the curves. After that, the function toPath() gets only a
 * few elements. This might indicate the problem lies is doClip(),
 * handleCrossingEdges() and the associated clipping functions.
 *
 */


#include <QtCore/qlist.h>

#pragma once
#include <QPainterPath>

#include <stdio.h>
#include "3rdparty/databuffer.h"



QT_BEGIN_NAMESPACE

class QBezier;
class KisWingedEdge;

class KisPathClipper
{
public:
    enum Operation {
        BoolAnd,
        BoolOr,
        BoolSub,
        Simplify
    };
public:
    KisPathClipper(const QPainterPath &subject,
                 const QPainterPath &clip);

    QPainterPath clip(Operation op = BoolAnd);

    bool intersect();
    bool contains();

    static bool pathToRect(const QPainterPath &path, QRectF *rect = nullptr);
    static QPainterPath intersect(const QPainterPath &path, const QRectF &rect);



private:
//    Q_DISABLE_COPY_MOVE(KisPathClipper)

    enum ClipperMode {
        ClipMode, // do the full clip
        CheckMode // for contains/intersects (only interested in whether the result path is non-empty)
    };

    bool handleCrossingEdges(KisWingedEdge &list, qreal y, ClipperMode mode);
    bool doClip(KisWingedEdge &list, ClipperMode mode);

    QPainterPath subjectPath;
    QPainterPath clipPath;
    Operation op;

    int aMask;
    int bMask;


};

struct QPathVertex
{
public:
    QPathVertex(const QPointF &p = QPointF(), int e = -1);
    operator QPointF() const;

    int edge;

    qreal x;
    qreal y;
};

class KisPathEdge
{
public:
    enum Traversal {
        RightTraversal,
        LeftTraversal
    };

    enum Direction {
        Forward,
        Backward
    };

    enum Type {
        Line,
        Curve
    };

    explicit KisPathEdge(int a = -1, int b = -1);

    mutable int flag;

    int windingA;
    int windingB;

    int first;
    int second;

    double angle;
    double invAngle;

    int curveID; //stores the curve from which this was obtained after flattening

    int next(Traversal traversal, Direction direction) const;

    void setNext(Traversal traversal, Direction direction, int next);
    void setNext(Direction direction, int next);

    Direction directionTo(int vertex) const;
    int vertex(Direction direction) const;

private:
    int m_next[2][2];
};

class KisPathSegments
{
public:
    struct Intersection {
        qreal t;
        int vertex;
        int next;

        bool operator<(const Intersection &o) const {
            return t < o.t;
        }
    };

    struct Segment {
        Segment(int pathId, int vertexA, int vertexB, int curveNumber = -1)
            : path(pathId)
            , va(vertexA)
            , vb(vertexB)
            , intersection(-1)
            , curveID(curveNumber)
        {
        }

        int path;

        // vertices
        int va;
        int vb;

        // intersection index
        int intersection;

        // curve number
        int curveID;

        QRectF bounds;
    };


    KisPathSegments(int reserve);

    /*
     * These functions carry out the flattening required by Qt to clip the
     * shapes. Here, we mark the flattened segments with the curve numbers.
     * This way, after deciding which edges are to be added, and when we add
     * the edges, we can substitute these edges with the curves they are meant
     * to represent. As we mark every single edge, there should be no loss of
     * accuracy. The only possible issue could arise if the flattened edges
     * do intersect, and the curves do not. However, this problem would not be
     * a regression as the default Qt algorithm behaved similarly, and it can
     * be avoided by flattening the curves to a higher extent. (It can also be
     * detected by using Qt's intersection-finding routine, and checking if it
     * finds a point other than the vertices.)
     *
     * We provide an out parameter, the QVector curves, so that we can
     * store the curves in the order in which they were flattened. This is
     * needed as the function which converts the clipping structure
     * (KisWingedEdge) lies outside this class. We can substitute the curves
     * exactly as they were while flattening them.
     *
     * This approach is implemented, but isn't currently used. There is a bug
     * which makes it choose the wrong edges or lead to missing edges. However,
     * solving this bug will lead to a much more reliable clipping and a much
     * quicker solution.
     */

    void setPath(const QPainterPath &path);
    void setPath(const QPainterPath &path, QVector<QBezier> &curves);

    void addPath(const QPainterPath &path);
    void addPath(const QPainterPath &path, QVector<QBezier> &curves);

    int intersections() const;
    int segments() const;
    int points() const;

    const Segment &segmentAt(int index) const;
    const QLineF lineAt(int index) const;
    const QRectF &elementBounds(int index) const;
    int pathId(int index) const;

    const QPointF &pointAt(int vertex) const;
    int addPoint(const QPointF &point);

    const Intersection *intersectionAt(int index) const;
    void addIntersection(int index, const Intersection &intersection);

    void mergePoints();

    QVector<QBezier> pathCurves();
private:
    QDataBuffer<QPointF> m_points;
    QDataBuffer<Segment> m_segments;
    QDataBuffer<Intersection> m_intersections;

    int m_pathId;
    int curveNumber;
};

class Q_AUTOTEST_EXPORT KisWingedEdge
{
public:
    struct TraversalStatus
    {
        int edge;
        KisPathEdge::Traversal traversal;
        KisPathEdge::Direction direction;

        void flipDirection();
        void flipTraversal();

        void flip();
    };

    KisWingedEdge();
    KisWingedEdge(const QPainterPath &subject, const QPainterPath &clip);

    void simplify();
    QPainterPath toPath() const;

    int edgeCount() const;

    KisPathEdge *edge(int edge);
    const KisPathEdge *edge(int edge) const;

    int vertexCount() const;

    int addVertex(const QPointF &p);

    QPathVertex *vertex(int vertex);
    const QPathVertex *vertex(int vertex) const;

    TraversalStatus next(const TraversalStatus &status) const;

    int addEdge(const QPointF &a, const QPointF &b);
    int addEdge(int vertexA, int vertexB, int curveId = -1);

    bool isInside(qreal x, qreal y) const;

    static KisPathEdge::Traversal flip(KisPathEdge::Traversal traversal);
    static KisPathEdge::Direction flip(KisPathEdge::Direction direction);

    KisPathSegments m_segments;

        QVector<QBezier> pathCurves();
        QVector<QBezier> curves;

private:
    void intersectAndAdd();

    void printNode(int i, FILE *handle);

    void removeEdge(int ei);

    int insert(const QPathVertex &vertex);
    TraversalStatus findInsertStatus(int vertex, int edge) const;

    qreal delta(int vertex, int a, int b) const;

    QDataBuffer<KisPathEdge> m_edges;
    QDataBuffer<QPathVertex> m_vertices;

    QVector<qreal> m_splitPoints;


};

inline KisPathEdge::KisPathEdge(int a, int b)
    : flag(0)
    , windingA(0)
    , windingB(0)
    , first(a)
    , second(b)
    , angle(0)
    , invAngle(0)
{
    m_next[0][0] = -1;
    m_next[1][0] = -1;
    m_next[0][0] = -1;
    m_next[1][0] = -1;
}

inline int KisPathEdge::next(Traversal traversal, Direction direction) const
{
    return m_next[int(traversal)][int(direction)];
}

inline void KisPathEdge::setNext(Traversal traversal, Direction direction, int next)
{
    m_next[int(traversal)][int(direction)] = next;
}

inline void KisPathEdge::setNext(Direction direction, int next)
{
    m_next[0][int(direction)] = next;
    m_next[1][int(direction)] = next;
}

inline KisPathEdge::Direction KisPathEdge::directionTo(int vertex) const
{
    return first == vertex ? Backward : Forward;
}

inline int KisPathEdge::vertex(Direction direction) const
{
    return direction == Backward ? first : second;
}

inline QPathVertex::QPathVertex(const QPointF &p, int e)
    : edge(e)
    , x(p.x())
    , y(p.y())
{
}

inline QPathVertex::operator QPointF() const
{
    return QPointF(x, y);
}

inline KisPathSegments::KisPathSegments(int reserve) :
    m_points(reserve),
    m_segments(reserve),
    m_intersections(reserve),
    m_pathId(0),
    curveNumber(0)
{
}

inline int KisPathSegments::segments() const
{
    return m_segments.size();
}

inline int KisPathSegments::points() const
{
    return m_points.size();
}

inline const QPointF &KisPathSegments::pointAt(int i) const
{
    return m_points.at(i);
}

inline int KisPathSegments::addPoint(const QPointF &point)
{
    m_points << point;
    return m_points.size() - 1;
}

inline const KisPathSegments::Segment &KisPathSegments::segmentAt(int index) const
{
    return m_segments.at(index);
}

inline const QLineF KisPathSegments::lineAt(int index) const
{
    const Segment &segment = m_segments.at(index);
    return QLineF(m_points.at(segment.va), m_points.at(segment.vb));
}

inline const QRectF &KisPathSegments::elementBounds(int index) const
{
    return m_segments.at(index).bounds;
}

inline int KisPathSegments::pathId(int index) const
{
    return m_segments.at(index).path;
}

inline const KisPathSegments::Intersection *KisPathSegments::intersectionAt(int index) const
{
    const int intersection = m_segments.at(index).intersection;
    if (intersection < 0)
        return nullptr;
    else
        return &m_intersections.at(intersection);
}

inline int KisPathSegments::intersections() const
{
    return m_intersections.size();
}

inline void KisPathSegments::addIntersection(int index, const Intersection &intersection)
{
    m_intersections << intersection;

    Segment &segment = m_segments.at(index);
    if (segment.intersection < 0) {
        segment.intersection = m_intersections.size() - 1;
    } else {
        Intersection *isect = &m_intersections.at(segment.intersection);

        while (isect->next != 0)
            isect += isect->next;

        isect->next = (m_intersections.size() - 1) - (isect - m_intersections.data());
    }
}

inline int KisWingedEdge::edgeCount() const
{
    return m_edges.size();
}

inline KisPathEdge *KisWingedEdge::edge(int edge)
{
    return edge < 0 ? nullptr : &m_edges.at(edge);
}

inline const KisPathEdge *KisWingedEdge::edge(int edge) const
{
    return edge < 0 ? nullptr : &m_edges.at(edge);
}

inline int KisWingedEdge::vertexCount() const
{
    return m_vertices.size();
}

inline int KisWingedEdge::addVertex(const QPointF &p)
{
    m_vertices << p;
    return m_vertices.size() - 1;
}

inline QPathVertex *KisWingedEdge::vertex(int vertex)
{
    return vertex < 0 ? nullptr : &m_vertices.at(vertex);
}

inline const QPathVertex *KisWingedEdge::vertex(int vertex) const
{
    return vertex < 0 ? nullptr : &m_vertices.at(vertex);
}

inline KisPathEdge::Traversal KisWingedEdge::flip(KisPathEdge::Traversal traversal)
{
    return traversal == KisPathEdge::RightTraversal ? KisPathEdge::LeftTraversal : KisPathEdge::RightTraversal;
}

inline void KisWingedEdge::TraversalStatus::flipTraversal()
{
    traversal = KisWingedEdge::flip(traversal);
}

inline KisPathEdge::Direction KisWingedEdge::flip(KisPathEdge::Direction direction)
{
    return direction == KisPathEdge::Forward ? KisPathEdge::Backward : KisPathEdge::Forward;
}

inline void KisWingedEdge::TraversalStatus::flipDirection()
{
    direction = KisWingedEdge::flip(direction);
}

inline void KisWingedEdge::TraversalStatus::flip()
{
    flipDirection();
    flipTraversal();
}

QT_END_NAMESPACE

//#endif // KisPathClipper_P_H
