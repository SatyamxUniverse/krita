/*
 *  SPDX-FileCopyrightText: 2015 Dmitry Kazakov <dimula73@gmail.com>
 *  SPDX-FileCopyrightText: 2018 Bernhard Liebl <poke1024@gmx.de>
 *  SPDX-FileCopyrightText: 2022 Alvin Wong <alvin@alvinhc.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_fps_decoration.h"
#include "kis_fps_decoration_p.h"

#include <QApplication>
#include <QPainter>
#include "kis_canvas2.h"
#include "kis_coordinates_converter.h"
#include "opengl/kis_opengl_canvas_debugger.h"
#include <KisStrokeSpeedMonitor.h>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsDropShadowEffect>
#include <QQuickItem>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlProperty>

const QString KisFpsDecoration::idTag = "fps_decoration";

KisFpsDecoration::KisFpsDecoration(QPointer<KisView> view)
    : KisCanvasDecoration(idTag, view)
    , m_font(QApplication::font())
    , m_pixmap(1, 1) // need non-zero pixmap for initial setup
{
    setVisible(true);

    m_shadow = new QGraphicsDropShadowEffect(this);
    m_shadow->setBlurRadius(0.5);
    m_shadow->setOffset(0);
    m_shadow->setColor(QColor(0x30, 0x30, 0x30));

    m_scene = new QGraphicsScene(this);
    m_pixmapItem = m_scene->addPixmap(m_pixmap);
    m_pixmapItem->setGraphicsEffect(m_shadow);
}

KisFpsDecoration::~KisFpsDecoration()
{
    // Delete quickItem before data to prevent TypeError messages
    delete m_quickItem;
    delete m_data;
}

void KisFpsDecoration::drawDecoration(QPainter& gc, const QRectF& /*updateRect*/, const KisCoordinatesConverter */*converter*/, KisCanvas2* /*canvas*/)
{
    // we always paint into a pixmap instead of directly into gc, as the latter
    // approach is known to cause garbled graphics on macOS, Windows, and even
    // sometimes Linux.

    const QString text = getText();

    // note that USUALLY the pixmap will have the right size. in very rare cases
    // (e.g. on the very first call) the computed bounding rect will not be right
    // and the pixmap will need a resize. it is faster to NOT usually calculate
    // the necessary bounds with an extra call like QFontMetrics::boundingRect()
    // here, as USUALLY the pixmap will be right and thus an extra call would be
    // unnecessary overhead.

    QSize size;

    if (!draw(text, size)) {
        // the pixmap is too small, we need to make it larger. make it 10% wider
        // than the measured width to avoid resizing again as soon as the text
        // gets a bit wider due to different content.

        m_pixmap = QPixmap(size.width() * 1.1f, size.height());

        KIS_ASSERT(draw(text, size));
    }

    QRectF r = m_pixmap.rect();
    r |= m_shadow->boundingRectFor(r);

    m_pixmapItem->setPixmap(m_pixmap);
    m_scene->render(&gc, r.translated(20, 20), r);
}

bool KisFpsDecoration::draw(const QString &text, QSize &outSize)
{
    m_pixmap.fill(Qt::transparent);

    const int flags = Qt::AlignLeft | Qt::AlignTop | Qt::TextDontClip;
    QRect bounds;

    QPainter painter(&m_pixmap);
    painter.setFont(m_font);

    painter.setPen(QPen(QColor(0xF0, 0xF0, 0xF0)));
    painter.drawText(m_pixmap.rect().translated(1, 1), flags, text, &bounds);

    outSize = bounds.size() + QSize(1, 1);

    if (m_pixmap.width() < outSize.width() || m_pixmap.height() != outSize.height()) {
        return false; // pixmap is too small and needs a resize. rarely happens.
    }

    return true;
}

QString KisFpsDecoration::getText() const
{
    QStringList lines;

    if (KisOpenglCanvasDebugger::instance()->showFpsOnCanvas()) {
        const qreal value = KisOpenglCanvasDebugger::instance()->accumulatedFps();
        lines << QString("Canvas FPS: %1").arg(QString::number(value, 'f', 1));
    }

    KisStrokeSpeedMonitor *monitor = KisStrokeSpeedMonitor::instance();

    if (monitor->haveStrokeSpeedMeasurement()) {
        lines << QString("Last cursor/brush speed (px/ms): %1/%2%3")
                .arg(monitor->lastCursorSpeed(), 0, 'f', 1)
                .arg(monitor->lastRenderingSpeed(), 0, 'f', 1)
                .arg(monitor->lastStrokeSaturated() ? " (!)" : "");
        lines << QString("Last brush framerate: %1 fps")
                .arg(monitor->lastFps(), 0, 'f', 1);

        lines << QString("Average cursor/brush speed (px/ms): %1/%2")
                .arg(monitor->avgCursorSpeed(), 0, 'f', 1)
                .arg(monitor->avgRenderingSpeed(), 0, 'f', 1);
        lines << QString("Average brush framerate: %1 fps")
                .arg(monitor->avgFps(), 0, 'f', 1);
    }

    return lines.join('\n');
}

QQuickItem *KisFpsDecoration::initOrGetQuickItem(QQmlEngine *engine)
{
    if (m_quickItem) {
        return m_quickItem;
    }

    static bool registerOnce = []() {
        qmlRegisterUncreatableType<KisFpsDecorationData>(
                "org.krita.ui", 1, 0,"KisFpsDecorationData",
                QStringLiteral("Uncreatable"));
        return true;
    }();
    Q_UNUSED(registerOnce)

    QQmlComponent component(engine, QUrl("qrc:/kisqml/canvas/decorations/KisFpsDecoration.qml"), QQmlComponent::PreferSynchronous);
    QObject *qmlObject = component.beginCreate(engine->rootContext());
    if (!qmlObject) {
        qWarning() << "KisFpsDecoration: Failed to create QML component";
        Q_FOREACH(const QQmlError &error, component.errors()) {
            qWarning() << "KisFpsDecoration:" << error.url() << error.line() << error;
        }
        return nullptr;
    }
    m_quickItem = qobject_cast<QQuickItem *>(qmlObject);
    if (!m_quickItem) {
        qWarning() << "KisFpsDecoration: Created QML component is not QQuickItem!";
        delete qmlObject;
        return nullptr;
    }
    m_data = new KisFpsDecorationData(this);
    m_quickItem->setProperty("d", QVariant::fromValue(m_data));
    component.completeCreate();

    m_quickItem->setParent(this);
    return m_quickItem;
}

QQuickItem *KisFpsDecoration::quickItem() const
{
    return m_quickItem;
}

void KisFpsDecoration::updateQuickItem()
{
    if (!m_quickItem) {
        return;
    }

    if (KisOpenglCanvasDebugger::instance()->showFpsOnCanvas()) {
        m_data->setShowFps(true);
        m_data->setFps(KisOpenglCanvasDebugger::instance()->accumulatedFps());
    } else {
        m_data->setShowFps(false);
    }

    KisStrokeSpeedMonitor *monitor = KisStrokeSpeedMonitor::instance();
    if (monitor->haveStrokeSpeedMeasurement()) {
        m_data->setShowStrokeSpeed(true);
        m_data->setLastCursorSpeed(monitor->lastCursorSpeed());
        m_data->setLastRenderingSpeed(monitor->lastRenderingSpeed());
        m_data->setLastStrokeSaturated(monitor->lastStrokeSaturated());
        m_data->setLastFps(monitor->lastFps());
        m_data->setAvgCursorSpeed(monitor->avgCursorSpeed());
        m_data->setAvgRenderingSpeed(monitor->avgRenderingSpeed());
        m_data->setAvgFps(monitor->avgFps());
    } else {
        m_data->setShowStrokeSpeed(false);
    }
}
