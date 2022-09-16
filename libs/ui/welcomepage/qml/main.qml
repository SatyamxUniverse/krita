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

        Kirigami.ScrollablePage {
            id: homePage
            Text {
                text: "Home page"
            }
        }

        Kirigami.ScrollablePage {
            id: projectsPage

            KisGridView {
                id: projectsGridView
                anchors.fill: parent
                model: projectsModel

                delegate: Kirigami.Card {
                    hoverEnabled: true
                    banner {
                        title: model.title
                        source: model.thumbnail
                    }

                    Layout.maximumHeight: projectsGridView.maximumCardHeight
                    onClicked: projectsGridView.itemSelected(model.url)
                }
                onItemSelected: function(url) {
                    welcomePage.openProjectsUrl(url)
                }
            }
        }

        Kirigami.ScrollablePage {
            id: featuredPage
            Text {
                text: "Featured Page"
            }
        }

        Kirigami.ScrollablePage {
            id: referencesPage

            KisGridView {
                id: referencesGridView
                anchors.fill: parent
                maximumCardHeight: 250

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

        Kirigami.ScrollablePage {
            id: tutorialsPage

            KisGridView {
                id: tutorialsGridView
                anchors.fill: parent
                maximumCardHeight: 300
                maximumColumnWidth: 420

                model: tutorialsModel

                delegate: Kirigami.Card {
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
