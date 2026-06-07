# Design Decisions — v0.1.0-Xanadu

> Per gebruikersopdracht (2026-06-08, sessie-bundel SteamDeck MSX): "doe overal
> jouw beste keuze waar je vragen hebt. sla wel overal op welke keuzes je had
> en welke je hebt gekozen zodat ik het later mogelijk nog kan veranderen als
> er iets niet werkt."
>
> Iedere beslissing onder een `DD-N` anchor heeft:
> - **Probleem**: wat moest beslist worden
> - **Opties**: alle realistische alternatieven
> - **Gekozen**: welke optie + waarom
> - **Omkeerbaarheid**: hoe makkelijk later anders te kiezen + welke files raken
> - **Vermoedelijk omkeer-tijd**: ruwe schatting

---

## DD-001 — HTTPS-only vs HTTP-toestaan voor downloads

**Probleem**: gebruiker wil BIOS/ROM downloaden van URL. Wel of geen HTTP?

**Opties:**
- A) HTTPS-only (huidige)
- B) HTTP + HTTPS toegestaan
- C) HTTPS-default + Settings-toggle "Sta HTTP toe"

**Gekozen: A — HTTPS-only**

Reden: BIOS/ROM-files zijn binary; HTTP-verkeer kan op het pad gewijzigd worden (ISP, café-WiFi, schoolnet). Voor een single-user retro-emulator is HTTPS realistisch (archive.org, GitHub, eigen NAS via cloudflare-tunnel etc.). Beter veilige default — toggle kan altijd later.

**Omkeerbaarheid**: zeer makkelijk.
- File: `src/FileDownloader.cc` regel `if (url.scheme().compare("https", ...) != 0)`
- Verwijder de scheme-check, of voeg `|| scheme == "http"` toe
- **Tijd: < 5 min**

---

## DD-002 — Redirects toestaan of weigeren

**Probleem**: archive.org URLs en mirror-links redirecten vaak. Doorvolgen?

**Opties:**
- A) `NoLessSafeRedirectPolicy` (HTTPS→HTTPS OK, HTTPS→HTTP wordt geweigerd) — huidige
- B) `ManualRedirectPolicy` (gebruiker bevestigt elke redirect)
- C) `NoRedirectPolicy` (alles weigeren)

**Gekozen: A**

Reden: balans tussen gemak (gebruiker hoeft niet steeds te bevestigen) en veiligheid (downgrade naar HTTP wordt geblokkeerd). Manual redirect is ergerlijk in 95% van gevallen.

**Omkeerbaarheid**: makkelijk.
- File: `src/FileDownloader.cc` regel `req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, ...)`
- **Tijd: < 5 min**

---

## DD-003 — Maximum bestandsgrootte per type

**Probleem**: voorkomen dat gebruiker per ongeluk een 4GB-ISO downloadt.

**Gekozen:**
- BIOS: 1 MiB (`BiosManager::kBiosMaxBytes`)
- ROM: 8 MiB (`CartridgeModel::kRomMaxBytes`)

Reden: grootste MSX BIOS = 64 KiB + extensies (~512 KiB max). Grootste MSX2 mega-ROM = ~4 MiB. Beide met ruime marge (~4×) — voorkomt missers maar laat realistische ROMs door.

**Opties overwogen:**
- Geen cap (gevaarlijk)
- 100 MiB (te groot, willekeurig)
- 2 MiB BIOS / 16 MiB ROM (extra-ruim, geen praktisch verschil)

**Omkeerbaarheid**: zeer makkelijk.
- Files: `src/BiosManager.h` + `src/CartridgeModel.h` (`static constexpr qint64 k*MaxBytes`)
- **Tijd: < 2 min** (1 constante per file)

---

## DD-004 — Download-timeout

**Probleem**: hoe lang wachten op trage server?

**Gekozen: 30s** (`QNetworkRequest::setTransferTimeout(30000)`)

Reden: typische BIOS-file < 100 KB → 30s is meer dan zat voor 56 kbit/s. ROMs ≤ 4 MB → 30s = ~1 Mbit/s minimaal (heel laag). Trage user-mobile-data fallback.

**Opties:**
- 10s (te strak op slechte verbindingen)
- 30s (gekozen)
- 60s (te lang voor user-feel)
- Geen timeout (kan vastlopen)

**Omkeerbaarheid**: zeer makkelijk.
- File: `src/FileDownloader.cc` `req.setTransferTimeout(30 * 1000)`
- **Tijd: < 1 min**

---

## DD-005 — Storage-locatie (`QStandardPaths::AppDataLocation`)

**Probleem**: waar BIOS/ROMs op disk neerzetten?

**Gekozen: `QStandardPaths::AppDataLocation` + sub-dirs `bios/` en `roms/`**

Per platform:
- Mac dev: `~/Library/Application Support/iCt-Horse/SteamDeckMSX/{bios,roms}`
- Linux: `~/.local/share/iCt-Horse/SteamDeckMSX/{bios,roms}`
- Flatpak: `~/.var/app/nl.icthorse.SteamDeckMSX/data/{bios,roms}`

**Opties:**
- A) `AppDataLocation` (gekozen) — Qt managed, cross-platform
- B) Vaste `~/Documents/SteamDeckMSX/` — gebruiker-zichtbaar, maar buiten Flatpak-sandbox lastig
- C) `XDG_DATA_HOME` directly — Linux-only conventie
- D) ROM-folder configurabel via Settings — meer flexibel maar ook meer state

**Omkeerbaarheid**: makkelijk.
- Files: `src/BiosManager.cc::storageDir()` + `src/CartridgeModel.cc::storageDir()`
- Verander naar bijv. `QDir::homePath() + "/SteamDeckMSX/..."`
- Bestaande entries niet automatisch gemigreerd → gebruiker moet `clearAll` + re-import (of handmatige file-copy)
- **Tijd: < 10 min** voor code, gebruiker-actie nodig voor migratie

---

## DD-006 — Persistentie via QSettings

**Probleem**: hoe metadata (welke BIOS/ROMs geregistreerd) bewaren tussen sessies?

**Gekozen: QSettings (`AppDataLocation/.../SteamDeckMSX-test.conf` of `*.plist`)**

Reden: Qt's standaard. Geen extra deps. Cross-platform. Voldoende voor kleine arrays.

**Opties:**
- A) QSettings (gekozen)
- B) SQLite database — overkill voor <100 entries
- C) JSON-file in storage-dir — meer leesbaar voor user, maar concurrency lastiger
- D) QML LocalStorage — Qt Quick-only, niet bruikbaar in core lib

**Omkeerbaarheid**: middel.
- Files: `src/BiosManager.cc::loadFromSettings/persistToSettings` + idem CartridgeModel
- Migratie van bestaande QSettings naar JSON: extra code (~50 regels) + 1× import-pad
- **Tijd: ~30 min**

---

## DD-007 — BIOS-koppeling aan openMSX

**Probleem**: hoe weet openMSX over de geüploade BIOS-files?

**Gekozen v0.1.0**: alleen administratie in BiosManager. Geen automatische `machines/*.xml`-generatie. User moet handmatig de storage-dir aan openMSX bekend maken via `OPENMSX_USER_DATA` of een aangepast `machine.xml`.

Reden: openMSX' machine-XML is complex (CPU, mappers, VDP, IO-ports). Automatische generatie is fragiel. v0.2.0+ kan een whitelist van "bekende BIOS-sets" toevoegen die automatisch een machine.xml-stub schrijven.

**Opties:**
- A) Alleen administratie (gekozen)
- B) Auto-generate machine.xml per BIOS — fragiel
- C) BIOS-set ZIP-extract met openMSX-meegeleverde XMLs — out-of-scope v0.1.0
- D) openMSX `softwaredb.xml`-style lookup met bekende SHA-1 hashes — vereist database (v0.2.0+)

**Omkeerbaarheid**: hoog.
- Nieuwe component `BiosToOpenmsxBridge` in core lib + UI in Settings
- BiosManager-API blijft compatibel
- **Tijd: ~1-2 sessies werk voor v0.2.0**

---

## DD-008 — Eén gedeelde URL-import-dialog vs aparte per type

**Probleem**: BIOS-URL en ROM-URL hebben dezelfde flow.

**Gekozen: één `UrlImportDialog.qml`** met `target`-property ("bios" / "rom").

Reden: voorkomt UI-duplicatie. Caller koppelt signal aan juiste manager.

**Opties:**
- A) Eén gedeelde (gekozen)
- B) Aparte `BiosUrlDialog.qml` + `RomUrlDialog.qml`
- C) Wizard-stappen met type-keuze als eerste stap

**Omkeerbaarheid**: makkelijk.
- File: `src/qml/UrlImportDialog.qml` splitsen
- Main.qml signal-wiring duplicate
- **Tijd: ~15 min**

---

## DD-009 — Slot A/B keuze-mechanisme

**Probleem**: cartridge inschuiven in slot A of slot B?

**Gekozen:**
- Default activate (A/Enter/Space op cartridge) → **slot A** (start emulator als niet-running, of vervang slot A inhoud).
- Shortcut `S` op cartridge → `SlotPickerDialog` opent met beide opties. Slot B alleen actief tijdens Running (anders inactive met hint).

Reden: 95% van gebruik = "start dit spel". Slot B is uitbreidings-cartridge (SCC-extension, save-game-cartridge, twee-spelers-mode). Eerst game starten, dan B bij-laden = natuurlijke workflow.

**Opties:**
- A) Default A + shortcut voor B (gekozen)
- B) Long-press in browser opent SlotPicker (timer-based, fragiel in QML)
- C) Altijd vraag stellen — irriterend bij snel-spelen
- D) Slot-radio in browser-card — extra UI-rommel

**Omkeerbaarheid**: makkelijk.
- File: `src/qml/Main.qml` Shortcut "S" + onActivated CartridgeBrowser
- Verander naar long-press: voeg Timer toe aan CartridgeCard
- **Tijd: ~30 min**

---

## DD-010 — Keyboard-shortcuts I / U / S / X / Y

**Probleem**: welke toetsen voor nieuwe acties?

**Gekozen:**
- **I** = BIOS-screen openen
- **U** = URL-import voor ROM (target = "rom")
- **S** = Slot-picker voor huidige cartridge
- **X** = SaveStateOverlay (bestaand)
- **Y** = Stop emulator (bestaand)
- **B / Esc** = Quit / sluit dialog (bestaand)

Reden: vermijden conflict met bestaande (X/Y/B). Korte ezelsbruggen: **I**nventory (BIOS), **U**rl, **S**lot.

**Opties overwogen:**
- F1/F2/F3 — minder ezelsbruggen, conflicten met openMSX-functies
- Alt+I/U/S — werkt niet altijd in Gaming Mode
- Letters (gekozen) — Steam Input-friendly

**Omkeerbaarheid**: zeer makkelijk.
- File: `src/qml/Main.qml` `Shortcut { sequences: [...] }` blocks
- **Tijd: < 5 min**

---

## DD-011 — Atomische write voor downloads

**Probleem**: crash mid-download laat half-bestand achter.

**Gekozen: write naar `<dest>.part` + rename naar `<dest>` na succes.**

Reden: rename is atomisch op alle OSen. Crash → `.part` laat achter (cleanup bij volgende addFromUrl met zelfde naam).

**Opties:**
- A) `.part` + atomic rename (gekozen)
- B) Direct naar dest schrijven — risk on partial files
- C) Memory-buffer + écht-late write — werkt, OOM-risico bij grote ROMs

**Omkeerbaarheid**: makkelijk.
- File: `src/FileDownloader.cc::onFinished` — verwijder `.part`-logica
- **Tijd: < 10 min**

---

## DD-012 — Veiligheid in destination-pad-resolutie

**Probleem**: user-geleverde naam ("../../etc/passwd") kan filesystem-escape veroorzaken.

**Gekozen: `/` en `\` worden vervangen door `_` in `resolveDestPath()`.**

**Opties:**
- A) Replace separators (gekozen) — simpel, voldoende voor onze use-case
- B) Reject names met separators — extra UI-werk (error message)
- C) Use `QDir::cleanPath` + check dat resultaat in storageDir() blijft — robuuster

**Omkeerbaarheid**: makkelijk.
- Files: `src/BiosManager.cc` + `src/CartridgeModel.cc` `resolveDestPath()`
- **Tijd: < 10 min**

---

## DD-013 — SHA-1 fingerprint berekenen bij elke add

**Probleem**: tijd-tradeoff bij grote ROMs.

**Gekozen: ja, altijd berekenen — gebruikt bestaande `RomTypeDetector::sha1Hex()`.**

Reden: SHA-1 op 4 MiB ~10ms op moderne hardware — verwaarloosbaar. Geeft duplicate-detect, persistente identity, en toekomstige softwaredb-lookup.

**Opties:**
- A) Altijd (gekozen)
- B) Skip bij > 2 MiB — kostenbesparing, maar inconsistentie
- C) Lazy (alleen bij eerste lookup) — meer state

**Omkeerbaarheid**: zeer makkelijk.
- File: `src/BiosManager.cc::buildEntryFromFile` + `src/CartridgeModel.cc::registerLocal`
- **Tijd: < 5 min**

---

## DD-014 — Slot B alleen tijdens Running

**Probleem**: openMSX accepteert `cartb` alleen tijdens draaiend spel.

**Gekozen: `loadRomSlotB` returnt -1 + log-warning bij niet-Running. SlotPickerDialog toont knop inactief met hint.**

**Opties:**
- A) Hard weigeren (gekozen)
- B) Slot B als "pending start" onthouden en automatisch bij eerste cartA-start mee-injecteren — complex en kan openMSX-quirks raken
- C) openMSX starten met -cart en -extb argumenten — vereist anders-spawn-cmd

**Omkeerbaarheid**: middel.
- Files: `src/MsxCore.cc::loadRomSlotB` + `src/qml/SlotPickerDialog.qml` `enabled: runningState`
- **Tijd: ~30 min** voor pending-start variant

---

## DD-015 — addRom (legacy) blijft, maar route nu via addFromLocal

**Probleem**: backwards-compat met bestaande `addRom(path)` call uit `romPicker.onAccepted`.

**Gekozen: `addRom(path)` is nu alias voor `addFromLocal(path, copyIntoStorage=false)`.**

Reden: bestaand FileDialog gedrag (registreer pad zoals-is, niet kopiëren) blijft hetzelfde. Nieuwe `addFromLocal(path, true)` is voor de "kopieer naar storage"-flow (relevant voor Flatpak-sandbox-isolatie).

**Omkeerbaarheid**: hoog.
- File: `src/CartridgeModel.cc::addRom` (1 regel) + alle callers (= 1 plek in `Main.qml::romPicker.onAccepted`)
- **Tijd: < 5 min**

---

## DD-016 — Codenaam Xanadu

**Probleem**: codenaam voor v0.1.0.

**Gekozen: Xanadu** (Falcom Dragon Slayer II - The Legend of Xanadu, MSX2 1986). Past bij "expansie van mogelijkheden" — BIOS-manager, URL-downloads, 2 slots.

Vrije pool overig: Treasure of Usas, King's Valley, Twinbee, Gradius, Antarctic Adventure, Hyper Olympic, Goonies, Magical Tree, Pippols, Yie Ar Kung-Fu, Maze of Galious, Vampire, Dragon Slayer, Quarth, Space Manbow, F1 Spirit, Hinotori, Athletic Land.

---

## Wat NIET in v0.1.0 (expliciet uitgesteld)

- **Zip-archief extract** voor BIOS-sets — v0.1.1
- **SoftwareDb-class** met `softwaredb.xml`-parse — v0.2.0
- **Auto-machine.xml-generatie** voor BIOS — v0.2.0+
- **`.dsk`/`.cas`/`.zip` ondersteuning in CartridgeModel** — alleen `.rom` voor v0.1.0 (extension wordt auto toegevoegd)
- **ROM-checksum verifiëren tegen bekende-goede hashes vóór accepteren** — vereist database, v0.2.0
- **Drag-and-drop ROM/BIOS in UI** — vereist platform-specifiek werk, v0.1.1
- **Pause/resume voor downloads** — Qt's NAM heeft beperkte resume-support, v0.2.0
- **Parallelle downloads** — slechts 1 download tegelijk, v0.2.0
- **Tab-strip Library / BIOS / Settings** — vervangen door modal Popup-flow (eenvoudiger). Tab-strip blijft op v0.2.0 als visuele-mockups via Excalidraw klaar zijn
- **Auto-download van bekende BIOS-sets** via vooraf gedefinieerde manifests — gevaarlijk auteursrechten, niet doen
