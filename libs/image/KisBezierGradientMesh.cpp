#include "KisBezierGradientMesh.h"

#include "kis_grid_interpolation_tools.h"
#include "kis_debug.h"

namespace KisBezierGradientMeshDetail {

struct QImageGradientOp
{
    QImageGradientOp(const std::array<QColor, 4> &colors, QImage &dstImage,
                    const QPointF &dstImageOffset)
        : m_colors(colors), m_dstImage(dstImage),
          m_dstImageOffset(dstImageOffset),
          m_dstImageRect(m_dstImage.rect())
    {
    }

    void operator() (const QPolygonF &srcPolygon, const QPolygonF &dstPolygon) {
        this->operator() (srcPolygon, dstPolygon, dstPolygon);
    }

    void operator() (const QPolygonF &srcPolygon, const QPolygonF &dstPolygon, const QPolygonF &clipDstPolygon) {
        QRect boundRect = clipDstPolygon.boundingRect().toAlignedRect();
        KisFourPointInterpolatorBackward interp(srcPolygon, dstPolygon);

        for (int y = boundRect.top(); y <= boundRect.bottom(); y++) {
            interp.setY(y);
            for (int x = boundRect.left(); x <= boundRect.right(); x++) {

                QPointF srcPoint(x, y);
                if (clipDstPolygon.containsPoint(srcPoint, Qt::OddEvenFill)) {

                    interp.setX(srcPoint.x());
                    QPointF dstPoint = interp.getValue();

                    // about srcPoint/dstPoint hell please see a
                    // comment in PaintDevicePolygonOp::operator() ()

                    srcPoint -= m_dstImageOffset;

                    QPoint srcPointI = srcPoint.toPoint();

                    if (!m_dstImageRect.contains(srcPointI)) continue;

                    // TODO: move vertical calculation into the upper loop
                    const QColor c1 = lerp(m_colors[0], m_colors[1], qBound(0.0, dstPoint.x(), 1.0));
                    const QColor c2 = lerp(m_colors[2], m_colors[3], qBound(0.0, dstPoint.x(), 1.0));

                    m_dstImage.setPixelColor(srcPointI, lerp(c1, c2, qBound(0.0, dstPoint.y(), 1.0)));
                }
            }
        }
    }

    const std::array<QColor, 4> &m_colors;
    QImage &m_dstImage;
    QPointF m_dstImageOffset;
    QRect m_dstImageRect;
};

}

void KisBezierGradientMesh::renderPatch(const KisBezierGradientMeshDetail::GradientMeshPatch &patch,
                                        const QPoint &dstQImageOffset,
                                        QImage *dstImage)
{
    QVector<QPointF> originalPointsLocal;
    QVector<QPointF> transformedPointsLocal;
    QSize gridSize;

    //patch.sampleRegularGrid(gridSize, originalPointsLocal, transformedPointsLocal, QPointF(16,16));
    patch.sampleIrregularGrid(gridSize, originalPointsLocal, transformedPointsLocal);

    const QRect dstBoundsI = patch.dstBoundingRect().toAlignedRect();
    const QRect imageSize = QRect(dstQImageOffset, dstImage->size());
    KIS_SAFE_ASSERT_RECOVER_NOOP(imageSize.contains(dstBoundsI));

    {
        QImageGradientOp polygonOp(patch.colors, *dstImage, dstQImageOffset);


        GridIterationTools::RegularGridIndexesOp indexesOp(gridSize);
        GridIterationTools::iterateThroughGrid
                <GridIterationTools::AlwaysCompletePolygonPolicy>(polygonOp, indexesOp,
                                                                  gridSize,
                                                                  originalPointsLocal,
                                                                  transformedPointsLocal);

    }
}

void KisBezierGradientMesh::renderMesh(const QPoint &dstQImageOffset,
                                       QImage *dstImage) const
{
    for (auto it = begin(); it != end(); ++it) {
        renderPatch(*it, dstQImageOffset, dstImage);
    }
}
