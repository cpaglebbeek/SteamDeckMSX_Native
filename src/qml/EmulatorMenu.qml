import QtQuick
import QtQuick.Controls.Basic

// Menu dat bovenop een lopend spel komt: verder spelen, terug naar de galerij,
// of alles afsluiten. Nodig omdat de galerij tijdens het spelen verborgen is
// (BUG-022) en de emulator zelf geen uitweg bood behalve een sneltoets — op de
// Deck heb je geen toetsenbord om die te vinden.
//
// De emulator wordt gepauzeerd zolang dit menu open staat: zonder pauze loopt
// het spel door terwijl je naar het menu kijkt, en dat kost levens.
Popup {
    id: root

    property string gameTitle: ""
    signal resumeRequested()
    signal savesRequested()
    signal galleryRequested()
    signal quitRequested()

    modal: true
    focus: true
    closePolicy: Popup.NoAutoClose      // alleen via een expliciete keuze weg
    anchors.centerIn: Overlay.overlay
    width: 420
    padding: Tokens.space5

    background: Rectangle {
        color: Tokens.bgElevated
        border.color: Tokens.accentPrimary
        border.width: 1
        radius: 10
    }

    onOpened: resumeBtn.forceActiveFocus()

    contentItem: Column {
        spacing: Tokens.space4

        // Op het contentItem, niet op de Popup zelf: Keys is een attached
        // property voor Items en een Popup is er geen — daar wordt hij
        // stilzwijgend genegeerd ("Could not attach Keys property", BUG-027).
        // Escape/B sluit het menu niet af maar hervat — anders zit je vast in
        // een menu dat je niet weg kunt klikken zonder het spel te verlaten.
        Keys.onPressed: function(e) {
            if (e.key === Qt.Key_Escape) { root.resumeRequested(); e.accepted = true }
            // Zelfde toets als in de galerij (en controller-X via Steam Input).
            else if (e.key === Qt.Key_X) { root.savesRequested(); e.accepted = true }
        }

        Text {
            text: qsTr("Pauze")
            color: Tokens.fgPrimary
            font.family: Tokens.fontFamily
            font.pixelSize: Tokens.fontSizeTitle
            font.weight: Font.Bold
        }

        Text {
            visible: root.gameTitle.length > 0
            text: root.gameTitle
            color: Tokens.fgSecondary
            font.family: Tokens.fontFamily
            font.pixelSize: Tokens.fontSizeBody
            elide: Text.ElideRight
            width: root.width - Tokens.space5 * 2
        }

        MenuButton {
            id: resumeBtn
            width: root.width - Tokens.space5 * 2
            label: qsTr("Verder spelen")
            hint: qsTr("A")
            primary: true
            KeyNavigation.down: savesBtn
            onClicked: root.resumeRequested()
        }

        MenuButton {
            // v0.5.1: tijdens het spelen is de galerij verborgen en vangt de
            // X-sneltoets dus niets — dit menu was de enige plek waar de speler
            // nog kon komen, maar het bood geen weg naar save-states. Op de
            // Deck bestond de functie daardoor simpelweg niet.
            id: savesBtn
            width: root.width - Tokens.space5 * 2
            label: qsTr("Save-states")
            hint: qsTr("X")
            KeyNavigation.up: resumeBtn
            KeyNavigation.down: galleryBtn
            onClicked: root.savesRequested()
        }

        MenuButton {
            id: galleryBtn
            width: root.width - Tokens.space5 * 2
            label: qsTr("Terug naar de galerij")
            // Geen "B"-hint meer: met de controller-layout (BUG-023) stuurt
            // B een Escape en dat betekent hier juist "verder spelen".
            KeyNavigation.up: savesBtn
            KeyNavigation.down: quitBtn
            onClicked: root.galleryRequested()
        }

        MenuButton {
            id: quitBtn
            width: root.width - Tokens.space5 * 2
            label: qsTr("SteamDeckMSX afsluiten")
            KeyNavigation.up: galleryBtn
            onClicked: root.quitRequested()
        }
    }

}
