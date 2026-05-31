import QtQuick
import QtQuick.Controls.Basic
import SteamDeckMSX

ListView {
    id: list
    signal activated(int index)

    clip: true
    spacing: Tokens.space2
    boundsBehavior: Flickable.StopAtBounds

    keyNavigationEnabled: true
    keyNavigationWraps: false
    highlightMoveDuration: Tokens.motionFast
    highlightFollowsCurrentItem: true

    delegate: CartridgeCard {
        width: list.width
        title: model.title
        year: model.year
        publisher: model.publisher
        machine: model.machine
        focused: ListView.isCurrentItem
    }

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter ||
            event.key === Qt.Key_A || event.key === Qt.Key_Space) {
            list.activated(list.currentIndex)
            event.accepted = true
        }
    }
}
