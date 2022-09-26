/*
 * SPDX-FileCopyrightText: 2022 Srirupa Datta <srirupa.sps@gmail.com>
 */

#ifndef _PERSPECTIVE_ELLIPSE_ASSISTANT_H_
#define _PERSPECTIVE_ELLIPSE_ASSISTANT_H_

#include "kis_abstract_perspective_grid.h"
#include "kis_painting_assistant.h"
#include "Ellipse.h"
#include <QObject>

class PerspectiveEllipseAssistant : public KisPaintingAssistant
{
public:
    PerspectiveEllipseAssistant();
    KisPaintingAssistantSP clone(QMap<KisPaintingAssistantHandleSP, KisPaintingAssistantHandleSP> &handleMap) const override;
    QPointF adjustPosition(const QPointF& point, const QPointF& strokeBegin, const bool snapToAny) override;
    
    QPointF getEditorPosition() const override;
    int numHandles() const override { return 4; }
    bool isAssistantComplete() const override;
    
    void setAdjustedBrushPosition(const QPointF position) override;
    void setFollowBrushPosition(bool follow) override;
    void endStroke() override;
    
protected:
    QRect boundingRect() const override;
    void drawAssistant(QPainter& gc, const QRectF& updateRect, const KisCoordinatesConverter* converter, bool cached, KisCanvas2* canvas, bool assistantVisible=true, bool previewVisible=true) override;
    void drawCache(QPainter& gc, const KisCoordinatesConverter *converter,  bool assistantVisible=true) override;
private:
    QPointF project(const QPointF& pt, const QPointF& strokeBegin);
    // creates the convex hull, returns false if it's not a quadrilateral
    bool quad(QPolygonF& out) const;

    // finds the transform from perspective coordinates (a unit square) to the document
    bool getTransform(QPolygonF& polyOut, QTransform& transformOut);


    bool isEllipseValid();
    void updateCache();


     
    explicit PerspectiveEllipseAssistant(const PerspectiveEllipseAssistant &rhs, QMap<KisPaintingAssistantHandleSP, KisPaintingAssistantHandleSP> &handleMap);


    mutable QTransform m_cachedTransform;
    mutable QPolygonF m_cachedPolygon;
    mutable QPointF m_cachedPoints[4];


    QVector<QPointF> pointsToDraw;
    
    
    bool m_followBrushPosition;
    bool m_adjustedPositionValid;
    QPointF m_adjustedBrushPosition;

    class Private;
    QScopedPointer<Private> d;
    
};

class PerspectiveEllipseAssistantFactory : public KisPaintingAssistantFactory
{
public:
    PerspectiveEllipseAssistantFactory();
    ~PerspectiveEllipseAssistantFactory() override;
    QString id() const override;
    QString name() const override;
    KisPaintingAssistant* createPaintingAssistant() const override;
};

#endif
