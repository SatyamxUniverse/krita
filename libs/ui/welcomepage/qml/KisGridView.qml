import QtQuick 2.3
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.3 as Controls
import org.kde.kirigami 2.12 as Kirigami


Kirigami.CardsGridView {
    id: root

    signal itemSelected(url url)

    // The property is used to set the maximumHeight in Kirigami.Card, otherwise the card
    // can exceed the cellHeight set by the gridview.
    property real maximumCardHeight: 300
    cellHeight: maximumCardHeight + Kirigami.Units.gridUnit + 2
}
