pragma Singleton
import QtQuick

QtObject {
    // MSX-CRT-revival palette — zie DESIGN_TOKENS.md (single source of truth)
    readonly property color bgBase:      "#0A0E0F"
    readonly property color bgElevated:  "#1B2122"
    readonly property color bgOverlay:   Qt.rgba(0.04, 0.06, 0.06, 0.85)
    readonly property color fgPrimary:   "#E8F1E1"
    readonly property color fgSecondary: "#9BAFA1"
    readonly property color fgDisabled:  "#4A5651"
    readonly property color accentPrimary: "#39FF14"
    readonly property color accentWarm:    "#FFB000"
    readonly property color accentError:   "#FF3C28"
    readonly property color accentInfo:    "#3CCBFF"
    readonly property color borderSubtle:  "#2A3334"
    readonly property color borderStrong:  "#39FF14"

    // Spacing — 4pt grid
    readonly property int space1: 4
    readonly property int space2: 8
    readonly property int space3: 12
    readonly property int space4: 16
    readonly property int space5: 24
    readonly property int space6: 32
    readonly property int space7: 48
    readonly property int space8: 64

    // Typography (Noto Sans op Freedesktop runtime, fallback systeem)
    readonly property string fontFamily:     "Noto Sans"
    readonly property string fontFamilyMono: "Noto Sans Mono"
    readonly property int   fontSizeDisplay: 32
    readonly property int   fontSizeBody:    18
    readonly property int   fontSizeLabel:   14
    readonly property int   fontSizeMono:    16
    readonly property int   fontSizeOsd:     24

    // Geometry — Steam Deck 1280x800
    readonly property int safeMargin:        24
    readonly property int minInteractive:    64
    readonly property int listRowHeight:     96
    readonly property int focusRingWidth:    4

    // Motion
    readonly property int motionFast:  120
    readonly property int motionBase:  220
    readonly property int motionSlow:  480

    // Icon-resource-paths (auto-generated qrc-prefix per qt_add_qml_module).
    // Modulair: alle icons via currentColor → Qt6 Image laadt SVG met sourceSize.
    readonly property string iconDpadUp:    "qrc:/qt/qml/SteamDeckMSX/assets/icons/dpad/up.svg"
    readonly property string iconDpadDown:  "qrc:/qt/qml/SteamDeckMSX/assets/icons/dpad/down.svg"
    readonly property string iconDpadLeft:  "qrc:/qt/qml/SteamDeckMSX/assets/icons/dpad/left.svg"
    readonly property string iconDpadRight: "qrc:/qt/qml/SteamDeckMSX/assets/icons/dpad/right.svg"
    readonly property string iconBtnA:      "qrc:/qt/qml/SteamDeckMSX/assets/icons/btn/a.svg"
    readonly property string iconBtnB:      "qrc:/qt/qml/SteamDeckMSX/assets/icons/btn/b.svg"
    readonly property string iconBtnX:      "qrc:/qt/qml/SteamDeckMSX/assets/icons/btn/x.svg"
    readonly property string iconBtnY:      "qrc:/qt/qml/SteamDeckMSX/assets/icons/btn/y.svg"
}
