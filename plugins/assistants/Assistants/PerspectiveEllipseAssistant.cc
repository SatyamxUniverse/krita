/*
 * SPDX-FileCopyrightText: 2022 Srirupa Datta <srirupa.sps@gmail.com>
 */

#include "PerspectiveEllipseAssistant.h"
#include "PerspectiveBasedAssistantHelper.h"


#include <klocalizedstring.h>
#include "kis_debug.h"
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QTransform>

#include <kis_canvas2.h>
#include <kis_coordinates_converter.h>
#include "kis_algebra_2d.h"
#include <kis_dom_utils.h>
#include <Eigen/Eigenvalues>
#include "EllipseInPolygon.h"

// potentially to remove later, used for debug floating messages
//#include "KisPart.h"
//#include "KisViewManager.h"
//#include <KisMainWindow.h>

#include <math.h>
#include<QDebug>
#include <QtMath>

#include <functional>

// ################################## Perspective Ellipse Assistant #######################################


class PerspectiveEllipseAssistant::Private
{
public:
    EllipseInPolygon ellipseInPolygon;
    EllipseInPolygon concentricEllipseInPolygon;
    EllipseInPolygon concentricEllipseInPolygonMirrored;

    Ellipse simpleEllipse;
    Ellipse simpleConcentricEllipse;
    Ellipse simpleConcentricEllipseMirrored;


    bool cacheValid { false };

    bool isConcentric {false};
    bool ellipsesCorrectInStroke {false};
    bool useMirrored {false};
    bool useMirroredPreview {false};

    PerspectiveBasedAssistantHelper::CacheData cache;

    QVector<QPointF> cachedPoints; // points on the polygon

};

PerspectiveEllipseAssistant::PerspectiveEllipseAssistant(QObject *parent)
    : KisAbstractPerspectiveGrid(parent)
    , KisPaintingAssistant("perspective ellipse", i18n("Perspective Ellipse assistant"))
    , d(new Private())
{

}

PerspectiveEllipseAssistant::~PerspectiveEllipseAssistant() {}

PerspectiveEllipseAssistant::PerspectiveEllipseAssistant(const PerspectiveEllipseAssistant &rhs, QMap<KisPaintingAssistantHandleSP, KisPaintingAssistantHandleSP> &handleMap)
    : KisAbstractPerspectiveGrid(rhs.parent())
    , KisPaintingAssistant(rhs, handleMap)
    , d(new Private())
{
    d->isConcentric = rhs.isConcentric();
}

KisPaintingAssistantSP PerspectiveEllipseAssistant::clone(QMap<KisPaintingAssistantHandleSP, KisPaintingAssistantHandleSP> &handleMap) const
{
    return KisPaintingAssistantSP(new PerspectiveEllipseAssistant(*this, handleMap));
}

QPointF PerspectiveEllipseAssistant::project(const QPointF& pt, const QPointF& strokeBegin)
{
    Q_ASSERT(isAssistantComplete());

    if (d->isConcentric) {
        if (d->useMirrored) {
            //d->concentricEllipseInPolygon.projectModifiedEberlySecond(pt);
            return d->concentricEllipseInPolygon.project(pt, &strokeBegin);
        } else {
            //d->concentricEllipseInPolygonMirrored.projectModifiedEberlySecond(pt);
            return d->concentricEllipseInPolygonMirrored.project(pt, &strokeBegin);
        }

    } else {
        //d->ellipseInPolygon.projectModifiedEberlySecond(pt);
        return d->ellipseInPolygon.project(pt, &strokeBegin);
        //d->ellipseInPolygon.setSimpleEllipseVertices(d->simpleEllipse);
        //return d->simpleEllipse.project(pt);
    }
}

QPointF PerspectiveEllipseAssistant::adjustPosition(const QPointF& pt, const QPointF& strokeBegin, const bool /*snapToAny*/, qreal /*moveThresholdPt*/)
{
    return project(pt, strokeBegin);
}

void PerspectiveEllipseAssistant::adjustLine(QPointF &point, QPointF &strokeBegin)
{
    point = QPointF();
    strokeBegin = QPointF();
}

void PerspectiveEllipseAssistant::drawAssistant(QPainter& gc, const QRectF& updateRect, const KisCoordinatesConverter* converter, bool cached, KisCanvas2* canvas, bool assistantVisible, bool previewVisible)
{
    gc.save();
    gc.resetTransform();

    bool isEditing = false;
    
    QTransform initialTransform = converter->documentToWidgetTransform();

    // need to update ellipse cache
    updateCache();

    QPolygonF poly = d->ellipseInPolygon.polygon;
    QTransform transform = d->ellipseInPolygon.originalTransform; // unused, but computed for caching purposes


    if (isEllipseValid() && assistantVisible==true) {
        // draw vanishing points
        if (d->cache.vanishingPoint1) {
            drawX(gc, initialTransform.map(d->cache.vanishingPoint1.get()));
        }
        if (d->cache.vanishingPoint2) {
            drawX(gc, initialTransform.map(d->cache.vanishingPoint2.get()));
        }
    }
    
    QPointF mousePos = effectiveBrushPosition(converter, canvas);

    /*
    if (m_followBrushPosition && m_adjustedPositionValid) {
        QPainterPath lineBetweenMouseAndProjection;
        lineBetweenMouseAndProjection.moveTo(mousePos);
        lineBetweenMouseAndProjection.lineTo(mousePos + QPointF(50, 50)); //initialTransform.map(m_adjustedBrushPosition));
        //lineBetweenMouseAndProjection.lineTo(initialTransform.map(m_adjustedBrushPosition));
        drawPath(gc, lineBetweenMouseAndProjection);
        mousePos = initialTransform.map(m_adjustedBrushPosition);
    }
    */

    bool normalEllipseSeemsValid = false;
    bool mirroredEllipseSeemsValid = false;


    if (d->isConcentric && initialTransform.isInvertible()) {
        //ENTER_FUNCTION() << "Mouse pos was " << mousePos << "anty-transformed: " << initialTransform.inverted().map(mousePos);

        if (!(m_followBrushPosition && m_adjustedPositionValid)) // that would mean we're drawing, in which case, leave it alone
        {

            d->concentricEllipseInPolygon.updateToPointOnConcentricEllipse(d->ellipseInPolygon.originalTransform, initialTransform.inverted().map(mousePos), d->cache.horizon, false);
            normalEllipseSeemsValid = d->concentricEllipseInPolygon.setSimpleEllipseVertices(d->simpleConcentricEllipse);

            d->concentricEllipseInPolygonMirrored.updateToPointOnConcentricEllipse(d->ellipseInPolygon.originalTransform, initialTransform.inverted().map(mousePos), d->cache.horizon, true);
            mirroredEllipseSeemsValid = d->concentricEllipseInPolygonMirrored.setSimpleEllipseVertices(d->simpleConcentricEllipseMirrored);

            if (true || !d->ellipsesCorrectInStroke) {
                if (d->concentricEllipseInPolygon.onTheCorrectSideOfHorizon(initialTransform.inverted().map(mousePos))) {
                    d->useMirroredPreview = false;
                } else {
                    d->useMirroredPreview = true;
                }
            }

        }

        //ENTER_FUNCTION() << "Set points to simple ellipse:" << d->concentricEllipseInPolygon.finalVertices[0]
        //                 << d->concentricEllipseInPolygon.finalVertices[1] << d->concentricEllipseInPolygon.finalVertices[2];
        //ENTER_FUNCTION() << "Is transform identity? " << d->simpleConcentricEllipse.getTransform().isIdentity();
        //ENTER_FUNCTION() << "Transform:" << d->simpleConcentricEllipse.getTransform();
    }

    QPainterPath pathError;
    QPen pen10((qIsNaN(d->simpleConcentricEllipse.semiMajor()) || qIsInf(d->simpleConcentricEllipse.semiMajor()) || d->simpleConcentricEllipse.semiMajor() > 10000) ? Qt::darkBlue : Qt::darkMagenta);
    pen10.setWidth(10);

    gc.save();
    gc.setPen(pen10);

    pathError.addEllipse(QPointF(100, 100), 5, 5);

    gc.drawPath(pathError);
    QTransform tempTr = gc.transform();
    gc.setTransform(QTransform());
    gc.setFont(QFont("Helvetica", 6));
    QString semiM = QString::number(d->simpleConcentricEllipse.semiMajor());
    QTransform checkedTr = d->simpleConcentricEllipse.getTransform().inverted();
    QString message = semiM + " | " + QString::number(checkedTr.determinant()) + " | " + QString::number(checkedTr.m11()) + ", " + QString::number(checkedTr.m12()) + ", " + QString::number(checkedTr.m13());
    //KisPart::instance()->currentMainwindow()->viewManager()->showFloatingMessage(message, QIcon());
    gc.drawText(QPointF(200, 100), QString::number(d->simpleConcentricEllipse.semiMajor()));
    gc.setTransform(tempTr);
    gc.restore();

    // draw ellipse and axes
    if (isEllipseValid() && (assistantVisible || previewVisible || isEditing)) { // ensure that you only draw the ellipse if it's valid - otherwise it would just show some outdated one

        if (!isEditing && d->isConcentric && d->concentricEllipseInPolygon.isValid()) {

            gc.save();

            gc.setTransform(initialTransform);

            gc.setTransform(d->simpleConcentricEllipse.getTransform().inverted(), true);

            QPainterPath path2;

            //ENTER_FUNCTION() << "conc. ell. axis: " << d->simpleConcentricEllipse.semiMajor() << d->simpleConcentricEllipse.semiMinor()
            //                 << "normal ones: " << d->simpleEllipse.semiMajor() << d->simpleEllipse.semiMinor();
            //ENTER_FUNCTION() << "original radius: " << d->concentricEllipseInPolygon.originalCircleRadius;


            //QPen pen20(Qt::darkMagenta);
            //pen20.setWidth(50);

            //gc.setPen(pen20);
            path2.addEllipse(QPointF(0.0, 0.0), d->simpleConcentricEllipse.semiMajor(), d->simpleConcentricEllipse.semiMinor());
            //path2.addEllipse(QPointF(0.0, 0.0), 10, 10);


            //path2.addEllipse(d->simpleConcentricEllipse.getTransform().map(d->concentricEllipseInPolygon.finalVertices[0]), 5, 5);
            //path2.addEllipse(d->concentricEllipseInPolygon.finalVertices[1], 5, 5);
            //path2.addEllipse(d->concentricEllipseInPolygon.finalVertices[2], 5, 5);

            //drawPath(gc, path2, isSnappingActive());
            gc.drawPath(path2);

            //QPen pen2(QBrush(Qt::blue), 3);

            //gc.setPen(pen2);

            //gc.setTransform(initialTransform);
            //gc.setTransform(QTransform());
            //gc.drawRect(kisGrowRect(updateRect, -85));

            gc.restore();

            //QPen pen(QBrush(Qt::red), 3);
            //gc.save();
            //gc.setPen(pen);


            paintConcentricEllipse(gc, updateRect, initialTransform);
            //gc.restore();
        }
        if (!isEditing && d->isConcentric) {
            QPen pen2(QBrush(Qt::blue), 3);
            gc.save();
            gc.setPen(pen2);

            //gc.setTransform(initialTransform);
            gc.setTransform(QTransform());
            gc.drawRect(kisGrowRect(updateRect, -85));

            gc.restore();

            QPen pen(QBrush(Qt::red), 3);
            gc.save();
            gc.setPen(pen);

            paintConcentricEllipse(gc, updateRect, initialTransform);
            gc.restore();
        }



        gc.setTransform(initialTransform);
        gc.setTransform(d->simpleEllipse.getTransform().inverted(), true);

        QPainterPath path;

        // needed - it's the ellipse inside
        path.addEllipse(QPointF(0.0, 0.0), d->simpleEllipse.semiMajor(), d->simpleEllipse.semiMinor());
        drawPath(gc, path, isSnappingActive());

        if (isEditing) {
            QPainterPath axes;
            axes.moveTo(QPointF(-d->simpleEllipse.semiMajor(), 0));
            axes.lineTo(QPointF(d->simpleEllipse.semiMajor(), 0));

            axes.moveTo(QPointF(0, -d->simpleEllipse.semiMinor()));
            axes.lineTo(QPointF(0, d->simpleEllipse.semiMinor()));

            gc.save();

            QPen p(gc.pen());
            p.setCosmetic(true);
            p.setStyle(Qt::DotLine);
            QColor color = effectiveAssistantColor();
            if (!isSnappingActive()) {
                color.setAlpha(color.alpha()*0.2);
            }
            p.setWidthF(1.5);
            p.setColor(color);
            gc.setPen(p);

            gc.drawPath(axes);

            gc.restore();
        }

        gc.setTransform(converter->documentToWidgetTransform());
        gc.setTransform(d->ellipseInPolygon.originalTransform, true);

        // drawing original axes ("lines to touching points")
        QPointF pt1 = QPointF(0.5, 1.0);
        QPointF pt2 = QPointF(1.0, 0.5);
        QPointF pt3 = QPointF(0.5, 0.0);
        QPointF pt4 = QPointF(0.0, 0.5);

        QPainterPath touchingLine;

        touchingLine.moveTo(pt1);
        touchingLine.lineTo(pt3);

        touchingLine.moveTo(pt2);
        touchingLine.lineTo(pt4);
        
        touchingLine.moveTo(pt2);
        touchingLine.lineTo(pt4);

        if (assistantVisible) {
            drawPath(gc, touchingLine, isSnappingActive());
        }

        //gc.drawEllipse(QPointF(0.5, 0.5), d->concentricEllipseInPolygon.originalCircleRadius, d->concentricEllipseInPolygon.originalCircleRadius);
    }


    gc.setTransform(converter->documentToWidgetTransform());

    if (assistantVisible || isEditing) {
        if (!isEllipseValid()) {
            // color red for an invalid transform, but not for an incomplete one
            if(isAssistantComplete()) {
                QPainterPath path;
                QPolygonF polyAllConnected;
                // that will create a triangle with a point inside connected to all vertices of the triangle
                polyAllConnected << *handles()[0] << *handles()[1] << *handles()[2] << *handles()[3] << *handles()[0] << *handles()[2] << *handles()[1] << *handles()[3];
                path.addPolygon(polyAllConnected);
                drawError(gc, path);
            } else {
                QPainterPath path;
                path.addPolygon(poly);
                drawPath(gc, path, isSnappingActive());
            }
        } else {
            gc.setPen(QColor(0, 0, 0, 125));
            gc.setTransform(transform, true);
            QPainterPath path;
            for (int y = 0; y <= 1; ++y)
            {
                QLineF line = QLineF(QPointF(0.0, y), QPointF(1.0, y));
                KisAlgebra2D::cropLineToRect(line, gc.window(), false, false);
                path.moveTo(line.p1());
                path.lineTo(line.p2());
            }
            for (int x = 0; x <= 1; ++x)
            {
                QLineF line = QLineF(QPointF(x, 0.0), QPointF(x, 1.0));
                KisAlgebra2D::cropLineToRect(line, gc.window(), false, false);
                path.moveTo(line.p1());
                path.lineTo(line.p2());
            }

            drawPath(gc, path, isSnappingActive());
        }
    }
    
    gc.restore();
    KisPaintingAssistant::drawAssistant(gc, updateRect, converter, cached, canvas, assistantVisible, previewVisible);

}


void PerspectiveEllipseAssistant::drawCache(QPainter& gc, const KisCoordinatesConverter *converter, bool assistantVisible)
{
    Q_UNUSED(converter);
    Q_UNUSED(gc);
    Q_UNUSED(assistantVisible);
}

QRect PerspectiveEllipseAssistant::boundingRect() const
{
     if (!isAssistantComplete()) {
       return KisPaintingAssistant::boundingRect();
    }

    if (d->ellipseInPolygon.setSimpleEllipseVertices(d->simpleEllipse)) {
       return d->simpleEllipse.boundingRect().adjusted(-2, -2, 2, 2).toAlignedRect();
    } else {
       return QRect();
    }
}

QPointF PerspectiveEllipseAssistant::getDefaultEditorPosition() const
{
    QPointF centroid(0, 0);
    for (int i = 0; i < 4; ++i) {
        centroid += *handles()[i];
    }

    return centroid * 0.25;
}

bool PerspectiveEllipseAssistant::isEllipseValid()
{
    return isAssistantComplete() && d->ellipseInPolygon.isValid();
}

void PerspectiveEllipseAssistant::updateCache()
{
    // handles -> points -> polygon
    d->cacheValid = false;
    // check the cached points, whether they are the same as handles
    if (d->cachedPoints.size() == handles().size()) {
        for (int i = 0; i < handles().size(); ++i) {
            if (d->cachedPoints[i] != *handles()[i]) break;
            if (i == handles().size() - 1) {
                // that means the cache is up to date, because the loop was still going
                d->cacheValid = true;
                return;
            }
        }
    }

    d->cachedPoints = QVector<QPointF>();
    for (int i = 0; i < handles().size(); ++i) {
        d->cachedPoints << *handles()[i];
    }


    QPolygonF poly = QPolygonF(d->cachedPoints);

    if (!PerspectiveBasedAssistantHelper::getTetragon(handles(), isAssistantComplete(), poly)) { // this function changes poly to some "standarized" version, or a triangle when it cannot be achieved

        poly = QPolygonF(d->cachedPoints);
        poly << d->cachedPoints[0];

        PerspectiveBasedAssistantHelper::updateCacheData(d->cache, poly);

        d->ellipseInPolygon.updateToPolygon(poly, d->cache.horizon);
        d->cacheValid = true;
        return;
    }

    PerspectiveBasedAssistantHelper::updateCacheData(d->cache, poly);

    d->ellipseInPolygon.updateToPolygon(poly, d->cache.horizon);
    if (d->ellipseInPolygon.isValid()) {
        d->ellipseInPolygon.setSimpleEllipseVertices(d->simpleEllipse);
    }


    d->cacheValid = true;

}

void PerspectiveEllipseAssistant::paintConcentricEllipse(QPainter &gc, const QRectF &updateRect, const QTransform& initialTransform)
{
    if (d->useMirroredPreview) {
        d->concentricEllipseInPolygon.paintParametric(gc, updateRect, initialTransform);
    } else {
        d->concentricEllipseInPolygonMirrored.paintParametric(gc, updateRect, initialTransform);
    }

    gc.save();

    gc.setPen(QPen(Qt::blue));

    if (!d->useMirroredPreview) {
        d->concentricEllipseInPolygon.paintParametric(gc, updateRect, initialTransform);
    } else {
        d->concentricEllipseInPolygonMirrored.paintParametric(gc, updateRect, initialTransform);
    }

    gc.restore();
}

bool PerspectiveEllipseAssistant::isAssistantComplete() const
{   
    return handles().size() >= 4;
}

void PerspectiveEllipseAssistant::saveCustomXml(QXmlStreamWriter *xml)
{
    xml->writeStartElement("isConcentric");
    ENTER_FUNCTION() << ppVar(this->isConcentric()) << ppVar((int)this->isConcentric()) << ppVar(KisDomUtils::toString( (int)this->isConcentric()));
    xml->writeAttribute("value", KisDomUtils::toString( (int)this->isConcentric()));
    xml->writeEndElement();
}

bool PerspectiveEllipseAssistant::loadCustomXml(QXmlStreamReader *xml)
{
    if (xml && xml->name() == "isConcentric") {
        this->setConcentric((bool)KisDomUtils::toInt(xml->attributes().value("value").toString()));
        ENTER_FUNCTION() << ppVar(xml->attributes().value("value").toString())
                         << ppVar(KisDomUtils::toInt(xml->attributes().value("value").toString()))
                         << ppVar((bool)KisDomUtils::toInt(xml->attributes().value("value").toString()));
        ENTER_FUNCTION() << "Therefore, the assistant is now: " << ppVar(isConcentric());
    }
    return true;
}

void PerspectiveEllipseAssistant::endStroke()
{
    KisPaintingAssistant::endStroke();
    d->ellipsesCorrectInStroke = false;
}

void PerspectiveEllipseAssistant::setAdjustedBrushPosition(const QPointF position)
{
    KisPaintingAssistant::setAdjustedBrushPosition(position);
    // beginning of the stroke
    if (d->isConcentric && !d->ellipsesCorrectInStroke) {
        if (d->concentricEllipseInPolygon.onTheCorrectSideOfHorizon(position)) {
            d->concentricEllipseInPolygon.updateToPointOnConcentricEllipse(d->ellipseInPolygon.originalTransform, position, d->cache.horizon, false);
            d->useMirrored = false;
        } else {
            d->concentricEllipseInPolygonMirrored.updateToPointOnConcentricEllipse(d->ellipseInPolygon.originalTransform, position, d->cache.horizon, true);
            d->useMirrored = true;
        }
        d->ellipsesCorrectInStroke = true;
    }

}

bool PerspectiveEllipseAssistant::isConcentric() const
{
    return d->isConcentric;
}

void PerspectiveEllipseAssistant::setConcentric(bool isConcentric)
{
    d->isConcentric = isConcentric;
}

bool PerspectiveEllipseAssistant::contains(const QPointF &point) const
{

    QPolygonF poly;
    if (!PerspectiveBasedAssistantHelper::getTetragon(handles(), isAssistantComplete(), poly)) return false;
    return poly.containsPoint(point, Qt::OddEvenFill);
}

qreal PerspectiveEllipseAssistant::distance(const QPointF &point) const
{
    return PerspectiveBasedAssistantHelper::distanceInGrid(d->cache, point);
}

bool PerspectiveEllipseAssistant::isActive() const
{
    return isSnappingActive();
}

PerspectiveEllipseAssistantFactory::PerspectiveEllipseAssistantFactory()
{
}

PerspectiveEllipseAssistantFactory::~PerspectiveEllipseAssistantFactory()
{
}

QString PerspectiveEllipseAssistantFactory::id() const
{
    return "perspective ellipse";
}

QString PerspectiveEllipseAssistantFactory::name() const
{
    return i18n("Perspective Ellipse");
}

KisPaintingAssistant* PerspectiveEllipseAssistantFactory::createPaintingAssistant() const
{
    return new PerspectiveEllipseAssistant;
}

