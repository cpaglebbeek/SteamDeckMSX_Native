# openMSX control-protocol — onze interpretatie

> Bron-document voor MsxCore IPC met `openmsx -control stdio`. Live geverifieerd v0.0.4-Aleste op Mac met openMSX RELEASE_21_0 (commit `cb61db762`).
>
> **Status:** v0.0.4 leest line-based. v0.0.5 = volledige XML-stream-parser (QXmlStreamReader).

## Start-aanroep

```
openmsx -control stdio [-carta <rompath>] [-machine <name>]
```

- `-control stdio` = STDIN voor commands, STDOUT voor events (beide XML-encoded)
- `-carta <rompath>` = insert ROM in slot A bij boot (optioneel)
- `-machine <name>` = kies machine-config (default = C-BIOS_MSX2+; lijst via `machine_info`)

## STDOUT-formaat (van openMSX naar ons)

Alles in `<openmsx-output>...</openmsx-output>` stream. Binnen het stream:

### `<reply>` — antwoord op een commando

```xml
<reply result="ok" command-id="42">21.0</reply>
<reply result="nok" command-id="43">unknown command: foo</reply>
```

- `result="ok"` of `result="nok"` (= not ok)
- `command-id` matcht het ID dat wij in `<command id="N">...</command>` meesturen
- Body = output van het commando (Tcl-evaluation-result)

### `<update>` — passieve status-update (state-change)

```xml
<update type="status" name="paused">false</update>
<update type="setting" name="throttle">true</update>
<update type="led" name="caps">false</update>
```

- `type` = categorie (status / setting / led / ...)
- `name` = specifieke property
- Body = nieuwe waarde

### `<log>` — logging

```xml
<log level="warning">Could not find C-BIOS_MSX2+</log>
```

- `level` = info / warning / error

## STDIN-formaat (van ons naar openMSX)

Twee opties:
- **Plain text** — één commando per regel; openMSX parsed het als Tcl-expressie
- **XML-wrapped** — `<command id="N">cmd_with_args</command>\n` — geeft `command-id` terug in reply

In v0.0.4 wrappen we standaard XML (`MsxCore::sendCommand`) zodat replies te correleren zijn.

## Commando-set v0.0.4 (door MsxCore gebruikt)

| Commando | Doel | Gebruikt door |
|---|---|---|
| `quit` | openMSX afsluiten | `stop()` |
| `carta "<path>"` | ROM in cartridge-slot A | `loadRom()` (live state) |
| `machine_info machines` | Lijst van bekende machines (info) | v0.0.5 settings-UI |

### Geplande commando's v0.0.5+

| Commando | Doel |
|---|---|
| `set power on/off` | Reset machine |
| `set throttle on/off` | Frame-rate cap (60Hz) aan/uit |
| `savestate "<name>"` | State opslaan |
| `loadstate "<name>"` | State laden |
| `set machine <name>` | Wissel actieve machine |
| `set fullscreen on/off` | Fullscreen-mode |

## Quirks geobserveerd v0.0.4-Aleste op Mac

- **Stream-start vertraagd:** Eerste `<openmsx-output>` regel verschijnt ~150ms na proces-start (initialisatie van renderer + audio). MsxCore.state → Running pas op deze regel.
- **Stderr** krijgt ook `<openmsx-output>`-tags bij sommige bouwconfigs — we redirecten en loggen separaat, zonder state-impact.
- **`-control stdio` opent geen display-loop** zonder `display` plugin geladen — toch geeft het output bij ROM-load via stdout. Voor v0.0.4 op Mac: openMSX opent zijn eigen Cocoa-window (we kunnen niet embedden).

## Referenties

- openMSX manual: `doc/manual/console.html` + `doc/manual/commands.html` (in submodule)
- Tcl-syntax: <https://www.tcl-lang.org/man/tcl/TclCmd/contents.htm>
- Onze MsxCore parse-logica: `src/MsxCore.cc` `parseLine()`

## Wijzigings-impact-regel

Aanpassing van dit document zonder MsxCore.cc te updaten = **Geel-bug** (drift tussen protocol-spec en implementatie). Beide moeten samen wijzigen.
