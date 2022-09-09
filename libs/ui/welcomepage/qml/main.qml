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

                sourceComponent: Kirigami.ScrollablePage {
                    id: projectsPage

                    KisGridView {
                        id: projectsGridView
                        anchors.fill: parent
                        model: projectsModel

                        delegate: Kirigami.Card {
                            // because the parent doesn't set any constraint for the card size, we need to make
                            // sure we don't draw out of bounds. This is only mandatory if the card size is
                            // variable.
                            implicitHeight: projectsGridView.cellHeight - Kirigami.Units.largeSpacing * 2
                            hoverEnabled: true
                            banner {
                                title: model.title
                                source: model.thumbnail
                                titleAlignment: Qt.AlignBottom | Qt.AlignHCenter
                                sourceSize {
                                    width: 300
                                    height: 230
                                }
                            }

                            contentItem: Item {
                                anchors.fill: parent
                                implicitWidth: projectsInfoPanel.implicitWidth
                                implicitHeight: projectsInfoPanel.implicitHeight + Kirigami.Units.smallSpacing

                                ColumnLayout {
                                    id: projectsInfoPanel
                                    anchors.fill: parent
                                    Controls.Label {
                                        id: sizeLabel
                                        Layout.fillWidth: true
                                        text: model.size
                                        color: Kirigami.Theme.textColor
                                        font.pointSize: 11
                                    }
                                    Controls.Label {
                                        id: modifiedLabel
                                        Layout.fillWidth: true
                                        text: model.modified
                                        color: Kirigami.Theme.textColor
                                        font.pointSize: 11
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

                sourceComponent: Kirigami.ScrollablePage {
                    id: referencesPage

                    KisGridView {
                        id: referencesGridView
                        anchors.fill: parent

                        cardSizeRatio: 2
                        minimumCardHeight: 250

                        model: ReferencesModel {}
                        delegate: Kirigami.Card {
                            hoverEnabled: true
                            banner {
                                source: model.thumbnail
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
            }

            Loader {
                property bool activated: false
                onLoaded: activated = true
                active: Controls.SwipeView.isCurrentItem || activated

                sourceComponent: Kirigami.ScrollablePage {
                    id: tutorialsPage

                    KisGridView {
                        id: tutorialsGridView
                        anchors.fill: parent

                        cardSizeRatio: 1.333
                        minimumCardHeight: 210
                        maximumCardHeight: 360
                        maximumColumnWidth: 480

                        model: tutorialsModel

                        delegate: Kirigami.Card {
                            implicitHeight: tutorialsGridView.cellHeight - Kirigami.Units.largeSpacing * 2
                            hoverEnabled: true
                            banner {
                                source: model.thumbnail
                                // sourceClipRect: Qt.rect(0, 45, 480, 270)
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
}
