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

    function setGalleryVisible(on) {
        if (on === root.visible)
            return
        if (on) {
            root.show()
            root.raise()
            root.requestActivate()
        } else {
            root.hide()
        }
    }

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
            // BUG-022: de emulator opent een eigen fullscreen-venster. Twee
            // vensters die om de voorgrond vechten leveren op de Deck een
            // zwart scherm op, dus stapt de galerij opzij zolang er gespeeld
            // wordt en komt terug zodra de emulator weg is.
            // Uitzondering: staat het pauzemenu open, dan moet de galerij juist
            // zichtbaar blijven — daar staat het menu immers op.
            root.setGalleryVisible(emuMenu.opened ||
                                   !(fullscreen &&
                                     (state === MsxCore.Booting ||
                                      state === MsxCore.Running)))
            if (state !== MsxCore.Running && emuMenu.opened)
                emuMenu.close()
        }
        onMenuRequested: {
            // Pauzeren vóór het tonen: anders speelt het spel door achter het
            // menu en verlies je levens terwijl je kiest.
            msxCore.setPaused(true)
            root.setGalleryVisible(true)
            emuMenu.gameTitle = msxCore.currentRom.length > 0
                ? msxCore.currentRom.split("/").pop() : ""
            emuMenu.open()
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
            // Opslagmap als scanroot registreren: zonder dat blijft een net
            // opgehaald spel buiten de galerij omdat er nooit in die map
            // gekeken wordt.
            library.addScanRoot(romPath.substring(0, romPath.lastIndexOf("/")))
            library.rescan()
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

    // v0.2.0-TreasureOfUsas: software-database voor SHA-1 → machine lookup.
    SoftwareDb {
        id: softwaredb
        Component.onCompleted: loadBootstrapData()
    }

    // v0.3.0-MazeOfGalious: de bibliotheek achter de galerij. Scant bij elke
    // start opnieuw — een ROM die je er net in zette hoort er meteen te staan,
    // zonder dat je iets hoeft te importeren.
    RomLibrary {
        id: library
        Component.onCompleted: rescan()
        onScanFinished: function(total, added) {
            if (total === 0) {
                toast.show(qsTr("Geen spellen gevonden — sleep bestanden hierheen"), "warning")
            } else if (added > 0) {
                toast.show(qsTr("Bibliotheek: ") + total + qsTr(" spellen (")
                           + added + qsTr(" nieuw)"), "info")
            }
            // Ontbrekende tegels op de achtergrond aanvullen.
            thumbs.enqueueAll(library.entriesWithoutThumbnail())
        }
    }

    // v0.3.0-MazeOfGalious: screenshots als tegelbeeld. Draait headless
    // (SDL offscreen) zodat er tijdens het bladeren geen venster opflitst.
    ThumbnailGenerator {
        id: thumbs
        openmsxPath: OpenmsxLocator.found
        dataPath: OpenmsxLocator.dataPath
        onThumbnailReady: function(sha1, thumbPath) {
            library.setThumbnail(sha1, thumbPath)
        }
        onQueueDrained: {
            if (generated > 0) {
                toast.show(qsTr("Tegels bijgewerkt: ") + generated, "info")
            }
        }
    }

    // v0.5.0: externe index om in te zoeken. De bron publiceert zijn inhoud als
    // één tekstbestand; de HTML-lijst is JS-gehydrateerd en levert bij een fetch
    // niets op. Index wordt gecacht en offline doorzocht, zodat typen direct
    // filtert in plaats van per letter op het netwerk te wachten.
    OnlineIndex {
        id: online
        indexUrl: "https://download.file-hunter.com/allfiles.txt"
        baseUrl: "https://download.file-hunter.com/"
        onFailed: function(reason) {
            toast.show(qsTr("Lijst ophalen mislukt: ") + reason, "error")
        }
    }

    OnlineBrowser {
        id: onlineBrowser
        index: online
        onDownloadRequested: function(url, name) {
            // Downloaden gebruikt dezelfde weg als de bestaande URL-import:
            // https-only, groottegrens en atomic write zitten daar al in.
            cartridges.addFromUrl(url, name)
            toast.show(qsTr("Ophalen: ") + name, "info")
            close()
        }
    }

    // v0.2.0-TreasureOfUsas: ZIP-extract voor BIOS-sets.
    BiosZipExtractor {
        id: biosZipExtractor
        onFileExtracted: function(fileName) {
            // Iedere geslaagde file landt in bios-storage; registreer via BiosManager.
            const dest = bios.storageDir() + "/" + fileName
            bios.addFromLocal(dest)
        }
        onSkipped: function(fileName, reason) {
            toast.show(qsTr("ZIP-skip: ") + fileName + qsTr(" (") + reason + qsTr(")"), "warning")
        }
        onParseError: function(reason) {
            toast.show(qsTr("ZIP-fout: ") + reason, "error")
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
        onConfirmed: function(url, name, target, user, password) {
            urlImportDialog.busy = true
            urlImportDialog.progress = 0
            urlImportDialog.progressLabel = qsTr("Verbinden...")
            // Inloggegevens gelden alleen voor deze ene import; ze worden
            // nergens bewaard.
            cartridges.setDownloadCredentials(user, password)
            if (target === "bios") bios.addFromUrl(url, name)
            else                    cartridges.addFromUrl(url, name)
        }
    }

    // v0.2.0: ZIP-picker.
    FileDialog {
        id: biosZipPicker
        title: qsTr("Selecteer een BIOS-set ZIP")
        nameFilters: [ qsTr("ZIP-archief (*.zip)"), qsTr("Alle bestanden (*)") ]
        onAccepted: {
            const p = selectedFile.toString().replace("file://", "")
            const n = biosZipExtractor.extractTo(p, bios.storageDir())
            toast.show(qsTr("ZIP-extract: ") + n + qsTr(" BIOS-files toegevoegd"), "info")
        }
    }

    BiosManagerScreen {
        id: biosScreen
        parent: Overlay.overlay
        biosModel: bios
        onAddFromUrlClicked: { urlImportDialog.target = "bios"; urlImportDialog.open() }
        onAddFromLocalClicked: biosLocalPicker.open()
        onAddFromZipClicked: biosZipPicker.open()        // v0.2.0
        onRemoveBios: function(id) { bios.removeEntry(id) }
        onFilesDropped: function(urls) {                  // v0.2.0
            for (let i = 0; i < urls.length; ++i) {
                const u = urls[i].toString()
                const p = u.replace("file://", "")
                if (p.toLowerCase().endsWith(".zip")) {
                    biosZipExtractor.extractTo(p, bios.storageDir())
                } else {
                    bios.addFromLocal(p)
                }
            }
        }
    }

    LaunchPanel {
        id: launchPanel
        parent: Overlay.overlay
        romLibrary: library
        onStartRequested: function(slotA, slotB) {
            msxCore.start(slotA)
            if (slotB && slotB.length > 0) {
                // Zelfde volgorde als de bestaande slot-B-route: cartb direct
                // na start werkt, wachten op Running is niet nodig.
                msxCore.loadRomSlotB(slotB)
                toast.show(qsTr("Start + slot B: ") + launchPanel.slotBTitle, "info")
            } else {
                toast.show(qsTr("Start: ") + launchPanel.gameTitle, "info")
            }
        }
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

    // v0.3.1: eigen map aanwijzen. Vangnet voor collecties die buiten de
    // standaard scanmappen staan (externe schijf, ongebruikelijke locatie) —
    // zonder dit is een lege galerij een doodlopende weg voor de gebruiker.
    FolderDialog {
        id: romFolderPicker
        title: qsTr("Kies een map met MSX-spellen")
        onAccepted: {
            const dir = selectedFolder.toString().replace("file://", "")
            library.addScanRoot(dir)
            toast.show(qsTr("Map toegevoegd: ") + dir.split("/").pop(), "info")
            library.rescan()
        }
    }

    // Tweede cartridge erbij steken. Apart van romPicker omdat die het spel
    // start of slot A vervangt; hier hoort de eerste cartridge te blijven
    // zitten — dat is het hele punt van een tweede slot (DD-009).
    FileDialog {
        id: slotBPicker
        title: qsTr("Tweede cartridge kiezen (slot B)")
        nameFilters: [
            qsTr("ROM cartridges (*.rom)"),
            qsTr("MSX media (*.rom *.zip)"),
            qsTr("Alle bestanden (*)")
        ]
        onAccepted: {
            const p = selectedFile.toString().replace("file://", "")
            cartridges.addRom(p)
            // Slot B kan alleen in een draaiende machine; anders start de
            // emulator eerst met slot A leeg en valt er niets te combineren.
            if (msxCore.state !== MsxCore.Running) {
                msxCore.start("")
                toast.show(qsTr("Emulator starten voor slot B…"), "info")
            }
            msxCore.loadRomSlotB(p)
            toast.show(qsTr("Slot B → ") + p.split("/").pop(), "info")
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
            const lower = path.toLowerCase()
            // v0.2.0-TreasureOfUsas: route per media-type.
            if (lower.endsWith(".dsk")) {
                cartridges.addRom(path)
                if (msxCore.state !== MsxCore.Running) msxCore.start("")
                msxCore.loadDsk(path, 0)
                toast.show(qsTr("Floppy: ") + path.split("/").pop(), "info")
            } else if (lower.endsWith(".cas")) {
                cartridges.addRom(path)
                if (msxCore.state !== MsxCore.Running) msxCore.start("")
                msxCore.loadCas(path)
                toast.show(qsTr("Cassette: ") + path.split("/").pop(), "info")
            } else {
                cartridges.addRom(path)
                msxCore.start(path)
                toast.show(qsTr("Laden: ") + path.split("/").pop(), "info")
            }
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

                    // v0.4.0: was een tekstuele hint-strip. Op de Deck is er
                    // geen toetsenbord, dus de twee acties die je tijdens het
                    // spelen nodig hebt zijn nu echte knoppen — aanwijsbaar met
                    // touch én bereikbaar met de rechter joystick.
                    MenuButton {
                        id: slotBButton
                        anchors.verticalCenter: parent.verticalCenter
                        label: qsTr("Tweede cartridge")
                        hint: qsTr("S")
                        onClicked: slotBPicker.open()
                        KeyNavigation.right: onlineButton
                    }

                    MenuButton {
                        id: onlineButton
                        anchors.verticalCenter: parent.verticalCenter
                        label: qsTr("Zoeken")
                        onClicked: onlineBrowser.open()
                        KeyNavigation.left: slotBButton
                        KeyNavigation.right: openButton
                    }

                    MenuButton {
                        id: openButton
                        anchors.verticalCenter: parent.verticalCenter
                        label: qsTr("Openen")
                        hint: qsTr("O")
                        onClicked: romPicker.open()
                        KeyNavigation.left: onlineButton
                        KeyNavigation.right: biosButton
                    }

                    // v0.5.0: BIOS-beheer zat alleen achter sneltoets I — op de
                    // Deck onbereikbaar. Nu een echte knop, net als Zoeken.
                    MenuButton {
                        id: biosButton
                        anchors.verticalCenter: parent.verticalCenter
                        label: qsTr("BIOS")
                        hint: qsTr("I")
                        onClicked: biosScreen.open()
                        KeyNavigation.left: openButton
                    }

                    Text {
                        text: qsTr("M · map    R · scan    I · BIOS    U · URL")
                        color: Tokens.fgSecondary
                        font.family: Tokens.fontFamilyMono
                        font.pixelSize: Tokens.fontSizeLabel
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    // Voortgang van scan/tegels — anders lijkt het alsof er
                    // niets gebeurt terwijl de emulator screenshots maakt.
                    Text {
                        visible: library.scanning || thumbs.busy
                        text: library.scanning
                              ? qsTr("scannen: ") + library.scannedFiles
                              : qsTr("tegels: nog ") + thumbs.pending
                        color: Tokens.accentWarm
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

            // v0.3.0-MazeOfGalious: galerij van álle gevonden spellen.
            GameGrid {
                id: browser
                model: library
                thumbGen: thumbs
                width: parent.width
                height: parent.height - 240
                focus: true
                searchedPaths: library.scanRoots
                onRescanRequested: {
                    toast.show(qsTr("Opnieuw scannen…"), "info")
                    library.rescan()
                }
                onAddFolderRequested: romFolderPicker.open()
                onActivated: function(index, entry) {
                    if (!entry.romPath || entry.romPath.length === 0) return
                    // v0.3.2: start op de machine die bij dít spel hoort. Een
                    // MSX1-titel op een MSX2+-machine start vaak niet of geeft
                    // een afwijkend beeld; de keuze komt uit RomTypeDetector.
                    if (entry.machineId && entry.machineId !== msxCore.currentMachine) {
                        msxCore.currentMachine = entry.machineId
                    }
                    // v0.2.0-TreasureOfUsas: media-type-route.
                    const lower = entry.romPath.toLowerCase()
                    if (lower.endsWith(".dsk")) {
                        // Floppy → vereist running emulator. Start eerst (C-BIOS), dan diska.
                        if (msxCore.state !== MsxCore.Running) {
                            msxCore.start("")  // boot zonder cart
                        }
                        msxCore.loadDsk(entry.romPath, 0)
                        toast.show(qsTr("Floppy A: ") + entry.title, "info")
                    } else if (lower.endsWith(".cas")) {
                        if (msxCore.state !== MsxCore.Running) msxCore.start("")
                        msxCore.loadCas(entry.romPath)
                        toast.show(qsTr("Cassette: ") + entry.title, "info")
                    } else {
                        // v0.5.0-Goonies: niet meer meteen starten (was DD-009)
                        // maar eerst het startpaneel — slot A + optioneel B.
                        launchPanel.openFor(entry.title, entry.romPath, entry.machine)
                    }
                }
                // v0.2.0: drag-and-drop op browser.
                onFilesDropped: function(urls) {
                    for (let i = 0; i < urls.length; ++i) {
                        const p = urls[i].toString().replace("file://", "")
                        cartridges.addFromLocal(p, false)
                    }
                    toast.show(qsTr("Toegevoegd: ") + urls.length + qsTr(" bestand(en)"), "info")
                    // Gesleepte bestanden landen in storage; opnieuw scannen
                    // zet ze meteen als tegel in de galerij.
                    library.rescan()
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
                    // BUG-022: tijdens het spelen is dit venster verborgen, dus
                    // moet de uitgang vóóraf te lezen zijn.
                    visible: true

                    Text {
                        anchors.centerIn: parent
                        text: msxCore.state === MsxCore.Running || msxCore.state === MsxCore.Booting
                            ? qsTr("Y · Stop")
                            : qsTr("F12 · terug uit spel")
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

    // Pauzemenu bovenop een lopend spel (menu-toets in de emulator).
    EmulatorMenu {
        id: emuMenu
        onResumeRequested: {
            close()
            msxCore.setPaused(false)
            // Galerij weer opzij, anders staat hij vóór het spel — precies de
            // situatie die BUG-022 veroorzaakte.
            if (msxCore.fullscreen && msxCore.state === MsxCore.Running)
                root.setGalleryVisible(false)
        }
        onGalleryRequested: {
            close()
            // Eerst unpause: een gepauzeerde emulator reageert niet op `quit`.
            msxCore.setPaused(false)
            msxCore.stop()
            root.setGalleryVisible(true)
            toast.show(qsTr("Terug in de galerij"), "info")
        }
        onQuitRequested: {
            close()
            msxCore.setPaused(false)
            msxCore.stop()
            Qt.quit()
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
            // Slot-picker voor de tegel die nu focus heeft. entryAt() levert
            // benoemde velden, dus geen rol-nummers meer die stil verschuiven
            // zodra het model een rol krijgt (les uit de oude browser-code).
            const idx = browser.currentIndex
            if (idx < 0 || idx >= library.count) {
                toast.show(qsTr("Selecteer eerst een spel"), "warning")
                return
            }
            const entry = library.entryAt(idx)
            if (!entry.romPath || entry.mediaType !== "rom") {
                toast.show(qsTr("Slots gelden alleen voor cartridges"), "warning")
                return
            }
            slotPicker.romPath  = entry.romPath
            slotPicker.romTitle = entry.title
            slotPicker.open()
        }
    }

    // v0.3.0-MazeOfGalious: opnieuw scannen + bestand toevoegen.
    Shortcut {
        sequences: ["R"]
        onActivated: {
            toast.show(qsTr("Opnieuw scannen…"), "info")
            library.rescan()
        }
    }
    Shortcut {
        sequences: ["O"]
        onActivated: romPicker.open()
    }
    Shortcut {
        sequences: ["M"]
        onActivated: romFolderPicker.open()
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
