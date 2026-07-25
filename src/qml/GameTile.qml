import QtQuick
import SteamDeckMSX

// GameTile — v0.3.0-MazeOfGalious: één spel in de galerij.
//
// Homebrew-Channel-gevoel: de tegel zelf is het beeld, de tekst zit eronder,
// en focus wordt getoond door de tegel te laten opschalen met een gloeiende
// rand — niet door een selectiebalk. Er is altijd iets te zien: zolang de
// screenshot nog niet gemaakt is, staat er een gegenereerde achtergrond met
// de titel, zodat het grid nooit leeg of "kapot" oogt tijdens de eerste scan.
Item {
    id: tile

    property string title
    property string machine
    property string mediaType: "rom"
    property string sha1
    property url thumbSource
    property bool hasThumb: false
    property bool focused: false

    // Kleur uit de fingerprint: dezelfde ROM krijgt altijd dezelfde tegel,
    // en een map vol spellen levert vanzelf een gevarieerd palet op.
    readonly property real _hue: {
        if (!sha1 || sha1.length < 4) return 0.45
        return (parseInt(sha1.substring(0, 4), 16) % 1000) / 1000
    }

    scale: focused ? 1.06 : 1.0
    Behavior on scale { NumberAnimation { duration: Tokens.motionFast; easing.type: Easing.OutCubic } }

    Column {
        anchors.fill: parent
        spacing: Tokens.space2

        // ---- beeldvlak ----
        Rectangle {
            id: art
            width: parent.width
            height: parent.height - labels.height - Tokens.space2
            radius: 8
            clip: true
            color: Tokens.bgElevated
            border.color: tile.focused ? Tokens.borderStrong : Tokens.borderSubtle
            border.width: tile.focused ? Tokens.focusRingWidth : 1

            Behavior on border.color { ColorAnimation { duration: Tokens.motionFast } }

            // Gegenereerde achtergrond — zichtbaar tot de screenshot er is.
            Rectangle {
                anchors.fill: parent
                visible: !tile.hasThumb
                gradient: Gradient {
                    GradientStop { position: 0.0; color: Qt.hsla(tile._hue, 0.45, 0.28, 1.0) }
                    GradientStop { position: 1.0; color: Qt.hsla(tile._hue, 0.55, 0.12, 1.0) }
                }

                Text {
                    anchors.centerIn: parent
                    width: parent.width - Tokens.space5
                    horizontalAlignment: Text.AlignHCenter
                    text: tile.title
                    color: Tokens.fgPrimary
                    font.family: Tokens.fontFamily
                    font.pixelSize: Tokens.fontSizeBody
                    font.weight: Font.Bold
                    wrapMode: Text.WordWrap
                    maximumLineCount: 3
                    elide: Text.ElideRight
                    opacity: 0.92
                }
            }

            Image {
                id: shot
                anchors.fill: parent
                source: tile.hasThumb ? tile.thumbSource : ""
                visible: tile.hasThumb && status === Image.Ready
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                cache: false          // grid van honderden tegels: geen pixmap-cache volpompen
                sourceSize.width: 512 // MSX-beeld is 640x480; 512 is ruim zat voor een tegel
            }

            // Mediatype-badge: een floppy of tape gedraagt zich anders dan een
            // cartridge, dus dat moet je vóór het starten kunnen zien.
            Rectangle {
                visible: tile.mediaType !== "rom"
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: Tokens.space2
                width: badgeText.implicitWidth + Tokens.space3
                height: badgeText.implicitHeight + Tokens.space1
                radius: 4
                color: Tokens.bgOverlay
                border.color: Tokens.accentInfo
                border.width: 1

                Text {
                    id: badgeText
                    anchors.centerIn: parent
                    text: tile.mediaType.toUpperCase()
                    color: Tokens.accentInfo
                    font.family: Tokens.fontFamilyMono
                    font.pixelSize: Tokens.fontSizeLabel
                }
            }

            // Startaanwijzing, alleen op de tegel die focus heeft.
            Rectangle {
                visible: tile.focused
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.margins: Tokens.space2
                width: startHint.implicitWidth + Tokens.space4
                height: startHint.implicitHeight + Tokens.space2
                radius: 4
                color: Tokens.bgOverlay

                Text {
                    id: startHint
                    anchors.centerIn: parent
                    text: qsTr("A · start")
                    color: Tokens.accentPrimary
                    font.family: Tokens.fontFamily
                    font.pixelSize: Tokens.fontSizeLabel
                    font.weight: Font.DemiBold
                }
            }
        }

        // ---- bijschrift ----
        Column {
            id: labels
            width: parent.width
            spacing: 0

            Text {
                width: parent.width
                text: tile.title
                color: tile.focused ? Tokens.fgPrimary : Tokens.fgSecondary
                font.family: Tokens.fontFamily
                font.pixelSize: Tokens.fontSizeLabel
                font.weight: tile.focused ? Font.DemiBold : Font.Normal
                elide: Text.ElideRight
                maximumLineCount: 1
            }
            Text {
                width: parent.width
                text: tile.machine
                color: Tokens.fgDisabled
                font.family: Tokens.fontFamilyMono
                font.pixelSize: Tokens.fontSizeLabel
                elide: Text.ElideRight
                maximumLineCount: 1
            }
        }
    }
}
