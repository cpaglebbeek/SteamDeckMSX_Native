# Ontwerp — spellen zoeken en ophalen vanuit de galerij

Status: **ontwerp, nog niet gebouwd** (v0.5.0-kandidaat)
Aanleiding: vanuit de galerij willen kunnen zoeken op een titel, met een
schermtoetsenbord, en het gevonden bestand direct in de bibliotheek krijgen.

## Wat er al ligt

De bouwstenen bestaan grotendeels:

| Onderdeel | Bestaat | Waar |
|---|---|---|
| Downloaden via URL | ja | `FileDownloader` — HTTPS-only, size-cap, atomic `.part`-rename (DD-001/DD-011) |
| ROM opnemen in de collectie | ja | `CartridgeModel::addFromUrl` + `UrlImportDialog` |
| Bibliotheek bijwerken | ja | `RomLibrary::rescan()` — incrementeel, hash-gebaseerd |
| Tegel genereren | ja | `ThumbnailGenerator` |
| Bedienbaar zonder toetsenbord | deels | `MenuButton` (v0.4.0); een schermtoetsenbord ontbreekt |

Wat ontbreekt is dus vooral **vinden**: een lijst om in te zoeken, en een manier
om letters in te voeren zonder toetsenbord.

## Bron: één plat indexbestand

`download.file-hunter.com` blijkt zijn volledige index als één tekstbestand te
publiceren (`/allfiles.txt`), één bestandsnaam per regel. De HTML-directorylijst
zelf is JavaScript-gehydrateerd en dus nutteloos voor een fetch — de site laadt
diezelfde tekstindex in zijn eigen zoekscript (`assets/search.js`).

Dat is gunstig: geen HTML-parser, geen scraper die breekt bij een
opmaakwijziging, geen anti-bot-omweg nodig (gewone `curl` krijgt HTTP 200).
Eén bestand ophalen, lokaal cachen, lokaal doorzoeken.

**Ontwerpkeuze:** de index periodiek ophalen (bijv. één keer per dag, plus een
handmatige verversing) en op schijf bewaren. Zoeken gebeurt dan volledig offline
en direct — belangrijk op een handheld, waar elke netwerk-round-trip tijdens het
typen als traagheid voelt.

## Schermtoetsenbord

Qt levert geen bruikbaar schermtoetsenbord in deze opstelling: `QtVirtualKeyboard`
zit niet in de KDE-runtime van de Flatpak, en Steam's eigen toetsenbord verschijnt
alleen in Gaming Mode en niet betrouwbaar over een niet-Steam-venster.

Daarom een eigen, klein raster-toetsenbord in QML, met dezelfde `MenuButton`-basis
als het pauzemenu:

- 6×6 raster: A-Z, 0-9, spatie, backspace
- Navigatie met d-pad én met de cursor van de rechter joystick (hover volgt de
  selectie, zoals in de galerij)
- Filteren terwijl je typt: bij drie letters is de lijst meestal al kort genoeg
- Geen shift/interpunctie in v1 — titels zoeken werkt hoofdletterongevoelig

## Twee manieren om te vinden, één manier om binnen te halen

1. **Zoeken** — je typt een paar letters, je krijgt een gefilterde lijst.
2. **Bladeren** — dezelfde lijst zonder filter, per map (MSX1 / MSX2 / MSX2+),
   zodat je ook iets kunt vinden waarvan je de naam niet weet.

Beide leiden naar dezelfde actie: het gekozen bestand gaat via de bestaande
`FileDownloader` naar de ROM-map, waarna `RomLibrary::rescan()` het oppikt en
`ThumbnailGenerator` er een tegel bij maakt. Er is dus geen aparte
"download-bibliotheek": wat je ophaalt is meteen een gewoon spel in de galerij.

## Wat eerst gebouwd moet worden

1. `OnlineIndex` (C++): index ophalen, cachen, doorzoeken. Teruggeven als model.
2. `OnScreenKeyboard.qml`: raster-toetsenbord op `MenuButton`-basis.
3. `OnlineBrowser.qml`: zoekveld + lijst + downloadknop, opgeroepen vanuit de
   galerij met een eigen knop in de header.
4. Koppeling: gekozen item → `FileDownloader` → ROM-map → `rescan()`.

## Randvoorwaarden die niet vergeten mogen worden

- **Netwerk in de sandbox.** Het manifest heeft `--share=network`, dus dit kan.
  Let op: `ARCHITECTURE.md` beschrijft nog de oorspronkelijke offline-first-opzet
  ("GEEN `--share=network`"); die passage is achterhaald en moet meebewegen als
  dit gebouwd wordt.
- **Alleen HTTPS**, size-cap en atomic write — die regels zitten al in
  `FileDownloader` en moeten niet omzeild worden voor deze bron.
- **Auteursrecht.** Dit is de reden dat het project zelf geen ROM's meelevert
  (P-SDM-05, alleen C-BIOS). Een functie die naar een externe bron bladert is
  iets anders dan zelf verspreiden, maar veel MSX-software is nog beschermd. Het
  ontwerp laat de keuze daarom expliciet bij de gebruiker: geen automatische of
  bulk-downloads, één titel per handeling, en geen spiegeling van de bron op
  eigen infrastructuur.
- **De index is geen catalogus.** `allfiles.txt` bevat bestandsnamen, geen
  metadata — geen jaartal, uitgever of machinetype. Het machinetype leidt de app
  al zelf af (`RomTypeDetector`), maar sorteren op iets anders dan naam kan niet
  zonder aanvullende bron.

## Stand van zaken na v0.5.0 (geïmplementeerd, deels geverifieerd)

Gebouwd: `OnlineIndex` (ophalen, cachen, offline filteren), `OnScreenKeyboard.qml`
(raster A-Z/0-9, spatie, wissen) en `OnlineBrowser.qml`, met een "Zoeken"-knop in
de header. Downloaden loopt via de bestaande `CartridgeModel::addFromUrl`, en de
opslagmap wordt na afloop als scanroot geregistreerd — zonder dat blijft een net
opgehaald spel buiten de galerij omdat er nooit in die map gekeken wordt.

**Wel geverifieerd:** de index is bereikbaar zonder anti-bot-omweg (HTTP 200),
bevat enkele duizenden regels, en de site construeert zijn eigen downloadlink als
`https://download.file-hunter.com/` + de indexwaarde — exact wat `OnlineIndex`
doet. Build groen, 6/6 tests, app start zonder QML-meldingen.

**Niet geverifieerd, en dat is een echt gat:** er is geen geslaagde
end-to-end-download gedaan. Twee losse eindjes die daarbij opvielen:

1. De regels in het opgehaalde fragment bevatten géén mapnamen (geen `/`),
   terwijl de bron wel mappen heeft. Of de index verderop wél paden bevat, of
   dat de root plat is, is niet vastgesteld.
2. Het bestand gebruikt Windows-regeleinden. De parser vangt dat op met
   `trimmed()`, maar shell-controles erop gaven tegenstrijdige tellingen — reden
   te meer om de download één keer echt te draaien in plaats van af te leiden.

Volgende stap is dus niet meer bouwen maar meten: één titel kiezen in de UI,
downloaden, en controleren dat het bestand in de ROM-map landt, dat de scan hem
oppikt en dat er een tegel bij komt. Pas daarna mag dit als werkend gelden.
