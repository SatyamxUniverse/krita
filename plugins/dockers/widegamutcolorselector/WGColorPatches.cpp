/*
 * SPDX-FileCopyrightText: 2020 Mathias Wein <lynx.mw+kde@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "WGColorPatches.h"

#include <kis_display_color_converter.h>

#include <QMouseEvent>
#include <QPainter>

WGColorPatches::WGColorPatches(QWidget *parent) : QWidget(parent)
{

}

void WGColorPatches::addUniqueColor(const KoColor &color)
{
    int colIndex = m_colors.indexOf(color);
    if (colIndex >= 0) {
        m_colors.removeAt(colIndex);
    }
    m_colors.prepend(color);
    // keep up to 200 colors
    if (m_colors.size() > 200) {
        m_colors.removeLast();
    }
    update();
}

void WGColorPatches::setAdditionalButtons(QList<QWidget *> buttonList)
{
    for (int i = 0; i < buttonList.size(); i++) {
        buttonList[i]->setParent(this);
        buttonList[i]->setGeometry(patchRect(i));
    }
    m_buttonList = buttonList;
}

void WGColorPatches::setColors(QList<KoColor> colors)
{
    m_colors = colors;
    m_scrollValue = qBound(0, m_scrollValue, maxScroll());
    update();
}

void WGColorPatches::setDisplayRenderer(const KoColorDisplayRendererInterface *displayRenderer)
{
    m_displayRenderer = displayRenderer ? displayRenderer : KoDumbColorDisplayRenderer::instance();
}

void WGColorPatches::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        int index = indexAt(event->pos());
        if (index >= 0 && index != m_mouseIndex) {
            emit sigColorChanged(m_colors[index]);
            m_mouseIndex = index;
        }
    }
}

void WGColorPatches::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit sigInteraction(true);
        m_mouseIndex = indexAt(event->pos());
        if (m_mouseIndex >= 0) {
            emit sigColorChanged(m_colors[m_mouseIndex]);
        }
    }
}

void WGColorPatches::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit sigInteraction(false);
    }
}

void WGColorPatches::wheelEvent(QWheelEvent *event)
{
    if (m_allowScrolling) {
        // scroll two patches per "tick"
        int scrollAmount = 2 * (m_orientation == Qt::Vertical ? m_patchHeight : m_patchWidth);
        scrollAmount = (event->angleDelta().y() * scrollAmount) / QWheelEvent::DefaultDeltasPerStep;
        m_scrollValue = qBound(0, m_scrollValue - scrollAmount, maxScroll());
        update();
    }
}

void WGColorPatches::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    int numColors = m_colors.size();
    if (numColors <= 0) {
        return;
    }

    QPainter painter(this);
    int scrollCount = 0;
    if (m_allowScrolling) {
        if (m_orientation == Qt::Vertical) {
            scrollCount = m_scrollValue / m_patchHeight;
            painter.translate(0, -m_scrollValue);
        } else {
            scrollCount = m_scrollValue / m_patchWidth;
            painter.translate(-m_scrollValue, 0);
        }
    }

    for (int i = scrollCount * m_numLines; i < qMin(m_patchCount, m_colors.size()); i++) {
        QColor qcolor = m_displayRenderer->toQColor(m_colors[i]);

        painter.fillRect(patchRect(i + m_buttonList.size()), qcolor);
    }
}

void WGColorPatches::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    m_scrollValue = qBound(0, m_scrollValue, maxScroll());
}

QSize WGColorPatches::sizeHint() const
{
    int lineLen = (m_patchCount + m_buttonList.size() + m_numLines - 1) / m_numLines;
    if (m_orientation == Qt::Vertical) {
        return QSize(m_numLines * m_patchWidth, lineLen * m_patchHeight);
    } else {
        return QSize(lineLen * m_patchWidth, m_numLines * m_patchHeight);
    }
}

bool WGColorPatches::colorAt(const QPoint &pos, KoColor &result) const
{
    int patchNr = indexAt(pos);

    if (patchNr >= 0) {
        result = m_colors[patchNr];
        return true;
    }
    return false;
}

int WGColorPatches::indexAt(const QPoint &pos) const
{
    if(!rect().contains(pos))
        return -1;

    int scrollX = m_orientation == Qt::Horizontal ? m_scrollValue : 0;
    int scrollY = m_orientation == Qt::Vertical ? m_scrollValue : 0;
    int column = (pos.x() - scrollX) / m_patchWidth;
    int row = (pos.y() - scrollY) / m_patchHeight;

    int patchNr;
    if (m_orientation == Qt::Vertical) {
        patchNr = row * m_numLines + column;
    }
    else {
        // Horizontal
        patchNr = column * m_numLines + row;
    }

    patchNr -= m_buttonList.size();

    if (patchNr >= 0 && patchNr < qMin(m_patchCount, m_colors.size())) {
        return patchNr;
    }
    return -1;
}

int WGColorPatches::maxScroll() const
{
    QSize fullSize = sizeHint();
    if (m_orientation == Qt::Vertical) {
        if (height() >= fullSize.height()) {
            return 0;
        }
        return fullSize.height() - height();
    }
    else {
        if (width() >= fullSize.width()) {
            return 0;
        }
        return fullSize.width() - width();
    }
}

QRect WGColorPatches::patchRect(int gridIndex) const
{
    int row, col;

    if (m_orientation == Qt::Vertical) {
        row =  gridIndex / m_numLines;
        col = gridIndex % m_numLines;
    } else {
        row = gridIndex % m_numLines;
        col = gridIndex / m_numLines;
    }
    return QRect(col * m_patchWidth, row * m_patchHeight, m_patchWidth, m_patchHeight);
}