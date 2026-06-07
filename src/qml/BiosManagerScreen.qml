import QtQuick
import QtQuick.Controls.Basic
import SteamDeckMSX

// BiosManagerScreen — full-screen overlay (Popup) voor BIOS-beheer.
// Toont lijst, knoppen "+ URL" + "+ Lokaal", remove per entry.
// v0.1.0-Xanadu DD-007.
Popup {
    id: scr
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape

    // Caller verzorgt model + callbacks.
    property var biosModel
    signal addFromUrlClicked()
    signal addFromLocalClicked()
    signal removeBios(string id)
    signal closed()  // user-close

    width: parent ? parent.width * 0.95 : 1200
    height: parent ? parent.height * 0.92 : 760
    anchors.centerIn: Overlay.overlay

    background: Rectangle {
        color: Tokens.bgOverlay
        border.color: Tokens.borderSubtle
        border.width: 1
        radius: 8
    }

    contentItem: Column {
        spacing: Tokens.space4
        padding: Tokens.space5

        // Header
        Row {
            width: parent.width - 2 * Tokens.space5
            spacing: Tokens.space4

            Text {
                text: qsTr("BIOS-bibliotheek")
                color: Tokens.fgPrimary
                font.family: Tokens.fontFamily
                font.pixelSize: Tokens.fontSizeDisplay
                font.weight: Font.Bold
                anchors.verticalCenter: parent.verticalCenter
            }

            Item { width: parent.width - 360 - 460; height: 1 }  // spacer

            Button {
                text: qsTr("+ Lokaal")
                width: 160; height: Tokens.minInteractive
                onClicked: scr.addFromLocalClicked()
            }
            Button {
                text: qsTr("+ Vanaf URL")
                width: 180; height: Tokens.minInteractive
                onClicked: scr.addFromUrlClicked()
            }
            Button {
                text: qsTr("Sluiten (B)")
                width: 140; height: Tokens.minInteractive
                onClicked: { scr.closed(); scr.close() }
            }
        }

        Rectangle {
            width: parent.width - 2 * Tokens.space5
            height: 1
            color: Tokens.borderSubtle
        }

        // Lijst
        ListView {
            id: list
            width: parent.width - 2 * Tokens.space5
            height: parent.height - 200
            clip: true
            spacing: Tokens.space2
            model: scr.biosModel
            keyNavigationEnabled: true

            delegate: Rectangle {
                width: list.width
                height: 96
                color: ListView.isCurrentItem ? Tokens.bgElevated : Tokens.bgBase
                border.color: ListView.isCurrentItem ? Tokens.accentPrimary : Tokens.borderSubtle
                border.width: ListView.isCurrentItem ? Tokens.focusRingWidth : 1
                radius: 4

                Row {
                    anchors.fill: parent
                    anchors.margins: Tokens.space4
                    spacing: Tokens.space4

                    Column {
                        width: parent.width - 200
                        spacing: Tokens.space1
                        anchors.verticalCenter: parent.verticalCenter

                        Text {
                            text: fileName
                            color: Tokens.fgPrimary
                            font.family: Tokens.fontFamily
                            font.pixelSize: Tokens.fontSizeBody
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                            width: parent.width
                        }
                        Text {
                            text: qsTr("sha1 ") + (sha1Hex.length > 0 ? sha1Hex.substring(0, 12) + "…" : "(onbekend)")
                                  + qsTr(" · size ") + (sizeBytes / 1024).toFixed(1) + " KiB"
                                  + (source ? qsTr(" · ") + source : "")
                            color: Tokens.fgSecondary
                            font.family: Tokens.fontFamilyMono
                            font.pixelSize: Tokens.fontSizeLabel
                            elide: Text.ElideRight
                            width: parent.width
                        }
                    }

                    Button {
                        text: qsTr("Verwijder")
                        width: 180
                        height: Tokens.minInteractive
                        anchors.verticalCenter: parent.verticalCenter
                        onClicked: scr.removeBios(biosId)
                    }
                }
            }

            // Leeg-staat hint
            Text {
                visible: list.count === 0
                anchors.centerIn: parent
                text: qsTr("Nog geen BIOS-files. Voeg toe via \"+ Lokaal\" of \"+ Vanaf URL\".")
                color: Tokens.fgSecondary
                font.family: Tokens.fontFamily
                font.pixelSize: Tokens.fontSizeBody
            }
        }

        Text {
            text: qsTr("Tip: BIOS-files horen in openMSX' machines-dir. v0.1.0 toont alleen administratie — koppeling aan openMSX volgt v0.2.0 (auto machine.xml-link).")
            color: Tokens.fgSecondary
            font.family: Tokens.fontFamily
            font.pixelSize: Tokens.fontSizeLabel
            wrapMode: Text.Wrap
            width: parent.width - 2 * Tokens.space5
        }
    }
}
