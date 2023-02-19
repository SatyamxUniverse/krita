/*
 *  SPDX-FileCopyrightText: 2021 Tanmay Chavan <earendil01tc@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef NUMERICALENGINE_H
#define NUMERICALENGINE_H

#include <QCoreApplication>
#include <QtMath>
#include <QVector>
#include <QPointF>
#include <QPainterPath>



constexpr double lowestDouble = std::numeric_limits<double>::min();
constexpr double highestDouble = std::numeric_limits<double>::max();


using namespace std::chrono;

class Expression;

 /*
  * class Term:
  * Represents an algebraic Term in a polynomial. has 3 data members:
  * coef, for the coefficient of a Term
  * xIndex for the degree of x, and YIndex for the degree of y
  * supports changing the values and elementary arithmetic operations like
  * addition, subtraction, and multiplication
  */

class Term
    {

    public:
        friend class Expression;
        Term ();
        Term (qreal coef, int xIndex, int YIndex);
        ~Term ();

        qreal getCoefficient ();
        int getXIndex ();
        int getYIndex();

        // increments the coefficient by the specified value
        void addCoefficient (qreal coeff);

        void setCoefficient (qreal coeff);
        void setXIndex (int xIndex_arg);
        void setYIndex (int YIndex_arg);


        /* multiplies two Terms.
         * returns a new multiplied Term, without modifying the original Terms
         */
        Term operator*(Term& t2);

        bool operator!=(  Term &t2);
        bool operator==( const Term &t2 );
        bool operator==( Term t2 );

    private:
        qreal coefficient;
        int xIndex;
        int yIndex;

    };


/*
 *
 * class Expression:
 *
 * Used to represent an algebraic expression .
 * the data member express is a QVector containing Term elements.
 * Expression supports upto two variables, and various arithmetic operations
 * on expressions like addition, subtraction, multiplication.
 *
 * In general, there are two functions to do these operations: the overloaded
 * operators and named functions (for example, operator+() and add() )
 * The key difference is that the overloaded operators do not change the
 * two expressions given to it as operands. On the other hand, the named
 * functions modify the object which calls them. This is made further
 * clear by the return types of the functions.
 *
 * The class also supports functions to substitute values for variables and
 * obtain the evaluate the expression, maintain non-zero leading coefficient,
 * sort the expression according to the powers/indices of the variable, and
 * exponentiate the expression.
 *
 *
 */


class Expression
    {

    public:
//        Expression (Term t);
        Expression ();
        ~Expression ();


        /*
         * Multiplies two expressions and returns the resultant expression,
         * without modifying the two operand terms.
         */
        Expression operator*(Expression ex2);


        /*
         * Multiplies an expression by a constant numberand returns the
         * resultant expression, without modifying the expression.
         */
        Expression operator*(qreal multiplying_factor);


        /*
         * Adds two expressions and returns the resultant expression, without
         * modifying the two operand terms.
         */
        Expression operator+(Expression ex);


        /*
         * Subtracts the second expression from the first and returns the
         * resultant expression, without modifying the two operand terms.
         */
        Expression operator-(Expression ex);


        bool operator==(Expression &ex);


        /*
         * Subtracts the argument expression from the caller expression,
         * changing the value of the caller expression
         */
        void subtract (Expression ex);
        void subtract (Term t);


        /*
         * Adds the argument expression to the caller expression, changing
         * the value of the caller expression
         */
        void add (Expression ex);
        void add (Term t);


        /*
         * Returns a vector of coefficients of each term in the expression
         */
        QVector<qreal> coeffs ();


        /*
         * Substitutes the given value for x variable in the Expression and
         * returns value of the Expression. this function
         * ignores the y exponent and assumes the exponent to be zero
         */
        qreal evaluate (qreal x);


        /*
         * Substitutes the given values for x and y variables in the
         * Expression and returns value of the Expression.
         */
        qreal evaluate2d(qreal x, qreal y);


        /*
         * Checks if the leading Term has coefficient with value zero, and
         * removes it until the leading Term has a non-zero coefficient
         * works only for polynomials in one variable.
         */
        void validateLeadingCoefficient();


        /*
         * Exponentiates the Expression by multiplying a copy of the
         * expression n times. returns the resultant exponentiated term
         * without modifying the caller term.
         *
         */
        Expression powExpression (int n);


        /*
         * Returns a vector containing all the terms in expression
         * (the member 'express').
         */
        QVector<Term> getExpression();

        enum sortOrder{
            ascending,
            descending
        };

        static bool greaterThan(Term &t1, Term &t2);
        static bool smallerThan(Term &t1, Term &t2);


        /*
         * Sorts the Expression in ascending or descending order with respect
         * to index of the first variable (x or t)
         */
        void sortExpress(sortOrder order);

    private:
        QVector<Term> express;

    };


/*
 * class CubicBezier:
 * This class is used to mathematically represent a Bezier curve of
 * degree-three. Contains equations and algorithms needed to find the points
 * of intersection between two curves or a curve and a line.
 *
 * It stores the 4 control points of the curve, the resultant matrix, the
 * implicit equation and pair of parametric equations. Most of the code
 *  is structured in a way to find the points of intersection.
 *
 */

class CubicBezier
    {

    public:

        friend class KisIntersectionFinder;


        CubicBezier ();
        CubicBezier (QPointF& cp1, QPointF& cp2, QPointF& cp3, QPointF& cp4);
        CubicBezier (QPainterPath::Element curve);
        ~CubicBezier ();

        /*
         * splits the Bezier curve into two curves using De-Casteljau's
         * algorithm.
         */
        QVector<CubicBezier> splitCurve(qreal parameter);


        /*
         * enforces splitting about the provided point in cartesian co-ordinates.
         */
        QVector<CubicBezier> splitCurve(qreal parameter, QPointF splittingPoint);

        /*
         * Generates the parametric equations of the bezier curve. Here,
         * the power of y should always be zero, and x resembles the paramter
         *  variable. The equations are generated in the standard polynomial
         *  form i.e   ax^3 + bx^2 + cx + d
         *
         */
        void generateParametricEquations ();

        /*
         * Generates an expression which is an element in the resultant
         * matrix needed for implicitization
         *
         * for the element (i,j) in the resultant matrix, the expression
         * needed is given by:
         *
         *   l i,j (x, y) = 3Ci  *  3Cj * | x    y    1 |
         *                                | xi   yi   1 |
         *                                | xj   yj   1 |
         */
        Expression element_entry (int first_pt, int second_pt);


        /*
         *
         * Calculates the determinant of the resultant matrix, which gives us the
         * implicit equation. A degree-3 Bezier curve is implicitized as follows:
         *   f (x, y) =  | l 32 (x, y)        l 31 (x, y)             l 30 (x, y) |
         *               | l 31 (x, y)   l 30 (x, y) + l 21 (x, y)    l 20 (x, y) |
         *               | l 30 (x, y)        l 20 (x, y)             l 10 (x, y) |
         *
         * where each l(i,j) is calculated as described in element_entry()
         *
         */
        Expression getImplicitEquation();


        /*
         * Generates an axis-oriented bounding box of the curve, with the edges
         * of the box being at minimum and maximum x and y co-ordinates.
         *
         */
        void computeBoundingBox();

        QRectF boundingBox();


        void computeExactBoundingBox();

        /*
         * Substitutes the parametric equations of the second curve in the
         * implicit equation of the first curve, and then uses GSL to find
         * the roots of the 9th degree polynomial giving us the parameters
         * of the second curve for the intersection points. This routine does
         * not give us points of self-intersection.
         *
         * IMP: GSL requires the coefficients in ascending order with respect
         * to the power of the variable
         * i.e 1 + 2x + 3x^2, and thus the array passed to it should be
         * [1, 2, 3].
         *
         */
        QVector<qreal> findRoots (CubicBezier& cb2);


        /*
         * Returns the parametric equation of the curve in x.
         */
        Expression getParametricX() {
            return parametric_x;
        };


        /*
         * Returns the parametric equation of the curve in y.
         */
        Expression getParametricY() {
            return parametric_y;
        };


        /*
         * Returns a vector containing all the control points of the curve.
         */
        QVector<QPointF> getControlPoints() {return control_points;};


        /*
         * An inversion equation basically gives us the parameter t of the
         * point on the curve if we give the point co-ordinates. This is
         * useful to verify if a point lies on the curve between the first
         * and last control points, as the curve can also extend beyond these
         * points by changing the paramter t to be greater than 1 or lesser
         * than zero.
         *
         */
        qreal inversionEquationEvaluated(QPointF &p);


    private:

        QPointF cp0;
        QPointF cp1;
        QPointF cp2;
        QPointF cp3;
        QVector<QPointF> control_points;
        QVector<QVector<Expression>> bezierMatrix;

        QRectF bbox;

        Expression implicit_eq;
        Expression parametric_x;
        Expression parametric_y;
        Expression inversionEquationNum;
        Expression inversionEquationDen;

    };


/*
 * class Line:
 * This class is used to mathematically represent a line. Contains equations
 * and algorithms needed to find the points of intersection between two lines
 * or a line and a curve, and to verify whether the point lies on the line
 * segment.
 *
 * As the main purpose of this class is to find the intersection points, a
 * line is divided into three categories: vertical, horizontal, and oblique.
 * The implicit representation of a line is used to find the intersection
 * points. If the line is perfectly vertical, it cannot be represented in such
 * a manner. Hence, the algorithm treats these as special cases and uses
 * a different but simple approach for these cases.
 *
 * the line is stored in the form:  mx - y + c = 0, where m represents slope
 * and c represents the intercept.
 *
 * there are also functions to verify the validity of an intersection point,
 * by checking whether it actually lies on the line segment or not. As most
 * of the functionality is already implemented in QLineF, the class member
 * qtLine is used for most of the functions.
 *
 */

class Line {
    
public:
    Line();
    Line(QPointF p1, QPointF p2);
    Line(QLineF line);
    ~Line();


    /*
     * Returns the QLineF constructed from the points provided
     */
    QLineF getQLine();


    /*
     * Splits the line into two Lines, about the given parameter.
     */
    QVector<Line> splitLine(qreal parameter);


    /*
     * Splits the line into multiple Lines about the points provided.
     */
    QVector<Line> splitLine(QVector<QPointF> points);


    /*
     * Returns the implicit equation of the line. The equation is generated
     * in the constructor itself.
     */
    Expression getImplicitEquation();


    /*
     * Finds intersection points of a line with a curve. This function
     * assumes the line does intersect with the bounding box of curve.
     * Substitutes the parametric equations of the Bezier curve in the
     * implicit equation of the Line, and then uses GSL to find the
     * roots of the 3rd degree polynomial giving us the parameters of
     * the curve for the intersection points.
     *
     * If the line is vertical or horizontal, then at least one of the
     * co-ordinates of the intersection points will be known by the
     * intercept of the line. We use this property to simplify the
     * calculations.
     *
     */
    QVector<qreal> findRoots(CubicBezier &cb);


    /*
     * Checks whether the two Lines intersect or not.
     */
    bool checkIntersection(Line &l2);


    /*
     * Checks whether the Line intersects with given QLineF or not.
     */
    bool checkIntersection(QLineF &l2);


    /*
     * Checks if the given point lies on the Line.
     */
    bool checkIntersection(QPointF &p2);


    /*
     * Returns the parametric equation of the line in x.
     */
    Expression getParametricX();


    /*
     * Returns the parametric equation of the line in y.
     */
    Expression getParametricY();


    enum lineTypeEnum {
        Vertical,
        Horizontal,
        Oblique
    };


    bool isVertical();
    bool isHorizontal();
    bool isOblique();


private:
    QLineF qtLine;
    qreal slope;
    qreal intercept;
    Expression implicitEquation;
    Expression parametrixX;
    Expression parametricY;
    lineTypeEnum lineType;
    
};

#endif
