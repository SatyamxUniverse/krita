import QtQuick 2.3
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.3 as Controls

import org.kde.kirigami 2.12 as Kirigami

Kirigami.Page {
    id: root
    anchors.fill: parent

    header: Controls.TabBar {
        id: tabBar
        currentIndex: swipeView.currentIndex

        Controls.TabButton {
            text: "Home"
        }
        Controls.TabButton {
            text: "Projects"
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

    Controls.SwipeView {
        id: swipeView
        anchors.fill: parent
        currentIndex: tabBar.currentIndex

        Loader {
            active: Controls.SwipeView.isCurrentItem
            sourceComponent: Kirigami.ScrollablePage {
                id: homePage
                Text {
                    text: "Home page"
                }
            }
        }

        Loader {
            active: Controls.SwipeView.isCurrentItem
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

                        Layout.maximumHeight: projectsGridView.maximumCardHeight
                        onClicked: projectsGridView.itemSelected(model.url)
                    }
                    onItemSelected: function(url) {
                        welcomePage.openProjectsUrl(url)
                    }
                }
            }
        }

        Loader {
            active: Controls.SwipeView.isCurrentItem
            sourceComponent: Kirigami.ScrollablePage {
                id: featuredPage
                Text {
                    text: "Featured Page"
                }
            }
        }

        Loader {
            active: Controls.SwipeView.isCurrentItem
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

                        Layout.maximumHeight: referencesGridView.maximumCardHeight
                        onClicked: referencesGridView.itemSelected(model.url)
                    }

                    onItemSelected: function(url) {
                        Qt.openUrlExternally(url)
                    }
                }
            }
        }

        Loader {
            active: Controls.SwipeView.isCurrentItem
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

                        Layout.maximumHeight: tutorialsGridView.maximumCardHeight
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
