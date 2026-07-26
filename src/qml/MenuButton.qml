import QtQuick
import QtQuick.Controls.Basic

// Knop die met muis, touch, toetsenbord én joystick te bedienen is.
// Bestaat omdat de galerij tot nu toe alleen sneltoetsen kende: op de Deck is
// er geen toetsenbord, dus alles wat je met een toets kon moet ook aanwijsbaar
// zijn. De focus-ring is daarbij niet cosmetisch — zonder zichtbare focus weet
// je met een joystick niet waar je bent.
Rectangle {
    id: root

    property string label: ""
    property string hint: ""          // sneltoets-aanduiding, mag leeg
    property bool primary: false
    property bool enabled: true
    signal clicked()

    implicitWidth: Math.max(Tokens.minInteractive * 2,
                            txt.implicitWidth + Tokens.space4 * 2)
    implicitHeight: Tokens.minInteractive
    radius: 6
    color: !root.enabled ? Tokens.bgBase
         : mouse.pressed ? Tokens.accentPrimary
         : root.activeFocus ? Tokens.bgElevated
         : root.primary ? Tokens.bgElevated : "transparent"
    border.color: root.activeFocus ? Tokens.accentPrimary
                : root.enabled ? Tokens.borderSubtle : Tokens.borderSubtle
    border.width: root.activeFocus ? Tokens.focusRingWidth : 1

    activeFocusOnTab: true
    Accessible.role: Accessible.Button
    Accessible.name: root.label

    Behavior on border.width { NumberAnimation { duration: Tokens.motionFast } }

    Row {
        id: txt
        anchors.centerIn: parent
        spacing: Tokens.space2

        Text {
            text: root.label
            color: root.enabled ? Tokens.fgPrimary : Tokens.fgDisabled
            font.family: Tokens.fontFamily
            font.pixelSize: Tokens.fontSizeBody
            font.weight: root.primary ? Font.DemiBold : Font.Normal
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            visible: root.hint.length > 0
            text: root.hint
            color: Tokens.fgSecondary
            font.family: Tokens.fontFamilyMono
            font.pixelSize: Tokens.fontSizeLabel
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        enabled: root.enabled
        cursorShape: Qt.PointingHandCursor
        onClicked: { root.forceActiveFocus(); root.clicked() }
    }

    // A op de controller komt binnen als Return/Enter; Space voor toetsenbord.
    Keys.onPressed: function(e) {
        if (!root.enabled) return
        if (e.key === Qt.Key_Return || e.key === Qt.Key_Enter || e.key === Qt.Key_Space) {
            root.clicked()
            e.accepted = true
        }
    }
}
