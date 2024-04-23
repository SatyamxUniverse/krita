/*
 *  SPDX-FileCopyrightText: 2024 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
import QtQuick 2.15
import org.krita.flake.text 1.0

Column {
    property int firstColumnWidth: 32;
    property int columnSpacing: 5;
    padding: columnSpacing;
    width: parent? parent.width - (padding*2): 100;
    height: visible? childrenRect.height+padding: 0;


    property KoSvgTextPropertiesModel properties : textPropertiesModel;
    signal propertiesUpdated; ///< Used by each text property panel to update the data on the controls.
    property bool blockSignals; ///< When setting the data on the controls, this needs to be enabled and checked while returning data from the controls.


    Rectangle {
        color: sysPalette.base;
        height: 1;
        width: parent.width;
    }
}
