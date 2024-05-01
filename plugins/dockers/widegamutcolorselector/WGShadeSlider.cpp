/*
 * SPDX-FileCopyrightText: 2020 Mathias Wein <lynx.mw+kde@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "WGShadeSlider.h"

#include "KoColorDisplayRendererInterface.h"

#include <QImage>
#include <QMouseEvent>
#include <QPainter>
#include <QVector4D>
#include <QtMath>

struct WGShadeSlider::Private
{
    Private() {}
    QImage background;
    QVector4D range;
    QVector4D offset;
    QVector4D baseValues;
    qreal handleValue {0};
    qreal leftStart {-1};
    qreal leftEnd {0};
    qreal rightStart {0};
    qreal rightEnd {-1};
    qreal center {0};
    qreal totalLength {-1};
    KisVisualColorModelSP selectorModel;
    WGSelectorDisplayConfigSP displayConfig;
    Qt::Orientation orientation {Qt::Horizontal};
    int cursorWidth {11};
    int strokeWidth {1};
    int numPatches {9};
    bool widgetSizeOk {false};
    bool sliderMode {true};
    bool imageNeedsUpdate {true};
};

WGShadeSlider::WGShadeSlider(WGSelectorDisplayConfigSP config, QWidget *parent, KisVisualColorModelSP model)
    : QWidget(parent)
    , m_d(new Private)
{
    m_d->selectorModel = model;
    m_d->displayConfig = config;
    recalculateParameters();
    connect(config.data(), &WGSelectorDisplayConfig::sigDisplayConfigurationChanged,
            this, &WGShadeSlider::slotDisplayConfigurationChanged);
}

WGShadeSlider::~WGShadeSlider()
{}

void WGShadeSlider::setGradient(const QVector4D &range, const QVector4D &offset)
{
    m_d->range = range;
    m_d->offset = offset;
    m_d->imageNeedsUpdate = true;
    resetHandle();
}

void WGShadeSlider::setDisplayMode(bool slider, int numPatches)
{
    if (slider != m_d->sliderMode ||
        (!slider && numPatches != m_d->numPatches)) {
        m_d->sliderMode = slider;
        if (!slider && numPatches > 2) {
            m_d->numPatches = numPatches;
        }
        m_d->widgetSizeOk = sizeRequirementsMet();
        m_d->imageNeedsUpdate = true;
        resetHandle();
    }
}

void WGShadeSlider::setModel(KisVisualColorModelSP model)
{
    m_d->selectorModel = model;
    m_d->imageNeedsUpdate = true;
    update();
}

void WGShadeSlider::setOrientation(Qt::Orientation orientation)
{
    if (orientation != m_d->orientation) {
        m_d->orientation = orientation;
        recalculateParameters();
        m_d->imageNeedsUpdate = true;
        // test...
        if (orientation == Qt::Horizontal) {
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        } else {
            setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        }
        updateGeometry();
    }
}

QVector4D WGShadeSlider::channelValues() const
{
    return calculateChannelValues(m_d->handleValue);
}

const QImage *WGShadeSlider::background()
{
    if (m_d->imageNeedsUpdate) {
        m_d->background = renderBackground();
        m_d->imageNeedsUpdate = false;
    }
    return &m_d->background;
}

QSize WGShadeSlider::minimumSizeHint() const
{
    return m_d->orientation == Qt::Vertical ?  QSize(8, 50) : QSize(50, 8);
}

void WGShadeSlider::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit sigInteraction(true);
        if (adjustHandleValue(event->localPos())) {
            emit sigChannelValuesChanged(channelValues());
            update();
        }
    } else {
        event->ignore();
    }
}

void WGShadeSlider::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        if (adjustHandleValue(event->localPos())) {
            emit sigChannelValuesChanged(channelValues());
            update();
        }
    } else {
        event->ignore();
    }
}

void WGShadeSlider::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit sigInteraction(false);
    } else {
        event->ignore();
    }
}

void WGShadeSlider::paintEvent(QPaintEvent*)
{
    if (m_d->imageNeedsUpdate) {
        m_d->background = renderBackground();
        m_d->imageNeedsUpdate = false;
    }
    QPainter painter(this);
    painter.drawImage(0, 0, m_d->background);
    painter.scale(1.0/devicePixelRatioF(), 1.0/devicePixelRatioF());

    QPair<qreal, qreal> handlePos;

    if (m_d->sliderMode) {
        int sliderX = qRound(convertSliderValueToLinePosition(m_d->handleValue));
        handlePos.first = sliderX - m_d->cursorWidth/2;
        handlePos.second = m_d->cursorWidth;
    } else if (m_d->handleValue >= 0) {
        handlePos = patchLocation(static_cast<int>(m_d->handleValue));
    } else {
        // patch mode with none selected
        return;
    }

    QPen pen(QColor(175,175,175), m_d->strokeWidth, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
    painter.setPen(pen);
    strokeRect(painter, handlePos.first, handlePos.second, devicePixelRatioF(), 0);
    pen.setColor(QColor(75,75,75));
    painter.setPen(pen);
    strokeRect(painter, handlePos.first, handlePos.second, devicePixelRatioF(), 1);
}

void WGShadeSlider::resizeEvent(QResizeEvent *)
{
    recalculateParameters();
}

void WGShadeSlider::slotSetChannelValues(const QVector4D &values)
{
    m_d->baseValues = values;
    m_d->imageNeedsUpdate = true;
    resetHandle();
}

void WGShadeSlider::resetHandle()
{
    m_d->handleValue = m_d->sliderMode ? 0 : -1;
    update();
}

void WGShadeSlider::slotDisplayConfigurationChanged()
{
    m_d->imageNeedsUpdate = true;
    update();
}

bool WGShadeSlider::adjustHandleValue(const QPointF &widgetPos)
{
    if (!m_d->widgetSizeOk) {
        return false;
    }

    if (m_d->sliderMode) {
        qreal linearPos = linePosition(widgetPos);
        qreal sliderPos = convertLinePositionToSliderValue(linearPos);
        if (!qFuzzyIsNull(m_d->handleValue - sliderPos)) {
            m_d->handleValue = sliderPos;
            return true;
        }
    } else {
        int patchNum = getPatch(widgetPos);
        if (patchNum >= 0 && patchNum != (int)m_d->handleValue) {
            m_d->handleValue = patchNum;
            return true;
        }
    }
    return false;
}

qreal WGShadeSlider::convertSliderValueToLinePosition(qreal value)
{
    qreal widgetPos;
    if (value < 0) {
        widgetPos = (m_d->leftStart - value * (m_d->leftEnd - m_d->leftStart));
    }
    else if (value > 0) {
        widgetPos = (m_d->rightStart + value * (m_d->rightEnd - m_d->rightStart));
    }
    else {
        widgetPos = m_d->center;
    }
    return widgetPos;
}

qreal WGShadeSlider::convertLinePositionToSliderValue(qreal position)
{
    if (position < m_d->leftEnd) {
        return -1.0;
    }
    else if (position < m_d->leftStart) {
        return  (m_d->leftStart - position) / (m_d->leftEnd - m_d->leftStart);
    }
    else if (position < m_d->rightStart) {
        return 0.0;
    }
    else if (position < m_d->rightEnd) {
        return (position - m_d->rightStart) / (m_d->rightEnd - m_d->rightStart);
    }
    return 1.0;
}

QVector4D WGShadeSlider::calculateChannelValues(qreal sliderPos) const
{
    float delta = 0.0f;
    if (m_d->sliderMode) {
        delta = (float)sliderPos;
    } else if (sliderPos >= 0 || m_d->numPatches > 1) {
        delta = 2.0f * float(sliderPos)/(m_d->numPatches - 1.0f) - 1.0f;
    }

    QVector4D channelVec = m_d->baseValues + m_d->offset + delta * m_d->range;
    // Hue wraps around
    if (m_d->selectorModel->isHSXModel()) {
        channelVec[0] = (float)fmod(channelVec[0], 1.0);
        if (channelVec[0] < 0) {
            channelVec[0] += 1.f;
        }
    }
    else {
        channelVec[0] = qBound(0.f, channelVec[0], 1.f);
    }

    for (int i = 1; i < 3; i++) {
        channelVec[i] = qBound(0.f, channelVec[i], 1.f);
    }
    return channelVec;
}

qreal WGShadeSlider::linePosition(const QPointF &widgetPos) const
{
    return m_d->orientation == Qt::Horizontal ? widgetPos.x() : height() - 1 - widgetPos.y();
}

int WGShadeSlider::getPatch(const QPointF pos) const
{
    int patch = m_d->numPatches * linePosition(pos) / m_d->totalLength;
    if (patch >= 0 && patch < m_d->numPatches) {
        return patch;
    }
    return -1;
}

QPair<qreal, qreal> WGShadeSlider::patchLocation(int patchIndex) const
{
    QPair<qreal, qreal> range(0, -1);
    qreal patchWidth = m_d->totalLength / qreal(m_d->numPatches);
    qreal margin = 1.5;
    range.first = patchIndex * patchWidth + margin;
    range.second = patchWidth - 2 * margin;
    return range;
}

QRectF WGShadeSlider::patchRect(int patchIndex) const
{
    QPair<qreal, qreal> range = patchLocation(patchIndex);
    if (m_d->orientation == Qt::Horizontal) {
        return QRectF(range.first, 0, range.second, height());
    } else {
        qreal bottom = height() - 1 - range.first;
        return QRectF(0, bottom - range.second, width(), range.second);
    }
}

void WGShadeSlider::recalculateParameters()
{
    int total = m_d->orientation == Qt::Horizontal ? width() : height();
    // integer division was on purpose...I think...
    m_d->center = (total - 1) / 2;
    m_d->totalLength = total;
    int halfCursor = m_d->cursorWidth / 2;

    m_d->leftEnd = halfCursor;
    m_d->leftStart = m_d->center - halfCursor;

    m_d->rightStart = m_d->center + halfCursor;
    m_d->rightEnd = 2 * m_d->center  - halfCursor;

    m_d->strokeWidth = qRound(devicePixelRatioF() - 0.05);
    m_d->widgetSizeOk = sizeRequirementsMet();
    m_d->imageNeedsUpdate = true;
}

bool WGShadeSlider::sizeRequirementsMet() const
{
    if (m_d->sliderMode) {
        return m_d->leftStart - m_d->leftEnd > 0 &&  m_d->rightEnd - m_d->rightStart > 0;
    } else {
        return m_d->totalLength > (m_d->numPatches * 2);
    }
}

QImage WGShadeSlider::renderBackground()
{
    if (!m_d->widgetSizeOk || !m_d->selectorModel || !m_d->selectorModel->colorSpace()) {
        return QImage();
    }

    // Hi-DPI aware rendering requires that we determine the device pixel dimension;
    // actual widget size in device pixels is not accessible unfortunately, it might be 1px smaller...
    const qreal deviceDivider = 1.0 / devicePixelRatioF();
    const int deviceWidth = qCeil(width() * devicePixelRatioF());
    const int deviceHeight = qCeil(height() * devicePixelRatioF());
    if (m_d->sliderMode) {
        const KoColorSpace *currentCS = m_d->selectorModel->colorSpace();
        const quint32 pixelSize = currentCS->pixelSize();
        const int pixelCount = m_d->orientation == Qt::Horizontal ? deviceWidth : deviceHeight;
        quint32 imageSize = pixelCount * pixelSize;
        QScopedArrayPointer<quint8> raw(new quint8[imageSize] {});
        quint8 *dataPtr = raw.data();

        for (int x = 0; x < pixelCount; x++, dataPtr += pixelSize) {
            int linePos = m_d->orientation == Qt::Horizontal ? x : pixelCount - 1 - x;
            qreal sliderVal = convertLinePositionToSliderValue(linePos * deviceDivider);
            QVector4D coordinates = calculateChannelValues(sliderVal);
            KoColor c = m_d->selectorModel->convertChannelValuesToKoColor(coordinates);
            memcpy(dataPtr, c.data(), pixelSize);
        }

        QSize sz = m_d->orientation == Qt::Horizontal ? QSize(pixelCount, 1) : QSize(1, pixelCount);
        QImage image = m_d->displayConfig->displayConverter()->toQImage(currentCS, raw.data(), sz,
                                                                        m_d->displayConfig->previewInPaintingCS());
        image = image.scaled(QSize(deviceWidth, deviceHeight));

        QPainter painter(&image);
        QPen pen(QColor(175,175,175), m_d->strokeWidth, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
        painter.setPen(pen);
        strokeRect(painter, m_d->leftStart, m_d->cursorWidth, devicePixelRatioF(), 0);
        pen.setColor(QColor(75,75,75));
        painter.setPen(pen);
        strokeRect(painter, m_d->leftStart, m_d->cursorWidth, devicePixelRatioF(), 1);

        image.setDevicePixelRatio(devicePixelRatioF());
        return image;
    } else {
        QImage image(deviceWidth, deviceHeight, QImage::Format_ARGB32);
        image.fill(Qt::transparent);
        image.setDevicePixelRatio(devicePixelRatioF());
        QPainter painter(&image);
        painter.setPen(Qt::NoPen);

        for (int i = 0; i < m_d->numPatches; i++) {
            QVector4D values = calculateChannelValues(i);
            KoColor col = m_d->selectorModel->convertChannelValuesToKoColor(values);
            QColor qCol = m_d->displayConfig->displayConverter()->toQColor(col, m_d->displayConfig->previewInPaintingCS());
            painter.setBrush(qCol);
            painter.drawRect(patchRect(i));
        }
        return image;
    }
}

void WGShadeSlider::strokeRect(QPainter &painter, qreal start, qreal length, qreal pixelSize, qreal shrinkX)
{
    qreal lineWidth = painter.pen().widthF();
    qreal indent = 0.5 * lineWidth;
    qreal recLength = qRound(length * pixelSize - (2 * shrinkX) * lineWidth) - lineWidth;

    QRectF finalRect;
    if (m_d->orientation == Qt::Horizontal) {
        qreal left = qRound(start * pixelSize) + (shrinkX + 0.5) * lineWidth;
        finalRect = QRectF(left, indent, recLength, qRound(height() * pixelSize) - lineWidth);
    } else {
        qreal top = qRound((height() - 1 - start - length) * pixelSize) + (shrinkX + 0.5) * lineWidth;
        finalRect = QRectF(indent, top, qRound(width() * pixelSize) - lineWidth, recLength);
    }

    painter.drawRect(finalRect);
}
