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

    signal confirmed(string url, string name, string target, string user, string password)
    signal canceled()

    // Geen vaste hoogte: sinds het schermtoetsenbord standaard uitgeklapt staat
    // (v0.5.0) groeit de dialoog met de inhoud mee; 360 sneed hem af.
    width: 720
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
                onActiveFocusChanged: if (activeFocus) kbPanel.field = "url"
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
                onActiveFocusChanged: if (activeFocus) kbPanel.field = "name"
            }
        }

        // Inloggegevens voor bronnen achter een wachtwoord (eigen NAS of
        // webserver). Leeg laten als de bron openbaar is. Er is bewust geen
        // opgeslagen standaardwaarde: een wachtwoord dat met de app meereist
        // is leesbaar voor iedereen die de app heeft en beschermt dus niets.
        Column {
            spacing: Tokens.space2
            width: parent.width - 2 * Tokens.space5

            Row {
                spacing: Tokens.space3
                Text {
                    text: qsTr("Inloggegevens (alleen als de bron erom vraagt)")
                    color: Tokens.fgSecondary
                    font.family: Tokens.fontFamily
                    font.pixelSize: Tokens.fontSizeLabel
                    anchors.verticalCenter: parent.verticalCenter
                }
                MenuButton {
                    label: kbPanel.visible ? qsTr("toetsenbord verbergen") : qsTr("toetsenbord")
                    onClicked: kbPanel.visible = !kbPanel.visible
                }
            }

            Row {
                width: parent.width
                spacing: Tokens.space3

                TextField {
                    id: userField
                    width: (parent.width - Tokens.space3) / 2
                    placeholderText: qsTr("gebruiker")
                    font.family: Tokens.fontFamilyMono
                    font.pixelSize: Tokens.fontSizeBody
                    color: Tokens.fgPrimary
                    background: Rectangle {
                        color: Tokens.bgBase
                        border.color: userField.activeFocus ? Tokens.accentPrimary : Tokens.borderSubtle
                        border.width: userField.activeFocus ? 2 : 1
                        radius: 4
                    }
                    enabled: !dlg.busy
                    onActiveFocusChanged: if (activeFocus) kbPanel.field = "user"
                }

                TextField {
                    id: passField
                    width: (parent.width - Tokens.space3) / 2
                    placeholderText: qsTr("wachtwoord")
                    echoMode: TextInput.Password
                    font.family: Tokens.fontFamilyMono
                    font.pixelSize: Tokens.fontSizeBody
                    color: Tokens.fgPrimary
                    background: Rectangle {
                        color: Tokens.bgBase
                        border.color: passField.activeFocus ? Tokens.accentPrimary : Tokens.borderSubtle
                        border.width: passField.activeFocus ? 2 : 1
                        radius: 4
                    }
                    enabled: !dlg.busy
                    onActiveFocusChanged: if (activeFocus) kbPanel.field = "pass"
                }
            }

            // Schermtoetsenbord: op de Deck is er geen fysiek toetsenbord, dus
            // zonder dit zijn deze velden daar niet in te vullen. Sinds v0.5.0
            // bedient het álle velden (URL/naam/gebruiker/wachtwoord): het veld
            // met focus is het doel, en bij het wisselen neemt het toetsenbord
            // de bestaande veldinhoud over in plaats van die te overschrijven.
            Column {
                id: kbPanel
                visible: true
                width: parent.width
                property string field: "url"
                onFieldChanged: credKb.text =
                    field === "url"  ? urlField.text
                  : field === "name" ? nameField.text
                  : field === "user" ? userField.text
                  : passField.text

                OnScreenKeyboard {
                    id: credKb
                    width: parent.width
                    doneLabel: qsTr("klaar")
                    onTextChanged: {
                        if (kbPanel.field === "url") urlField.text = text
                        else if (kbPanel.field === "name") nameField.text = text
                        else if (kbPanel.field === "user") userField.text = text
                        else passField.text = text
                    }
                    onAccepted: kbPanel.visible = false
                }
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
                onClicked: dlg.confirmed(urlField.text.trim(), nameField.text.trim(), dlg.target, userField.text.trim(), passField.text)
            }
        }
    }

    onOpened: {
        urlField.text = ""; nameField.text = ""
        kbPanel.field = "url"; credKb.text = ""; kbPanel.visible = true
        urlField.forceActiveFocus()
    }
}
