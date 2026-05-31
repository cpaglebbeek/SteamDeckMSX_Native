---
date: 2026-05-31
repo: SteamDeckMSX_Native
status: open
resume: "verder met SteamDeckMSX_Native v0.0.2 — openMSX submodule toevoegen + Qt6/GTK4-beslissing + vanilla openMSX build op Mac"
---

# Sessie 2026-05-31 — newp SteamDeckMSX_Native (variant 1)

**Agent:** Claude Opus 4.7 (1M context)
**Repo:** SteamDeckMSX_Native (`cpaglebbeek/SteamDeckMSX_Native`)
**Branche:** main
**Cross-repo werk:** Meta_SteamDeckMSX (master), zusterrepos Stream_Server / Stream_Client
**Eindstand commits:** (initial commit, hash volgt)

---

## Opdracht (samengevat)

Onderdeel van newp "native Steamdeck MSX Emulator" — deze repo is variant 1 (native Flatpak offline). Skeleton zonder code, alleen documentatie + structuur. openMSX-fork + UI-laag komen v0.0.2+.

---

## Prompts en acties — chronologisch

Zie `Meta_SteamDeckMSX/prompts/2026-05-31_newp_skeleton.md` voor de volledige conversatie. Voor deze repo specifiek:

### Actie — skeleton voor Native variant
README, CLAUDE.md (codenaam-thema MSX-game-helden, kleurcodes, build-protocol, niet-scope), .gitignore (build/, Flatpak, ROM/BIOS-uitsluiting), VERSION (`0.0.1-BubbleBobble`), ARCHITECTURE.md (componentdiagram + 4 open architectuurvragen), BUGLIST.md, CHANGELOG.md, `releases/.gitkeep`.

---

## Belangrijke keuzes deze sessie

| Keuze | Reden |
|---|---|
| Codenaam v0.0.1 = Bubble Bobble | MSX1 launch-title-icoon |
| Qt6 (voorlopig) | KDE-native op Deck Desktop; QML voor TV-stijl UI |
| openMSX als submodule onder `externals/openmsx` | P-SDM-01 upstream-first |
| Flatpak als distributie | Steam Deck Discover-route, sandboxed |
| Niet in repo: ROMs/BIOS | P-SDM-05 juridisch |

---

## Open eindjes na deze sessie

**Wacht op v0.0.2-werk:**
- openMSX submodule toevoegen (fork bij cpaglebbeek/openMSX of direct openMSX/openMSX)
- Qt6 vs GTK4 beslissing
- CMakePresets.json
- Vanilla openMSX build op Mac (cross-platform smoke-test, niet als support-target)
- Flatpak manifest stub `nl.icthorse.SteamDeckMSX.yaml`

**Klaar:**
- Skeleton-commit

**Onafhankelijk:**
- C-BIOS releases volgen — laatste stabiele versie pinnen v0.0.2
