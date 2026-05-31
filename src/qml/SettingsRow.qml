import QtQuick
import SteamDeckMSX

Rectangle {
    id: row
    property string label
    property string value

    height: Tokens.minInteractive
    color: Tokens.bgElevated
    radius: 4
    border.color: Tokens.borderSubtle
    border.width: 1

    Row {
        anchors.fill: parent
        anchors.margins: Tokens.space4
        spacing: Tokens.space4

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: row.label
            color: Tokens.fgPrimary
            font.family: Tokens.fontFamily
            font.pixelSize: Tokens.fontSizeBody
            width: 240
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: row.value
            color: Tokens.fgSecondary
            font.family: Tokens.fontFamilyMono
            font.pixelSize: Tokens.fontSizeMono
            elide: Text.ElideRight
            width: parent.width - 240 - Tokens.space4
        }
    }
}
