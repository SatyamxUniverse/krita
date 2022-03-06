/*
 * SPDX-FileCopyrightText: 2022 Alvin Wong <alvin@alvinhc.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick 2.12
import QtQuick.Layouts 1.12
import org.krita.ui 1.0

Item {
    property KisFpsDecorationData d

    objectName: "FpsDecoration" // for display only
    id: root
    anchors.fill: parent

    ColumnLayout {
        anchors.left: root.left
        anchors.top: root.top
        anchors.leftMargin: 20
        anchors.topMargin: 20
        spacing: 0

        Text {
            text: `Canvas FPS: ${d.fps.toFixed(1)}`
            textFormat: Text.PlainText
            visible: d.showFps
        }

        Text {
            text: `Last cursor/brush speed (px/ms): ${d.lastCursorSpeed.toFixed(1)}/${d.lastRenderingSpeed.toFixed(1)}${d.lastStrokeSaturated ? " (!)" : ""}`
            textFormat: Text.PlainText
            visible: d.showStrokeSpeed
        }

        Text {
            text: `Last brush framerate: ${d.lastFps.toFixed(1)} fps`
            textFormat: Text.PlainText
            visible: d.showStrokeSpeed
        }

        Text {
            text: `Average cursor/brush speed (px/ms): ${d.avgCursorSpeed.toFixed(1)}/${d.avgRenderingSpeed.toFixed(1)}`
            textFormat: Text.PlainText
            visible: d.showStrokeSpeed
        }

        Text {
            text: `Average brush framerate: ${d.avgFps.toFixed(1)} fps`
            textFormat: Text.PlainText
            visible: d.showStrokeSpeed
        }
    }
}
