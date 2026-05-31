# CLAUDE.md — SteamDeckMSX_Native

> Variant 1: native Flatpak op Steam Deck. Erft globale regels van `Meta_Master/CLAUDE.md` (WhatIf, ZSH, sessie-protocol, over-en-uit).

## Rol

Native, offline MSX-emulator voor Steam Deck. Fork van openMSX + eigen UI-laag.

## Codenaam-thema

MSX-game-helden. v0.0.1 = **Bubble Bobble**. **Vrije pool + rotatie-regels:** zie `Meta_SteamDeckMSX/CLAUDE.md` § Codenaam-thema (single source of truth).

## Feature & Bugfix Protocol (Color-Coded)

**Nieuwe Feature:**
- **Groen** — Pure UI-toevoeging zonder openMSX-patch → +0.0.1
- **Oranje** — Nieuwe component/scherm of openMSX-patch (upstream-conform) → +0.1.0
- **Rood** — Architectuurwijziging (bv. UI-toolkit-wissel Qt6↔GTK4) → +1.0.0

**Bugfix:**
- **Groen** — Snel fysiek herstel (typo, off-by-one)
- **Geel** — Logische bug (input-mapping verkeerd, save-state-pad fout)
- **Rood** — Architecturale bug (drift met openMSX upstream, AGPL-licentieconflict)
- **Loop** — debug-loop, nieuwe invalshoek

**RCA verplicht bij elke bugfix:** Functioneel / Technisch / Architectonisch.

## Versioning Mandate

Bump VERSION (semver + codenaam) vóór elke build. Zie `Meta_Master/CLAUDE.md` regel.

## WhatIf Protocol

Verplicht vóór code, build, of openMSX-patch. Zie `Meta_Master/CLAUDE.md`.

## Build & Testing

**Vanaf v0.0.2:**
```bash
# Native debug-build op Mac (cross-platform smoke-test)
cmake --preset native-debug
cmake --build build/native-debug

# Steam Deck Flatpak build
flatpak-builder --force-clean build-flatpak nl.icthorse.SteamDeckMSX.yaml
```

**Test-corpus:**
- C-BIOS boot test (moet 8 LSI-keys tonen)
- Bubble Bobble (MSX1 cartridge, Konami-mapper-test)
- Metal Gear (MSX2 cartridge)
- Nemesis (SCC-audio-test)

## Release Protocol

1. Bump VERSION + codenaam
2. CHANGELOG-entry met kleurcode
3. `git tag v0.X.Y-Codename` + push
4. Flatpak build → `releases/SteamDeckMSX_Native-v0.X.Y-Codename.flatpak`
5. (Later) Upload naar Flathub of GitHub Release

## Niet-scope

- Andere platforms (Mac/Win/Android) — zie P-SDM-08 in Meta-repo
- ROMs/BIOS-distributie — zie P-SDM-05
- Steam SDK / Discord SDK — zie P-SDM-07

## Sessie-MD's

`prompts/YYYY-MM-DD_<slug>.md` met frontmatter — verplicht per Meta_Master protocol.
