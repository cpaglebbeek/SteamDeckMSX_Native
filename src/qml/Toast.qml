import QtQuick
import SteamDeckMSX

Rectangle {
    id: toast
    property string text: ""
    property string kind: "info"  // info | warning | error
    property int duration: 3000

    width: Math.max(420, t.contentWidth + Tokens.space6)
    height: Tokens.minInteractive
    radius: 8
    color: Tokens.bgOverlay
    border.width: 2
    border.color: {
        switch (kind) {
            case "warning": return Tokens.accentWarm
            case "error":   return Tokens.accentError
            default:        return Tokens.accentInfo
        }
    }
    opacity: 0
    visible: opacity > 0

    Text {
        id: t
        anchors.centerIn: parent
        text: toast.text
        color: Tokens.fgPrimary
        font.family: Tokens.fontFamily
        font.pixelSize: Tokens.fontSizeBody
        elide: Text.ElideRight
    }

    Timer {
        id: hideTimer
        interval: toast.duration
        onTriggered: toast.opacity = 0
    }

    function show(msg, k) {
        toast.text = msg
        if (k) toast.kind = k
        toast.opacity = 1
        hideTimer.restart()
    }

    Behavior on opacity { NumberAnimation { duration: Tokens.motionBase } }
}
