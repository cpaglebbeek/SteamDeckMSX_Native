# CHANGELOG — SteamDeckMSX_Native

## v0.0.6-PenguinAdventure (2026-06-01) — D-pad iconen + save-state slot-grid

### Iconen — 8 eigen SVGs (AGPL-compatible)
- `src/assets/icons/dpad/{up,down,left,right}.svg` — minimal driehoeken,
  `currentColor` strokes (Qt6 SVG-render compat)
- `src/assets/icons/btn/{a,b,x,y}.svg` — cirkel met letter
- 64×64 viewBox, ~5KB totaal voor alle 8
- Tokens.qml: `iconDpadUp/Down/Left/Right` + `iconBtnA/B/X/Y` path-constants
  (qrc:/qt/qml/SteamDeckMSX/assets/icons/...)
- `CMakeLists.txt`: Qt6::Svg toegevoegd, 8 SVGs in RESOURCES van qt_add_qml_module

### Save-state slot-grid
- **MsxCore**: `savestate(name)` + `loadstate(name)` slots — Tcl-cmd-wrappers
  (`savestate "<name>"` / `loadstate "<name>"`), retourneren command-id
- **SaveStateModel** (nieuw, ~180 regels) — QAbstractListModel met:
  - 10 vaste slots (MSX-traditie 0..9)
  - QSettings persist per slot: occupied/rom/name/timestamp
  - "Stage save" pattern: model-state updaten ook zonder core attached;
    Tcl-cmd alleen als core en running
  - Naam-conventie: `slot_<N>_<rom_stem>` (default `slot_<N>_nopath`)
  - Roles: SlotRole/NameRole/RomStemRole/LastUsedRole/OccupiedRole/LabelRole
  - Q_INVOKABLE saveTo/loadFrom/clear voor QML
- **SaveStateOverlay.qml** — modaal Popup, 5×2 GridView, X-shortcut opent,
  A = save/load (afhankelijk van occupied), Y = clear, B/Esc = close
- **SaveStateCard.qml** — slot-tile met nummer + naam + timestamp + Save/Load hint
- **Main.qml** integration:
  - SaveStateModel id `saves`, currentRomStem auto-derived van msxCore.currentRom
  - Overlay-instance op Overlay.overlay parent
  - Shortcut "X" → opent overlay alleen als state = Running, anders warning-toast

### Tests 10 → 17 cases (2 binaries)
- **test_msxcore**: ongewijzigd 10/10 ✅
- **test_savestatemodel** (nieuw, 7 cases) ✅:
  - T1: Initial state — 10 slots, alle empty
  - T2: saveTo zonder core → -1 maar model-state wel geupdate
  - T3: saveTo + persistence roundtrip via nieuwe instance
  - T4: clear()
  - T5: LabelRole formatting ("Slot N · <stem> · <ts>" of "Slot N · empty")
  - T6: loadFrom van empty slot → -1
  - T7: out-of-range slots safe (no crash)

### Niet inbegrepen v0.0.6 (gepland v0.0.7+)
- **Stap 21 — eerste Flatpak-build op Steam Deck** (5e poging skip)
- Save-state thumbnails (vereist openMSX framebuffer-extract)
- Steam Input preset
- Icoon-colorize per status (huidig: white-on-black fixed)
- Stream_Server-werk (andere sessie)

## v0.0.5-SolidSnake (2026-05-31) — XML-stream-parser + BUG-004 fix + machine-keuze

### BUG-004 fix — OPENMSX_SYSTEM_DATA env-var
- `OpenmsxLocator` uitgebreid met `dataPath` property + auto-discovery:
  - Mac dev bindist: `<prefix>/bindist/openMSX.app/Contents/Resources/share`
  - Linux Flatpak:   `/app/share/openmsx`
  - Linux dev:       `<binDir>/../share`
- Dev-fallback paths repaired (3 levels relative-root: `/../../../`, `/../../`, `/../`)
- `MsxCore.setDataPath()` zet `QProcessEnvironment OPENMSX_SYSTEM_DATA` op spawn
- **Live-verified op Mac:** `OPENMSX_SYSTEM_DATA=<share> openmsx --version` →
  "openMSX 21.0\nflavour: opt\ncomponents: CORE GL LASERDISC" ✅

### MsxCore — volledige XML-stream-parser
- `QXmlStreamReader` incremental — buffer-friendly, parsed zodra complete element
- `PrematureEndOfDocumentError` graceful (wacht op meer chunks)
- Parse-handlers: `<openmsx-output>` (open/close = Running/Quitting),
  `<reply result="ok|nok" command-id="N">body</reply>`,
  `<update type="..." name="...">value</update>`,
  `<log level="...">msg</log>`
- Nieuwe signals: `replyReceived(int, bool, QString)`, `stateUpdate(QString, QString, QString)`,
  `logMessage(QString, QString)`, `rawLine(QString)` (debug)
- `sendCommand()` retourneert nu int command-id (counter-based, >= 1)
- `requestMachineList()` slot — `machine_info machines` Tcl-call
- `currentMachine` property + `set machine <name>` hot-swap (alleen Running)

### MachineModel (nieuw)
- QAbstractListModel met `name` + `isCurrent` roles
- Fallback hardcoded lijst (`C-BIOS_MSX1` / `MSX2` / `MSX2+`) tot dynamic load slaagt
- Dynamic load via `MsxCore.requestMachineList()` → parse Tcl-list-syntax
  (space-separated, brace-quoted, `{multi word names}`)
- Persist current machine in QSettings (`machine/current`)
- QML-bindable via `core` property; auto-binds `replyReceived` signal

### MachineSelector.qml — inline bottom-bar dropdown
- 360×64 px control met current machine + dropdown indicator
- Popup boven control (opent omhoog, popup-anchor `y: -360`)
- ListView met focus-navigatie, A/Enter = select, B/Esc = close
- `isCurrent` ● dot vs ○ voor andere; accent-primary focus-border
- Persisteert keuze in QSettings via MachineModel

### Tests uitgebreid 6 → 10 cases
- T1-T6: ongewijzigd (initial state, path roundtrip, probe-no-path, stop-idle, sendCommand-idle, dataPath setter)
- **T7 (nieuw): XML `<reply>` parsing** — verifieert command-id + ok/nok + body
- **T8 (nieuw): XML `<update>` parsing** — verifieert type/name/value
- **T9 (nieuw): XML `<log>` parsing** — verifieert level/message met search door history
- **T10 (nieuw): state-transitions** — verifieert Booting → Running → Idle via shell mock-script
- Mock-script-helper in test schrijft tijdelijke `.sh` files die XML dumpen
- 10/10 ✅

### QML — Main.qml uitbreidingen
- MachineModel toegevoegd, gekoppeld aan msxCore
- `onStateChanged: Running → machines.refresh()` (dynamic machine-load)
- `onLogMessage` debug-logging
- Bottom-bar: Y-Stop button + MachineSelector + openmsx-path display (compact)
- OpenmsxLocator.dataPath wordt nu propagated naar msxCore.dataPath

### Niet inbegrepen v0.0.5 (gepland v0.0.6+)
- D-pad SVG iconen
- Save-state slot-grid (savestate/loadstate commands)
- Audio/video render-instellingen
- **Stap 21 — eerste Flatpak-build op Steam Deck** (skip-decision blijft staan)

## v0.0.4-Aleste (2026-05-31) — ROM-loading flow + openMSX IPC

### MsxCore — state-machine + subprocess-IPC
- 7-state enum (Idle/Probing/Probed/Booting/Running/Quitting/Failed) als Q_ENUM,
  via QML bindable
- `probeVersion()` — `openmsx --version` (was `-version`, gefixt — BUG-003)
- `start(romPath="")` — spawn `openmsx -control stdio [-carta <path>]`
- `stop()` — XML-wrapped `quit` command + 2s graceful + terminate-fallback
- `loadRom(path)` — als Running: hot-swap via `carta` cmd, anders start()
- `sendCommand(cmd)` — wrapt in `<command>cmd</command>\n`
- `parseLine(line)` — v0.0.4 minimaal line-based; XML-stream-detectie voor
  `<openmsx-output>` (Booting→Running) en `</openmsx-output>` (→Quitting)
- Stderr aparte log, geen state-impact
- `eventReceived(line)` signal voor QML-debugging

### OpenmsxLocator — singleton
- Zoekvolgorde: QSettings user-path → $PATH → `/app/bin/openmsx` (Flatpak) →
  dev-fallback `externals/openmsx/derived/*/bin/openmsx`
- QML-toegankelijk via `OpenmsxLocator.found` + `OpenmsxLocator.searched`
- `setUserPath()` persisteert in QSettings

### CartridgeModel — recent-list + sentinel
- QAbstractListModel met QSettings persistentie (recent ROMs, max 8)
- `IsSentinelRole` voor "+ Add ROM…" entry
- `addRom(path)` — dedupe + LRU-rotate + persist + signal
- Machine-heuristic per filename (turbor/msx2+/msx2/msx1 detect)
- Dummy 8 MSX-titels uit v0.0.3 verwijderd — vervangen door echte recent + sentinel

### QML
- `Main.qml` — FileDialog (QtQuick.Dialogs), state-LED + state-label header,
  Y-shortcut = stop, openMSX-binary-path in SettingsRow, toast bij start/error
- `CartridgeBrowser.qml` — Loader delegate (cartridge-card vs add-rom-card)
- `AddRomCard.qml` — accent-warm gestyled "+ Add ROM…" met file-picker hook
- `Toast.qml` — info/warning/error overlay, 3s fade

### Tests
- `tests/test_msxcore.cc` — 6 unit-cases (initial state, path roundtrip,
  probe-no-path Failed, probe-true Probed, stop-idle no-op, sendCommand-idle warn)
- Opt-in via `-DSTEAMDECKMSX_BUILD_TESTS=ON`; alle 6 ✅

### Docs
- `docs/openmsx_control_protocol.md` — XML-stream-formaat, command-set v0.0.4,
  v0.0.5+ planning, Mac-quirks geobserveerd

### Mac smoke
- App start ✅ — `SteamDeckMSX 0.0.4-Aleste target= native`
- Geen QML-warnings, geen runtime-errors
- Test-suite test_msxcore: 6/6 ✅
- **Niet end-to-end gemeten:** openMSX-binary heeft op Mac OPENMSX_SYSTEM_DATA
  env-var nodig om machine-configs te vinden (BUG-003) — niet relevant voor
  Steam Deck Flatpak (paths in /app/share/openmsx)

### Bug-fixes (allen Geel — sessie-intern)
- BUG-003 (Geel): `openmsx --version` (not `-version`) — single-dash unknown opt
- BUG-004 (Geel, gedocumenteerd niet gefixt v0.0.4): OPENMSX_SYSTEM_DATA env-var
  vereist voor Mac dev-fallback; Flatpak heeft het impliciet via runtime-bundle
- BUG-005 (Geel): test_msxcore mist Qt6::Qml linkage (qqmlregistration.h in
  MsxCore.h) — opgelost via tests/CMakeLists.txt link

### Stap 21 (eerste Flatpak-build op Deck) — overgeslagen op verzoek
Eerste echte Deck-build pas v0.0.5 (samen met share-data env-var fix, mocht
Flatpak-runtime hetzelfde issue hebben).

### Niet inbegrepen v0.0.4 (gepland v0.0.5+)
- Echte XML-stream-parser (QXmlStreamReader incremental)
- D-pad SVG iconen-set
- C-BIOS machine-selectie-UI
- Save-state slot-grid
- Audio/video render-instellingen

## v0.0.3-Castlevania (2026-05-31) — Eerste runnable code (Qt6/QML)

### Toegevoegd — code
- `CMakeLists.txt` top-level — C++23, find_package Qt6, sub-dirs, opt-in tests
- `src/CMakeLists.txt` — qt_add_executable + qt_add_qml_module met SteamDeckMSX URI
- `src/main.cc` — QGuiApplication + QQmlApplicationEngine, load `SteamDeckMSX/Main`
- `src/MsxCore.{h,cc}` — QObject + QML_ELEMENT, QProcess-stub voor `openmsx -version`
  probe (subprocess-pattern PoC per P-SDM-01 + ARCHITECTURE.md beslissing #2)
- `src/CartridgeModel.{h,cc}` — QAbstractListModel met 8 dummy MSX-titels
  (Metal Gear, Bubble Bobble, Knightmare, Vampire Killer, Nemesis, Aleste,
  Penguin Adventure, Snatcher)
- `src/qml/Tokens.qml` (Singleton) — DESIGN_TOKENS.md waarden ge-encodeerd:
  MSX-CRT-revival palette, Noto-typografie, 4pt-grid, motion, geometry
- `src/qml/Main.qml` — root ApplicationWindow 1280×800, header met openMSX-status,
  CartridgeBrowser, SettingsRow stub, Esc/B = quit
- `src/qml/CartridgeBrowser.qml` — ListView met focus-navigatie, A/Enter = activate
- `src/qml/CartridgeCard.qml` — machine-pictogram + titel + publisher + year,
  focus-ring per Tokens.borderStrong (4px MSX-groen)
- `src/qml/SettingsRow.qml` — label + value placeholder voor BIOS-pad-toekomst
- `tests/CMakeLists.txt` + `tests/test_placeholder.cc` — opt-in via
  `-DSTEAMDECKMSX_BUILD_TESTS=ON`, alleen Qt6::Core smoke

### Toegevoegd — build/deploy
- `deploy/sync-to-deck.sh` — rsync vanaf Mac/laptop naar Deck Desktop Mode
- `deploy/flatpak-build-on-deck.sh` — flatpak-builder runner (op Deck, NIET op Mac)
- `nl.icthorse.SteamDeckMSX.yaml` — echte build-commands voor beide modules
  (openmsx homemade Make + steamdeckmsx-ui CMake)

### Smoke-test (Mac)
- `brew install qt` gestart in background voor `cmake --preset native-debug`
- Build-resultaat zie sessie-MD `prompts/2026-05-31_v0.0.3_first_code.md`

### Stap 21 (eerste Flatpak-build op Deck) — overgeslagen deze sessie
Op verzoek gebruiker. Eerste echte Flatpak-build = v0.0.4-fase.

### Niet inbegrepen v0.0.3 (gepland v0.0.4)
- ROM-loading (echte cartridge-load flow)
- openMSX subprocess-IPC voor save-state/run (alleen `-version` probe nu)
- Steam Input preset
- C-BIOS-machines daadwerkelijk laden in UI
- Echte iconen (D-pad SVG) — placeholders nu

## v0.0.2-Nemesis (2026-05-31) — Architectuur-beslissingen + openMSX submodule

### Toegevoegd
- `externals/openmsx` — git submodule naar `cpaglebbeek/openMSX-steamdeckmsx`
  (fork van `openMSX/openMSX`), gepinned op tag **RELEASE_21_0** commit `cb61db762`
- `CMakePresets.json` — drie presets: `steamdeck-release`, `steamdeck-debug`,
  `native-debug` (Mac smoke). Cache-variabelen `STEAMDECKMSX_UI_TOOLKIT=qt6` +
  `STEAMDECKMSX_OPENMSX_LINK=subprocess` codificeren de architectuur-beslissingen
- `nl.icthorse.SteamDeckMSX.yaml` — Flatpak manifest stub. Runtime
  `org.freedesktop.Platform 23.08`, geen `--share=network`, geen
  `--filesystem=home`. Build-commands placeholder voor v0.0.3
- `DESIGN_TOKENS.md` — MSX-CRT-revival palette, Noto-typografie, 4pt-grid,
  D-pad icoon-set, animatie-tokens, donker-only motivering, wijzigings-impact-regel

### Beslissingen vastgelegd (Oranje — architectuur-impact)
1. UI-toolkit: **Qt6** (was: voorlopig Qt6)
2. openMSX-koppeling: **subprocess + control-channel** (TCP/stdin)
3. Flatpak permissies: definitief (geen netwerk, geen home)
4. Touchscreen: **nee** (gamepad-first per P-SDM-02)

### ARCHITECTURE.md update
- 4 open architectuur-vragen → beslist met rationale + datum-stempel
- Nieuw subprocess-koppelingsdiagram (UI-Qt6 ↔ openmsx -control TCP/stdio)
- openMSX-commit-pin gedocumenteerd (cross-repo sync-regel met Stream_Server)

### Smoke-test — ✅ openMSX RELEASE_21_0 schoon gebouwd op Mac
- brew deps geïnstalleerd: sdl2_ttf, glew, tcl-tk 9.0.3, libogg, libvorbis, theora
- `make -j 8` → EXIT=0, ~2 min, binary `derived/x86_64-darwin-opt/bin/openmsx` (Mach-O 64-bit)
- Linker-warnings cosmetisch (Homebrew SDK 14.0 vs openMSX-default 10.15)
- Niet getest op runtime (P-SDM-08); Mac ≠ support-target
- Conclusie: submodule-pin werkbaar; Flatpak-build v0.0.3 zal vergelijkbaar zijn

## v0.0.1-BubbleBobble (2026-05-31) — Skeleton

- Initiële repo-structuur (newp)
- README, LICENSE (AGPL-3.0), CLAUDE.md, ARCHITECTURE.md, BUGLIST.md
- Geen code, geen build, geen Flatpak — alleen documentatie
- Vrije pool codenamen vastgelegd in CLAUDE.md (per sanitycheck P1 verplaatst
  naar Meta_SteamDeckMSX/CLAUDE.md als single source of truth)
