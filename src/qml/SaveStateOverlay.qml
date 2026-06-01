import QtQuick
import QtQuick.Controls.Basic
import SteamDeckMSX

// Modaal overlay voor save-state slot-grid. X-knop opent, A = save/load,
// B/Esc = close, Y = clear slot. 5×2 grid (10 slots, MSX-traditie).
Popup {
    id: root
    property var model: null
    property string currentRomStem: ""
    signal slotActivated(int slot, bool wasOccupied)

    modal: true
    focus: true
    width: parent ? parent.width - 2 * Tokens.safeMargin : 800
    height: parent ? parent.height - 2 * Tokens.safeMargin : 600
    anchors.centerIn: parent ? Overlay.overlay : null
    padding: Tokens.space5

    background: Rectangle {
        color: Tokens.bgOverlay
        border.color: Tokens.accentPrimary
        border.width: 2
        radius: 12
    }

    contentItem: Item {
        Column {
            anchors.fill: parent
            spacing: Tokens.space4

            Row {
                spacing: Tokens.space4
                width: parent.width

                Text {
                    text: qsTr("Save-state slots")
                    color: Tokens.fgPrimary
                    font.family: Tokens.fontFamily
                    font.pixelSize: Tokens.fontSizeDisplay
                    font.weight: Font.Bold
                }
                Item { width: parent.width - 500; height: 1 }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.currentRomStem.length > 0
                        ? qsTr("ROM: ") + root.currentRomStem
                        : qsTr("(geen ROM geladen)")
                    color: Tokens.fgSecondary
                    font.family: Tokens.fontFamilyMono
                    font.pixelSize: Tokens.fontSizeMono
                }
            }

            GridView {
                id: grid
                width: parent.width
                height: parent.height - 80 - 56
                cellWidth: Math.floor(width / 5)
                cellHeight: Math.floor(height / 2)
                model: root.model
                clip: true
                focus: true
                keyNavigationEnabled: true
                highlightMoveDuration: Tokens.motionFast

                delegate: Item {
                    width: grid.cellWidth
                    height: grid.cellHeight
                    SaveStateCard {
                        anchors.fill: parent
                        anchors.margins: Tokens.space2
                        slotNum: model.slot
                        occupied: model.occupied
                        label: model.label
                        focused: GridView.isCurrentItem
                    }
                }

                Keys.onPressed: function(event) {
                    if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter ||
                        event.key === Qt.Key_A || event.key === Qt.Key_Space) {
                        const slot = grid.currentIndex
                        const occupied = root.model.data(
                            root.model.index(slot, 0),
                            Qt.UserRole + 5  // OccupiedRole
                        )
                        root.slotActivated(slot, occupied)
                        event.accepted = true
                    } else if (event.key === Qt.Key_Y) {
                        // clear
                        if (root.model) {
                            root.model.clear(grid.currentIndex)
                        }
                        event.accepted = true
                    } else if (event.key === Qt.Key_Escape || event.key === Qt.Key_B) {
                        root.close()
                        event.accepted = true
                    }
                }
            }

            // Footer hints
            Row {
                spacing: Tokens.space5
                width: parent.width

                Row {
                    spacing: Tokens.space2
                    anchors.verticalCenter: parent.verticalCenter
                    Image { source: Tokens.iconBtnA; width: 24; height: 24; sourceSize: Qt.size(24,24) }
                    Text {
                        text: qsTr("Save / Load")
                        color: Tokens.fgSecondary
                        font.family: Tokens.fontFamily
                        font.pixelSize: Tokens.fontSizeLabel
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                Row {
                    spacing: Tokens.space2
                    anchors.verticalCenter: parent.verticalCenter
                    Image { source: Tokens.iconBtnY; width: 24; height: 24; sourceSize: Qt.size(24,24) }
                    Text {
                        text: qsTr("Clear slot")
                        color: Tokens.fgSecondary
                        font.family: Tokens.fontFamily
                        font.pixelSize: Tokens.fontSizeLabel
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                Row {
                    spacing: Tokens.space2
                    anchors.verticalCenter: parent.verticalCenter
                    Image { source: Tokens.iconBtnB; width: 24; height: 24; sourceSize: Qt.size(24,24) }
                    Text {
                        text: qsTr("Sluit")
                        color: Tokens.fgSecondary
                        font.family: Tokens.fontFamily
                        font.pixelSize: Tokens.fontSizeLabel
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
        }
    }
}
