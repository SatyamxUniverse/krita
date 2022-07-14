import QtQuick 2.3
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.3 as Controls
import org.kde.kirigami 2.12 as Kirigami


Kirigami.CardsGridView {
    id: root

    signal itemSelected(url url)

    /**
     * The ratio to use to calculate the height of the card from width. Typically
     * you can get idea for what this might be from the banner + description size.
     */
    property real cardSizeRatio: 1.2

    /**
     * This allows us to dynamically shrink/increase the gap in between cards.
     */
    readonly property real cellHeightFromWidth: Math.min(cellWidth, maximumColumnWidth) / cardSizeRatio

    /**
     * The property is used to set the maximumHeight in Kirigami.Card, otherwise the card
     * can exceed the cellHeight set by the gridview.
     */
    property real maximumCardHeight: 300

    /**
     * The card height beyond which we won't let the card shrink
     */
    property real minimumCardHeight: 300

    cellHeight: Math.min(Math.max(minimumCardHeight, cellHeightFromWidth),
                         maximumCardHeight)
        + Kirigami.Units.gridUnit + 2
}
