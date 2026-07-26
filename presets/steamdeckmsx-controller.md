# Controller-indeling SteamDeckMSX (Steam Input)

De app kent geen eigen gamepad-driver. Op de Deck loopt alle invoer via Steam
Input, en dat is bewust: Steam Input werkt ook in Gaming Mode, overleeft
updates, en de speler kan het zelf aanpassen. Een eigen driver zou daar
bovenop komen en met Steam vechten om dezelfde knoppen.

Deze indeling maakt de galerij bedienbaar zonder toetsenbord en geeft in het
spel toegang tot het pauzemenu.

## In te stellen in Steam

Steam → SteamDeckMSX → controller-icoon → **Bewerk indeling**.

| Knop / stick | Toewijzing | Waarom |
|---|---|---|
| **Rechter joystick** | Muis (`Mouse Joystick`), versnelling laag | Cursor over de galerij; de tegels lichten op onder de cursor en zijn direct aanklikbaar |
| **R2 (rechter trekker)** | Linker muisknop | Klikken = spel starten |
| **A** | `Return` | Geselecteerde tegel starten; in het pauzemenu: de gemarkeerde knop |
| **B** | `Escape` | Terug / pauzemenu sluiten |
| **D-pad** | Pijltjestoetsen | Tegel-voor-tegel door de galerij, zonder cursor |
| **Start (☰)** | `F12` | **Pauzemenu in het spel** — verder spelen, terug naar de galerij, of afsluiten |
| **X** | `X` | Save-states |
| **Y** | `Y` | Emulator stoppen |
| **L1 / R1** | `PageUp` / `PageDown` | Snel door een grote collectie |

## Waarom Start op F12

Tijdens het spelen is het galerijvenster verborgen (anders zie je het spel niet,
BUG-022). De emulator vangt dan alle invoer. openMSX luistert op `F12` en meldt
het indrukken terug aan de app, die daarop pauzeert en het menu toont. Zonder
deze binding is er in Gaming Mode geen weg terug behalve de app doodmaken.

Kies je zelf een andere knop, map die dan óók op `F12` — de app luistert op de
toets, niet op de knop.

## Niet ingesteld

De MSX-joystickpoort zelf (spelbesturing) laat Steam Input met rust: openMSX
leest de gamepad rechtstreeks als joystick 1. Zou je die knoppen ook op toetsen
mappen, dan krijgt het spel de invoer dubbel.
