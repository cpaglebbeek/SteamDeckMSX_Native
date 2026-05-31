# CHANGELOG — SteamDeckMSX_Native

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
