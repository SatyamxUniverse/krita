import QtQuick 2.15
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.3 as Controls
import org.kde.kirigami 2.12 as Kirigami


GridView {
    id: root

    signal itemSelected(url url)

    property real minimumColumnWidth: 300
    property real maximumColumnWidth: 300
    property real preferredColumnWidth: 300

    property real minimumColumnHeight: 300
    property real maximumColumnHeight: 300
    property real preferredColumnHeight: 300

    property real minimumColumnSpacing: 8
    property real maximumColumnSpacing: 15

    property real numColumns: Math.max(1, Math.min(Math.floor(width / minimumColumnWidth), Math.ceil(width / maximumColumnWidth)))

    cellWidth: width / numColumns
    cellHeight: Math.max(minimumColumnHeight, Math.min(preferredColumnHeight, maximumColumnHeight))

    reuseItems: true
}
