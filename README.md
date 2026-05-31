# SteamDeckMSX_Native

**Variant 1** van het [SteamDeckMSX](https://github.com/cpaglebbeek/Meta_SteamDeckMSX) ecosysteem: offline MSX-emulator als Flatpak voor de Valve Steam Deck.

Fork van [openMSX](https://github.com/openMSX/openMSX) met een eigen Deck-UI-laag (cartridge-browser, OSD, Steam Input-mapping).

## Status

- **Versie:** v0.0.1-BubbleBobble (skeleton)
- **Fase:** documentatie + structuur. Geen runnable code. openMSX-submodule volgt v0.0.2.
- **Codenaam-thema:** MSX-game-helden — zie [Meta_SteamDeckMSX/CLAUDE.md](https://github.com/cpaglebbeek/Meta_SteamDeckMSX)

## Plan

1. **v0.0.1** — Skeleton + docs (deze release)
2. **v0.0.2** — openMSX submodule + Qt6/GTK4-beslissing + lokale build van vanilla openMSX
3. **v0.0.3** — Cartridge-browser (read-only) + gamepad-navigatie
4. **v0.1.0** — In-game OSD + save-states + Flatpak-manifest
5. **v0.2.0** — Stream-detectie (zie variant 2)

## Bouwen (toekomst)

```bash
# v0.0.2+
cmake --preset steamdeck-release
cmake --build build
```

## Licentie

AGPL-3.0 — zie [LICENSE](LICENSE).

openMSX (upstream) = GPL-2.0; AGPL-3.0 is een upgrade-compatible licentie voor onze meta-laag. C-BIOS = BSD-3-Clause.

## Niet inbegrepen

- Microsoft MSX BIOS (juridisch verboden in de repo) — gebruiker importeert eigen BIOS
- Cartridge-images (`.rom`/`.dsk`) — geen ROM-distributie

Zie [Meta_SteamDeckMSX/docs/PRINCIPLES.md#P-SDM-05](https://github.com/cpaglebbeek/Meta_SteamDeckMSX/blob/main/docs/PRINCIPLES.md) voor de juridische rationale.
