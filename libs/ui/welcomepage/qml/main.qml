import QtQuick 2.3
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.3 as Controls

import org.kde.kirigami 2.12 as Kirigami

Item {
    Kirigami.Page {
        id: root
        anchors {
            top: parent.top
            bottom: parent.bottom
        }

        width: parent.width - sideBarDelta.x

        transform: Translate {
            id: sideBarDelta
            x: createFileDrawer.implicitWidth * createFileDrawer.position
        }

        header: Controls.TabBar {
            id: tabBar
            currentIndex: swipeView.currentIndex

            Controls.TabButton {
                text: "Home"
            }
            Controls.TabButton {
                text: "Featured"
            }
            Controls.TabButton {
                text: "Reference"
            }
            Controls.TabButton {
                text: "Tutorials"
            }
        }

        KisDrawer {
            id: createFileDrawer

            visible: tabBar.currentIndex === 0
            implicitWidth: buttonsColumn.implicitWidth + Kirigami.Units.largeSpacing * 10

            ColumnLayout {
                id: buttonsColumn
                anchors.horizontalCenter: parent.horizontalCenter
                y: 60

                Controls.Button {
                    text: "New File"
                    onClicked: welcomePage.newFile()
                }
                Controls.Button {
                    text: "Open File"
                    onClicked: welcomePage.openFile()
                }
            }
        }

        Controls.SwipeView {
            id: swipeView
            anchors.fill: parent
            currentIndex: tabBar.currentIndex
            clip: true

            Loader {
                property bool activated: false
                onLoaded: activated = true
                active: Controls.SwipeView.isCurrentItem || activated

                sourceComponent: KisGridView {
                    id: projectsGridView
                    model: projectsModel

                    anchors.fill: parent

                    minimumColumnWidth: 170
                    maximumColumnWidth: 300

                    minimumColumnHeight: 150
                    maximumColumnHeight: 350

                    delegate: Kirigami.Card {
                        id: delegate

                        // because the parent doesn't set any constraint for the card size, we need to make
                        // sure we don't draw out of bounds. This is only mandatory if the card size is
                        // variable.
                        implicitWidth: projectsGridView.cellWidth - Kirigami.Units.largeSpacing * 2
                        implicitHeight: projectsGridView.cellHeight - Kirigami.Units.largeSpacing * 2

                        hoverEnabled: true
                        banner {
                            title: model.title
                            source: model.thumbnail
                            titleAlignment: Qt.AlignBottom | Qt.AlignHCenter
                            sourceSize {
                                width: projectsGridView.cellWidth
                                // NOTE: Here we have to make some custom adjustments (not very declarative!),
                                // due to this: https://bugs.kde.org/show_bug.cgi?id=441672
                                height: delegate.implicitHeight - (projectsInfoPanel.implicitHeight + Kirigami.Units.largeSpacing * 4)
                            }
                        }

                        // I tried, I really did in trying to figure out why we need to have an Item for this to show properly.
                        // But unfortunately, all attempts were put down by the complexity that is involved. Ultimately, I have
                        // given up - (sh_zam).
                        contentItem: Item {
                            implicitWidth: projectsInfoPanel.implicitWidth
                            implicitHeight: projectsInfoPanel.implicitHeight + Kirigami.Units.smallSpacing

                            ColumnLayout {
                                id: projectsInfoPanel
                                anchors.fill: parent
                                clip: true

                                Controls.Label {
                                    id: sizeLabel
                                    Layout.fillWidth: true
                                    color: Kirigami.Theme.textColor
                                    font.pointSize: 11

                                    text: model.size
                                }
                                Controls.Label {
                                    id: modifiedLabel
                                    Layout.fillWidth: true
                                    color: Kirigami.Theme.textColor
                                    font.pointSize: 11

                                    text: model.modified
                                }
                            }
                        }

                        onClicked: projectsGridView.itemSelected(model.url)
                    }
                    onItemSelected: function(url) {
                        welcomePage.openProjectsUrl(url)
                    }
                }
            }

            Loader {
                property bool activated: false
                onLoaded: activated = true
                active: Controls.SwipeView.isCurrentItem || activated

                sourceComponent: Kirigami.ScrollablePage {
                    id: featuredPage
                    Text {
                        text: "Featured Page"
                    }
                }
            }

            Loader {
                property bool activated: false
                onLoaded: activated = true
                active: Controls.SwipeView.isCurrentItem || activated

                sourceComponent: KisGridView {
                    id: referencesGridView
                    anchors.fill: parent

                    minimumColumnWidth: 300
                    maximumColumnWidth: 400

                    minimumColumnHeight: 150
                    maximumColumnHeight: 250

                    model: ReferencesModel {}

                    delegate: Kirigami.Card {
                        hoverEnabled: true
                        implicitHeight: referencesGridView.cellHeight - Kirigami.Units.largeSpacing * 2
                        implicitWidth: referencesGridView.cellWidth - Kirigami.Units.largeSpacing * 2

                        banner {
                            source: model.thumbnail
                            sourceSize {
                                width: referencesGridView.cellWidth
                                height: referencesGridView.cellHeight - 100
                            }
                            title: model.title
                            titleAlignment: Qt.AlignBottom | Qt.AlignLeft
                        }

                        contentItem: Kirigami.Heading {
                            text: model.description
                            wrapMode: Text.WordWrap
                            color: Kirigami.Theme.textColor
                            level: 3

                            Controls.ToolTip.visible: hovered
                            Controls.ToolTip.text: "Open In Browser"
                            Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                        }

                        onClicked: referencesGridView.itemSelected(model.url)
                    }

                    onItemSelected: function(url) {
                        Qt.openUrlExternally(url)
                    }
                }
            }

            Loader {
                property bool activated: false
                onLoaded: activated = true
                active: Controls.SwipeView.isCurrentItem || activated

                sourceComponent: KisGridView {
                    id: tutorialsGridView
                    anchors.fill: parent

                    minimumColumnWidth: 150
                    maximumColumnWidth: 480

                    minimumColumnHeight: 210
                    maximumColumnHeight: 360

                    model: tutorialsModel

                    delegate: Kirigami.Card {
                        id: delegate
                        implicitWidth: tutorialsGridView.cellWidth - Kirigami.Units.largeSpacing * 2
                        implicitHeight: tutorialsGridView.cellHeight - Kirigami.Units.largeSpacing * 2
                        hoverEnabled: true
                        banner {
                            source: model.thumbnail
                            sourceSize {
                                width: tutorialsGridView.cellWidth
                                height: delegate.implicitHeight

                            }
                            title: model.title
                            titleAlignment: Qt.AlignBottom | Qt.AlignLeft
                            titleWrapMode: Text.WordWrap
                        }

                        Controls.ToolTip.text: "Open In Browser"
                        Controls.ToolTip.visible: hovered
                        Controls.ToolTip.delay: Kirigami.Units.toolTipDelay

                        onClicked: tutorialsGridView.itemSelected(model.url)
                    }

                    onItemSelected: function(url) {
                        Qt.openUrlExternally(url)
                    }
                }
            }
        }
    }
}
