# BUGLIST — SteamDeckMSX_Native

> Per `Meta_Master/templates/BUGLIST_TEMPLATE.md`. Kleurcodes: Groen / Geel / Rood / Loop.

## Open

_(geen tot v0.0.3-Mac-smoke-build is gevalideerd)_

## Open

_(geen open bugs op v0.0.5-SolidSnake)_

## Opgelost

### BUG-004 — Geel (runtime) — OPENMSX_SYSTEM_DATA env-var (✅ FIXED v0.0.5)
- **Geopend:** 2026-05-31 (v0.0.4-Aleste)
- **Symptoom:** `Couldn't find machines/C-BIOS_MSX2+.xml in any of: ~/.openMSX/share, <repo>/externals/openmsx/share`
- **RCA:** Mac-dev-binary heeft geen impliciete machines-zoekpad zoals Flatpak; standalone bin/openmsx zoekt enkel ~/.openMSX/share + relative paths die niet matchen
- **Fix v0.0.5:** `OpenmsxLocator.dataPath` property + auto-discovery (Mac bindist + Linux Flatpak + Linux dev). `MsxCore.setDataPath()` zet `OPENMSX_SYSTEM_DATA` env-var via `QProcessEnvironment` op spawn
- **Live-verified:** `OPENMSX_SYSTEM_DATA=<share> openmsx --version` → "openMSX 21.0\nflavour: opt\ncomponents: CORE GL LASERDISC" ✅



### BUG-005 — Geel (build) — test_msxcore mist Qt6::Qml link
- **Datum:** 2026-05-31 (v0.0.4-Aleste)
- **Symptoom:** `'qqmlregistration.h' file not found` in test_msxcore.cc build
- **RCA:** MsxCore.h gebruikt `QML_ELEMENT` macro die qqmlregistration.h header vereist; test linkt alleen Qt6::Core+Test
- **Fix:** `target_link_libraries(test_msxcore PRIVATE Qt6::Core Qt6::Qml Qt6::Test)` in tests/CMakeLists.txt

### BUG-003 — Geel (runtime) — `openmsx -version` is unknown option
- **Datum:** 2026-05-31 (v0.0.4-Aleste)
- **Symptoom:** Probe-call faalt: `Error parsing command line: -version`
- **RCA:** openMSX CLI gebruikt GNU-style `--version` (double-dash), niet `-version`
- **Fix:** MsxCore.cc `probeVersion()`: `-version` → `--version`

### BUG-001 — Geel (build) — `qt_add_qml_module` dir-conflict met exe-output
- **Datum:** 2026-05-31 (v0.0.3-Castlevania)
- **Symptoom:** Linker EISDIR (errno=21) op `src/steamdeckmsx` tijdens link-stap
- **RCA functioneel:** qt_add_qml_module schreef QML-resources naar `src/steamdeckmsx/` (afgeleid van target-name) op exact pad waar CMake het exe wilde schrijven
- **RCA technisch:** Default CMAKE_RUNTIME_OUTPUT_DIRECTORY ontbreekt → exe komt in source-tree-relative `src/`
- **RCA architectonisch:** Qt6 QML-module-systeem reserveert target-name als dir; convention-conflict niet expliciet gedocumenteerd in upstream
- **Fix:** Top-level CMakeLists `set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")` + target rename naar `steamdeckmsx_app` met `OUTPUT_NAME=steamdeckmsx`

### BUG-002 — Geel (runtime QML) — `Keys.onPressed` op ApplicationWindow werkt niet
- **Datum:** 2026-05-31 (v0.0.3-Castlevania)
- **Symptoom:** "Could not attach Keys property to: Main is not an Item"
- **RCA functioneel:** Esc/B-toets globaal voor quit werkt niet
- **RCA technisch:** ApplicationWindow erft van `Window` niet van `Item`; `Keys` is `attached property` op `Item`
- **RCA architectonisch:** Qt6 design-keuze; eigen P-SDM-02 (gamepad-first) eist global key-handling
- **Fix:** Vervangen door `Shortcut { sequences: ["Escape","B"]; onActivated: Qt.quit() }` — werkt cross-Window-types

## Verwacht v0.0.3 (waar te letten bij eerste Mac-smoke + eerste Deck-build)

### Build-tijd

- **Groen** — Qt6 niet gevonden door CMake → check `find_package(Qt6 6.6 REQUIRED ...)` PATH; oplossing: `cmake --preset native-debug -DCMAKE_PREFIX_PATH=$(brew --prefix qt)`
- **Geel** — `qt_add_qml_module` herkent Tokens.qml niet als singleton → check `QT_QML_SINGLETON_TYPE` property + `pragma Singleton` in Tokens.qml
- **Geel** — `QML_ELEMENT` macro vereist `qqmlregistration.h` + Qt6.5+ — als oudere Qt: degradeer naar `qmlRegisterType` in main.cc

### Runtime (Mac smoke)

- **Geel** — Tokens singleton-import faalt → "module SteamDeckMSX not found" → moc-generated qmldir te checken in build-dir
- **Geel** — `KeyNavigationEnabled: true` op ListView werkt niet met items zonder explicit focus-handling — momentaneel via `ListView.isCurrentItem` op delegate
- **Groen** — `Qt.UserRole + 1` hard-coded in Main.qml console.log — gebruik role-naam string i.p.v. enum

### Runtime (Deck-Flatpak, v0.0.4 verwacht)

- **Rood** — Flatpak runtime 23.08 heeft mogelijk Qt 6.5 i.p.v. 6.6 → CMake requirement bump pessimistisch
- **Geel** — `--device=input` portal blokkeert mogelijk Steam Input rebindings → moet getest op Deck
- **Rood** — openMSX `make` in Flatpak-sandbox kan falen door tcl-detectie → mogelijk apart module met tcl als source

## Cross-repo bug-referenties

Zie `Meta_Master/BUGS_GLOBAL.md`. Specifiek voor Native:
- **CACHE/QML** — Qt6 QML-compiler cache `.qmlc` kan stale zijn bij CMake-only rebuilds; rebuild `build/` bij module-wijzigingen
- **DEPLOY/Flatpak** — submodule openMSX moet ge-add zijn vóór flatpak-builder run

## Terugkerende patronen

_(in te vullen vanaf eerste echte bugs in v0.0.3/v0.0.4)_
