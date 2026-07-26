import QtQuick

// Eigen schermtoetsenbord. Nodig omdat er op de Deck geen toetsenbord is en de
// twee voor de hand liggende alternatieven allebei afvallen: QtVirtualKeyboard
// zit niet in de KDE-runtime van de Flatpak, en het toetsenbord van Steam komt
// niet betrouwbaar over een niet-Steam-venster heen.
//
// v0.4.1: QWERTY in plaats van alfabetisch — letters zoek je op spiergeheugen.
// v0.5.0: shift + symboolrij, zodat ook een URL te typen is (BIOS-import).
Item {
    id: kb

    property string text: ""
    // Zonder shift komen er kleine letters — zoeken is toch hoofdletter-
    // ongevoelig en URLs zijn in de praktijk lowercase.
    property bool shift: false
    property string doneLabel: qsTr("zoek")
    signal accepted()

    readonly property var rows: [
        "1234567890".split(""),
        "QWERTYUIOP".split(""),
        "ASDFGHJKL".split(""),
        "ZXCVBNM".split(""),
        [":", "/", ".", "-", "_", "=", "?", "&", "~", "%"]
    ]

    implicitHeight: grid.implicitHeight + Tokens.space4

    Column {
        id: grid
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: Tokens.space2

        Repeater {
            model: kb.rows
            Row {
                spacing: Tokens.space2
                anchors.horizontalCenter: parent.horizontalCenter
                property int rowIndex: index
                Repeater {
                    model: modelData
                    MenuButton {
                        readonly property bool isLetter: modelData >= "A" && modelData <= "Z"
                        implicitWidth: Tokens.minInteractive
                        label: isLetter && !kb.shift ? modelData.toLowerCase() : modelData
                        onClicked: kb.text += (isLetter && !kb.shift)
                            ? modelData.toLowerCase() : modelData
                    }
                }
            }
        }

        Row {
            spacing: Tokens.space2
            anchors.horizontalCenter: parent.horizontalCenter
            MenuButton {
                implicitWidth: Tokens.minInteractive * 2
                label: qsTr("⇧ ABC")
                primary: kb.shift
                onClicked: kb.shift = !kb.shift
            }
            MenuButton {
                implicitWidth: Tokens.minInteractive * 3
                label: qsTr("spatie")
                onClicked: kb.text += " "
            }
            MenuButton {
                implicitWidth: Tokens.minInteractive * 2
                label: qsTr("wis")
                onClicked: kb.text = kb.text.slice(0, -1)
                // Ingedrukt houden wist alles: bij een typefout is letter voor
                // letter terug op een handheld onnodig traag.
                Timer {
                    id: clearAll
                    interval: 600
                    onTriggered: kb.text = ""
                }
            }
            MenuButton {
                implicitWidth: Tokens.minInteractive * 3
                label: kb.doneLabel
                primary: true
                onClicked: kb.accepted()
            }
        }
    }
}
