import QtQuick
import QtQuick.Controls.Basic

// LaunchPanel — v0.5.0-Goonies: tussenstap vóór het starten van een spel.
//
// Een tegel activeren start niet meer meteen (was DD-009), maar toont eerst
// dit paneel: slot A staat vooringevuld op het gekozen spel, slot B is leeg
// en optioneel te vullen met een tweede cartridge (SCC, FM-PAC, combo-carts).
// De focus staat op START, dus twee keer klikken is nog steeds spelen —
// het snelle pad blijft snel, het rijke pad wordt bereikbaar.
Popup {
    id: root

    property string gameTitle: ""
    property string slotAPath: ""
    property string machineLabel: ""
    property var romLibrary: null       // RomLibrary, voor de slot-B-kiezer
    property string slotBPath: ""
    property string slotBTitle: ""

    signal startRequested(string slotA, string slotB)

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    anchors.centerIn: Overlay.overlay
    width: 560
    padding: Tokens.space5

    background: Rectangle {
        color: Tokens.bgElevated
        border.color: Tokens.accentPrimary
        border.width: 1
        radius: 10
    }

    function openFor(title, romPath, machine) {
        gameTitle = title
        slotAPath = romPath
        machineLabel = machine
        slotBPath = ""
        slotBTitle = ""
        slotBChooser.visible = false
        open()
        startBtn.forceActiveFocus()
    }

    contentItem: Column {
        spacing: Tokens.space4

        // Op het contentItem — Keys attacht niet aan een Popup (BUG-027).
        Keys.onPressed: function(e) {
            if (e.key === Qt.Key_Escape) { root.close(); e.accepted = true }
        }

        Text {
            text: root.gameTitle
            width: root.width - Tokens.space5 * 2
            color: Tokens.fgPrimary
            font.family: Tokens.fontFamily
            font.pixelSize: Tokens.fontSizeTitle
            font.weight: Font.Bold
            elide: Text.ElideRight
        }

        // Slot A: het gekozen spel. Vast — een ander spel kies je in de galerij.
        Row {
            spacing: Tokens.space3
            Text {
                text: qsTr("Slot A")
                color: Tokens.fgSecondary
                font.family: Tokens.fontFamilyMono
                font.pixelSize: Tokens.fontSizeLabel
                width: 64
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                text: root.slotAPath.split("/").pop()
                width: root.width - Tokens.space5 * 2 - 64 - Tokens.space3
                color: Tokens.fgPrimary
                font.family: Tokens.fontFamily
                font.pixelSize: Tokens.fontSizeBody
                elide: Text.ElideMiddle
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        Row {
            spacing: Tokens.space3
            Text {
                text: qsTr("Slot B")
                color: Tokens.fgSecondary
                font.family: Tokens.fontFamilyMono
                font.pixelSize: Tokens.fontSizeLabel
                width: 64
                anchors.verticalCenter: parent.verticalCenter
            }
            MenuButton {
                label: root.slotBTitle.length > 0 ? root.slotBTitle : qsTr("— leeg — kies…")
                onClicked: slotBChooser.visible = !slotBChooser.visible
            }
            MenuButton {
                visible: root.slotBPath.length > 0
                label: qsTr("leegmaken")
                onClicked: { root.slotBPath = ""; root.slotBTitle = "" }
            }
        }

        // Slot-B-kiezer: dezelfde bibliotheek als de galerij, alleen cartridges —
        // een floppy of cassette hoort niet in een cartridge-slot.
        Rectangle {
            id: slotBChooser
            visible: false
            width: root.width - Tokens.space5 * 2
            height: 220
            radius: 6
            color: Tokens.bgBase
            border.color: Tokens.borderSubtle
            border.width: 1

            ListView {
                anchors.fill: parent
                anchors.margins: Tokens.space2
                clip: true
                model: root.romLibrary
                boundsBehavior: Flickable.StopAtBounds

                delegate: Rectangle {
                    width: ListView.view.width
                    height: visible ? Tokens.minInteractive : 0
                    visible: model.mediaType === "rom" && model.romPath !== root.slotAPath
                    color: slotBHover.containsMouse ? Tokens.bgOverlay : "transparent"

                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: Tokens.space3
                        anchors.verticalCenter: parent.verticalCenter
                        text: model.title
                        color: Tokens.fgPrimary
                        font.family: Tokens.fontFamily
                        font.pixelSize: Tokens.fontSizeBody
                    }

                    MouseArea {
                        id: slotBHover
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            root.slotBPath = model.romPath
                            root.slotBTitle = model.title
                            slotBChooser.visible = false
                        }
                    }
                }
            }
        }

        Text {
            text: qsTr("Machine: ") + root.machineLabel
            color: Tokens.fgSecondary
            font.family: Tokens.fontFamilyMono
            font.pixelSize: Tokens.fontSizeLabel
        }

        Row {
            spacing: Tokens.space4

            MenuButton {
                id: startBtn
                label: qsTr("▶  Start")
                primary: true
                KeyNavigation.right: cancelBtn
                onClicked: {
                    root.close()
                    root.startRequested(root.slotAPath, root.slotBPath)
                }
            }
            MenuButton {
                id: cancelBtn
                label: qsTr("Annuleer")
                hint: qsTr("B")
                KeyNavigation.left: startBtn
                onClicked: root.close()
            }
        }
    }
}
