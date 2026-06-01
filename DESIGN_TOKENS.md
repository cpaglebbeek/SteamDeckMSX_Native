# DESIGN_TOKENS — SteamDeckMSX_Native

> Per `feedback_expliciete_vastlegging.md` (alle UI-projecten verplicht). Tokens als formele input voor v0.0.3 UI-code. Wijzigingen aan tokens raken de UI-look-and-feel cross-scherm; behandel als architectuur-impact (Oranje per kleurcode).

## Doelplatform-raster

| Eigenschap | Waarde | Reden |
|---|---|---|
| Resolutie | **1280 × 800** | Steam Deck LCD + OLED native |
| Pixel-density | ~215 ppi (OLED) / ~190 ppi (LCD) | Tekst moet leesbaar op 50cm afstand |
| Safe-zone | 24px marge alle randen | Overscan-vrij + Gaming-Mode chrome-vrij |
| Min interactief element | 64 × 64 px | Gamepad-cursor-vingerverwantschap |
| Standaard rij-hoogte (lijsten) | 96 px | Comfortabele D-pad-navigatie |
| Focus-ring breedte | 4 px | Zichtbaar op afstand |

## Kleur-palette — "MSX-CRT-revival"

Geïnspireerd op MSX2-systeem-paletten en CRT-fosfor-tonen. Bewust hoog contrast voor on-couch leesbaarheid.

| Token | Hex | Toepassing |
|---|---|---|
| `--color-bg-base` | `#0A0E0F` | App-achtergrond (near-black, niet pure #000 — minder OLED-burnin-risico bij statische UI) |
| `--color-bg-elevated` | `#1B2122` | Kaarten, modale dialogen |
| `--color-bg-overlay` | `rgba(10,14,15,0.85)` | OSD-overlay tijdens spel |
| `--color-fg-primary` | `#E8F1E1` | Primaire tekst (off-white met groene tint, CRT-fosfor-warm) |
| `--color-fg-secondary` | `#9BAFA1` | Secundaire tekst, labels |
| `--color-fg-disabled` | `#4A5651` | Disabled state |
| `--color-accent-primary` | `#39FF14` | "MSX-groen" — focus-ring, primaire CTA, save-indicator |
| `--color-accent-warm` | `#FFB000` | Amber-CRT — waarschuwingen, BIOS-status |
| `--color-accent-error` | `#FF3C28` | Errors, destructive actions |
| `--color-accent-info` | `#3CCBFF` | Info, links, stream-active-indicator |
| `--color-border-subtle` | `#2A3334` | 1px borders op kaarten |
| `--color-border-strong` | `#39FF14` | Focus-borders |

**Rationale "MSX-CRT-revival":** spelers van klassieke MSX-software associëren de scherm-look met CRT-fosfor (groen, amber, wit). Modern strak grijs voelt niet bij het genre. Tegelijk: geen retro-skeuomorfisme (geen scanlines in UI-chrome — alleen optioneel in de speel-render).

## Typografie

Qt6 default. Op Steam Deck zijn Noto Sans + Noto Sans Mono beschikbaar via Freedesktop runtime.

| Token | Font | Grootte | Weight | Toepassing |
|---|---|---|---|---|
| `--font-display` | Noto Sans | 32 px | 700 | Cartridge-titels in browser, scherm-headers |
| `--font-body` | Noto Sans | 18 px | 400 | Body-tekst, list-items |
| `--font-label` | Noto Sans | 14 px | 500 | Labels, hints (D-pad-icoon-bij-tekst) |
| `--font-mono` | Noto Sans Mono | 16 px | 400 | Debug-info, savestate-IDs, ROM-hashes |
| `--font-osd` | Noto Sans | 24 px | 600 | In-game OSD (groter want overlay) |

**Line-height:** `1.4` voor body, `1.2` voor display.

**Tracking:** `+0.02em` op display (kleine letter-spacing voor CRT-feel).

## Spacing-scale (4-pt grid)

| Token | Waarde |
|---|---|
| `--space-1` | 4 px |
| `--space-2` | 8 px |
| `--space-3` | 12 px |
| `--space-4` | 16 px |
| `--space-5` | 24 px |
| `--space-6` | 32 px |
| `--space-7` | 48 px |
| `--space-8` | 64 px |

Cartridge-browser uses `--space-4` tussen kaarten en `--space-3` interne padding.

## Iconen — v0.0.6 (PenguinAdventure)

Op Steam Deck zonder cursor: iconen + tekst beide. **Eigen SVG icoon-set (AGPL-compatible)**, geleverd via qt_add_qml_module RESOURCES.

### Set v0.0.6 (8 files, ~5KB totaal)

| Pad in repo | Tokens-constant | Doel |
|---|---|---|
| `src/assets/icons/dpad/up.svg`    | `Tokens.iconDpadUp`    | "Boven" hint |
| `src/assets/icons/dpad/down.svg`  | `Tokens.iconDpadDown`  | "Onder" hint |
| `src/assets/icons/dpad/left.svg`  | `Tokens.iconDpadLeft`  | "Links" hint |
| `src/assets/icons/dpad/right.svg` | `Tokens.iconDpadRight` | "Rechts" hint |
| `src/assets/icons/btn/a.svg`      | `Tokens.iconBtnA`      | A-knop = bevestigen / save / load |
| `src/assets/icons/btn/b.svg`      | `Tokens.iconBtnB`      | B-knop = terug / sluit overlay |
| `src/assets/icons/btn/x.svg`      | `Tokens.iconBtnX`      | X-knop = save-state overlay |
| `src/assets/icons/btn/y.svg`      | `Tokens.iconBtnY`      | Y-knop = stop emulator / clear slot |

### Gepland v0.0.7+
- `bumper/l1.svg` / `r1.svg` — Tab-wisseling
- `trigger/l2.svg` / `r2.svg` — Snelle-scroll-modus
- Colorize-shader voor dynamic tint (huidige: white-on-currentColor)

### Render-conventie
- ViewBox `0 0 64 64`
- Stroke = `currentColor`, fill-opacity 0.15 voor "glas-effect"
- Qt6 `Image { source: Tokens.iconXxx; sourceSize: Qt.size(W,W) }` met AOT-cache

## Componenten — basis-set v0.0.3

| Component | Tokens-gebruik |
|---|---|
| **CartridgeCard** | bg `--elevated`, focus-border `--accent-primary` 4px, padding `--space-4`, titel `--font-display` |
| **MenuList** | rij-hoogte 96px, hover/focus `--bg-elevated`, A-knop-icoon links |
| **OSD-Pause-Overlay** | bg `--bg-overlay`, content centered, knoppen 256×96px |
| **Toast/Notification** | bottom-bar, accent-warm voor saves, accent-error voor failures |
| **Settings-row** | label links (`--font-label`), control rechts, divider `--border-subtle` 1px |

## Animaties / transitions

| Token | Waarde | Toepassing |
|---|---|---|
| `--motion-fast` | 120 ms | Focus-shift, hover |
| `--motion-base` | 220 ms | Menu-overgangen |
| `--motion-slow` | 480 ms | Scherm-transities (cartridge → spel-start) |
| `--motion-easing` | `cubic-bezier(0.2, 0.0, 0.0, 1.0)` | Standaard "decel" |

Geen animaties tijdens emulatie-render (60 fps prioriteit).

## Donker-only

Geen lichte modus in v0.x — Gaming Mode-context is donker, CRT-thema werkt alleen donker.

## Niet-tokens (over architectuur)

- Geen accent-kleuren per ecosysteem-variant (Native vs Stream) — gebruikers moeten visueel niet hoeven onderscheiden
- Geen seizoenswisselingen / events — out of scope

## Wijzigings-impact-regel

Een wijziging aan `--color-accent-primary` of fonts = **Oranje** (alle schermen krijgen ander look). Een wijziging aan rij-hoogte of `--space-*` = **Oranje** (layout-shift). Een nieuw component-type met nieuwe tokens = **Groen** (additief). Hex-tweak binnen 5% delta = **Groen**.

Zie `CLAUDE.md` § Color-Coded.
