import QtQuick
import QtQuick.Window
import QtQuick.Controls.Basic
import SteamDeckMSX

ApplicationWindow {
    id: root
    visible: true
    width: 1280
    height: 800
    color: Tokens.bgBase
    title: qsTr("SteamDeckMSX")

    MsxCore {
        id: msxCore
        Component.onCompleted: probeVersion()
    }

    CartridgeModel {
        id: cartridges
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: Tokens.safeMargin
        color: "transparent"

        Column {
            anchors.fill: parent
            spacing: Tokens.space4

            // Header
            Row {
                width: parent.width
                spacing: Tokens.space4

                Text {
                    text: qsTr("SteamDeckMSX")
                    color: Tokens.fgPrimary
                    font.family: Tokens.fontFamily
                    font.pixelSize: Tokens.fontSizeDisplay
                    font.weight: Font.Bold
                    font.letterSpacing: Tokens.fontSizeDisplay * 0.02
                }
                Item { width: parent.width - 600; height: 1 }
                Text {
                    text: qsTr("openMSX: ") + (msxCore.version.length > 0 ? msxCore.version : qsTr("─ ") + msxCore.status)
                    color: Tokens.fgSecondary
                    font.family: Tokens.fontFamilyMono
                    font.pixelSize: Tokens.fontSizeMono
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            // Cartridge browser
            CartridgeBrowser {
                id: browser
                model: cartridges
                width: parent.width
                height: parent.height - 240
                focus: true
                onActivated: function(index) {
                    var t = cartridges.data(cartridges.index(index, 0), Qt.UserRole + 1)
                    console.log("[v0.0.3] activate placeholder:", t)
                }
            }

            // Settings stub
            SettingsRow {
                width: parent.width
                label: qsTr("BIOS-pad")
                value: qsTr("(niet geconfigureerd — v0.0.4)")
            }
        }
    }

    // Globale toets: Esc / B = back (v0.0.3 = quit). Window erft van Window
    // niet van Item — gebruik Shortcut i.p.v. Keys.onPressed (P-SDM Qt6-quirk).
    Shortcut {
        sequences: ["Escape", "B"]
        onActivated: Qt.quit()
    }
}
