import QtQuick
import QtQuick.Controls.Basic
import SteamDeckMSX

// GameGrid — v0.3.0-MazeOfGalious: de galerij.
//
// Vervangt de recents-lijst als hoofdweergave: alles wat de scanner vindt komt
// hier te staan. Navigatie is bewust volledig met knoppen te doen (dpad, L1/R1
// per pagina, A start) — op een Steam Deck is een muis er niet.
GridView {
    id: grid

    signal activated(int index, var entry)
    signal filesDropped(var urls)
    signal rescanRequested()
    signal addFolderRequested()

    // Mappen waarin de laatste scan heeft gezocht. Staat in de lege staat, want
    // "niets gevonden" zonder te tonen wáár is gezocht is niet te debuggen —
    // precies het probleem dat v0.3.0 opleverde.
    property var searchedPaths: []
    // Bron voor de frame-reeksen van de tegel-animaties; doorgegeven vanuit
    // Main.qml omdat de generator daar leeft.
    property var thumbGen: null

    // Vier op een rij bij 1280px breed: groot genoeg om de screenshot te lezen,
    // klein genoeg om te bladeren zonder eindeloos scrollen.
    readonly property int columns: Math.max(2, Math.floor(width / 280))
    readonly property int tileSpacing: Tokens.space4

    cellWidth: Math.floor(width / columns)
    cellHeight: Math.floor(cellWidth * 0.86)

    clip: true
    focus: true
    boundsBehavior: Flickable.StopAtBounds
    keyNavigationEnabled: true
    keyNavigationWraps: false
    highlightMoveDuration: Tokens.motionFast
    cacheBuffer: 800   // paar rijen vooruit renderen: vloeiend scrollen op de Deck

    delegate: Item {
        width: grid.cellWidth
        height: grid.cellHeight

        GameTile {
            anchors.fill: parent
            anchors.margins: grid.tileSpacing / 2
            title: model.title
            machine: model.machine
            mediaType: model.mediaType
            sha1: model.sha1
            thumbGen: grid.thumbGen
            hasThumb: model.hasThumb
            thumbSource: model.hasThumb ? "file://" + model.thumbPath : ""
            focused: GridView.isCurrentItem

            MouseArea {
                anchors.fill: parent
                // De rechter joystick van de Deck stuurt een muiscursor. Zonder
                // hover verspringt de selectie pas bij de klik, en zie je tijdens
                // het bewegen niet waar je bent — dan navigeer je blind.
                hoverEnabled: true
                onEntered: grid.currentIndex = index
                onClicked: {
                    grid.currentIndex = index
                    grid.activateCurrent()
                }
            }
        }
    }

    function activateCurrent() {
        if (currentIndex < 0 || currentIndex >= count) return
        const m = grid.model
        if (!m || typeof m.entryAt !== "function") return
        grid.activated(currentIndex, m.entryAt(currentIndex))
    }

    // Lege staat: zonder dit ziet een verse installatie eruit als een bug.
    Column {
        anchors.centerIn: parent
        width: parent.width * 0.7
        spacing: Tokens.space4
        visible: grid.count === 0

        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Nog geen spellen gevonden")
            color: Tokens.fgPrimary
            font.family: Tokens.fontFamily
            font.pixelSize: Tokens.fontSizeDisplay
            font.weight: Font.Bold
        }
        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: qsTr("Zet je ROM-, disk- of tapebestanden ergens in je persoonlijke map "
                     + "of op de SD-kaart, en druk op R om opnieuw te scannen. "
                     + "Staat je collectie elders? Druk op M en wijs de map aan.")
            color: Tokens.fgSecondary
            font.family: Tokens.fontFamily
            font.pixelSize: Tokens.fontSizeBody
        }

        // Zonder deze lijst is "niets gevonden" niet te onderscheiden van
        // "op de verkeerde plek gezocht".
        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WrapAnywhere
            visible: grid.searchedPaths.length > 0
            text: qsTr("Gezocht in: ") + grid.searchedPaths.join("  ·  ")
            color: Tokens.fgDisabled
            font.family: Tokens.fontFamilyMono
            font.pixelSize: Tokens.fontSizeLabel
        }
    }

    DropArea {
        anchors.fill: parent
        keys: ["text/uri-list"]
        onDropped: function(drop) {
            const urls = []
            for (let i = 0; i < drop.urls.length; ++i) urls.push(drop.urls[i])
            if (urls.length > 0) {
                grid.filesDropped(urls)
                drop.accept()
            }
        }

        Rectangle {
            anchors.fill: parent
            color: Tokens.accentInfo
            opacity: parent.containsDrag ? 0.15 : 0
            border.color: Tokens.accentInfo
            border.width: parent.containsDrag ? Tokens.focusRingWidth : 0
            radius: 8
            Behavior on opacity { NumberAnimation { duration: Tokens.motionFast } }
        }
    }

    readonly property int pageJump: columns * 2

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter
            || event.key === Qt.Key_A || event.key === Qt.Key_Space) {
            grid.activateCurrent()
            event.accepted = true
        } else if (event.key === Qt.Key_R) {
            grid.rescanRequested()
            event.accepted = true
        } else if (event.key === Qt.Key_M) {
            grid.addFolderRequested()
            event.accepted = true
        } else if (event.key === Qt.Key_PageUp) {
            grid.currentIndex = Math.max(0, grid.currentIndex - grid.pageJump)
            event.accepted = true
        } else if (event.key === Qt.Key_PageDown) {
            grid.currentIndex = Math.min(grid.count - 1, grid.currentIndex + grid.pageJump)
            event.accepted = true
        } else if (event.key === Qt.Key_Home) {
            grid.currentIndex = 0
            event.accepted = true
        } else if (event.key === Qt.Key_End) {
            grid.currentIndex = Math.max(0, grid.count - 1)
            event.accepted = true
        }
    }
}
