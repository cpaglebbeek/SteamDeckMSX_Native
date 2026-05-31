import QtQuick
import SteamDeckMSX

Rectangle {
    id: card
    property string title
    property string year
    property string publisher
    property string machine
    property bool focused: false

    height: Tokens.listRowHeight
    radius: 6
    color: Tokens.bgElevated
    border.color: focused ? Tokens.borderStrong : Tokens.borderSubtle
    border.width: focused ? Tokens.focusRingWidth : 1

    Behavior on border.color { ColorAnimation { duration: Tokens.motionFast } }

    Row {
        anchors.fill: parent
        anchors.margins: Tokens.space4
        spacing: Tokens.space4

        // Machine-pictogram (tekst-fallback v0.0.3 — echte iconen v0.0.4)
        Rectangle {
            width: Tokens.minInteractive
            height: Tokens.minInteractive
            radius: 4
            color: Tokens.bgBase
            border.color: Tokens.borderSubtle
            border.width: 1
            anchors.verticalCenter: parent.verticalCenter

            Text {
                anchors.centerIn: parent
                text: card.machine
                color: Tokens.accentPrimary
                font.family: Tokens.fontFamilyMono
                font.pixelSize: Tokens.fontSizeLabel
                font.weight: Font.Bold
            }
        }

        Column {
            anchors.verticalCenter: parent.verticalCenter
            spacing: Tokens.space1
            width: parent.width - Tokens.minInteractive - Tokens.space4 - 200

            Text {
                text: card.title
                color: Tokens.fgPrimary
                font.family: Tokens.fontFamily
                font.pixelSize: Tokens.fontSizeBody
                font.weight: Font.DemiBold
                elide: Text.ElideRight
                width: parent.width
            }
            Text {
                text: card.publisher + " · " + card.year
                color: Tokens.fgSecondary
                font.family: Tokens.fontFamily
                font.pixelSize: Tokens.fontSizeLabel
            }
        }

        Item {
            width: 160
            height: 1
            anchors.verticalCenter: parent.verticalCenter
            visible: card.focused

            Text {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("A · start")
                color: Tokens.accentPrimary
                font.family: Tokens.fontFamily
                font.pixelSize: Tokens.fontSizeLabel
                font.weight: Font.DemiBold
            }
        }
    }
}
