# CHANGELOG — SteamDeckMSX_Native

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

### Smoke-test
- Vanilla openMSX build poging op Mac (homemade Make build-system, niet CMake)
- brew deps geïnstalleerd: sdl2_ttf, glew, tcl-tk (9.0.3), libogg, libvorbis, theora
- Build-resultaat: zie sessie-MD `prompts/2026-05-31_v0.0.2_native_decisions.md`
- Mac = geen support-target (P-SDM-08); falen acceptabel

## v0.0.1-BubbleBobble (2026-05-31) — Skeleton

- Initiële repo-structuur (newp)
- README, LICENSE (AGPL-3.0), CLAUDE.md, ARCHITECTURE.md, BUGLIST.md
- Geen code, geen build, geen Flatpak — alleen documentatie
- Vrije pool codenamen vastgelegd in CLAUDE.md (per sanitycheck P1 verplaatst
  naar Meta_SteamDeckMSX/CLAUDE.md als single source of truth)
