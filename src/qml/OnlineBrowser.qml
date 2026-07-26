import QtQuick
import QtQuick.Controls.Basic
import SteamDeckMSX

// Zoeken en bladeren in een externe MSX-index, en het gekozen bestand ophalen.
// Wat binnenkomt gaat naar de gewone ROM-map, waarna de bibliotheekscan het
// oppikt: er is dus geen aparte "downloads"-lijst, het is meteen een spel in
// de galerij.
Popup {
    id: root

    property var index: null          // OnlineIndex
    signal downloadRequested(string url, string name)

    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(980, parent ? parent.width - Tokens.space5 * 2 : 980)
    height: Math.min(720, parent ? parent.height - Tokens.space5 * 2 : 720)
    padding: Tokens.space4

    background: Rectangle {
        color: Tokens.bgElevated
        border.color: Tokens.borderSubtle
        border.width: 1
        radius: 10
    }

    onOpened: {
        if (index) index.refresh()
        kb.text = index ? index.query : ""
    }

    contentItem: Column {
        spacing: Tokens.space3

        // Op het contentItem, niet op de Popup: Keys attacht alleen aan Items
        // en werd op de Popup stilzwijgend genegeerd (BUG-027).
        Keys.onPressed: function(e) {
            if (e.key === Qt.Key_Escape) { root.close(); e.accepted = true }
        }

        Row {
            width: parent.width
            spacing: Tokens.space4

            Text {
                text: qsTr("Spellen zoeken")
                color: Tokens.fgPrimary
                font.family: Tokens.fontFamily
                font.pixelSize: Tokens.fontSizeTitle
                font.weight: Font.Bold
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                text: root.index ? root.index.status : ""
                color: Tokens.fgSecondary
                font.family: Tokens.fontFamilyMono
                font.pixelSize: Tokens.fontSizeLabel
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        // Wat je tot nu toe getypt hebt — zonder dit typ je blind.
        Rectangle {
            width: parent.width
            height: Tokens.minInteractive
            radius: 6
            color: Tokens.bgBase
            border.color: Tokens.borderSubtle
            border.width: 1

            Text {
                anchors.left: parent.left
                anchors.leftMargin: Tokens.space3
                anchors.verticalCenter: parent.verticalCenter
                text: kb.text.length > 0 ? kb.text : qsTr("typ een titel…")
                color: kb.text.length > 0 ? Tokens.fgPrimary : Tokens.fgDisabled
                font.family: Tokens.fontFamily
                font.pixelSize: Tokens.fontSizeBody
            }
        }

        // A–Z-balk: bladeren zonder typen. De index is alfabetisch gesorteerd,
        // dus één letter kiezen toont aaneengesloten alles wat ermee begint —
        // met de cursor (rechter stick) is dat sneller dan het toetsenbord
        // voor wie de titel niet precies weet (gebruikersfeedback Deck, v0.4.1).
        Row {
            width: parent.width
            spacing: 0

            Repeater {
                model: ["✕"].concat("ABCDEFGHIJKLMNOPQRSTUVWXYZ".split(""))
                Rectangle {
                    readonly property bool isReset: modelData === "✕"
                    readonly property bool active: root.index
                        && (isReset ? root.index.letter === "" && root.index.query === ""
                                    : root.index.letter === modelData)
                    width: (parent.width) / 27
                    height: Tokens.minInteractive * 0.8
                    radius: 4
                    color: active ? Tokens.accentPrimary
                         : letterHover.containsMouse ? Tokens.bgOverlay : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: modelData
                        color: active ? Tokens.bgBase : Tokens.fgSecondary
                        font.family: Tokens.fontFamilyMono
                        font.pixelSize: Tokens.fontSizeLabel
                        font.weight: active ? Font.Bold : Font.Normal
                    }

                    MouseArea {
                        id: letterHover
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            if (!root.index) return
                            if (isReset) { root.index.letter = ""; root.index.query = ""; kb.text = "" }
                            else { kb.text = ""; root.index.letter = modelData }
                        }
                    }
                }
            }
        }

        // Resultaten. Filtert al tijdens het typen, dus de lijst is meestal na
        // drie letters kort genoeg om te overzien.
        ListView {
            width: parent.width
            height: root.height * 0.34
            clip: true
            model: root.index
            currentIndex: -1
            boundsBehavior: Flickable.StopAtBounds

            delegate: Rectangle {
                width: ListView.view.width
                height: Tokens.minInteractive
                color: hover.containsMouse ? Tokens.bgOverlay : "transparent"

                Column {
                    anchors.left: parent.left
                    anchors.leftMargin: Tokens.space3
                    anchors.verticalCenter: parent.verticalCenter
                    Text {
                        text: model.name
                        color: Tokens.fgPrimary
                        font.family: Tokens.fontFamily
                        font.pixelSize: Tokens.fontSizeBody
                    }
                    Text {
                        text: model.folder
                        color: Tokens.fgDisabled
                        font.family: Tokens.fontFamilyMono
                        font.pixelSize: Tokens.fontSizeLabel
                    }
                }

                MouseArea {
                    id: hover
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.downloadRequested(model.url, model.name)
                }
            }

            Text {
                anchors.centerIn: parent
                visible: parent.count === 0
                text: root.index && root.index.loading
                      ? qsTr("lijst ophalen…")
                      : qsTr("niets gevonden")
                color: Tokens.fgDisabled
                font.family: Tokens.fontFamily
                font.pixelSize: Tokens.fontSizeBody
            }
        }

        OnScreenKeyboard {
            id: kb
            width: parent.width
            onTextChanged: if (root.index) root.index.query = text
            onAccepted: if (root.index) root.index.query = text
        }

        MenuButton {
            label: qsTr("Sluiten")
            hint: qsTr("B")
            onClicked: root.close()
        }
    }

}
