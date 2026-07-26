import QtQuick

// Eigen schermtoetsenbord. Nodig omdat er op de Deck geen toetsenbord is en de
// twee voor de hand liggende alternatieven allebei afvallen: QtVirtualKeyboard
// zit niet in de KDE-runtime van de Flatpak, en het toetsenbord van Steam komt
// niet betrouwbaar over een niet-Steam-venster heen.
//
// Bewust klein gehouden: letters, cijfers, spatie en wissen. Voor het zoeken
// naar een speltitel is dat genoeg, en elke extra rij maakt de weg naar een
// letter langer.
Item {
    id: kb

    property string text: ""
    signal accepted()

    readonly property var rows: [
        "ABCDEFGHIJ".split(""),
        "KLMNOPQRST".split(""),
        "UVWXYZ0123".split(""),
        "456789".split("")
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
                property int rowIndex: index
                Repeater {
                    model: modelData
                    MenuButton {
                        implicitWidth: Tokens.minInteractive
                        label: modelData
                        onClicked: kb.text += modelData
                    }
                }
            }
        }

        Row {
            spacing: Tokens.space2
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
                label: qsTr("zoek")
                primary: true
                onClicked: kb.accepted()
            }
        }
    }
}
