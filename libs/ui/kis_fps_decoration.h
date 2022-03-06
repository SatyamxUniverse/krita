/*
 *  SPDX-FileCopyrightText: 2015 Dmitry Kazakov <dimula73@gmail.com>
 *  SPDX-FileCopyrightText: 2018 Bernhard Liebl <poke1024@gmx.de>
 *  SPDX-FileCopyrightText: 2022 Alvin Wong <alvin@alvinhc.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef __KIS_FPS_DECORATION_H
#define __KIS_FPS_DECORATION_H

#include "canvas/kis_canvas_decoration.h"

#include <QFont>

class QGraphicsScene;
class QGraphicsPixmapItem;
class QGraphicsDropShadowEffect;
class QQmlEngine;
class QQuickItem;
class KisFpsDecorationData;

class KisFpsDecoration : public KisCanvasDecoration
{
public:
    KisFpsDecoration(QPointer<KisView> view);
    ~KisFpsDecoration() override;

    void drawDecoration(QPainter& gc, const QRectF& updateRect, const KisCoordinatesConverter *converter, KisCanvas2* canvas) override;

    QQuickItem *initOrGetQuickItem(QQmlEngine *engine) override;
    QQuickItem *quickItem() const override;
    void updateQuickItem() override;

    static const QString idTag;

private:
    bool draw(const QString &text, QSize &outSize);
	QString getText() const;

    QFont m_font;
    QPixmap m_pixmap;

    QGraphicsScene *m_scene;
    QGraphicsPixmapItem *m_pixmapItem;
    QGraphicsDropShadowEffect *m_shadow;

    KisFpsDecorationData *m_data {nullptr};
    QQuickItem *m_quickItem {nullptr};
};

#endif /* __KIS_FPS_DECORATION_H */
