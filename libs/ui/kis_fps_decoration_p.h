/*
 * SPDX-FileCopyrightText: 2022 Alvin Wong <alvin@alvinhc.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef KIS_FPS_DECORATION_P_H
#define KIS_FPS_DECORATION_P_H

#include <QObject>

#define AUTO_PROP(TYPE, NAME, SETTER) \
        Q_PROPERTY(TYPE NAME READ NAME NOTIFY NAME ## Changed) \
    public: \
        TYPE NAME() { \
            return m_ ## NAME; \
        } \
    Q_SIGNALS: \
        void NAME ## Changed(TYPE NAME); \
    private: \
        void SETTER(TYPE NAME) { \
            if (m_ ## NAME == NAME) { \
                return; \
            } \
            m_ ## NAME = NAME; \
            emit NAME ## Changed(NAME); \
        } \
        TYPE m_ ## NAME {};

class KisFpsDecorationData
    : public QObject
{
    Q_OBJECT
    AUTO_PROP(bool, showFps, setShowFps)
    AUTO_PROP(qreal, fps, setFps)
    AUTO_PROP(bool, showStrokeSpeed, setShowStrokeSpeed)
    AUTO_PROP(qreal, lastCursorSpeed, setLastCursorSpeed)
    AUTO_PROP(qreal, lastRenderingSpeed, setLastRenderingSpeed)
    AUTO_PROP(bool, lastStrokeSaturated, setLastStrokeSaturated)
    AUTO_PROP(qreal, lastFps, setLastFps)
    AUTO_PROP(qreal, avgCursorSpeed, setAvgCursorSpeed)
    AUTO_PROP(qreal, avgRenderingSpeed, setAvgRenderingSpeed)
    AUTO_PROP(qreal, avgFps, setAvgFps)

private:
    KisFpsDecorationData(QObject *parent)
        : QObject(parent)
    {}
    ~KisFpsDecorationData() override = default;

    friend class KisFpsDecoration;
};

#undef AUTO_PROP

#endif // KIS_FPS_DECORATION_P_H
