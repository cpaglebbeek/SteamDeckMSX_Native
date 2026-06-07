import QtQuick
import QtQuick.Window
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import SteamDeckMSX

ApplicationWindow {
    id: root
    visible: true
    width: 1280
    height: 800
    color: Tokens.bgBase
    title: qsTr("SteamDeckMSX")

    MsxCore {
        id: msxCore
        Component.onCompleted: {
            if (OpenmsxLocator.found.length > 0) {
                openmsxPath = OpenmsxLocator.found
                dataPath = OpenmsxLocator.dataPath
                probeVersion()
            }
        }
        onErrorMessageChanged: {
            if (errorMessage.length > 0) {
                toast.show(qsTr("openMSX: ") + errorMessage, "error")
            }
        }
        onStateChanged: {
            // Bij eerste Running-state: laad machine-lijst dynamisch
            if (state === MsxCore.Running && !machines.loaded) {
                machines.refresh()
            }
        }
        onLogMessage: function(level, message) {
            if (level === "warning" || level === "stderr") {
                console.log("[openmsx " + level + "]", message)
            }
        }
    }

    MachineModel {
        id: machines
        core: msxCore
        Component.onCompleted: {
            if (currentMachine.length > 0) {
                msxCore.currentMachine = currentMachine
            }
        }
    }

    SaveStateModel {
        id: saves
        core: msxCore
        currentRomStem: {
            const r = msxCore.currentRom
            if (r.length === 0) return ""
            const fn = r.split("/").pop()
            const dot = fn.lastIndexOf(".")
            return dot > 0 ? fn.substring(0, dot) : fn
        }
    }

    CartridgeModel {
        id: cartridges
        onDownloadFinished: function(title, romPath) {
            toast.show(qsTr("ROM toegevoegd: ") + title, "info")
            urlImportDialog.busy = false
            urlImportDialog.close()
        }
        onDownloadFailed: function(reason) {
            toast.show(qsTr("ROM-download faalde: ") + reason, "error")
            urlImportDialog.busy = false
        }
        onDownloadProgress: function(received, total) {
            if (urlImportDialog.target === "rom") {
                urlImportDialog.progress = total > 0 ? received / total : 0
                urlImportDialog.progressLabel = (received / 1024).toFixed(0) + " / "
                    + (total > 0 ? (total / 1024).toFixed(0) : "?") + " KiB"
            }
        }
    }

    // v0.1.0-Xanadu: BIOS-bibliotheek (DD-007).
    BiosManager {
        id: bios
        onEntryAdded: function(id, fileName) {
            toast.show(qsTr("BIOS toegevoegd: ") + fileName, "info")
            urlImportDialog.busy = false
            urlImportDialog.close()
        }
        onAddFailed: function(reason) {
            toast.show(qsTr("BIOS-add faalde: ") + reason, "error")
            urlImportDialog.busy = false
        }
        onDownloadProgress: function(received, total) {
            if (urlImportDialog.target === "bios") {
                urlImportDialog.progress = total > 0 ? received / total : 0
                urlImportDialog.progressLabel = (received / 1024).toFixed(0) + " / "
                    + (total > 0 ? (total / 1024).toFixed(0) : "?") + " KiB"
            }
        }
    }

    // v0.1.0-Xanadu: BIOS-lokaal-import + gedeelde URL-dialog + BIOS-screen + slot-picker (DD-007/008/009).
    FileDialog {
        id: biosLocalPicker
        title: qsTr("Selecteer een BIOS-bestand")
        nameFilters: [
            qsTr("BIOS / ROM (*.rom *.sys *.ic *.bin)"),
            qsTr("Alle bestanden (*)")
        ]
        onAccepted: {
            const p = selectedFile.toString().replace("file://", "")
            bios.addFromLocal(p)
        }
    }

    UrlImportDialog {
        id: urlImportDialog
        onConfirmed: function(url, name, target) {
            urlImportDialog.busy = true
            urlImportDialog.progress = 0
            urlImportDialog.progressLabel = qsTr("Verbinden...")
            if (target === "bios") bios.addFromUrl(url, name)
            else                    cartridges.addFromUrl(url, name)
        }
    }

    BiosManagerScreen {
        id: biosScreen
        parent: Overlay.overlay
        biosModel: bios
        onAddFromUrlClicked: { urlImportDialog.target = "bios"; urlImportDialog.open() }
        onAddFromLocalClicked: biosLocalPicker.open()
        onRemoveBios: function(id) { bios.removeEntry(id) }
    }

    SlotPickerDialog {
        id: slotPicker
        runningState: msxCore.state === MsxCore.Running
        onSlotAChosen: {
            if (msxCore.state === MsxCore.Running) {
                msxCore.loadRomSlotA(slotPicker.romPath)
                toast.show(qsTr("Slot A → ") + slotPicker.romTitle, "info")
            } else {
                msxCore.start(slotPicker.romPath)
                toast.show(qsTr("Start: ") + slotPicker.romTitle, "info")
            }
        }
        onSlotBChosen: {
            msxCore.loadRomSlotB(slotPicker.romPath)
            toast.show(qsTr("Slot B → ") + slotPicker.romTitle, "info")
        }
    }

    FileDialog {
        id: romPicker
        title: qsTr("Selecteer een MSX-ROM, schijf of tape")
        nameFilters: [
            qsTr("MSX media (*.rom *.dsk *.cas *.zip)"),
            qsTr("ROM cartridges (*.rom)"),
            qsTr("Disk images (*.dsk)"),
            qsTr("Tape images (*.cas)"),
            qsTr("Alle bestanden (*)")
        ]
        onAccepted: {
            const path = selectedFile.toString().replace("file://", "")
            cartridges.addRom(path)
            msxCore.start(path)
            toast.show(qsTr("Laden: ") + path.split("/").pop(), "info")
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: Tokens.safeMargin
        color: "transparent"

        Column {
            anchors.fill: parent
            spacing: Tokens.space4

            // Header
            Item {
                width: parent.width
                height: 56

                Text {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("SteamDeckMSX")
                    color: Tokens.fgPrimary
                    font.family: Tokens.fontFamily
                    font.pixelSize: Tokens.fontSizeDisplay
                    font.weight: Font.Bold
                    font.letterSpacing: Tokens.fontSizeDisplay * 0.02
                }

                Row {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: Tokens.space4

                    // v0.1.0-Xanadu DD-010: hint-strip rechtsboven.
                    Text {
                        text: qsTr("I · BIOS    U · URL-ROM    S · Slot")
                        color: Tokens.fgSecondary
                        font.family: Tokens.fontFamilyMono
                        font.pixelSize: Tokens.fontSizeLabel
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: qsTr("openMSX: ") +
                              (msxCore.version.length > 0 ? msxCore.version : "─")
                        color: Tokens.fgSecondary
                        font.family: Tokens.fontFamilyMono
                        font.pixelSize: Tokens.fontSizeMono
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Rectangle {
                        width: 12; height: 12; radius: 6
                        anchors.verticalCenter: parent.verticalCenter
                        color: {
                            switch (msxCore.state) {
                                case MsxCore.Running:  return Tokens.accentPrimary
                                case MsxCore.Booting:  return Tokens.accentWarm
                                case MsxCore.Probed:   return Tokens.accentInfo
                                case MsxCore.Failed:   return Tokens.accentError
                                default:               return Tokens.fgDisabled
                            }
                        }
                    }
                    Text {
                        text: msxCore.stateLabel
                        color: Tokens.fgSecondary
                        font.family: Tokens.fontFamilyMono
                        font.pixelSize: Tokens.fontSizeMono
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            // Cartridge browser (with recent + sentinel)
            CartridgeBrowser {
                id: browser
                model: cartridges
                width: parent.width
                height: parent.height - 240
                focus: true
                onActivated: function(index, entry) {
                    if (entry.isSentinel) {
                        romPicker.open()
                    } else if (entry.romPath && entry.romPath.length > 0) {
                        msxCore.start(entry.romPath)
                        toast.show(qsTr("Start: ") + entry.title, "info")
                    }
                }
            }

            // Footer: stop + settings hint
            Row {
                width: parent.width
                spacing: Tokens.space4

                Rectangle {
                    width: 200; height: Tokens.minInteractive
                    color: msxCore.state === MsxCore.Running ? Tokens.accentError : Tokens.bgElevated
                    border.color: Tokens.borderSubtle
                    border.width: 1
                    radius: 4
                    visible: msxCore.state === MsxCore.Running || msxCore.state === MsxCore.Booting

                    Text {
                        anchors.centerIn: parent
                        text: qsTr("Y · Stop")
                        color: Tokens.fgPrimary
                        font.family: Tokens.fontFamily
                        font.pixelSize: Tokens.fontSizeBody
                        font.weight: Font.DemiBold
                    }
                }

                MachineSelector {
                    width: 360
                    model: machines
                    currentMachine: machines.currentMachine
                    onMachineChosen: function(name) {
                        machines.currentMachine = name
                        toast.show(qsTr("Machine: ") + name, "info")
                    }
                }

                SettingsRow {
                    width: parent.width - 200 - 360 - 2 * Tokens.space4
                    label: qsTr("openmsx")
                    value: OpenmsxLocator.found.length > 0
                        ? OpenmsxLocator.found.split("/").pop()
                        : qsTr("(niet gevonden)")
                }
            }

            // v0.1.0-Xanadu: slot-status onderin (DD-009).
            Row {
                width: parent.width
                spacing: Tokens.space4

                Text {
                    text: qsTr("Slot A: ") + (msxCore.slotARom.length > 0
                        ? msxCore.slotARom.split("/").pop()
                        : qsTr("(leeg)"))
                    color: msxCore.slotARom.length > 0 ? Tokens.fgPrimary : Tokens.fgDisabled
                    font.family: Tokens.fontFamilyMono
                    font.pixelSize: Tokens.fontSizeLabel
                    elide: Text.ElideMiddle
                    width: (parent.width - Tokens.space4) / 2
                }
                Text {
                    text: qsTr("Slot B: ") + (msxCore.slotBRom.length > 0
                        ? msxCore.slotBRom.split("/").pop()
                        : qsTr("(leeg)"))
                    color: msxCore.slotBRom.length > 0 ? Tokens.fgPrimary : Tokens.fgDisabled
                    font.family: Tokens.fontFamilyMono
                    font.pixelSize: Tokens.fontSizeLabel
                    elide: Text.ElideMiddle
                    width: (parent.width - Tokens.space4) / 2
                }
            }
        }
    }

    Toast {
        id: toast
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Tokens.space5
        anchors.horizontalCenter: parent.horizontalCenter
    }

    SaveStateOverlay {
        id: savesOverlay
        parent: Overlay.overlay
        model: saves
        currentRomStem: saves.currentRomStem
        onSlotActivated: function(slot, wasOccupied) {
            if (wasOccupied) {
                saves.loadFrom(slot)
                toast.show(qsTr("Load slot ") + slot, "info")
            } else {
                saves.saveTo(slot)
                toast.show(qsTr("Save slot ") + slot, "info")
            }
            savesOverlay.close()
        }
    }

    // v0.1.0-Xanadu DD-010: keyboard-shortcuts voor BIOS/URL/Slot.
    Shortcut {
        sequences: ["I"]
        onActivated: biosScreen.open()
    }
    Shortcut {
        sequences: ["U"]
        onActivated: {
            urlImportDialog.target = "rom"
            urlImportDialog.open()
        }
    }
    Shortcut {
        sequences: ["S"]
        onActivated: {
            // Open slot-picker voor huidige cartridge in browser.
            const idx = browser.currentIndex
            const m = cartridges
            if (idx < 0 || idx >= m.rowCount()) return
            const isSentinel = m.data(m.index(idx, 0), Qt.UserRole + 4)
            if (isSentinel) {
                toast.show(qsTr("Selecteer eerst een cartridge"), "warning")
                return
            }
            slotPicker.romPath  = m.data(m.index(idx, 0), Qt.UserRole + 2)
            slotPicker.romTitle = m.data(m.index(idx, 0), Qt.UserRole + 1)
            slotPicker.open()
        }
    }

    Shortcut { sequences: ["Escape", "B"]; onActivated: Qt.quit() }
    Shortcut {
        sequences: ["Y"]
        onActivated: {
            if (msxCore.state === MsxCore.Running ||
                msxCore.state === MsxCore.Booting) {
                msxCore.stop()
                toast.show(qsTr("Stopping openMSX…"), "warning")
            }
        }
    }
    Shortcut {
        sequences: ["X"]
        onActivated: {
            if (msxCore.state === MsxCore.Running) {
                savesOverlay.open()
            } else {
                toast.show(qsTr("Save-states alleen tijdens spel (X-toets)"), "warning")
            }
        }
    }
}
