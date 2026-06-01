import QtQuick
import SteamDeckMSX

Rectangle {
    id: card
    property bool focused: false
    property bool occupied: false
    property string label: ""
    property int slotNum: 0

    radius: 6
    color: occupied ? Tokens.bgElevated : Tokens.bgBase
    border.color: focused
        ? Tokens.accentPrimary
        : (occupied ? Tokens.borderSubtle : Tokens.fgDisabled)
    border.width: focused ? Tokens.focusRingWidth : 1

    Behavior on border.color { ColorAnimation { duration: Tokens.motionFast } }

    Column {
        anchors.fill: parent
        anchors.margins: Tokens.space3
        spacing: Tokens.space2

        Row {
            spacing: Tokens.space2
            width: parent.width

            Rectangle {
                width: 40; height: 40; radius: 4
                color: Tokens.bgBase
                border.color: occupied ? Tokens.accentPrimary : Tokens.fgDisabled
                border.width: 1
                anchors.verticalCenter: parent.verticalCenter
                Text {
                    anchors.centerIn: parent
                    text: card.slotNum
                    color: occupied ? Tokens.accentPrimary : Tokens.fgDisabled
                    font.family: Tokens.fontFamilyMono
                    font.pixelSize: Tokens.fontSizeBody
                    font.weight: Font.Bold
                }
            }
            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 0
                width: parent.width - 40 - Tokens.space2
                Text {
                    text: occupied ? qsTr("Slot ") + card.slotNum : qsTr("empty")
                    color: occupied ? Tokens.fgPrimary : Tokens.fgDisabled
                    font.family: Tokens.fontFamily
                    font.pixelSize: Tokens.fontSizeLabel
                    font.weight: Font.DemiBold
                }
                Text {
                    text: card.label
                    color: Tokens.fgSecondary
                    font.family: Tokens.fontFamilyMono
                    font.pixelSize: 12
                    elide: Text.ElideRight
                    width: parent.width
                }
            }
        }

        Item { width: parent.width; height: parent.height - 56 }

        Row {
            spacing: Tokens.space2
            anchors.bottom: parent.bottom
            visible: card.focused

            Image {
                source: Tokens.iconBtnA
                width: 24; height: 24
                sourceSize: Qt.size(24, 24)
                opacity: 0.95
                // currentColor via colorize-fragment niet beschikbaar in QML zonder
                // GraphicsEffects — v0.0.6 toont gewoon SVG met embedded currentColor=#fff
            }
            Text {
                text: occupied ? qsTr("Load") : qsTr("Save")
                color: Tokens.accentPrimary
                font.family: Tokens.fontFamily
                font.pixelSize: Tokens.fontSizeLabel
                font.weight: Font.DemiBold
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
}
