# Design Decisions — v0.2.0-TreasureOfUsas

> User-instructie: "doe overal jouw beste keuze waar je vragen hebt. sla wel
> overal op welke keuzes je had en welke je hebt gekozen zodat ik het later
> mogelijk nog kan veranderen als er iets niet werkt."

DD-101 t/m DD-114 in deze release. Format identiek aan `DESIGN_DECISIONS_v0.1.0.md`.

---

## DD-101 — SoftwareDb XML-parse: stream vs DOM

**Probleem**: openMSX `softwaredb.xml` is ~10 MB met >10000 entries. DOM verdubbelt memory.

**Gekozen: `QXmlStreamReader` (stream-based).**

**Opties:**
- A) `QXmlStreamReader` (gekozen) — incremental, low-mem
- B) `QDomDocument` — makkelijker random-access maar 30+ MB resident
- C) `QSaxReader` (deprecated)

**Omkeerbaarheid**: middel.
- File: `src/SoftwareDb.cc::loadFromXmlFile`
- DOM-variant: ~50 regels herschrijven
- **Tijd: ~45 min**

---

## DD-102 — SoftwareDb XML-mapping: één software-blok → N hash-entries

**Probleem**: openMSX `<software>` kan meerdere `<dump>`/`<rom>`/`<megarom>` met meerdere `<hash>` bevatten (dump-varianten / regio-versies). Hoe mappen naar `QHash<sha1, machine>`?

**Gekozen: alle SHA-1's krijgen dezelfde title + machine van de parent `<software>`.**

Reden: gebruiker hoeft regio niet te weten — software-titel = software-titel.

**Opties:**
- A) Allemaal zelfde meta (gekozen)
- B) Per-dump-variant aparte entry met sub-title (`"Bubble Bobble (EU)"`)
- C) Voorkeur "main" dump (eerste in XML) — overige negeren

**Omkeerbaarheid**: makkelijk.
- File: `src/SoftwareDb.cc::loadFromXmlFile` flush-loop
- **Tijd: ~15 min**

---

## DD-103 — SoftwareDb algo-filter

**Probleem**: openMSX-XML bevat ook `md5` + `crc32` hashes. Welke accepteren?

**Gekozen: `algo="sha1"` only (case-insensitive).**

Reden: SHA-1 is wat `RomTypeDetector` berekent. MD5/CRC32 zouden tweede pad vereisen.

**Omkeerbaarheid**: hoog.
- File: `src/SoftwareDb.cc` + uitbreiden `RomTypeDetector` met `md5Hex` / `crc32Hex`
- **Tijd: ~30 min**

---

## DD-104 — SoftwareDb `changed()` signal: bulk vs per-entry

**Probleem**: bij XML-load met 10000 entries, emit per-add → 10000 signal-emits → QML binding-storm.

**Gekozen: één `changed()` na bulk-load; per-entry emit alleen bij `addEntry()` (test/bootstrap pad).**

**Omkeerbaarheid**: zeer makkelijk.
- File: `src/SoftwareDb.cc::loadFromXmlFile` einde
- **Tijd: < 5 min**

---

## DD-105 — Bootstrap-data hardcoded vs file

**Probleem**: hoe smoke-test SoftwareDb zonder echte `softwaredb.xml`?

**Gekozen: `loadBootstrapData()` voegt 5 fictieve maar plausibele entries toe (sample-SHA1's).**

Reden: test moet hash-roundtrip valideren, niet de hash-correctheid zelf.

**Omkeerbaarheid**: makkelijk.
- File: `src/SoftwareDb.cc::loadBootstrapData`
- **Tijd: < 5 min** (entries vervangen)

---

## DD-106 — ZIP-extract: QuaZip vs Qt-private `QZipReader`

**Probleem**: Qt6 bundelt geen QuaZip. Externe dep of private API?

**Gekozen: `<QtGui/private/qzipreader_p.h>` (private API).**

Reden: stabiel sinds Qt5 (gebruikt in Qt's eigen ODF-writer), geen extra dep, geen CMake-vendor-hell. Trade-off: geen ABI-garantie tussen minor-versies. Acceptabel voor enkele-doelplatform-app (Steam Deck Flatpak met gepinde Qt-versie).

**Opties:**
- A) Qt-private (gekozen)
- B) QuaZip via vcpkg/system — werkt overal maar deploy-complexer
- C) minizip C-lib + eigen Qt-wrapper — laagste-niveau, meest werk
- D) Wachten tot Qt6 publieke ZIP API krijgt — onbepaalde wachttijd

**Omkeerbaarheid**: middel.
- Files: `src/BiosZipExtractor.{h,cc}` + `src/CMakeLists.txt` (vervang `Qt6::GuiPrivate` met `QuaZip::QuaZip`)
- **Tijd: ~2 uur** (incl. CMake-FetchContent of vcpkg-setup)

---

## DD-107 — ZIP anti-zip-bomb caps

**Probleem**: een ZIP met 1 miljoen 1-byte files zou alle inodes uitputten.

**Gekozen: dubbele cap:**
- Per file: 1 MiB (zelfde als `BiosManager::kBiosMaxBytes`)
- Per ZIP: 64 files total
- Header-size check + post-decompress-size check (beide)

**Omkeerbaarheid**: zeer makkelijk.
- File: `src/BiosZipExtractor.h` `kPerFileMaxBytes` + `kMaxFilesPerZip` constanten
- **Tijd: < 2 min**

---

## DD-108 — ZIP-extract accept-extensies: `.rom/.sys/.bin`

**Probleem**: BIOS-ZIPs bevatten vaak ook `.xml` (machine-config), `.txt` (readme), `.md5sum` (checksums). Allemaal accepteren?

**Gekozen: alleen `.rom`, `.sys`, `.bin` (case-insensitive).**

Reden: `BiosManager` is voor BIOS-files. Andere files horen niet in de bios-storage.

**Opties:**
- A) Alleen `.rom/.sys/.bin` (gekozen)
- B) `.xml` ook accepteren voor latere machine.xml-import — v0.3.0
- C) Whitelist breder maken `.rom/.sys/.bin/.ic/.bios` — meer false-positives

**Omkeerbaarheid**: zeer makkelijk.
- File: `src/BiosZipExtractor.cc` extension-check
- **Tijd: < 2 min**

---

## DD-109 — DSK/CAS vereisen Running-state (geen pending-start)

**Probleem**: openMSX accepteert `diska/cassetteplayer` alleen bij draaiende emulator. Wat als user `.dsk` activeert vóór start?

**Gekozen: `Main.qml` start emulator zonder cart (`msxCore.start("")`) en dán `loadDsk`. `MsxCore::loadDsk` zelf weigert hard niet-Running.**

Reden: 2-step is verwarrend; UI regelt het transparent.

**Opties:**
- A) Auto-boot + loadDsk (gekozen)
- B) Hard weigeren + user-confirm dialog
- C) Pending-start onthouden — race-conditions tussen state-transitions

**Omkeerbaarheid**: middel.
- File: `src/qml/Main.qml::CartridgeBrowser.onActivated` + `romPicker.onAccepted`
- **Tijd: ~15 min** (refactor naar dialog of pending-state)

---

## DD-110 — Slot-routing voor DSK/CAS: hard-coded slot A / single tape

**Probleem**: openMSX heeft `diska/diskb` (2 floppy-drives) + `cassetteplayer` (1 tape).

**Gekozen v0.2.0**: `loadDsk` default `drive=0` (slot A) + alleen Main.qml route naar slot A. Slot B-floppy alleen via expliciete code-call.

Reden: scope. v0.2.1 voegt UI voor drive-B toe.

**Omkeerbaarheid**: middel.
- Files: `src/qml/Main.qml` (routes) + `src/qml/SlotPickerDialog.qml` (uitbreiden voor floppy-slot-keuze) + nieuwe shortcut
- **Tijd: ~1 uur**

---

## DD-111 — RomTypeDetector mapper-priority SCC > ASCII16 > ASCII8

**Probleem**: een ROM kan tegelijk SCC- en ASCII-write-patterns hebben (false-positive).

**Gekozen: SCC > ASCII16 > ASCII8 > Konami-default.**

Reden: SCC-pattern (5 bytes vs 3 bytes ASCII) is meer specifiek → minder false-positive. ASCII16 vóór ASCII8: 0x6000+0x7000-write past in beide signalen, maar 16KB-banks zijn moderner en correcter.

**Omkeerbaarheid**: zeer makkelijk.
- File: `src/RomTypeDetector.cc::detectMapper` regel-volgorde
- **Tijd: < 2 min**

---

## DD-112 — Thumbnails: `screenshot` Tcl-cmd vs framebuffer XML-stream-parse

**Probleem**: hoe snapshot van scherm krijgen?

**Gekozen: `screenshot -prefix <pad>/slot_N` Tcl-cmd.**

Reden: openMSX schrijft PNG direct naar disk; Qt's `QImage` leest met `file://`-URL. Framebuffer XML-stream-parse zou ~150 regels base64-decode + RGB-pack vereisen.

**Opties:**
- A) `screenshot` Tcl-cmd (gekozen)
- B) Framebuffer XML-stream `<framebuffer>` → base64 → `QImage::loadFromData`
- C) Externe `imagemagick` aanroep — overkill

**Omkeerbaarheid**: middel.
- Files: `src/SaveStateModel.cc` + framebuffer-parser in `RomTypeDetector.cc` of nieuwe class
- **Tijd: ~2 uur**

---

## DD-113 — Thumbnails async-race: geen reload-trigger v0.2.0

**Probleem**: `screenshot`-cmd is async. Bij `requestThumbnail()` emit `dataChanged` direct, maar PNG staat nog niet op disk.

**Gekozen v0.2.0**: accept de race. Eerste load van Image kan falen (status=Error), volgende load (na overlay-heropenen) werkt.

Reden: pragmatisch — werkt vaak goed genoeg. Voor robuuste versie: vereist `commandFinished(commandId, ok, body)` signal van core, met match op naam-prefix.

**Omkeerbaarheid**: middel.
- Files: `src/MsxCore.cc` + `src/SaveStateModel.cc`
- **Tijd: ~1 uur**

---

## DD-114 — Codenaam TreasureOfUsas

**Probleem**: codenaam voor v0.2.0.

**Gekozen: Treasure of Usas** (Konami MSX2 1987 platformer met dual-character mechanic).

Reden: "dubbel" past bij sessie — 4 agents parallel + 2 cart slots + 2 floppy-drives + 2 media-routes.

Resterende vrije pool: King's Valley, Twinbee, Gradius, Antarctic Adventure, Hyper Olympic, Goonies, Magical Tree, Pippols, Yie Ar Kung-Fu, Maze of Galious, Vampire, Dragon Slayer, Quarth, Space Manbow, F1 Spirit, Hinotori, Athletic Land.

---

## Wat NIET in v0.2.0 (volledige uitstel-lijst)

- Auto-`setCurrentMachine` via SoftwareDb-lookup → v0.2.1
- `commandFinished`-signal vanuit core voor sync thumbnails → v0.2.1
- Pause/resume + parallelle downloads → v0.3.0
- Tab-strip Main.qml-restructuur (Library/BIOS/Streams/Settings) → v0.3.0
- SettingsScreen voor BIOS-dir/openMSX-pad config → v0.3.0
- Slot B-floppy UI → v0.2.1
- Auto-machine.xml generatie per BIOS-set → v0.3.0+
- Flatpak-build op Steam Deck (Stap 21 6e poging) — vereist Deck SSH
- Unit-tests voor BiosManager / FileDownloader / SaveStateModel-thumbnails / BiosZipExtractor / DSK-CAS Drift / DropArea — alleen RomTypeDetector (13 cases) + SoftwareDb (6 cases) hebben tests
- ROM-header parse publisher/year → v0.3.0
- Pause/resume voor downloads → v0.3.0 (Qt's NAM heeft beperkte resume-support)
- Drag-and-drop bestand-type-detect via MIME-database (huidige: alleen extensie) → v0.2.1
- Steam Input visuele validatie van `msx-browser.vdf` op Deck → blokt op Deck SSH
