import QtQuick
import SteamDeckMSX

Rectangle {
    id: card
    property bool focused: false

    height: Tokens.listRowHeight
    radius: 6
    color: Tokens.bgElevated
    border.color: focused ? Tokens.accentWarm : Tokens.borderSubtle
    border.width: focused ? Tokens.focusRingWidth : 1

    Behavior on border.color { ColorAnimation { duration: Tokens.motionFast } }

    Row {
        anchors.fill: parent
        anchors.margins: Tokens.space4
        spacing: Tokens.space4

        Rectangle {
            width: Tokens.minInteractive
            height: Tokens.minInteractive
            radius: 4
            color: "transparent"
            border.color: Tokens.accentWarm
            border.width: 2
            anchors.verticalCenter: parent.verticalCenter

            Text {
                anchors.centerIn: parent
                text: "+"
                color: Tokens.accentWarm
                font.family: Tokens.fontFamily
                font.pixelSize: 36
                font.weight: Font.Bold
            }
        }

        Column {
            anchors.verticalCenter: parent.verticalCenter
            spacing: Tokens.space1

            Text {
                text: qsTr("Add ROM…")
                color: Tokens.fgPrimary
                font.family: Tokens.fontFamily
                font.pixelSize: Tokens.fontSizeBody
                font.weight: Font.DemiBold
            }
            Text {
                text: qsTr("Bestand kiezen — .rom / .dsk / .cas")
                color: Tokens.fgSecondary
                font.family: Tokens.fontFamily
                font.pixelSize: Tokens.fontSizeLabel
            }
        }
    }
}
