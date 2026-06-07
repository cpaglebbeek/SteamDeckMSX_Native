import QtQuick
import QtQuick.Controls.Basic
import SteamDeckMSX

ListView {
    id: list
    signal activated(int index, var entry)

    clip: true
    spacing: Tokens.space2
    boundsBehavior: Flickable.StopAtBounds

    keyNavigationEnabled: true
    keyNavigationWraps: false
    highlightMoveDuration: Tokens.motionFast
    highlightFollowsCurrentItem: true

    delegate: Loader {
        width: list.width
        sourceComponent: model.isSentinel ? addRomComponent : cartridgeComponent

        property var modelData: model

        Component {
            id: cartridgeComponent
            CartridgeCard {
                width: list.width
                title: modelData.title
                machine: modelData.machine
                publisher: ""   // v0.0.5: derive from ROM metadata
                year: ""        // v0.0.5
                focused: ListView.isCurrentItem || (parent && parent.ListView.isCurrentItem)
            }
        }
        Component {
            id: addRomComponent
            AddRomCard {
                width: list.width
                focused: ListView.isCurrentItem || (parent && parent.ListView.isCurrentItem)
            }
        }
    }

    // v0.0.8-Snatcher: L1/R1 = page-up/page-down (5 items per keer) voor snelle
    // navigatie in lange ROM-lijsten. Steam Input mapt L1/R1 op PageUp/PageDown
    // via launcher-preset (v0.0.9 buiten Gaming Mode; voor nu KB-events).
    readonly property int pageJump: 5

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter ||
            event.key === Qt.Key_A || event.key === Qt.Key_Space) {
            const m = list.model
            const idx = list.currentIndex
            const entry = {
                title:      m.data(m.index(idx, 0), Qt.UserRole + 1),
                romPath:    m.data(m.index(idx, 0), Qt.UserRole + 2),
                machine:    m.data(m.index(idx, 0), Qt.UserRole + 3),
                isSentinel: m.data(m.index(idx, 0), Qt.UserRole + 4),
            }
            list.activated(idx, entry)
            event.accepted = true
        } else if (event.key === Qt.Key_PageUp) {
            list.currentIndex = Math.max(0, list.currentIndex - list.pageJump)
            event.accepted = true
        } else if (event.key === Qt.Key_PageDown) {
            list.currentIndex = Math.min(list.count - 1, list.currentIndex + list.pageJump)
            event.accepted = true
        }
    }
}
