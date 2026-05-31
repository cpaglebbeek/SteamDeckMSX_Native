import QtQuick
import QtQuick.Controls.Basic
import SteamDeckMSX

// Inline dropdown voor MSX-machine-keuze. Bedoeld voor in de bottom-bar.
// Gamepad: A = open popup, D-pad up/down = navigeer, A = select, B = close.
Rectangle {
    id: root
    property var model: null              // MachineModel
    property string currentMachine: ""    // bound from MsxCore / MachineModel
    signal machineChosen(string name)

    height: Tokens.minInteractive
    radius: 4
    color: Tokens.bgElevated
    border.color: focus ? Tokens.borderStrong : Tokens.borderSubtle
    border.width: focus ? Tokens.focusRingWidth : 1

    activeFocusOnTab: true

    Behavior on border.color { ColorAnimation { duration: Tokens.motionFast } }

    Row {
        anchors.fill: parent
        anchors.margins: Tokens.space4
        spacing: Tokens.space3

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Machine")
            color: Tokens.fgSecondary
            font.family: Tokens.fontFamily
            font.pixelSize: Tokens.fontSizeLabel
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: root.currentMachine.length > 0 ? root.currentMachine : qsTr("(default)")
            color: Tokens.fgPrimary
            font.family: Tokens.fontFamilyMono
            font.pixelSize: Tokens.fontSizeMono
            font.weight: Font.DemiBold
        }
        Item { width: 8; height: 1 }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: "▾"
            color: Tokens.accentPrimary
            font.pixelSize: Tokens.fontSizeBody
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: popup.open()
    }

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter ||
            event.key === Qt.Key_A || event.key === Qt.Key_Space) {
            popup.open()
            event.accepted = true
        }
    }

    Popup {
        id: popup
        x: 0
        y: -Math.min(360, (root.model ? root.model.rowCount() : 3) * Tokens.minInteractive + Tokens.space4)
        width: 360
        height: Math.min(360, (root.model ? root.model.rowCount() : 3) * Tokens.minInteractive + Tokens.space4)
        modal: true
        focus: true
        padding: Tokens.space2

        background: Rectangle {
            color: Tokens.bgElevated
            border.color: Tokens.accentPrimary
            border.width: 2
            radius: 6
        }

        ListView {
            id: machineList
            anchors.fill: parent
            model: root.model
            clip: true
            spacing: Tokens.space1
            keyNavigationEnabled: true
            highlightMoveDuration: Tokens.motionFast
            focus: true

            delegate: Rectangle {
                width: machineList.width
                height: Tokens.minInteractive
                radius: 4
                color: ListView.isCurrentItem ? Tokens.bgBase : "transparent"
                border.color: ListView.isCurrentItem ? Tokens.accentPrimary : "transparent"
                border.width: 2

                Row {
                    anchors.fill: parent
                    anchors.margins: Tokens.space3
                    spacing: Tokens.space3

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: model.isCurrent ? "●" : "○"
                        color: model.isCurrent ? Tokens.accentPrimary : Tokens.fgDisabled
                        font.pixelSize: Tokens.fontSizeBody
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: model.name
                        color: Tokens.fgPrimary
                        font.family: Tokens.fontFamilyMono
                        font.pixelSize: Tokens.fontSizeMono
                    }
                }
            }

            Keys.onPressed: function(event) {
                if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter ||
                    event.key === Qt.Key_A || event.key === Qt.Key_Space) {
                    if (machineList.currentIndex >= 0 && root.model) {
                        const name = root.model.data(
                            root.model.index(machineList.currentIndex, 0),
                            Qt.UserRole + 1
                        )
                        root.machineChosen(name)
                        popup.close()
                    }
                    event.accepted = true
                } else if (event.key === Qt.Key_Escape || event.key === Qt.Key_B) {
                    popup.close()
                    event.accepted = true
                }
            }
        }

        onOpened: machineList.forceActiveFocus()
    }
}
