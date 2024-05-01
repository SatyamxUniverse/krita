/*
 * SPDX-FileCopyrightText: 2020 Mathias Wein <lynx.mw+kde@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WGSHADESLIDER_H
#define WGSHADESLIDER_H

#include "WGSelectorWidgetBase.h"

#include <KisVisualColorModel.h>

#include <QWidget>
#include <QScopedPointer>
#include <QVector4D>

class QImage;

class WGShadeSlider : public QWidget
{
    Q_OBJECT
public:
    explicit WGShadeSlider(WGSelectorDisplayConfigSP config, QWidget *parent = nullptr, KisVisualColorModelSP model = nullptr);
    ~WGShadeSlider() override;

    void setGradient(const QVector4D &range, const QVector4D &offset);
    void setDisplayMode(bool slider, int numPatches = -1);
    void setModel(KisVisualColorModelSP model);
    void setOrientation(Qt::Orientation orientation);
    QVector4D channelValues() const;
    const QImage* background();

public Q_SLOTS:
    void slotSetChannelValues(const QVector4D &values);
    void resetHandle();

protected Q_SLOTS:
    void slotDisplayConfigurationChanged();

protected:
    QSize minimumSizeHint() const override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    //void tabletEvent(QTabletEvent *event) override;
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent *) override;

    bool adjustHandleValue(const QPointF &widgetPos);
    qreal convertSliderValueToLinePosition(qreal value);
    qreal convertLinePositionToSliderValue(qreal position);
    QVector4D calculateChannelValues(qreal sliderPos) const;
    /**
     * @brief Convert widget position to an orientation independent line position
     * @param widgetPos Postions reported QMouseEvent::localPos()
     * @return
     */
    qreal linePosition(const QPointF &widgetPos) const;
    int getPatch(const QPointF pos) const;
    /**
     * @brief Get the line location of a patch (only useful in patch mode)
     * @param patchIndex valid values are 0 <= patchIndex < m_d->numPatches
     * @return a pair with (startPosition, patchWidth)
     */
    QPair<qreal, qreal> patchLocation(int patchIndex) const;
    QRectF patchRect(int patchIndex) const;
    void recalculateParameters();
    bool sizeRequirementsMet() const;
    QImage renderBackground();
    /*!
     * \brief strokeRect
     * \param painter shall already be scaled so that 1 unit == 1 real Pixel
     * \param start the line position of the lower handle edge
     * \param length the length (width or heigh depending on oriantation) of the handle
     * \param pixelSize devicePixelRatioF() that was used to determine the real dimensions
     * \param shrinkX shrinks the rect by a multiple of the line width used to stroke
     */
    void strokeRect(QPainter &painter, qreal start, qreal length, qreal pixelSize, qreal shrinkX);

Q_SIGNALS:
    void sigChannelValuesChanged(const QVector4D &values);
    void sigInteraction(bool active);
private:
    struct Private;
    const QScopedPointer<Private> m_d;
};

#endif // WGSHADESLIDER_H
