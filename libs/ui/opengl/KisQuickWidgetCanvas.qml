/*
 * SPDX-FileCopyrightText: 2022 Alvin Wong <alvin@alvinhc.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick 2.12
import QtQuick.Window 2.12

Item {
    id: root

    Rectangle {
        id: block
        width: 240; height: 24
        color: "red"
        opacity: 0.2
        anchors.horizontalCenter: root.horizontalCenter

        Text {
            id: helloText
            text: "KisQuickWidgetCanvas"
            y: 3
            anchors.horizontalCenter: block.horizontalCenter
            font.pointSize: 12; font.bold: true
        }
    }

    Rectangle {
        id: debugBlock
        color: "white"
        opacity: 0.4
        anchors.right: root.right
        anchors.bottom: root.bottom
        width: debugText.width
        height: debugText.height

        Text {
            id: debugText
            anchors.right: debugBlock.right
            anchors.bottom: debugBlock.bottom
            text: `\
Canvas width: ${root.width}
Canvas height: ${root.height}
Screen width: ${Screen.width}
Screen height: ${Screen.height}
Screen dpr: ${Screen.devicePixelRatio}`
        }
    }
}
