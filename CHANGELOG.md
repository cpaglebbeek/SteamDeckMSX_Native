# CHANGELOG — SteamDeckMSX_Native

## v0.3.1-MazeOfGalious (2026-07-25) — de galerij bleef leeg op de Deck

> Melding van de gebruiker: "scan levert geen spellen op". De scanner werkte,
> maar zocht in mappen die binnen de Flatpak-sandbox niet bestaan.

- **Oorzaak (bevestigd, niet vermoed):** in de sandbox is de home-map leeg op wat
  expliciet gemount is. `ls /root` binnen de sandbox toonde alleen `.local` en
  `.var` — `~/ROMs` en `~/Downloads` bestonden er domweg niet. Ook
  `--filesystem=xdg-download:ro` hielp niet: dat resolveert naar niets op een
  systeem zonder XDG-user-dirs, en `/run/media` bestond evenmin. Elke scanroot
  faalde dus stil op `QFileInfo::exists()`, en de galerij bleef per definitie leeg
  terwijl de scanner zelf correct werkte.
- **Fix:** `--filesystem=home:ro` (vervangt xdg-download/xdg-documents) en de
  home-map zelf als scanroot in plaats van een lijstje vermoede submappen. Read-only,
  en veilig omdat de scanner alleen `.rom/.dsk/.cas` leest, verborgen en zware
  mappen overslaat en niets schrijft. Dit is ook wat andere emulator-frontends doen.
- **Tegels verschijnen nu tijdens de scan** in plaats van pas aan het eind. Een
  home-scan duurt tientallen seconden; alles ophouden tot `finishScan()` liet de
  galerij al die tijd leeg — voor de gebruiker niet te onderscheiden van "er is
  niets gevonden". Entries worden nu direct op hun alfabetische plek ingevoegd,
  en aan het eind wordt alleen nog opgeruimd wat van schijf verdwenen is. Bijwerking:
  geen lege galerij meer tijdens een rescan.
- **Tick-budget gold niet binnen een map** (gevonden door een nieuwe test): de
  lus controleerde het budget alleen tússen mappen en hashte zo een complete map
  in één tick. Een MSX-collectie is vaak precies dat — één map met duizenden
  ROMs — dus de UI blokkeerde alsnog. Er is nu een wachtrij die het budget ook
  binnen een map respecteert.
- **Zelf een map aanwijzen**: sneltoets `M` opent een mapkiezer die de map als
  scanroot toevoegt en meteen opnieuw scant. Vangnet voor collecties op
  ongebruikelijke locaties.
- **De lege staat toont nu in welke mappen is gezocht.** Zonder dat is "niets
  gevonden" niet te onderscheiden van "op de verkeerde plek gezocht" — precies
  waarom deze bug pas op de Deck aan het licht kwam.
- Scan van dezelfde home-map: **21s → 12s**. Tests: **16 cases**, suite 6/6 groen.

## v0.3.0-MazeOfGalious (2026-07-25) — ORANJE: galerij met alle lokale spellen

> Tot nu toe moest elk spel handmatig geïmporteerd worden en toonde de app een
> lijstje van maximaal 8 recente items. Vanaf nu scant de app bij elke start
> alle lokale ROM-mappen en presenteert alles als tegelgalerij met echte
> gamebeelden — Homebrew-Channel-stijl.

- **RomLibrary** (`src/RomLibrary.{h,cc}`) — volledige bibliotheek naast de
  bestaande recents-lijst. Scant `.rom/.dsk/.cas/.zip` recursief (max 6 niveaus
  diep) in: eigen storage, Downloads, Documenten, `~/ROMs`, `~/roms`,
  `~/Games/MSX`, `~/MSX` en op de Deck `/run/media/*` (SD-kaart).
  - Scan loopt **incrementeel op de UI-thread** via een 0ms-timer, 24 bestanden
    per tick. Bewust geen threads: geen races op het model, deterministisch
    testbaar, en de UI blijft responsief.
  - **SHA-1 alleen bij wijziging**: entries met ongewijzigde (mtime, size) komen
    uit de JSON-cache, dus een rescan van een ongewijzigde map kost vrijwel niets.
  - **Dedup op SHA-1** — dezelfde dump in twee mappen is één tegel.
  - **Titel-opschoning**: `Nemesis 2 (1987)(Konami)[SCC].rom` → `Nemesis 2`.
    Scene-dumps zijn anders onleesbaar op een tegel.
  - Verdwenen bestanden worden bij het laden van de cache overgeslagen — een
    tegel die niet start is erger dan een tegel die ontbreekt.
- **ThumbnailGenerator** (`src/ThumbnailGenerator.{h,cc}`) — echte screenshots als
  tegelbeeld. Start openMSX per ROM, laat 7s emuleren en laat de emulator zelf
  via Tcl een PNG wegschrijven.
  - **`SDL_VIDEODRIVER=offscreen`** is de kern: SDL rendert zonder venster terwijl
    het screenshot-commando gewoon werkt. Empirisch vastgesteld op HC55 —
    beeld identiek aan een run mét venster. Zonder deze driver flitst er per ROM
    een venster op tijdens het bladeren.
  - Strikt serieel (één emulator tegelijk) + kill-timeout per ROM, zodat één
    ROM die niet boot de wachtrij niet blokkeert.
  - Succes wordt afgemeten aan het PNG-bestand, niet aan de exitcode: openMSX
    kan met code 0 eindigen zonder screenshot én met een fout nádat de PNG er al is.
- **GameGrid.qml + GameTile.qml** — de galerij. Vier tegels per rij op 1280px,
  focus door opschalen + gloeiende rand (geen selectiebalk), badge voor DSK/CAS,
  en volledige knopnavigatie (dpad, L1/R1 per pagina, Home/End, A start).
  Tegels zonder screenshot krijgen een uit de SHA-1 afgeleide kleurverloop met de
  titel, zodat het grid tijdens de eerste scan nooit leeg of kapot oogt.
- **Main.qml** — galerij vervangt de recents-lijst als hoofdweergave; nieuwe
  sneltoetsen `O` (bestand openen) en `R` (opnieuw scannen); voortgangsindicator
  voor scan en tegels in de header. De `S`-sneltoets leest nu benoemde velden via
  `entryAt()` in plaats van harde rol-nummers (`Qt.UserRole + 2`), die stil
  verschuiven zodra het model een rol krijgt.
- **Flatpak finish-args** — `--filesystem=xdg-download:ro`, `xdg-documents:ro` en
  `/run/media:ro`. Zonder deze rechten ziet de sandbox alleen zijn eigen storage
  en blijft de galerij leeg. Bewust géén `--filesystem=home`.
- **Tests**: `test_romlibrary` met 9 cases (extensie-filter, subdirectories,
  dedup, titel-opschoning, mediatype, thumbnail-queue, behoud van thumbnails over
  een rescan, lege map, niet-bestaande map). Suite: **6/6 groen**.
- **Codenaam-drift voorkomen**: `STEAMDECKMSX_VERSION_CODENAME` is een CMake
  *cache*-variabele — na de bump bleef de build `KingsValley` melden tot expliciet
  herconfigureren. Dit is precies BUG-008; de app-launch-gate ving het.

### Drie problemen die pas op een échte machine zichtbaar werden

De eerste versie was groen op tests én sandbox, maar viel om zodra hij op een
gewone werkmachine met echte mappen draaide. Alle drie zijn opgelost en met
regressietests vastgelegd (`test_romlibrary`, nu 14 cases):

1. **Scan blokkeerde de start (45s)** — de mappenboom werd volledig geënumereerd
   vóór de eerste tick, dus de incrementele verwerking hielp niets. Bij een
   scanroot met een grote boom (Documenten met repo's) stond de app seconden
   stil. Nu wordt de boom *tijdens* de ticks afgelopen via een dir-stack.
2. **Zware mappen liepen de scan dood** — `.git`, `node_modules`, `Library` en
   verborgen mappen worden overgeslagen. Scan van dezelfde machine: **45s → 5s**.
3. **159 valse treffers uit Documenten** — `.zip` gold overal als speelbaar
   bestand, waardoor juridische dossiers, WeTransfer-bundels en screenshots als
   "spel" in de galerij verschenen. Nu: Documenten staat niet meer in de
   standaard scanroots (privacy + ruis), en een `.zip` telt alleen mee als de map
   waarin hij *direct* staat zich als ROM-map aankondigt (naam bevat `rom`/`msx`,
   of heet `games`). Bewust alleen de directe map: één bovenliggende map die
   toevallig "msx" bevat zou anders élke zip eronder accepteren — dat gebeurde
   letterlijk in de test, waar de tijdelijke map de applicatienaam draagt.
   Resultaat op dezelfde machine: **159 → 4 treffers**, allemaal echte ROMs.

## v0.2.1-KingsValley — update 2026-07-25: EERSTE GESLAAGDE FLATPAK-BUILD (HC55)

> Na 5 gefaalde builds op de Deck + debugronde 24-7: `SteamDeckMSX-v0.2.1-KingsValley.flatpak`
> (22 MB, x86_64) gebouwd op HC55 met flatpak-builder, KDE runtime 6.7.
> Gehost op https://horsecloud55.ddns.net/steam/flatpak/ voor Deck-install.

- **BUG-011 (geel) OPGELOST:** openMSX-probe in KDE-SDK-sandbox — Tcl 8.6.16-module +
  GLEW 2.2.0-module + TCL_CONFIG/LIBRARY_PATH + GLEW-linkpad-fork-patch (RCA retroactief
  in BUGLIST; fixes van 24-7 bevestigd werkend: volledige compile + link)
- **BUG-012 (geel):** install-stap verwees naar `bindist/` (bestaat alleen bij `make
  bindist`) → hele `share/.`-tree + `Contrib/cbios` (XML's→machines/, ROM's→systemroms/)
  naar `/app/share/openmsx/`; finish-arg `--env=OPENMSX_SYSTEM_DATA=/app/share/openmsx`
  (layout conform OpenmsxLocator kandidaat 2)
- **BUG-013 (geel):** KDE-SDK 6.7 levert geen Qt6CorePrivate/GuiPrivate cmake-packages →
  OPTIONAL_COMPONENTS + fallback op `<Module>_PRIVATE_INCLUDE_DIRS` (Mac-smoke 5/5 groen)
- **BUG-014 (groen):** `--device=input` vereist flatpak ≥1.15.6; HC55 = 1.14.6 →
  `--device=all`
- **Sandbox-verificatie op HC55:** install-smoke via `flatpak install --user`; beide
  binaries aanwezig (`steamdeckmsx` 813K + `openmsx` 10,8M), 19 C-BIOS-ROM's in
  systemroms/, C-BIOS machine-XML's, volledige share-tree; `openmsx --version` draait
  in de sandbox → "openMSX 21.0, components ALSAMIDI CORE GL LASERDISC"
- **BUG-015 (geel):** bundle zonder `--runtime-repo` → Deck kon org.kde.Platform//6.7
  niet vinden; runtime-repo=flathub ingebakken + buildscript vastgelegd
  (`deploy/run-build-hc55.sh`)
- **BUG-016 (geel):** app-launch was op ALLE platforms kapot (v0.2.x smoke was
  tests-only): custom `closed()`-signaal botste met Popup (Qt 6.7 hard error) +
  core-types uit static lib registreerden onbetrouwbaar → core is nu eigen
  QML-module `SteamDeckMSX.Core` met `IMPORTS` in de app-module; offscreen-launch
  (`QT_QPA_PLATFORM=offscreen`) toegevoegd als vast smoke-gate
- **BUNDLE-VERIFICATIE GROEN (25-7 20:28, HC55):** de herbouwde bundle mét BUG-016-fix is
  als distributie-artefact end-to-end geverifieerd — niet de dev-build maar de `.flatpak`
  zelf, schoon geherinstalleerd:
  1. `flatpak install --user` groen (na geforceerde uninstall, zodat écht het nieuwe
     artefact getest wordt en niet stilzwijgend de vorige install)
  2. **app-launch offscreen: 20s stabiel, nul output** — BUG-016 hiermee bewezen opgelost
     in het gedistribueerde artefact, niet alleen op Mac
  3. **C-BIOS 0.29 boot in de sandbox** (screenshot) — bewijst de BUG-012/BUG-004-keten
     (volledige share-tree + OPENMSX_SYSTEM_DATA + locator-layout) werkend op runtime
  4. **Cartridge end-to-end**: nemesis2.rom → Konami-logo (t=7s) → intro-cutscene (t=18s)
     — volledige emulatiepijplijn binnen Flatpak, inclusief mapper-detectie
  Bewijs-screenshots: `docs/verification/v0.2.1/`. Gate vastgelegd als herbruikbaar script
  `deploy/verify-flatpak-hc55.sh` (4 stappen, elk fataal) — directe invulling van de
  BUG-016-les dat "smoke groen" een echte app-launch moet bevatten.
- **Nog open (Deck):** echte install + Gaming Mode + gamepad/Steam Input + de
  Nemesis-URL-download-flow via de UI op de Steam Deck zelf. Emulatiekern en app-start
  zijn nu op x86_64-Linux bewezen; wat op de Deck rest is hardware- en sessie-specifiek
  (input, GPU-renderer, Gaming Mode-integratie).

## v0.2.1-KingsValley (2026-07-24) — Mac smoke-test + compile-fixes: v0.2.0 bouwt voor het eerst (groen)

> v0.2.0-TreasureOfUsas was geleverd zónder build (4-agent + solo, per user-besluit).
> Deze release maakt hem compileerbaar en bewijst 5/5 testbinaries groen (3× stabiel).

- **BUG-009 (geel):** Qt 6.11 verplaatste `qzipreader_p.h` van QtGui naar QtCore →
  `__has_include`-fallback in BiosZipExtractor.cc + `Qt6::CorePrivate` naast GuiPrivate
  in src/CMakeLists.txt (blijft compatibel met oudere Qt in Flatpak-runtime);
  plus ontbrekende `#include <QRegularExpression>`
- **BUG-010 (groen):** flaky `msxcore_smoke` — 1s-wachtlus (10×100ms) te krap onder
  buildload; verruimd naar 5s (4 plekken in test_msxcore.cc)
- Restscope v0.2.1-plan (auto-setCurrentMachine, commandFinished, slot B-floppy-UI,
  MIME-detect, tests-backfill, ROM-header parse) → verschoven; prioriteit user:
  Flatpak-route naar werkende Deck-app met Nemesis 1+2

## v0.2.0-TreasureOfUsas (2026-06-08) — SoftwareDb + ZIP-extract + DSK/CAS + thumbnails + Ascii8/16 + drag-and-drop (ORANJE)

> **Tweede 0.x release.** User-verzoek: "ga verder met bouwen. gebruik zoveel
> agents als nodig voor het verdelen van load en context. ga er vanuit dat alles
> gaat werken en vergeet tussentijds testen". 4 agents parallel + ik orchestratie.

### Feature 1 — `SoftwareDb` class (openMSX softwaredb-hash lookup)
- `src/SoftwareDb.{h,cc}` — QObject met QML_ELEMENT
- `Q_INVOKABLE loadFromXmlFile(QString)` — stream-based `QXmlStreamReader` (geen DOM voor 10MB+ files)
- `Q_INVOKABLE addEntry(sha1, machine, title)` — in-memory bootstrap voor tests
- `Q_INVOKABLE lookupMachine(sha1) → QString` / `lookupTitle(sha1) → QString` (case-insensitive)
- `Q_INVOKABLE loadBootstrapData()` — 5 sample-entries als smoke-test
- XML-parse: één `<software>`-blok = N entries (meerdere dump-varianten/regio's per game), `algo="sha1"` only, system → machine mapping (MSX/MSX2/MSX2+ → C-BIOS_MSX*)
- `tests/test_softwaredb.cc` — addEntry roundtrip, case-insensitive lookup, bootstrap-load, XML-fixture parse, clearAll

### Feature 2 — `BiosZipExtractor` (ZIP-archief BIOS-sets)
- `src/BiosZipExtractor.{h,cc}` — gebruikt `<QtGui/private/qzipreader_p.h>` (private API, ABI stabiel sinds Qt5)
- `Q_INVOKABLE extractTo(zipPath, destDir, fileNamesOut) → int` (aantal succes)
- Accept-extensies: `.rom`, `.sys`, `.bin` (case-insensitive)
- Per-file cap 1 MiB + hard cap 64 files/zip (anti-zip-bomb)
- Path-traversal-veiligheid: `..`-segmenten, absolute paden, drive-letters geweigerd
- Atomic write via `.part` + rename (consistent met FileDownloader)
- Dedupe binnen ZIP: `_2`, `_3` suffix bij dubbele basenname
- Symlinks/non-files via `isFile`-check geweigerd
- Signals: `fileExtracted`, `skipped(name, reason)`, `parseError`
- `CMakeLists.txt`: `Qt6::GuiPrivate` PUBLIC-linked op core lib

### Feature 3 — DSK + CAS media support in `MsxCore`
- `Q_INVOKABLE loadDsk(QString path, int drive)` → Tcl `diska "<path>"` of `diskb "<path>"`
- `Q_INVOKABLE loadCas(QString path)` → Tcl `cassetteplayer insert "<path>"`
- `Q_INVOKABLE ejectDsk(int drive)` / `ejectCas()` → `eject`
- Beide vereisen `Running`-state (anders warning + return -1)
- `CartridgeModel::mediaTypeFor(path)` static helper: extensie → "rom"/"dsk"/"cas"/"zip"
- `CartridgeModel` nieuwe `MediaTypeRole` in roleNames → QML kan `mediaType`-binding gebruiken
- `Main.qml` `CartridgeBrowser.onActivated` + `romPicker.onAccepted` routen per extensie:
  - `.dsk` → `start("")` (C-BIOS boot) als niet-running, dan `loadDsk(path, 0)`
  - `.cas` → idem met `loadCas`
  - `.rom` → bestaande `start(path)` blijft

### Feature 4 — Save-state thumbnails
- `SaveStateModel` nieuwe `ThumbnailPathRole` + `thumbnailPath`-veld per slot
- `Q_INVOKABLE requestThumbnail(int slot)` → openMSX `screenshot -prefix <dir>/slot_N` Tcl-cmd
- `Q_INVOKABLE thumbnailFor(int slot)` → absoluut pad (lege string als file niet bestaat)
- Storage: `QStandardPaths::AppDataLocation/savestates/thumbs/slot_N.png`
- QSettings persist uitgebreid met `thumbnail`-veld per slot
- `SaveStateCard.qml`: `thumbnailPath`-property + `Image`-laag (opacity 0.35, PreserveAspectCrop, cache:false)
- `SaveStateOverlay.qml` delegate-binding `thumbnailPath: model.thumbnailPath || ""`
- **Bekende beperking**: async timing-race tussen screenshot-write en QImage-load (geen reload-trigger v0.2.0 — herstart overlay vernieuwt; v0.2.1 plant `commandFinished`-signal vanuit core)

### Feature 5 — Ascii8 + Ascii16 mapper-detect in `RomTypeDetector`
- `static bool hasAscii8(rom)` — bank-switch write naar 0x6800/0x7000/0x7800 (≥2 van 3 hits)
- `static bool hasAscii16(rom)` — bank-switch write naar 0x6000 EN 0x7000
- `detectMapper`-prioriteit: SCC > ASCII16 > ASCII8 > Konami-default (ASCII16 voorrang voorkomt false-positive)
- `detect()` MSX2-reason gebruikt nu `mapperName(mapper)` — uniforme readout
- `tests/test_romtypedetector.cc`: T12 (ASCII8) + T13 (ASCII16-voorrang), 11 → 13 cases

### Feature 6 — Drag-and-drop ROM/BIOS in UI
- `CartridgeBrowser.qml` `DropArea { keys: ["text/uri-list"] }` + `filesDropped(urls)`-signal
- `BiosManagerScreen.qml` idem
- Visual feedback: `accentInfo` overlay bij `containsDrag`
- `Main.qml` routes drops:
  - Browser: `cartridges.addFromLocal(p, false)` per URL
  - BIOS-screen: `.zip` → `biosZipExtractor.extractTo()`, anders → `bios.addFromLocal()`

### Feature 7 — `presets/msx-browser.vdf` (Steam Input launcher-preset)
- D-pad navigatie, A=RETURN, B=B, X=X (SaveState), Y=Y (Stop)
- L1/R1 = PageUp/PageDown (lijst-jump 5 items)
- L2/R2 = Home/End
- Select = I (BIOS-screen), Start = U (URL-add ROM)
- Steam wisselt automatisch met in-game preset (`Stream_Client/presets/msx-gamepad.vdf`)
- English + Dutch lokalisatie

### Code-orchestratie
- **4 agents parallel** voor geïsoleerde componenten (SoftwareDb, BiosZipExtractor, RomTypeDetector Ascii uitbreiding, SaveStateModel thumbnails)
- **Solo**: MsxCore DSK/CAS API, CartridgeModel mediaType, drag-and-drop QML, Main.qml-wiring, presets/msx-browser.vdf, CMakeLists integratie (alle agents werkten zonder Cmake-edits — ik consolideerde)
- **Geen tussentijds testen** per user-instructie. Code-correctheid berust op static type-check + agent design-review + bestaande 28 ctest-cases die in v0.1.0 al groen waren.

### Codenaam — TreasureOfUsas
Konami MSX2 1987 platformer met dual-character mechanic. Past bij dual-thema:
4 agents tegelijk + 2 cart slots + 2 floppy drives + 2 media-routes — "dubbel" overal.

### Kleurcode: ORANJE (+0.1.0)
2 nieuwe core-componenten (SoftwareDb + BiosZipExtractor) + MsxCore-uitbreiding + SaveStateModel-uitbreiding + RomTypeDetector-uitbreiding + 2 QML-files uitgebreid + nieuwe Steam Input preset. Cmake +`Qt6::GuiPrivate`. Geen breaking change.

### Wat NIET in v0.2.0
- Auto-`setCurrentMachine` via SoftwareDb (vereist accuratesse-rapport — v0.2.1)
- Pause/resume + parallelle downloads (v0.3.0)
- Tab-strip Main.qml-restructuur (v0.3.0; huidige modal-flow blijft)
- SettingsScreen voor BIOS-dir/openMSX-pad config (v0.3.0)
- Flatpak-build op Deck (Stap 21, 6e poging — vereist Deck SSH)
- Auto-`screenshot`-trigger na save (timing-race — v0.2.1 met commandFinished signal)
- Unit-tests voor BiosManager / FileDownloader / SaveStateModel-thumbnails / BiosZipExtractor / DSK/CAS — alleen RomTypeDetector + SoftwareDb hebben tests (v0.2.1+)

## v0.1.0-Xanadu (2026-06-08) — BIOS-bibliotheek + URL-downloads + 2 cart slots (ORANJE)

> **Eerste 0.1.x release.** Op user-verzoek 3 features ineens, met **alle ontwerp-
> beslissingen gedocumenteerd in `docs/DESIGN_DECISIONS_v0.1.0.md`** (DD-001 t/m
> DD-016, ieder met opties + omkeerbaarheid + tijdsschatting).

### Feature 1 — BIOS-bibliotheek (DD-007)
- `BiosManager` (QAbstractListModel, in core lib):
  - `addFromUrl(QUrl, QString preferredName)` → HTTPS async-download
  - `addFromLocal(QString path, QString preferredName)` → kopieer naar storage
  - `removeEntry(id)` / `clearAll()`
  - Persistentie via QSettings (`BiosManager/entries/...`)
  - Storage: `QStandardPaths::AppDataLocation/bios/` (Mac/Linux/Flatpak portable)
  - Per entry: id (SHA-1[12]), fileName, absPath, sha1Hex, sizeBytes, addedAt, source
  - Max-size cap 1 MiB (DD-003)
- `BiosManagerScreen.qml` — modaal Popup met lijst + 2 Add-knoppen + remove
- Shortcut **I** opent screen (DD-010)

### Feature 2 — URL + lokaal-import voor ROMs én BIOS (DD-001/002/008/011/012)
- `FileDownloader` herbruikbare async fetcher in core lib:
  - **HTTPS-only** (DD-001) — HTTP geweigerd voor veiligheid
  - **NoLessSafeRedirectPolicy** (DD-002) — HTTPS→HTTPS automatic, downgrade naar HTTP geweigerd
  - 30s timeout (DD-004), max-size cap per call (DD-003)
  - **Atomische write** via `.part` + rename (DD-011)
  - SHA-1 berekend via `RomTypeDetector::sha1Hex` (single-source helper)
  - Q_INVOKABLE `start(url, destPath, maxBytes)` + `cancel()`
  - Signals: `progress`, `finished(path, sha1)`, `failed(reason)`
- `UrlImportDialog.qml` — gedeeld voor BIOS én ROM via `target` property (DD-008):
  - URL-input + optionele naam + progress-bar + cancel
  - Auto-focus URL-field bij open
- `CartridgeModel` uitgebreid:
  - `addFromUrl(QUrl, QString preferredName)` — HTTPS download → storage/roms/
  - `addFromLocal(QString path, bool copyIntoStorage)` — copy of register-in-place
  - `addRom(path)` blijft (backward-compat alias) — registreert zonder kopie (DD-015)
  - `kRomMaxBytes = 8 MiB` (DD-003)
  - Entry extra velden: `sha1Hex` + `source` ("url:..." of "local:...")
- Shortcut **U** opent URL-dialog voor ROM (DD-010)
- Filename-sanitatie: `/` en `\` → `_` (DD-012)

### Feature 3 — 2 cart slots (Slot A + Slot B) (DD-009/014)
- `MsxCore` uitgebreid:
  - Q_PROPERTY `slotARom`, `slotBRom` + change-signals
  - Q_INVOKABLE `loadRomSlotA(path)` → `carta "<path>"` Tcl-cmd (= primaire, oude `loadRom` is nu alias)
  - Q_INVOKABLE `loadRomSlotB(path)` → `cartb "<path>"` — **alleen tijdens Running** (DD-014)
  - Q_INVOKABLE `removeRomSlotA()` / `removeRomSlotB()` → `carta/cartb eject`
- `SlotPickerDialog.qml` — modaal: knop A (altijd) + knop B (alleen tijdens Running met hint)
- Shortcut **S** opent slot-picker voor huidige cartridge in browser (DD-010)
- Slot-status onderaan in Main.qml: `Slot A: bubblebobble.rom / Slot B: (leeg)`

### Andere wijzigingen
- `Main.qml` uitgebreid:
  - `BiosManager { id: bios }` + signal-wiring (toast bij add/fail/progress)
  - `CartridgeModel.onDownloadFinished/Failed/Progress` toast + dialog-state
  - Hint-strip "I · BIOS    U · URL-ROM    S · Slot" in header
  - 3 Shortcuts I/U/S
- `CartridgeModel` entry: `sha1Hex` + `source` velden in QSettings persist + roleNames
- `src/CMakeLists.txt`: 4 nieuwe core-files + 3 nieuwe QML-files toegevoegd
- Codenaam **Xanadu** verschoven uit vrije pool naar Toegewezen in Meta-repo

### Mac smoke-test 2026-06-08
```
cmake --preset native-debug -DSTEAMDECKMSX_BUILD_TESTS=ON
cmake --build build/native-debug
ctest --output-on-failure
→ 100% tests passed, 0 failed out of 4 (28 testcases)
→ steamdeckmsx_app: 42/42 ninja-stappen groen
```

### Wat NIET in v0.1.0 (zie DESIGN_DECISIONS § einde voor volledige lijst)
- ZIP-archief extract voor BIOS-sets → v0.1.1
- Drag-and-drop ROM/BIOS → v0.1.1
- SoftwareDb-class + auto-machine.xml → v0.2.0
- `.dsk`/`.cas`/`.zip` ondersteuning in browser → v0.1.1
- Pause/resume download → v0.2.0
- Parallelle downloads → v0.2.0
- Tab-strip Library/BIOS/Settings — vervangen door modal-Popup-flow (eenvoudiger)
- Flatpak-build op Deck (Stap 21, 6e poging) — vereist Deck SSH (apart blokkerend item)
- Thumbnails save-state — verschoven naar v0.2.0
- Tests voor BiosManager/FileDownloader/slot-API — bestaande 28 cases ongewijzigd groen,
  nieuwe tests verschoven naar v0.1.1 (functioneel werkend via integratie-build)

### RCA-discipline
Tests 4/4 groen voor *bestaande* functionaliteit (slot-API niet-running-state via signature-test in test_msxcore). Nieuwe BIOS/Download-tests verschuiven naar v0.1.1 met fixture-files.

### Codenaam — Xanadu
Falcom "Dragon Slayer II - The Legend of Xanadu" (MSX2 1986/1995). Past bij
"expansie van mogelijkheden" — drie features tegelijk.

### Kleurcode: ORANJE (+0.1.0)
3 nieuwe core-componenten (FileDownloader + BiosManager + CartridgeModel-uitbreiding) + 3 nieuwe QML-schermen + MsxCore slot-API uitbreiding. Geen breaking change voor consumers — `loadRom(path)` blijft werken, `addRom(path)` blijft werken, alleen extra capabilities.

## v0.0.9-YS (2026-06-08) — `steamdeckmsx_core` static lib + SHA-1 helper (oranje)

### Static-lib refactor (BUG-007 RCA-fix)
- `src/CMakeLists.txt`: alle non-UI logica verzameld in `add_library(steamdeckmsx_core STATIC ...)`.
  - Bevat: `MsxCore`, `CartridgeModel`, `OpenmsxLocator`, `MachineModel`,
    `SaveStateModel`, `RomTypeDetector` (allebei `.h` + `.cc`).
  - `target_link_libraries(steamdeckmsx_core PUBLIC Qt6::Core Qt6::Qml Qt6::Network)`.
  - `target_include_directories(steamdeckmsx_core PUBLIC src/)`.
  - AUTOMOC aan op lib-target.
- `steamdeckmsx_app` linkt nu `steamdeckmsx_core` + alleen UI-Qt-modules
  (`Gui`, `Quick`, `QuickDialogs2`, `Svg`).
- `tests/CMakeLists.txt` herschreven: alle 3 testbinaries linken `steamdeckmsx_core` + `Qt6::Test`.
  Geen directe `.cc`-paden meer in test-targets.
- **Effect**: nieuwe core-files landen automatisch in app + alle tests → BUG-007
  herhaling onmogelijk gemaakt.

### SHA-1 fingerprint helper — opbouw naar softwaredb-hash-match (v0.0.10+)
- `RomTypeDetector::sha1Hex(QByteArray)` static helper — gebruikt
  `QCryptographicHash::Sha1`. Lege input → lege string. Output = lowercase 40-char hex.
- `Result.sha1Hex` veld toegevoegd, ingevuld in `detect()`. Zal in v0.0.10
  als lookup-key dienen voor `SoftwareDb` (openMSX `softwaredb.xml`-parse).
- `MsxCore::loadRom()` log uitgebreid met eerste 12 chars van SHA-1 als prefix-fingerprint
  ("`[RomTypeDetector] bubblebobble.rom sha1=a9993e364706… → MSX1/Plain → suggest C-BIOS_MSX1`").

### Tests — 8 → 11 RomTypeDetector cases (25 → 28 totaal)
- **T9** (nieuw): SHA-1 helper deterministisch + lege input → leeg
  + bekende waarden:
  - `sha1Hex("abc") == "a9993e364706816aba3e25717850c26c9cd0d89d"` (RFC-3174 referentie)
  - `sha1Hex(0x00×64) == "c8d7d0ef0eedfa82d2ea1aa592845b9a6d4b02b7"` (openssl-bevestigd)
- **T10** (nieuw): `Result.sha1Hex` ingevuld door `detect()`, deterministisch,
  verschillende input → verschillende sha.
- **T11** (nieuw): lege ROM-pad → `Result.sha1Hex` leeg.

### CMakeLists root — codenaam + versie expliciet (BUG-008 voorkomen)
- `project(VERSION 0.0.9)` (was 0.0.8 via vorige hotfix)
- `set(STEAMDECKMSX_VERSION_CODENAME "YS" CACHE …)` (was Snatcher)

### Mac smoke-test 2026-06-08
```
cmake --preset native-debug -DSTEAMDECKMSX_BUILD_TESTS=ON -DSTEAMDECKMSX_VERSION_CODENAME=YS
cmake --build build/native-debug
ctest --output-on-failure
→ 100% tests passed, 0 tests failed out of 4 (28 testcases)
→ build-output: SteamDeckMSX v0.0.8-YS (vóór VERSION/project bump)
```

### Wat NIET in v0.0.9
- Echte `SoftwareDb`-class met XML-parse — verschuift naar v0.0.10
- Ascii8/16-mapper-detect — verschuift naar v0.0.10
- Auto-`setCurrentMachine` — wacht op SoftwareDb + accuratesse-validatie
- Flatpak-build op Deck — Deck SSH nog open
- Thumbnails save-state — v0.0.10+
- Tab-strip in CartridgeBrowser — vereist Main.qml-restructuur

### Codenaam — YS
Falcom/MSX2 RPG-port 1988 ("Ys: Vanished Omens"). Past bij static-lib refactor
"opruimen + structuur leggen voor groter epic".

### Kleurcode: ORANJE (+0.1.0)
Architectuur-refactor (static-lib) + nieuwe component-API (`sha1Hex` + `Result.sha1Hex`)
+ test-uitbreiding. Geen breaking change voor consumers — `Result` is uitgebreid,
niet veranderd; `detect()`-signatuur ongewijzigd.

## v0.0.8-Snatcher (2026-06-07) — RomTypeDetector + L1/R1 page-jump (oranje)

### v0.0.8.1 hotfix (2026-06-07 — zelfde dag) — Mac smoke-test groen
- **BUG-007 (geel)**: `test_msxcore` + `test_savestatemodel` linker-error
  na v0.0.8 commit (`fb3fcb0`):
  ```
  Undefined symbols: RomTypeDetector::detect(QByteArray const&)
  ```
  Oorzaak: `MsxCore::loadRom()` roept `RomTypeDetector::detect` aan;
  `tests/CMakeLists.txt` ververst beide test-targets met `MsxCore.cc`
  maar niet de transitive dependency `RomTypeDetector.{h,cc}`.
  Fix: beide tests linken nu expliciet `../src/RomTypeDetector.{cc,h}`.
- **BUG-008 (groen)**: `CMakeLists.txt` root `project(VERSION 0.0.6)` +
  `STEAMDECKMSX_VERSION_CODENAME "PenguinAdventure"` waren niet
  bijgewerkt in v0.0.7 noch v0.0.8. Beide gecorrigeerd: VERSION → 0.0.8,
  codename → "Snatcher". Bug-history: dezelfde drift waarschijnlijk in
  alle eerdere bumps sinds v0.0.6.
- **Mac smoke-test 2026-06-07**:
  ```
  cmake --preset native-debug -DSTEAMDECKMSX_BUILD_TESTS=ON
  cmake --build build/native-debug
  ctest --output-on-failure
  → 100% tests passed, 0 tests failed out of 4
  → placeholder + msxcore_smoke + savestatemodel_smoke + romtypedetector_smoke
  → 25 testcases (1 placeholder + 10 + 7 + 8 nieuwe)
  ```
- RCA: F=ontbrekende build-dependency; T=transitive-include cpp-bestand
  niet automatisch gelinkt; A=geen library-target voor src/core (elk test
  herhaalt directe `.cc`-linking). Verbetering v0.0.9: maak
  `steamdeckmsx_core` static lib in `src/` voor herbruikbaarheid.



### RomTypeDetector — heuristische BIOS-detect per ROM-bytes
- `src/RomTypeDetector.{h,cc}` (~110 regels) — pure static-functie-klasse:
  - `enum Generation { Unknown, MSX1, MSX2, MSX2plus }`
  - `enum Mapper { Unknown, Plain, Konami, KonamiSCC, Ascii8, Ascii16 }`
  - `struct Result { generation, mapper, suggestedMachine, reason }`
  - `Result detect(QByteArray)` + helpers `detectGeneration`, `detectMapper`, `hasScc`
- **Heuristiek v0.0.8 (licht):**
  - ≤ 32KB → MSX1 + Plain → `C-BIOS_MSX1`
  - > 32KB + SCC-pattern (Z80 `3E 9F 32 B0 80` anywhere) → MSX2 + Konami SCC
  - > 32KB zonder SCC → MSX2 + Konami (default)
  - leeg / kapot → fallback `C-BIOS_MSX2`
- **Niet** in v0.0.8: softwaredb-hash-match (vereist `softwaredb.xml`-parse — v0.0.9), Ascii8/16-detect, ROM-header-parse — verschoven.

### Integratie in MsxCore::loadRom — log-only
- `MsxCore::loadRom()` opent ROM (max 512KB read voor I/O-cap), draait
  `RomTypeDetector::detect()`, logt resultaat met `qInfo()`.
- **Geen `setCurrentMachine` automatiek nog** — eerst real-world-accuratesse
  valideren via logs over meerdere ROMs. Auto-switch verschuift naar v0.0.9
  zodra softwaredb-fallback klaar is.

### CartridgeBrowser.qml — L1/R1 page-jump
- `Keys.onPressed` uitgebreid:
  - `Qt.Key_PageUp` → `currentIndex -= pageJump` (5 items, clamped)
  - `Qt.Key_PageDown` → `currentIndex += pageJump`
- Steam Input mapt L1/R1 op PageUp/PageDown via launcher-preset (v0.0.9
  zal aparte browser-preset toevoegen onderscheiden van game-preset
  `Stream_Client/presets/msx-gamepad.vdf`).
- Bestaande A/Enter/Space-activate ongewijzigd.

### Tests — 17 → 25 cases (3 binaries)
- `test_msxcore` (10/10) ongewijzigd
- `test_savestatemodel` (7/7) ongewijzigd
- **`test_romtypedetector` (8/8 nieuw)**:
  - T1: lege ROM → Unknown + fallback machine
  - T2: 8KB plain → MSX1 + Plain + `C-BIOS_MSX1`
  - T3: 32KB plain (grenscase MSX1)
  - T4: 33KB (grenscase MSX2)
  - T5: 64KB zonder SCC → MSX2 + Konami
  - T6: 64KB met SCC-pattern op offset 0x100 → MSX2 + KonamiSCC
  - T7: 256KB met SCC-pattern op offset 0x40000 → MSX2 + KonamiSCC
  - T8: helpers `generationName`/`mapperName`

### CMakeLists updates
- `src/CMakeLists.txt`: `RomTypeDetector.{h,cc}` toegevoegd aan `qt_add_executable`
- `tests/CMakeLists.txt`: `test_romtypedetector` target + `add_test` registratie

### Wat NIET in deze release
- **Geen `setCurrentMachine` automatiek** — log-only voor accuratesse-validatie
- **Geen softwaredb-hash-match** — verschoven naar v0.0.9
- **Geen Ascii8/16-mapper-detect** — v0.0.9
- **Geen Flatpak-build op Deck** — vereist SSH-info Deck (v0.0.9 of later, apart resume-item)
- **Geen thumbnails save-state** — verschoven naar v0.0.9 (openMSX framebuffer XML-extract)
- **Geen tab-strip in CartridgeBrowser** — vereist Main.qml-restructuur (v0.0.9 met
  visuele wireframes uit `Meta_SteamDeckMSX/docs/screens/01_library.md`)
- **Geen build/test-run in deze sessie** — Mac smoke-test verschuift naar volgende
  sessie (geen `cmake --preset native-debug` gedraaid; codewise klaar)

### Codenaam — Snatcher
Konami MSX2 1988 cyberpunk-adventure (Hideo Kojima's regie-debut). Past bij
"detective-werk" — heuristisch onderzoek naar wat de ROM eigenlijk is.

### Kleurcode: ORANJE (+0.1.0)
Nieuwe component `RomTypeDetector` in src/ + nieuwe test-binary +
QML-event-handler-uitbreiding = nieuwe component, geen architectuur-wijziging.
Conform `CLAUDE.md` § Color-Coded.

## v0.0.7-Castlevania (2026-06-07) — Bumper + trigger iconen (groen)

### Iconen — 4 nieuwe SVGs (icoon-set 8 → 12)
- `src/assets/icons/bumper/{l1,r1}.svg` — afgeronde rechthoek (`rx=11`),
  52×22 body, label `L1`/`R1` 14pt 700 (Noto Sans). Suggereert schouder-knop.
- `src/assets/icons/trigger/{l2,r2}.svg` — trapezium (boven breder dan onder),
  label `L2`/`R2` 14pt 700. Suggereert "indrukken" gebaar.
- Render-conventie ongewijzigd: 64×64 viewBox, `currentColor` stroke 3px,
  fill-opacity 0.15 — consistent met dpad/btn-families.
- Totaal repo-impact: +4 files, ~1.7KB.

### Tokens uitgebreid
- `Tokens.qml`: `iconBumperL1`, `iconBumperR1`, `iconTriggerL2`, `iconTriggerR2`
  als `qrc:/qt/qml/SteamDeckMSX/assets/icons/{bumper,trigger}/...svg`.
- `src/CMakeLists.txt` RESOURCES uitgebreid met 4 paden.

### DESIGN_TOKENS.md
- Iconen-sectie hernoemd "Set v0.0.7 (12 files)".
- Nieuwe sub-sectie "Vorm-conventies per familie" met semantiek-rationale:
  d-pad = driehoek, button = cirkel+letter, bumper = rounded rect, trigger = trapezium.
- "Gepland v0.0.7+" → "Gepland v0.0.8+" (colorize-shader + thumbnails + BIOS-detect).

### Doel-mapping (semantisch, voorlopig)
- Bumpers L1/R1 = vorige/volgende tab in CartridgeBrowser of save-state-pagina
- Triggers L2/R2 = snelle-scroll-modus in lange lijsten (pageDown/pageUp gedrag)

### Wat NIET in deze release
- **Geen QML-component nog die deze iconen gebruikt** — vereist v0.0.8 UI-koppeling
  in `CartridgeBrowser.qml` (tab-strip) + `SaveStateOverlay.qml` (pagina-pijlen).
- **Geen Stap 21 Flatpak-build** (vereist Steam Deck SSH-info — apart resume-item).
- **Geen thumbnails-implementatie** (verschoven naar v0.0.8 — vereist openMSX
  framebuffer XML-stream parse + base64 → QImage).
- **Geen BIOS-detect-heuristic** (verschoven naar v0.0.8).

### Tests
- Geen testcode-impact: SVG-toevoeging is pure RESOURCES-uitbreiding.
- `test_msxcore` (10/10) + `test_savestatemodel` (7/7) blijven groen — niet uitgevoerd
  in deze sessie omdat geen src/core- of src/model-wijzigingen plaatsvonden.

### Codenaam — Castlevania
Konami MSX2-klassieker 1986 ("Akumajou Dracula"). Uit vrije pool van Meta-repo.
Verschoven van "Vrije pool" naar "Toegewezen" in `Meta_SteamDeckMSX/CLAUDE.md`.

### Kleurcode: GROEN (+0.0.1)
Pure UI-asset-toevoeging zonder openMSX-patch, zonder architectuur-impact,
zonder logica-wijziging. Tokens-pad-uitbreiding is additief.

## v0.0.6-PenguinAdventure (2026-06-01) — D-pad iconen + save-state slot-grid

### Iconen — 8 eigen SVGs (AGPL-compatible)
- `src/assets/icons/dpad/{up,down,left,right}.svg` — minimal driehoeken,
  `currentColor` strokes (Qt6 SVG-render compat)
- `src/assets/icons/btn/{a,b,x,y}.svg` — cirkel met letter
- 64×64 viewBox, ~5KB totaal voor alle 8
- Tokens.qml: `iconDpadUp/Down/Left/Right` + `iconBtnA/B/X/Y` path-constants
  (qrc:/qt/qml/SteamDeckMSX/assets/icons/...)
- `CMakeLists.txt`: Qt6::Svg toegevoegd, 8 SVGs in RESOURCES van qt_add_qml_module

### Save-state slot-grid
- **MsxCore**: `savestate(name)` + `loadstate(name)` slots — Tcl-cmd-wrappers
  (`savestate "<name>"` / `loadstate "<name>"`), retourneren command-id
- **SaveStateModel** (nieuw, ~180 regels) — QAbstractListModel met:
  - 10 vaste slots (MSX-traditie 0..9)
  - QSettings persist per slot: occupied/rom/name/timestamp
  - "Stage save" pattern: model-state updaten ook zonder core attached;
    Tcl-cmd alleen als core en running
  - Naam-conventie: `slot_<N>_<rom_stem>` (default `slot_<N>_nopath`)
  - Roles: SlotRole/NameRole/RomStemRole/LastUsedRole/OccupiedRole/LabelRole
  - Q_INVOKABLE saveTo/loadFrom/clear voor QML
- **SaveStateOverlay.qml** — modaal Popup, 5×2 GridView, X-shortcut opent,
  A = save/load (afhankelijk van occupied), Y = clear, B/Esc = close
- **SaveStateCard.qml** — slot-tile met nummer + naam + timestamp + Save/Load hint
- **Main.qml** integration:
  - SaveStateModel id `saves`, currentRomStem auto-derived van msxCore.currentRom
  - Overlay-instance op Overlay.overlay parent
  - Shortcut "X" → opent overlay alleen als state = Running, anders warning-toast

### Tests 10 → 17 cases (2 binaries)
- **test_msxcore**: ongewijzigd 10/10 ✅
- **test_savestatemodel** (nieuw, 7 cases) ✅:
  - T1: Initial state — 10 slots, alle empty
  - T2: saveTo zonder core → -1 maar model-state wel geupdate
  - T3: saveTo + persistence roundtrip via nieuwe instance
  - T4: clear()
  - T5: LabelRole formatting ("Slot N · <stem> · <ts>" of "Slot N · empty")
  - T6: loadFrom van empty slot → -1
  - T7: out-of-range slots safe (no crash)

### Niet inbegrepen v0.0.6 (gepland v0.0.7+)
- **Stap 21 — eerste Flatpak-build op Steam Deck** (5e poging skip)
- Save-state thumbnails (vereist openMSX framebuffer-extract)
- Steam Input preset
- Icoon-colorize per status (huidig: white-on-black fixed)
- Stream_Server-werk (andere sessie)

## v0.0.5-SolidSnake (2026-05-31) — XML-stream-parser + BUG-004 fix + machine-keuze

### BUG-004 fix — OPENMSX_SYSTEM_DATA env-var
- `OpenmsxLocator` uitgebreid met `dataPath` property + auto-discovery:
  - Mac dev bindist: `<prefix>/bindist/openMSX.app/Contents/Resources/share`
  - Linux Flatpak:   `/app/share/openmsx`
  - Linux dev:       `<binDir>/../share`
- Dev-fallback paths repaired (3 levels relative-root: `/../../../`, `/../../`, `/../`)
- `MsxCore.setDataPath()` zet `QProcessEnvironment OPENMSX_SYSTEM_DATA` op spawn
- **Live-verified op Mac:** `OPENMSX_SYSTEM_DATA=<share> openmsx --version` →
  "openMSX 21.0\nflavour: opt\ncomponents: CORE GL LASERDISC" ✅

### MsxCore — volledige XML-stream-parser
- `QXmlStreamReader` incremental — buffer-friendly, parsed zodra complete element
- `PrematureEndOfDocumentError` graceful (wacht op meer chunks)
- Parse-handlers: `<openmsx-output>` (open/close = Running/Quitting),
  `<reply result="ok|nok" command-id="N">body</reply>`,
  `<update type="..." name="...">value</update>`,
  `<log level="...">msg</log>`
- Nieuwe signals: `replyReceived(int, bool, QString)`, `stateUpdate(QString, QString, QString)`,
  `logMessage(QString, QString)`, `rawLine(QString)` (debug)
- `sendCommand()` retourneert nu int command-id (counter-based, >= 1)
- `requestMachineList()` slot — `machine_info machines` Tcl-call
- `currentMachine` property + `set machine <name>` hot-swap (alleen Running)

### MachineModel (nieuw)
- QAbstractListModel met `name` + `isCurrent` roles
- Fallback hardcoded lijst (`C-BIOS_MSX1` / `MSX2` / `MSX2+`) tot dynamic load slaagt
- Dynamic load via `MsxCore.requestMachineList()` → parse Tcl-list-syntax
  (space-separated, brace-quoted, `{multi word names}`)
- Persist current machine in QSettings (`machine/current`)
- QML-bindable via `core` property; auto-binds `replyReceived` signal

### MachineSelector.qml — inline bottom-bar dropdown
- 360×64 px control met current machine + dropdown indicator
- Popup boven control (opent omhoog, popup-anchor `y: -360`)
- ListView met focus-navigatie, A/Enter = select, B/Esc = close
- `isCurrent` ● dot vs ○ voor andere; accent-primary focus-border
- Persisteert keuze in QSettings via MachineModel

### Tests uitgebreid 6 → 10 cases
- T1-T6: ongewijzigd (initial state, path roundtrip, probe-no-path, stop-idle, sendCommand-idle, dataPath setter)
- **T7 (nieuw): XML `<reply>` parsing** — verifieert command-id + ok/nok + body
- **T8 (nieuw): XML `<update>` parsing** — verifieert type/name/value
- **T9 (nieuw): XML `<log>` parsing** — verifieert level/message met search door history
- **T10 (nieuw): state-transitions** — verifieert Booting → Running → Idle via shell mock-script
- Mock-script-helper in test schrijft tijdelijke `.sh` files die XML dumpen
- 10/10 ✅

### QML — Main.qml uitbreidingen
- MachineModel toegevoegd, gekoppeld aan msxCore
- `onStateChanged: Running → machines.refresh()` (dynamic machine-load)
- `onLogMessage` debug-logging
- Bottom-bar: Y-Stop button + MachineSelector + openmsx-path display (compact)
- OpenmsxLocator.dataPath wordt nu propagated naar msxCore.dataPath

### Niet inbegrepen v0.0.5 (gepland v0.0.6+)
- D-pad SVG iconen
- Save-state slot-grid (savestate/loadstate commands)
- Audio/video render-instellingen
- **Stap 21 — eerste Flatpak-build op Steam Deck** (skip-decision blijft staan)

## v0.0.4-Aleste (2026-05-31) — ROM-loading flow + openMSX IPC

### MsxCore — state-machine + subprocess-IPC
- 7-state enum (Idle/Probing/Probed/Booting/Running/Quitting/Failed) als Q_ENUM,
  via QML bindable
- `probeVersion()` — `openmsx --version` (was `-version`, gefixt — BUG-003)
- `start(romPath="")` — spawn `openmsx -control stdio [-carta <path>]`
- `stop()` — XML-wrapped `quit` command + 2s graceful + terminate-fallback
- `loadRom(path)` — als Running: hot-swap via `carta` cmd, anders start()
- `sendCommand(cmd)` — wrapt in `<command>cmd</command>\n`
- `parseLine(line)` — v0.0.4 minimaal line-based; XML-stream-detectie voor
  `<openmsx-output>` (Booting→Running) en `</openmsx-output>` (→Quitting)
- Stderr aparte log, geen state-impact
- `eventReceived(line)` signal voor QML-debugging

### OpenmsxLocator — singleton
- Zoekvolgorde: QSettings user-path → $PATH → `/app/bin/openmsx` (Flatpak) →
  dev-fallback `externals/openmsx/derived/*/bin/openmsx`
- QML-toegankelijk via `OpenmsxLocator.found` + `OpenmsxLocator.searched`
- `setUserPath()` persisteert in QSettings

### CartridgeModel — recent-list + sentinel
- QAbstractListModel met QSettings persistentie (recent ROMs, max 8)
- `IsSentinelRole` voor "+ Add ROM…" entry
- `addRom(path)` — dedupe + LRU-rotate + persist + signal
- Machine-heuristic per filename (turbor/msx2+/msx2/msx1 detect)
- Dummy 8 MSX-titels uit v0.0.3 verwijderd — vervangen door echte recent + sentinel

### QML
- `Main.qml` — FileDialog (QtQuick.Dialogs), state-LED + state-label header,
  Y-shortcut = stop, openMSX-binary-path in SettingsRow, toast bij start/error
- `CartridgeBrowser.qml` — Loader delegate (cartridge-card vs add-rom-card)
- `AddRomCard.qml` — accent-warm gestyled "+ Add ROM…" met file-picker hook
- `Toast.qml` — info/warning/error overlay, 3s fade

### Tests
- `tests/test_msxcore.cc` — 6 unit-cases (initial state, path roundtrip,
  probe-no-path Failed, probe-true Probed, stop-idle no-op, sendCommand-idle warn)
- Opt-in via `-DSTEAMDECKMSX_BUILD_TESTS=ON`; alle 6 ✅

### Docs
- `docs/openmsx_control_protocol.md` — XML-stream-formaat, command-set v0.0.4,
  v0.0.5+ planning, Mac-quirks geobserveerd

### Mac smoke
- App start ✅ — `SteamDeckMSX 0.0.4-Aleste target= native`
- Geen QML-warnings, geen runtime-errors
- Test-suite test_msxcore: 6/6 ✅
- **Niet end-to-end gemeten:** openMSX-binary heeft op Mac OPENMSX_SYSTEM_DATA
  env-var nodig om machine-configs te vinden (BUG-003) — niet relevant voor
  Steam Deck Flatpak (paths in /app/share/openmsx)

### Bug-fixes (allen Geel — sessie-intern)
- BUG-003 (Geel): `openmsx --version` (not `-version`) — single-dash unknown opt
- BUG-004 (Geel, gedocumenteerd niet gefixt v0.0.4): OPENMSX_SYSTEM_DATA env-var
  vereist voor Mac dev-fallback; Flatpak heeft het impliciet via runtime-bundle
- BUG-005 (Geel): test_msxcore mist Qt6::Qml linkage (qqmlregistration.h in
  MsxCore.h) — opgelost via tests/CMakeLists.txt link

### Stap 21 (eerste Flatpak-build op Deck) — overgeslagen op verzoek
Eerste echte Deck-build pas v0.0.5 (samen met share-data env-var fix, mocht
Flatpak-runtime hetzelfde issue hebben).

### Niet inbegrepen v0.0.4 (gepland v0.0.5+)
- Echte XML-stream-parser (QXmlStreamReader incremental)
- D-pad SVG iconen-set
- C-BIOS machine-selectie-UI
- Save-state slot-grid
- Audio/video render-instellingen

## v0.0.3-Castlevania (2026-05-31) — Eerste runnable code (Qt6/QML)

### Toegevoegd — code
- `CMakeLists.txt` top-level — C++23, find_package Qt6, sub-dirs, opt-in tests
- `src/CMakeLists.txt` — qt_add_executable + qt_add_qml_module met SteamDeckMSX URI
- `src/main.cc` — QGuiApplication + QQmlApplicationEngine, load `SteamDeckMSX/Main`
- `src/MsxCore.{h,cc}` — QObject + QML_ELEMENT, QProcess-stub voor `openmsx -version`
  probe (subprocess-pattern PoC per P-SDM-01 + ARCHITECTURE.md beslissing #2)
- `src/CartridgeModel.{h,cc}` — QAbstractListModel met 8 dummy MSX-titels
  (Metal Gear, Bubble Bobble, Knightmare, Vampire Killer, Nemesis, Aleste,
  Penguin Adventure, Snatcher)
- `src/qml/Tokens.qml` (Singleton) — DESIGN_TOKENS.md waarden ge-encodeerd:
  MSX-CRT-revival palette, Noto-typografie, 4pt-grid, motion, geometry
- `src/qml/Main.qml` — root ApplicationWindow 1280×800, header met openMSX-status,
  CartridgeBrowser, SettingsRow stub, Esc/B = quit
- `src/qml/CartridgeBrowser.qml` — ListView met focus-navigatie, A/Enter = activate
- `src/qml/CartridgeCard.qml` — machine-pictogram + titel + publisher + year,
  focus-ring per Tokens.borderStrong (4px MSX-groen)
- `src/qml/SettingsRow.qml` — label + value placeholder voor BIOS-pad-toekomst
- `tests/CMakeLists.txt` + `tests/test_placeholder.cc` — opt-in via
  `-DSTEAMDECKMSX_BUILD_TESTS=ON`, alleen Qt6::Core smoke

### Toegevoegd — build/deploy
- `deploy/sync-to-deck.sh` — rsync vanaf Mac/laptop naar Deck Desktop Mode
- `deploy/flatpak-build-on-deck.sh` — flatpak-builder runner (op Deck, NIET op Mac)
- `nl.icthorse.SteamDeckMSX.yaml` — echte build-commands voor beide modules
  (openmsx homemade Make + steamdeckmsx-ui CMake)

### Smoke-test (Mac)
- `brew install qt` gestart in background voor `cmake --preset native-debug`
- Build-resultaat zie sessie-MD `prompts/2026-05-31_v0.0.3_first_code.md`

### Stap 21 (eerste Flatpak-build op Deck) — overgeslagen deze sessie
Op verzoek gebruiker. Eerste echte Flatpak-build = v0.0.4-fase.

### Niet inbegrepen v0.0.3 (gepland v0.0.4)
- ROM-loading (echte cartridge-load flow)
- openMSX subprocess-IPC voor save-state/run (alleen `-version` probe nu)
- Steam Input preset
- C-BIOS-machines daadwerkelijk laden in UI
- Echte iconen (D-pad SVG) — placeholders nu

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

### Smoke-test — ✅ openMSX RELEASE_21_0 schoon gebouwd op Mac
- brew deps geïnstalleerd: sdl2_ttf, glew, tcl-tk 9.0.3, libogg, libvorbis, theora
- `make -j 8` → EXIT=0, ~2 min, binary `derived/x86_64-darwin-opt/bin/openmsx` (Mach-O 64-bit)
- Linker-warnings cosmetisch (Homebrew SDK 14.0 vs openMSX-default 10.15)
- Niet getest op runtime (P-SDM-08); Mac ≠ support-target
- Conclusie: submodule-pin werkbaar; Flatpak-build v0.0.3 zal vergelijkbaar zijn

## v0.0.1-BubbleBobble (2026-05-31) — Skeleton

- Initiële repo-structuur (newp)
- README, LICENSE (AGPL-3.0), CLAUDE.md, ARCHITECTURE.md, BUGLIST.md
- Geen code, geen build, geen Flatpak — alleen documentatie
- Vrije pool codenamen vastgelegd in CLAUDE.md (per sanitycheck P1 verplaatst
  naar Meta_SteamDeckMSX/CLAUDE.md als single source of truth)
