import QtQuick
import QtQuick.Controls.Basic
import SteamDeckMSX

// SlotPickerDialog — kies cart slot A of B bij ROM-load.
// v0.1.0-Xanadu DD-009: Slot A = default, Slot B alleen tijdens Running.
Popup {
    id: dlg
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    // Caller zet titel + pad. Caller hoort signals af.
    property string romPath: ""
    property string romTitle: ""
    property bool runningState: false  // slot B alleen actief als true

    signal slotAChosen()
    signal slotBChosen()
    signal canceled()

    width: 560
    height: 320
    anchors.centerIn: Overlay.overlay

    background: Rectangle {
        color: Tokens.bgElevated
        border.color: Tokens.borderSubtle
        border.width: 1
        radius: 8
    }

    contentItem: Column {
        spacing: Tokens.space4
        padding: Tokens.space5

        Text {
            text: qsTr("Welke cart slot?")
            color: Tokens.fgPrimary
            font.family: Tokens.fontFamily
            font.pixelSize: Tokens.fontSizeDisplay
            font.weight: Font.Bold
        }

        Text {
            text: dlg.romTitle
            color: Tokens.fgSecondary
            font.family: Tokens.fontFamilyMono
            font.pixelSize: Tokens.fontSizeBody
            elide: Text.ElideMiddle
            width: parent.width - 2 * Tokens.space5
        }

        Row {
            spacing: Tokens.space4
            anchors.horizontalCenter: parent.horizontalCenter

            Button {
                text: qsTr("Slot A (start)")
                width: 220; height: Tokens.minInteractive
                onClicked: { dlg.slotAChosen(); dlg.close() }
            }

            Button {
                text: dlg.runningState
                    ? qsTr("Slot B (uitbreiding)")
                    : qsTr("Slot B (start eerst)")
                width: 220; height: Tokens.minInteractive
                enabled: dlg.runningState
                onClicked: { dlg.slotBChosen(); dlg.close() }
            }
        }

        Text {
            text: dlg.runningState
                ? qsTr("Slot B: bv. SCC-extensie of game-cartridge naast hoofd-ROM.")
                : qsTr("Slot B beschikbaar zodra een spel draait (Slot A eerst).")
            color: Tokens.fgSecondary
            font.family: Tokens.fontFamily
            font.pixelSize: Tokens.fontSizeLabel
            wrapMode: Text.Wrap
            width: parent.width - 2 * Tokens.space5
        }

        Button {
            text: qsTr("Annuleren (B)")
            anchors.right: parent.right
            anchors.rightMargin: Tokens.space5
            onClicked: { dlg.canceled(); dlg.close() }
        }
    }
}
