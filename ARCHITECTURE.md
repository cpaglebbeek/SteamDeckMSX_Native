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

## Directory-layout

```
SteamDeckMSX_Native/
├─ README.md
├─ LICENSE                   ← AGPL-3.0
├─ CLAUDE.md
├─ VERSION                   ← 0.0.1-BubbleBobble
├─ ARCHITECTURE.md           ← dit bestand
├─ BUGLIST.md
├─ CHANGELOG.md
├─ .gitignore
├─ CMakeLists.txt            ← v0.0.2+
├─ CMakePresets.json         ← v0.0.2+
├─ nl.icthorse.SteamDeckMSX.yaml  ← Flatpak manifest, v0.1.0+
│
├─ src/                      ← v0.0.2+
│   ├─ ui/
│   ├─ bridge/               ← openMSX <-> UI bridge
│   └─ steam_input/
│
├─ externals/                ← v0.0.2+
│   └─ openmsx/              ← git submodule, fork
│
├─ docs/
│   └─ screens/              ← variant-specifieke wireframes
│
├─ patches/                  ← lokale openMSX patches (zie P-SDM-01)
│
├─ prompts/                  ← sessie-MD's
│
└─ releases/                 ← Flatpak artefacten (untracked >100MB)
```

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

## Open architectuurvragen (v0.0.2-onderwerpen)

1. **Qt6 vs GTK4** — Qt6 = beter cross-platform + QML voor TV-stijl UI. GTK4 = GNOME-native, lichter. Steam Deck KDE-desktop = pro-Qt. **Voorlopig: Qt6.**
2. **openMSX-API-koppeling** — embedded library vs subprocess + IPC? openMSX heeft geen stabiele C-API → subprocess + commando-channel waarschijnlijk veiliger (matcht openMSX's eigen "control" interface).
3. **Flatpak permissies** — minimum: read-only home, write `~/.var/app/...`, network=NEE (offline), gamepad device access via portal.
4. **Multitouch/touchscreen** — Steam Deck heeft touchscreen; UI moet ook tikbaar zijn? **Voorlopig: nee, alleen gamepad.**
