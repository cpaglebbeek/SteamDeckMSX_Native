import QtQuick
import QtQuick.Controls.Basic
import SteamDeckMSX

// UrlImportDialog — generieke URL-importer voor BIOS én ROM.
//
// Aanroepers (Main.qml) zetten `target` op "bios" of "rom" en
// koppelen `confirmed(url, name, target)` aan respectievelijk
// BiosManager.addFromUrl of CartridgeModel.addFromUrl.
//
// v0.1.0-Xanadu DD-008: één dialog herbruikt — voorkomt UI-duplicatie.
Popup {
    id: dlg
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape

    // Target-keuze door caller. Default "rom".
    property string target: "rom"
    // Externe busy + progress (gezet door caller op basis van manager-property).
    property bool busy: false
    property real progress: 0.0  // 0..1
    property string progressLabel: ""

    signal confirmed(string url, string name, string target)
    signal canceled()

    width: 720
    height: 360
    anchors.centerIn: Overlay.overlay

    background: Rectangle {
        color: Tokens.bgElevated
        border.color: Tokens.borderSubtle
        border.width: 1
        radius: 8
    }

    contentItem: Column {
        spacing: Tokens.space5
        padding: Tokens.space5

        Text {
            text: dlg.target === "bios"
                ? qsTr("BIOS toevoegen via URL")
                : qsTr("ROM toevoegen via URL")
            color: Tokens.fgPrimary
            font.family: Tokens.fontFamily
            font.pixelSize: Tokens.fontSizeDisplay
            font.weight: Font.Bold
        }

        Column {
            spacing: Tokens.space2
            width: parent.width - 2 * Tokens.space5

            Text {
                text: qsTr("URL (HTTPS verplicht)")
                color: Tokens.fgSecondary
                font.family: Tokens.fontFamily
                font.pixelSize: Tokens.fontSizeLabel
            }

            TextField {
                id: urlField
                width: parent.width
                placeholderText: qsTr("https://example.com/bios.rom")
                font.family: Tokens.fontFamilyMono
                font.pixelSize: Tokens.fontSizeBody
                color: Tokens.fgPrimary
                background: Rectangle {
                    color: Tokens.bgBase
                    border.color: urlField.activeFocus ? Tokens.accentPrimary : Tokens.borderSubtle
                    border.width: urlField.activeFocus ? 2 : 1
                    radius: 4
                }
                enabled: !dlg.busy
            }
        }

        Column {
            spacing: Tokens.space2
            width: parent.width - 2 * Tokens.space5

            Text {
                text: qsTr("Optionele naam (anders auto)")
                color: Tokens.fgSecondary
                font.family: Tokens.fontFamily
                font.pixelSize: Tokens.fontSizeLabel
            }

            TextField {
                id: nameField
                width: parent.width
                placeholderText: qsTr("bv. msx2plus.rom")
                font.family: Tokens.fontFamilyMono
                font.pixelSize: Tokens.fontSizeBody
                color: Tokens.fgPrimary
                background: Rectangle {
                    color: Tokens.bgBase
                    border.color: nameField.activeFocus ? Tokens.accentPrimary : Tokens.borderSubtle
                    border.width: nameField.activeFocus ? 2 : 1
                    radius: 4
                }
                enabled: !dlg.busy
            }
        }

        // Progress-bar (alleen zichtbaar tijdens download)
        ProgressBar {
            width: parent.width - 2 * Tokens.space5
            from: 0; to: 1
            value: dlg.progress
            visible: dlg.busy
        }

        Text {
            visible: dlg.busy
            text: dlg.progressLabel
            color: Tokens.fgSecondary
            font.family: Tokens.fontFamilyMono
            font.pixelSize: Tokens.fontSizeLabel
        }

        Row {
            spacing: Tokens.space4
            anchors.right: parent.right
            anchors.rightMargin: Tokens.space5

            Button {
                text: qsTr("Annuleren")
                enabled: !dlg.busy
                onClicked: { dlg.canceled(); dlg.close() }
            }
            Button {
                text: dlg.busy ? qsTr("Bezig…") : qsTr("Downloaden")
                enabled: !dlg.busy && urlField.text.trim().length > 0
                onClicked: dlg.confirmed(urlField.text.trim(), nameField.text.trim(), dlg.target)
            }
        }
    }

    onOpened: { urlField.text = ""; nameField.text = ""; urlField.forceActiveFocus() }
}
