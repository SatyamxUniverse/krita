import QtQuick 2.15
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.15 as Controls

import org.kde.kirigami 2.12 as Kirigami


Controls.Drawer {
    id: drawer

    height: parent.height

    background: Rectangle {
        Kirigami.Theme.colorSet: modal ? Kirigami.Theme.View : Kirigami.Theme.Window

        color: Kirigami.Theme.backgroundColor

        Rectangle {
            width: 1
            anchors {
                right: parent.right
                top: parent.top
                bottom: parent.bottom
            }
            color: Kirigami.Theme.textColor
        }
    }

    interactive: shouldBeModal()
    modal: shouldBeModal()
    visible: true

    function shouldBeModal() {
        return Kirigami.Settings.isMobile
    }
}
