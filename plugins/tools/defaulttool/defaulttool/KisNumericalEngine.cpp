/*
 *  SPDX-FileCopyrightText: 2021 Tanmay Chavan <earendil01tc@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QCoreApplication>
#include <iostream>

#include <chrono>

#include <gsl/gsl_math.h>
#include <gsl/gsl_poly.h>

#include <QList>
#include <QPainterPath>
#include <QPointF>
#include <QVector>
#include <QtMath>

#include "KisNumericalEngine.h"

using namespace std::chrono;

// helper functions

static inline bool fuzzyIsNull(qreal d)
{
    if (sizeof(qreal) == sizeof(double))
        return qAbs(d) <= 1e-12;
    else
        return qAbs(d) <= 1e-5f;
}

static inline bool comparePoints(const QPointF &a, const QPointF &b)
{
    // the epsilon in fuzzyIsNull is far too small for our use

    //    return fuzzyIsNull(a.x() - b.x())
    //           && fuzzyIsNull(a.y() - b.y());

    return qAbs(a.x() - b.x()) < 1e-6 && qAbs(a.y() - b.y()) < 1e-6;
}

bool fuzzyGreaterThan(qreal a, qreal b)
{
    return (a > (b - 1e-6)) ? true : false;
}

bool fuzzyLesserThan(qreal a, qreal b)
{
    return (a < (b + 1e-6)) ? true : false;
}

long int factorial(int num)
{
    long int factorial_num = num;
    if (num == 0) {
        factorial_num = 1;
    }

    else {
        for (int i = num - 1; i > 0; i--) {
            factorial_num *= i;
        }
    }

    return factorial_num;
}

int combinatorial(int n, int r)
{
    return (factorial(n) / (factorial(r) * factorial(n - r)));
}

class Expression;

Term::Term(qreal coef, int x_ind, int y_ind)
    : coefficient(coef)
    , xIndex(x_ind)
    , yIndex(y_ind)
{
}

Term::Term()
{
}

Term::~Term()
{
}

qreal Term::getCoefficient()
{
    return coefficient;
}

int Term::getXIndex()
{
    return xIndex;
}

int Term::getYIndex()
{
    return yIndex;
}

void Term::addCoefficient(qreal coeff)
{
    this->coefficient += coeff;
};

void Term::setCoefficient(qreal coeff)
{
    this->coefficient = coeff;
};

void Term::setXIndex(int xIndex_arg)
{
    this->xIndex = xIndex_arg;
};

void Term::setYIndex(int YIndex_arg)
{
    this->yIndex = YIndex_arg;
};

Term Term::operator*(Term &t2)
{
    Term res;
    res.setCoefficient((this->coefficient) * t2.coefficient);
    res.setXIndex((this->xIndex) + t2.xIndex);
    res.setYIndex((this->yIndex) + t2.yIndex);
    return res;
}

bool Term::operator!=(Term &t2)
{
    return (this->coefficient != t2.coefficient || this->xIndex != t2.xIndex || this->yIndex != t2.yIndex);
}

bool Term::operator==(const Term &t2)
{
    return (coefficient == t2.coefficient && xIndex == t2.xIndex && yIndex == t2.yIndex);
}

bool Term::operator==(Term t2)
{
    return (coefficient == t2.coefficient && xIndex == t2.xIndex && yIndex == t2.yIndex);
}

Expression::Expression()
{
    express.clear();
}

Expression::~Expression()
{
}

Expression Expression::operator*(Expression ex2)
{
    Expression result;
    Q_FOREACH (Term i, this->express) {
        qreal i_coef = i.getCoefficient();
        int i_x = i.getXIndex();
        int i_y = i.getYIndex();
        Q_FOREACH (Term j, ex2.express) {
            qreal j_coef = j.getCoefficient();
            int j_x = j.getXIndex();
            int j_y = j.getYIndex();

            Term resultant_Term;
            resultant_Term.setCoefficient(i_coef * j_coef);
            resultant_Term.setXIndex(i_x + j_x);
            resultant_Term.setYIndex(i_y + j_y);
            result.add(resultant_Term);
        }
    }

    return result;
}

Expression Expression::operator*(qreal multiplying_factor)
{
    Expression result;
    Q_FOREACH (Term i, this->express) {
        Term resTerm(i.getCoefficient() * multiplying_factor, i.getXIndex(), i.getYIndex());
        result.add(resTerm);
    }

    return result;
}

Expression Expression::operator+(Expression ex)
{
    Expression result;

    Q_FOREACH (Term i, express) {
        result.add(i);
    }
    result.add(ex);

    return result;
}

Expression Expression::operator-(Expression ex)
{
    Expression result;

    result.add(*this);
    result.subtract(ex);

    return result;
}

bool Expression::operator==(Expression &ex)
{
    if (this->express.size() != ex.express.size()) {
        return false;
    }

    for (int i = 0; i < express.size(); i++) {
        if (express[i] != ex.express[i]) {
            return false;
        }
    }

    return true;
}

void Expression::add(Term t)
{
    if (express.size() == 0) {
        express.push_back(t);
        return;
    }
    for (int i = 0; i < this->express.size(); i++) {
        Term curr = express.at(i);

        if ((curr.getXIndex() == t.getXIndex()) && (curr.getYIndex() == t.getYIndex())) {
            (express[i]).setCoefficient(curr.getCoefficient() + t.getCoefficient());
            return;
        }
    }
    express.push_back(t);
}

void Expression::add(Expression ex)
{
    Q_FOREACH (Term i, ex.express) {
        this->add(i);
    }
}

void Expression::subtract(Term t)
{
    if (express.size() == 0) {
        // empty Expression, Expression remains same
        return;
    }
    for (int i = 0; i < this->express.size(); i++) {
        Term curr = express.at(i);
        if ((curr.getXIndex() == t.getXIndex()) && (curr.getYIndex() == t.getYIndex())) {
            express[i].setCoefficient(curr.getCoefficient() - t.getCoefficient());
            return;
        }
    }
    t.setCoefficient(-t.getCoefficient());
    this->express.push_back(t);
}

void Expression::subtract(Expression ex)
{
    Q_FOREACH (Term i, ex.express) {
        this->subtract(i);
    }
}

QVector<qreal> Expression::coeffs()
{
    QVector<qreal> result;
    Q_FOREACH (Term i, express) {
        result.push_back(i.getCoefficient());
    }
    return result;
}

qreal Expression::evaluate(qreal x)
{
    qreal result = 0;
    Q_FOREACH (Term i, express) {
        if (i.yIndex != 0) {
            qWarning("WARNING: equation has two variables, only one is expected");
        }
        result += i.getCoefficient() * pow(x, i.getXIndex());
    }

    return result;
}

qreal Expression::evaluate2d(qreal x, qreal y)
{
    qreal result = 0;
    Q_FOREACH (Term i, express) {
        result += i.getCoefficient() * pow(x, i.getXIndex()) * pow(y, i.getYIndex());
    }

    return result;
}

void Expression::validateLeadingCoefficient()
{
    sortExpress(descending);
    while (express.first().coefficient == 0 && express.size() >= 0) {
        express.removeFirst();
    }
}

Expression Expression::powExpression(int n)
{
    Expression result;
    Expression result2;
    result.add(*this);

    for (int i = 1; i < n; i++) {
        result2 = result * (*this);
        result = result2;
    }

    return result;
}

QVector<Term> Expression::getExpression()
{
    return this->express;
}

bool Expression::greaterThan(Term &t1, Term &t2)
{
    // compares the x index of two Terms and returns true if first is
    // smaller than the other
    return t1.xIndex < t2.xIndex;
}

bool Expression::smallerThan(Term &t1, Term &t2)
{
    // compares the x index of two Terms and returns true if first is greater
    // than the other
    return t1.xIndex > t2.xIndex;
}

void Expression::sortExpress(Expression::sortOrder order = ascending)
{
    std::sort(this->express.begin(), this->express.end(), (order == ascending ? greaterThan : smallerThan));
}

CubicBezier::CubicBezier()
{
}

CubicBezier::~CubicBezier()
{
}

CubicBezier::CubicBezier(QPointF &cp_0, QPointF &cp_1, QPointF &cp_2, QPointF &cp_3)
    : cp0(cp_0)
    , cp1(cp_1)
    , cp2(cp_2)
    , cp3(cp_3)
{
    control_points.append(QVector<QPointF>{cp0, cp1, cp2, cp3});

    Expression ex;
    QVector<Expression> row{3, ex};

    for (int i = 0; i < 3; i++) {
        bezierMatrix.push_back(row);
    }

    generateParametricEquations();
    implicit_eq = getImplicitEquation();

    computeBoundingBox();
}

CubicBezier::CubicBezier(QPainterPath::Element curve)
{
    cp0 = curve;
}

QVector<CubicBezier> CubicBezier::splitCurve(qreal parameter)
{
    QVector<CubicBezier> result;

    QPointF cp0naught = (cp1 - cp0) * parameter + cp0;
    QPointF cp1naught = (cp2 - cp1) * parameter + cp1;
    QPointF cp2naught = (cp3 - cp2) * parameter + cp2;

    QPointF cp0naughtnaught = (cp1naught - cp0naught) * parameter + cp0naught;
    QPointF cp1naughtnaught = (cp2naught - cp1naught) * parameter + cp1naught;

    QPointF pointAtT(parametric_x.evaluate(parameter), parametric_y.evaluate(parameter));

    CubicBezier left(cp0, cp0naught, cp0naughtnaught, pointAtT);
    CubicBezier right(pointAtT, cp1naughtnaught, cp2naught, cp3);

    result.append(left);
    result.append(right);

    return result;
}

QVector<CubicBezier> CubicBezier::splitCurve(qreal parameter, QPointF splittingPoint)
{
    QVector<CubicBezier> result;

    qreal adjustedParameter = inversionEquationEvaluated(splittingPoint);

    QPointF cp0naught = (cp1 - cp0) * adjustedParameter + cp0;
    QPointF cp1naught = (cp2 - cp1) * adjustedParameter + cp1;
    QPointF cp2naught = (cp3 - cp2) * adjustedParameter + cp2;

    QPointF cp0naughtnaught = (cp1naught - cp0naught) * adjustedParameter + cp0naught;
    QPointF cp1naughtnaught = (cp2naught - cp1naught) * adjustedParameter + cp1naught;

    QPointF pointAtT(parametric_x.evaluate(adjustedParameter), parametric_y.evaluate(adjustedParameter));

    CubicBezier left(cp0, cp0naught, cp0naughtnaught, splittingPoint);
    CubicBezier right(splittingPoint, cp1naughtnaught, cp2naught, cp3);

    result.append(left);
    result.append(right);

    return result;
}

void CubicBezier::generateParametricEquations()
{
    Term Term_t3(cp3.x() - 3 * cp2.x() + 3 * cp1.x() - cp0.x(), 3, 0);
    Term Term_t2(3 * cp2.x() - 6 * cp1.x() + 3 * cp0.x(), 2, 0);
    Term Term_t1(3 * cp1.x() - 3 * cp0.x(), 1, 0);
    Term Term_t0(cp0.x(), 0, 0);

    Term Term_t3_y(cp3.y() - 3 * cp2.y() + 3 * cp1.y() - cp0.y(), 3, 0);
    Term Term_t2_y(3 * cp2.y() - 6 * cp1.y() + 3 * cp0.y(), 2, 0);
    Term Term_t1_y(3 * cp1.y() - 3 * cp0.y(), 1, 0);
    Term Term_t0_y(cp0.y(), 0, 0);

    parametric_x.add(Term_t3);
    parametric_x.add(Term_t2);
    parametric_x.add(Term_t1);
    parametric_x.add(Term_t0);

    parametric_y.add(Term_t3_y);
    parametric_y.add(Term_t2_y);
    parametric_y.add(Term_t1_y);
    parametric_y.add(Term_t0_y);
}

Expression CubicBezier::element_entry(int first_pt, int second_pt)
{
    // generates the element entry needed for implicitization matrix

    QPointF p1(control_points[first_pt]);
    QPointF p2(control_points[second_pt]);

    Term x_Term{p1.y() - p2.y(), 1, 0};
    Term y_Term{p2.x() - p1.x(), 0, 1};
    Term const_Term{(p1.x() * p2.y()) - (p1.y() * p2.x()), 0, 0};
    Expression entry;
    entry.add(x_Term);
    entry.add(y_Term);
    entry.add(const_Term);

    int combinatorial_factor = (combinatorial(3, first_pt) * combinatorial(3, second_pt));

    entry = entry * combinatorial_factor;

    return entry;
}

Expression CubicBezier::getImplicitEquation()
{
    Expression result;

    bezierMatrix[0][0] = element_entry(3, 2);
    bezierMatrix[0][1] = element_entry(3, 1);
    bezierMatrix[0][2] = element_entry(3, 0);

    bezierMatrix[1][0] = element_entry(3, 1);
    bezierMatrix[1][1] = element_entry(3, 0) + (element_entry(2, 1));
    bezierMatrix[1][2] = element_entry(2, 0);

    bezierMatrix[2][0] = element_entry(3, 0);
    bezierMatrix[2][1] = element_entry(2, 0);
    bezierMatrix[2][2] = element_entry(1, 0);

    Expression cofactor_00 = bezierMatrix[0][0] * ((bezierMatrix[1][1] * bezierMatrix[2][2]) - (bezierMatrix[1][2] * bezierMatrix[2][1]));

    Expression cofactor_01 = bezierMatrix[0][1] * ((bezierMatrix[1][0] * bezierMatrix[2][2]) - (bezierMatrix[1][2] * bezierMatrix[2][0])) * -1;

    Expression cofactor_02 = bezierMatrix[0][2] * ((bezierMatrix[1][0] * bezierMatrix[2][1]) - (bezierMatrix[1][1] * bezierMatrix[2][0]));

    result.add(cofactor_00);
    result.add(cofactor_01);
    result.add(cofactor_02);

    return result;
}

void CubicBezier::computeBoundingBox()
{
    qreal xMin = control_points[0].rx();
    qreal xMax = control_points[0].rx();

    qreal yMin = control_points[0].ry();
    qreal yMax = control_points[0].ry();

    Q_FOREACH (QPointF pt, control_points) {
        if (pt.rx() < xMin) {
            xMin = pt.rx();
        }

        if (pt.rx() > xMax) {
            xMax = pt.rx();
        }

        if (pt.ry() < yMin) {
            yMin = pt.ry();
        }

        if (pt.ry() > yMax) {
            yMax = pt.ry();
        }
    }

    bbox = QRectF(QPointF(xMax, yMax), QPointF(xMin, yMin));
}

QRectF CubicBezier::boundingBox()
{
    return bbox;
}

void CubicBezier::computeExactBoundingBox()
{
    qreal xMin, xMax, yMin, yMax;

    qreal ax = parametric_x.coeffs().at(0); //, xExtrema2, yExtrema1, yExtrema2;
    qreal bx = parametric_x.coeffs().at(1);
    qreal cx = parametric_x.coeffs().at(2);
    // qreal dx = parametric_x.coeffs().at(3);

    qreal ay = parametric_y.coeffs().at(0); //, xExtrema2, yExtrema1, yExtrema2;
    qreal by = parametric_y.coeffs().at(1);
    qreal cy = parametric_y.coeffs().at(2);
    // qreal dy = parametric_y.coeffs().at(3);

    if (4 * (bx * bx) >= 12 * ax * cx) {
        qreal xExtrema1 = ((-2 * bx) + (qSqrt(4 * (bx * bx) - 12 * ax * cx))) / (6 * ax);
        qreal xExtrema2 = ((-2 * bx) - (qSqrt(4 * (bx * bx) - 12 * ax * cx))) / (6 * ax);

        bool xExtrema1isValid = xExtrema1 >= 0 && xExtrema1 <= 1;
        bool xExtrema2isValid = xExtrema2 >= 0 && xExtrema2 <= 1;

        if (xExtrema1isValid && xExtrema2isValid) {
            xMin = qMin(qMin(cp0.rx(), cp3.rx()), qMin(parametric_x.evaluate(xExtrema1), parametric_x.evaluate(xExtrema2)));
            xMax = qMax(qMax(cp0.rx(), cp3.rx()), qMax(parametric_x.evaluate(xExtrema1), parametric_x.evaluate(xExtrema2)));
        }

        else if (xExtrema1isValid) {
            xMin = qMin(qMin(cp0.rx(), cp3.rx()), parametric_x.evaluate(xExtrema1));
            xMax = qMax(qMax(cp0.rx(), cp3.rx()), parametric_x.evaluate(xExtrema1));
        }

        else if (xExtrema2isValid) {
            xMin = qMin(qMin(cp0.rx(), cp3.rx()), parametric_x.evaluate(xExtrema2));
            xMax = qMax(qMax(cp0.rx(), cp3.rx()), parametric_x.evaluate(xExtrema2));
        }

        else {
            xMin = qMin(cp0.rx(), cp3.rx());
            xMax = qMax(cp0.rx(), cp3.rx());
        }
    }

    else {
        xMin = qMin(cp0.rx(), cp3.rx());
        xMax = qMax(cp0.rx(), cp3.rx());
    }

    if (4 * (by * by) >= 12 * ay * cy) { //

        qreal yExtrema1 = ((-2 * by) + (qSqrt(4 * (by * by) - 12 * ay * cy))) / (6 * ay);
        qreal yExtrema2 = ((-2 * by) - (qSqrt(4 * (by * by) - 12 * ax * cy))) / (6 * ay);

        bool yExtrema1isValid = yExtrema1 >= 0 && yExtrema1 <= 1;
        bool yExtrema2isValid = yExtrema2 >= 0 && yExtrema2 <= 1;

        if (yExtrema1isValid && yExtrema2isValid) {
            yMin = qMin(qMin(cp0.ry(), cp3.ry()), qMin(parametric_x.evaluate(yExtrema1), parametric_x.evaluate(yExtrema2)));
            yMax = qMax(qMax(cp0.ry(), cp3.ry()), qMax(parametric_x.evaluate(yExtrema1), parametric_x.evaluate(yExtrema2)));
        }

        else if (yExtrema1isValid) {
            yMin = qMin(qMin(cp0.ry(), cp3.ry()), parametric_x.evaluate(yExtrema1));
            yMax = qMax(qMax(cp0.ry(), cp3.ry()), parametric_x.evaluate(yExtrema1));
        }

        else if (yExtrema2isValid) {
            yMin = qMin(qMin(cp0.ry(), cp3.ry()), parametric_x.evaluate(yExtrema2));
            yMax = qMax(qMax(cp0.ry(), cp3.ry()), parametric_x.evaluate(yExtrema2));
        }

        else {
            yMin = qMin(cp0.ry(), cp3.ry());
            yMax = qMax(cp0.ry(), cp3.ry());
        }
    }

    else {
        yMin = qMin(cp0.ry(), cp3.ry());
        yMax = qMax(cp0.ry(), cp3.ry());
    }

    bbox = QRectF(QPointF(xMin, yMin), QPointF(xMax, yMax));

    // return QRectF( QPointF(xMax, yMax), QPointF(xMin,yMin));
}
qreal CubicBezier::inversionEquationEvaluated(QPointF &p)
{
    // l b (x, y) − l a (x, y)

    //    l a (x, y) = c 1 l 31 (x, y) + c 2 [l 30 (x, y) + l 21 (x, y)] + l 20
    //    (x, y) l b (x, y) = c 1 l 30 (x, y) + c 2 l 20 (x, y) + l 10 (x, y)

    if (inversionEquationDen.getExpression().size() == 0 && inversionEquationNum.getExpression().size() == 0) {
        qreal c1Num = cp0.x() * (cp1.y() - cp3.y()) - cp0.y() * (cp1.x() - cp3.x()) + cp1.x() * cp3.y() - cp1.y() * cp3.x();

        qreal c1Den = cp1.x() * (cp2.y() - cp3.y()) - cp1.y() * (cp2.x() - cp3.x()) + cp2.x() * cp3.y() - cp3.x() * cp2.y();
        qreal c1 = c1Num / (3 * c1Den);

        qreal c2Num = cp0.x() * (cp2.y() - cp3.y()) - cp0.y() * (cp2.x() - cp3.x()) + cp2.x() * cp3.y() - cp2.y() * cp3.x();
        qreal c2Den = c1Den;

        qreal c2 = -(c2Num / (3 * c2Den));

        Expression la = bezierMatrix[0][1] * c1 + bezierMatrix[1][1] * c2 + bezierMatrix[1][2];
        Expression lb = bezierMatrix[0][2] * c1 + bezierMatrix[1][2] * c2 + bezierMatrix[2][2];

        inversionEquationNum = lb;
        inversionEquationDen = lb - la;
    }

    qreal t = inversionEquationNum.evaluate2d(p.x(), p.y()) / inversionEquationDen.evaluate2d(p.x(), p.y());

    return t;
}

QVector<qreal> CubicBezier::findRoots(CubicBezier &cb2)
{
    Expression impl = cb2.getImplicitEquation();

    Expression result;
    Term unity{1, 0, 0};

    QVector<Term> implTerms = impl.getExpression();

    Q_FOREACH (auto i, implTerms) {
        Expression individual_result;

        Expression e1, e2;

        if (i.getXIndex()) {
            e1 = parametric_x.powExpression(i.getXIndex());
        }

        else {
            e1.add(unity);
        }

        if (i.getYIndex()) {
            e2 = parametric_y.powExpression(i.getYIndex());
        }

        else {
            e2.add(unity);
        }

        individual_result = (e1 * e2) * i.getCoefficient();

        result.add(individual_result);
    }

    result.validateLeadingCoefficient();

    result.sortExpress();

    int numTerms = result.getExpression().size();

    qreal arr[numTerms];

    for (int j = 0; j < numTerms; j++) {
        arr[j] = (result.getExpression()[j]).getCoefficient();
    }

    qreal z[2 * numTerms];
    gsl_poly_complex_workspace *w = gsl_poly_complex_workspace_alloc(numTerms);
    gsl_poly_complex_solve(arr, numTerms, w, z);
    gsl_poly_complex_workspace_free(w);

    QVector<qreal> t_roots;

    for (int i = 0; i < numTerms; i++) {
        if (z[2 * i] > 0 && z[2 * i] < 1 && qFuzzyIsNull(z[2 * i + 1])) {
            t_roots.push_back(z[2 * i]);
        }
    }

    return t_roots;
}

Line::Line(QPointF p1, QPointF p2)
    : qtLine(p1, p2)
{
    ;
    // form:  mx - y + c = 0, where m represents slope and c represents the
    // intercept.

    if (qFuzzyCompare(qtLine.y1(), qtLine.y2())) {
        // line is of the form y = a, i.e 0x + y - a = 0
        lineType = Horizontal;
        intercept = -qtLine.y1();

        implicitEquation.add(Term(1, 0, 1));
        implicitEquation.add(Term(intercept, 0, 0));

    }

    else if (qFuzzyCompare(qtLine.x1(), qtLine.x2())) {
        // line is of the form x = a, i.e x + 0y - a = 0
        lineType = Vertical;
        intercept = -qtLine.x1();

        implicitEquation.add(Term(1, 1, 0));
        implicitEquation.add(Term(intercept, 0, 0));
    }

    else {
        lineType = Oblique;
        slope = (qtLine.y2() - qtLine.y1()) / (qtLine.x2() - qtLine.x1());
        intercept = qtLine.y1() - (slope * qtLine.x1());

        implicitEquation.add(Term(slope, 1, 0));
        implicitEquation.add(Term(-1, 0, 1));
        implicitEquation.add(Term(intercept, 0, 0));

        parametrixX.add(Term(qtLine.x1(), 0, 0));
        parametrixX.add(Term(qtLine.dx(), 1, 0));

        parametricY.add(Term(qtLine.y1(), 0, 0));
        parametricY.add(Term(qtLine.dy(), 1, 0));
    }
}

Line::Line()
{
}

Line::~Line()
{
}

Line::Line(QLineF line)
    : qtLine(line)
{
    // form:  mx - y + c = 0, where m represents slope and c represents the
    // intercept.

    if (qFuzzyCompare(qtLine.y1(), qtLine.y2())) {
        // line is of the form y = a, i.e 0x + y - a = 0
        lineType = Horizontal;
        //        slope = 1;
        intercept = -line.y1();

        implicitEquation.add(Term(1, 0, 1));
        implicitEquation.add(Term(intercept, 0, 0));

    }

    else if (qFuzzyCompare(qtLine.x1(), qtLine.x2())) {
        // line is of the form x = a, i.e x + 0y - a = 0
        lineType = Vertical;
        intercept = -line.x1();

        implicitEquation.add(Term(1, 1, 0));
        implicitEquation.add(Term(intercept, 0, 0));
    }

    else {
        lineType = Oblique;
        slope = (line.y2() - line.y1()) / (line.x2() - line.x1());
        intercept = line.y1() - (slope * line.x1());

        implicitEquation.add(Term(slope, 1, 0));
        implicitEquation.add(Term(-1, 0, 1));
        implicitEquation.add(Term(intercept, 0, 0));

        parametrixX.add(Term(line.x1(), 0, 0));
        parametrixX.add(Term(line.dx(), 1, 0));

        parametricY.add(Term(line.y1(), 0, 0));
        parametricY.add(Term(line.dy(), 1, 0));
    }
}

QLineF Line::getQLine()
{
    return qtLine;
}

QVector<Line> Line::splitLine(qreal parameter)
{
    QVector<Line> result;
    result.push_back(Line(qtLine.p1(), qtLine.pointAt(parameter)));
    result.push_back(Line(qtLine.pointAt(parameter), qtLine.p2()));

    return result;
}

QVector<Line> Line::splitLine(QVector<QPointF> points)
{
    QVector<Line> res;
    QVector<qreal> params;
    params.push_back(0);
    params.push_back(1);

    if (isVertical()) {
        Q_FOREACH (QPointF p, points) {
            params.push_back((p.y() - qtLine.y1()) / qtLine.dy());
        }
    }

    else {
        Q_FOREACH (QPointF p, points) {
            params.push_back((p.x() - qtLine.x1()) / qtLine.dx());
        }
    }

    std::sort(params.begin(), params.end());
    auto uq = std::unique(params.begin(), params.begin() + params.count());
    points.resize(std::distance(params.begin(), uq));

    for (int i = 0; i < params.size() - 1; i++) {
        res.push_back(Line(qtLine.pointAt(params.at(i)), qtLine.pointAt(params.at(i + 1))));
    }

    return res;
}

Expression Line::getImplicitEquation()
{
    return implicitEquation;
}

QVector<qreal> Line::findRoots(CubicBezier &cb)
{
    Expression implicitExpression = this->getImplicitEquation();
    Expression result;

    if (isVertical()) {
        // all points on the line segment will have the same x co-ordinate,
        // hence we can obtain the parameter value t by substituting the
        // value of intercept in the parametric equation of x of the
        // Bezier curve.

        result.add(cb.getParametricX());
        result.add(Term(intercept, 0, 0));
    }

    else if (isHorizontal()) {
        result.add(cb.getParametricY());
        result.add(Term(intercept, 0, 0));
    }

    else if (isOblique()) {
        Q_FOREACH (auto i, implicitExpression.getExpression()) {
            Expression individual_result;
            Term unity{1, 0, 0};

            Expression e1, e2;
            if (i.getXIndex() != 0) {
                e1.add(cb.getParametricX());
            } else {
                e1.add(unity);
            }

            if (i.getYIndex() != 0) {
                e2.add(cb.getParametricY());
            } else {
                e2.add(unity);
            }

            individual_result = (e1 * e2) * i.getCoefficient();

            result.add(individual_result);
        }
    }

    result.sortExpress(Expression::descending);
    result.validateLeadingCoefficient();
    result.sortExpress(Expression::ascending);

    int numTerms = result.getExpression().size();

    qreal arr[numTerms];

    for (int j = 0; j < numTerms; j++) {
        arr[j] = (result.getExpression()[j]).getCoefficient();
    }

    qreal z[2 * numTerms];

    gsl_poly_complex_workspace *w = gsl_poly_complex_workspace_alloc(numTerms);

    gsl_poly_complex_solve(arr, numTerms, w, z);

    gsl_poly_complex_workspace_free(w);

    QVector<qreal> t_roots;

    for (int i = 0; i < numTerms; i++) {
        if (z[2 * i] > 0 && z[2 * i] < 1 && qFuzzyIsNull(z[2 * i + 1])) {
            t_roots.push_back(z[2 * i]);
        }
    }

    //    std::cout << " intersection points in findroots(line,curve)" <<
    //    std::endl; Q_FOREACH ( auto i , t_roots ) {

    //        std::cout << "x: " << cb.getParametricX().evaluate (i) << " y: " <<
    //        cb.getParametricY().evaluate (i) << std::endl;
    //        }

    return t_roots;
}

bool Line::checkIntersection(Line &l2)
{
    QPointF *intersection;
    // intersect() is deprecated, however the newer function was introduced in
    // version 5.14
    QLineF::IntersectType intersect = this->getQLine().intersect(l2.getQLine(), intersection);

    return (intersect == QLineF::BoundedIntersection) ? true : false;
}

bool Line::checkIntersection(QLineF &l2)
{
    QPointF *intersection;
    // intersect() is deprecated, however the newer function was introduced in
    // version 5.14

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QLineF::IntersectionType intersect2 = l2.intersects(this->getQLine(), intersection);
    return (intersect2 == QLineF::IntersectType::BoundedIntersection) ? true : false;
#else
    QLineF::IntersectType intersect = l2.intersect(this->getQLine(), intersection);
    return (intersect == QLineF::IntersectType::BoundedIntersection) ? true : false;
#endif
}

bool Line::checkIntersection(QPointF &PointToCheck)
{
    qreal resultant = this->getImplicitEquation().evaluate2d(PointToCheck.x(), PointToCheck.y());

    // as we're dealing with floating points, some error is generated. Hence,
    // resultant is not equal to zero even though the point lies on the line.
    if (abs(resultant) > 0.0000001) {
        return false;
    }

    // the point must lie on the line segment, hence its co-ordinates must be in
    // between the end points of the line segment
    if (qMin(qtLine.p1().x(), qtLine.p2().x()) < PointToCheck.x() + 1e-6 && qMax(qtLine.p1().x(), qtLine.p2().x()) > PointToCheck.x() - 1e-6
        && qMin(qtLine.p1().y(), qtLine.p2().y()) < PointToCheck.y() + 1e-6 && qMax(qtLine.p1().y(), qtLine.p2().y()) > PointToCheck.y() - 1e-6) {
        return true;
    }

    else {
        return false;
    }
}

Expression Line::getParametricX()
{
    return this->parametrixX;
}

Expression Line::getParametricY()
{
    return this->parametricY;
}

bool Line::isVertical()
{
    return lineType == Vertical ? true : false;
}

bool Line::isHorizontal()
{
    return lineType == Horizontal ? true : false;
}

bool Line::isOblique()
{
    return lineType == Oblique ? true : false;
}
