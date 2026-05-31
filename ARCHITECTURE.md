# ARCHITECTURE — SteamDeckMSX_Native

> Variant 1 van het SteamDeckMSX ecosysteem. Zie [Meta_SteamDeckMSX/ARCHITECTURE.md](https://github.com/cpaglebbeek/Meta_SteamDeckMSX/blob/main/ARCHITECTURE.md) voor de ecosysteem-brede architectuur.

## Componentdiagram

```
┌────────────────────────────────────────────────────────────────┐
│  Flatpak-container nl.icthorse.SteamDeckMSX                    │
│                                                                │
│   ┌─────────────────────────────┐                              │
│   │   Deck-UI-laag (eigen code) │                              │
│   │   ┌────────┐  ┌────────┐    │                              │
│   │   │Cart-   │  │OSD     │    │                              │
│   │   │browser │  │overlay │    │                              │
│   │   └────────┘  └────────┘    │                              │
│   │   ┌────────┐  ┌────────┐    │                              │
│   │   │Settings│  │SteamIn │    │                              │
│   │   │        │  │preset  │    │                              │
│   │   └────────┘  └────────┘    │                              │
│   └──────────────┬──────────────┘                              │
│                  │ Qt-signal / IPC                             │
│                  ▼                                             │
│   ┌─────────────────────────────┐                              │
│   │   openMSX-core (fork)       │                              │
│   │   ┌──────┐ ┌──────┐ ┌─────┐ │                              │
│   │   │ Z80  │ │V9938 │ │PSG  │ │                              │
│   │   │ R800 │ │V9958 │ │SCC  │ │                              │
│   │   └──────┘ └──────┘ └─────┘ │                              │
│   │   ┌──────┐ ┌──────┐ ┌─────┐ │                              │
│   │   │Mapper│ │BIOS  │ │FDC  │ │                              │
│   │   └──────┘ └──────┘ └─────┘ │                              │
│   └──────────────┬──────────────┘                              │
│                  │ SDL2                                        │
│                  ▼                                             │
│   ┌────────────┐ ┌────────────┐ ┌──────────────┐               │
│   │ Framebuffer│ │ Audio      │ │ Steam Input  │               │
│   │ → display  │ │ → speakers │ │ ← gamepad    │               │
│   └────────────┘ └────────────┘ └──────────────┘               │
└────────────────────────────────────────────────────────────────┘

Persistent storage:
  ~/.var/app/nl.icthorse.SteamDeckMSX/data/
    ├─ bios/        ← user-imported BIOS (NIET in repo)
    ├─ roms/        ← user-imported cartridges (NIET in repo)
    ├─ states/      ← save-states (slots 1-10 per ROM)
    └─ config/      ← settings, input-presets
```

## Build-stack

| Laag | Tooling |
|------|---------|
| Build-systeem | CMake 3.28+, presets in `CMakePresets.json` |
| Compiler | clang++ 18+ (Freedesktop SDK 23.08) |
| UI-toolkit | **TBD v0.0.2** — Qt6 of GTK4 |
| Emulator-core | openMSX fork (submodule `externals/openmsx`) |
| Flatpak-runtime | `org.freedesktop.Platform//23.08` |
| Manifest | `nl.icthorse.SteamDeckMSX.yaml` (v0.1.0) |

## Directory-layout (actueel v0.0.3-Castlevania)

```
SteamDeckMSX_Native/
├─ README.md
├─ LICENSE                          ← AGPL-3.0
├─ CLAUDE.md
├─ VERSION                          ← 0.0.3-Castlevania
├─ ARCHITECTURE.md                  ← dit bestand
├─ BUGLIST.md
├─ CHANGELOG.md
├─ DESIGN_TOKENS.md                 ← v0.0.2+ (MSX-CRT-revival)
├─ CMakeLists.txt                   ← v0.0.3+ — top-level, C++23, Qt6
├─ CMakePresets.json                ← v0.0.2+ — 3 presets
├─ nl.icthorse.SteamDeckMSX.yaml    ← v0.0.3+ — Flatpak manifest met build-commands
├─ .gitignore
├─ .gitmodules                      ← v0.0.2+
│
├─ src/                             ← v0.0.3+
│   ├─ CMakeLists.txt               ← qt_add_executable + qt_add_qml_module
│   ├─ main.cc                      ← QGuiApplication + load Main.qml
│   ├─ MsxCore.{h,cc}               ← QObject + QML_ELEMENT, QProcess-stub (subprocess PoC)
│   ├─ CartridgeModel.{h,cc}        ← QAbstractListModel, 8 dummy MSX-titels
│   └─ qml/
│       ├─ Tokens.qml               ← singleton, DESIGN_TOKENS waarden
│       ├─ Main.qml                 ← root window 1280×800
│       ├─ CartridgeBrowser.qml     ← ListView focus-nav
│       ├─ CartridgeCard.qml        ← per-titel kaart
│       └─ SettingsRow.qml          ← label/value placeholder
│
├─ tests/                           ← v0.0.3+ (opt-in: -DSTEAMDECKMSX_BUILD_TESTS=ON)
│   ├─ CMakeLists.txt
│   └─ test_placeholder.cc
│
├─ deploy/                          ← v0.0.3+
│   ├─ sync-to-deck.sh              ← rsync Mac → Deck Desktop Mode
│   └─ flatpak-build-on-deck.sh     ← flatpak-builder op Deck (niet op Mac)
│
├─ externals/                       ← v0.0.2+
│   └─ openmsx/                     ← git submodule cpaglebbeek/openMSX-steamdeckmsx
│                                     gepinned RELEASE_21_0 (cb61db762)
│
├─ patches/                         ← (gepland: lokale openMSX patches, P-SDM-01)
│
├─ prompts/                         ← sessie-MD's
│
└─ releases/                        ← Flatpak artefacten (untracked >100MB)
```

## QML module-systeem

URI: `SteamDeckMSX` 1.0 — geregistreerd via `qt_add_qml_module`. Bevat:
- 5 QML files (`Tokens` als singleton, `Main`/`CartridgeBrowser`/`CartridgeCard`/`SettingsRow`)
- 2 C++ types: `MsxCore`, `CartridgeModel` (via `QML_ELEMENT` macro)

`Tokens.qml` is singleton + bron-van-waarheid voor alle styling-waarden — verwijst naar `DESIGN_TOKENS.md` (manuele sync; wijziging in MD vereist update in Tokens.qml). Drift = **Geel bug**.

## Externe afhankelijkheden

Zie [Meta_SteamDeckMSX/docs/DEPENDENCIES.md](https://github.com/cpaglebbeek/Meta_SteamDeckMSX/blob/main/docs/DEPENDENCIES.md) — dit project is consument van die lijst.

Eigen extra's (alleen native variant):
- **Qt6** of **GTK4** (UI-toolkit) — beslis v0.0.2
- **libappindicator** (geen — Steam Deck Gaming Mode heeft geen system tray)
- **kdialog/zenity** (optioneel voor file-pickers in Desktop Mode)

## Relatie met andere repos

| Repo | Type relatie |
|------|--------------|
| Meta_SteamDeckMSX | Pull docs/principes; geen runtime-link |
| SteamDeckMSX_Stream_Server | Save-state-formaat compat (P-SDM-04) — bestand-pad-conventie |
| SteamDeckMSX_Stream_Client | Geen directe link; zelfde gebruiker kan beide installeren |

## Architectuurvragen — BESLOTEN v0.0.2-Nemesis (2026-05-31)

| # | Vraag | Beslissing | Rationale |
|---|---|---|---|
| 1 | UI-toolkit | **Qt6** (KDE-native + QML voor TV-stijl) | Steam Deck Desktop = KDE; QML maakt focus-navigatie + gamepad-bindings expliciet; openMSX zelf gebruikt geen Qt → geen conflict |
| 2 | openMSX-koppeling | **Subprocess + control-channel** | openMSX heeft een ontworpen control-interface (TCP/stdin) voor externe UIs (matcht onze use-case 1-op-1). Voordelen: AGPL-isolatie schoon, geen ABI-instabiliteit, core-crash neemt UI niet mee, upstream-changes minder breaking |
| 3 | Flatpak permissies | `--socket=wayland`, `--socket=fallback-x11`, `--socket=pulseaudio`, `--device=input`, `--device=dri`, `--persist=.var/app/...` + **GEEN `--share=network`** + **GEEN `--filesystem=home`** | Variant 1 = offline-first; BIOS/ROMs via Flatpak portal-file-picker (P-SDM-05 + P-SDM-06) |
| 4 | Touchscreen | **Nee — alleen gamepad** | P-SDM-02 gamepad-first; touchscreen werkt automatisch in Desktop Mode maar geen design-target |

Zie `CMakePresets.json` (toolkit + link-mode als CMake-cache-variabelen) en `nl.icthorse.SteamDeckMSX.yaml` (Flatpak permissies).

## openMSX-koppeling — detail

```
┌──────────────────────────┐         ┌──────────────────────────┐
│  SteamDeckMSX-UI (Qt6)   │         │  openmsx-core (process)  │
│                          │         │  RELEASE_21_0 commit     │
│   QtNetwork QTcpSocket   │ ───────►│  cb61db762               │
│        ▲                 │  TCP    │                          │
│        │ control-cmds    │  4321   │  openmsx -control stdio  │
│        │ (text protocol) │ ◄────── │  of -control tcp:port    │
│        ▼                 │ events  │                          │
│   ROM-load / save-state  │         │  framebuffer→SDL2→display│
│   gamepad-passthrough    │         │  audio→SDL2→pulseaudio   │
└──────────────────────────┘         └──────────────────────────┘
```

UI start `openmsx-headless-stub` (in v0.0.3 — voor nu is openMSX zelf met `-control stdio` voldoende), stuurt control-commands (`load_rom`, `savestate`, `set_machine`, `quit`), ontvangt events (`update FPS`, `framebuffer ready`).

**Voordelen subprocess-pattern:**
- Crash van core ≠ crash van UI → user kan diagnostiek tonen
- AGPL-3.0 + openMSX GPL-2.0 koppeling: subprocess = "aggregate" niet "derivative" → schone licentie-grens
- openMSX-upstream blijft onafhankelijk → upstream-first (P-SDM-01) eenvoudiger
- Test-isolatie: UI test los van core

**Nadelen:**
- IPC-overhead (verwaarloosbaar voor MSX-frame-rate)
- Gamepad-events moeten dubbelheen → gemitigeerd via SDL2-shared-state of via Steam Input doorgeven aan core direct (voorkeur)

## openMSX-commit-pin

| Aspect | Waarde |
|---|---|
| Tag | `RELEASE_21_0` |
| Commit | `cb61db762` |
| Fork | `cpaglebbeek/openMSX-steamdeckmsx` |
| Upstream | `openMSX/openMSX` |
| Sync-regel | Stream_Server moet **identieke commit** gebruiken (zie `Meta_SteamDeckMSX/docs/DEPENDENCIES.md` save-state-compat) |

Bump van commit = **Oranje** (kan game-compat raken). Regression-corpus: Bubble Bobble / Metal Gear / Nemesis (testset).
