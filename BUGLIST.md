# BUGLIST — SteamDeckMSX_Native

> Per `Meta_Master/templates/BUGLIST_TEMPLATE.md`. Kleurcodes: Groen / Geel / Rood / Loop.

## Open

_(geen tot v0.0.3-Mac-smoke-build is gevalideerd)_

## Open

_(geen open bugs op v0.0.5-SolidSnake)_

### BUG-016 (geel) — app-launch kapot op ALLE platforms: duplicate signal + type-registratie static lib — v0.2.1-KingsValley 2026-07-25 ✅ OPGELOST (offscreen-launch Mac groen + BEVESTIGD IN FLATPAK-ARTEFACT 20:28)
- **Functioneel:** eerste echte app-start op de Deck (foto ClaudeBug): "Duplicate signal name" (BiosManagerScreen.qml:21) + "MsxCore is not a type" (Main.qml:15) → QML-root laadt niet, exit 255. Reproductie op Mac via `QT_QPA_PLATFORM=offscreen`: óók kapot ("MachineModel is not a type") — de v0.2.x "Mac smoke groen" was tests-only, de app zelf is sinds v0.2.0 nooit gestart.
- **Technisch:** (1) custom `signal closed()` in BiosManagerScreen botst met het ingebouwde `Popup.closed()`; Qt 6.7 = harde fout, en die ene kapotte component maakt de hele impliciete module-import stuk (vandaar cascade "MsxCore is not a type" op de Deck). Fix: custom signaal verwijderd (er was geen listener; `scr.close()` volstaat). (2) QML_ELEMENT-types in de gelinkte static lib `steamdeckmsx_core` registreren onbetrouwbaar in de app-qml-module: Qt 6.7 registreert géén enkele, Qt 6.11 slechts een deel (afhankelijk van welke headers main.cc toevallig include't). Fix: core-lib is nu een eigen QML-module `SteamDeckMSX.Core` (qt_add_qml_module) die de app-module via `IMPORTS` meelevert — `import SteamDeckMSX` blijft voor QML ongewijzigd.
- **Architectonisch:** metatype-collectie over target-grenzen is geen gegarandeerd Qt-gedrag; types uit een lib horen in een eigen qml-module (canonieke Qt-route). Automatisme behouden (BUG-007-les): nieuwe QML_ELEMENT-types in core registreren voortaan vanzelf. Test-les: "smoke groen" moet óók een app-launch bevatten — offscreen-launch (`QT_QPA_PLATFORM=offscreen`) is nu het minimale extra gate.
- **Verificatie op het artefact (2026-07-25 20:28, HC55):** de Mac-fix alleen bewijst niets over de bundle — de Qt-versie verschilt (6.11 Mac vs 6.7 KDE-runtime) en juist 6.7 gaf de harde fout. Daarom herbouwde bundle schoon geherinstalleerd (forced uninstall eerst) en offscreen gestart in de échte sandbox: 20s stabiel, nul output. Daarnaast C-BIOS-boot + nemesis2.rom tot Konami-logo, screenshots in `docs/verification/v0.2.1/`. De vier stappen zijn nu een herbruikbare release-gate: `deploy/verify-flatpak-hc55.sh`. Les geborgd: een fix valideren op het dev-platform is geen validatie van het distributie-artefact.

### BUG-015 (geel) — bundle zonder --runtime-repo: Deck vindt org.kde.Platform//6.7 niet — v0.2.1-KingsValley 2026-07-25 ✅ OPGELOST (bundle herbouwd + workaround via clipboard)
- **Functioneel:** eerste echte Deck-install (foto via ClaudeBug): `flatpak install --user` faalt met "vereist de runtime org.kde.Platform/x86_64/6.7, welke niet kon worden gevonden". Download (21,1 MB) + clipboard-flow werkten.
- **Technisch:** `flatpak build-bundle` draaide zonder `--runtime-repo=` → de bundle bevat geen verwijzing naar een repo die de runtime kan leveren; de user-installatie op de Deck had geen (user-)flathub-remote voor 6.7. Fix: (a) workaround naar Deck gepusht via clipboard (remote-add flathub --user + expliciete runtime-install + bundle-install); (b) structureel: `--runtime-repo=https://dl.flathub.org/repo/flathub.flatpakrepo` in build-bundle-stap, bundle herbouwd (cache warm) en opnieuw gehost.
- **Architectonisch:** single-file-bundles zijn niet self-contained qua runtime; distributie-artefacten moeten runtime-herkomst-metadata meedragen. Buildscript stond bovendien alleen op HC55 (untracked) — nu vastgelegd als `deploy/run-build-hc55.sh` (vastleggings-protocol).

### BUG-014 (groen) — --device=input vereist flatpak ≥1.15.6 — v0.2.1-KingsValley 2026-07-25 ✅ OPGELOST (build groen 25-7)
- **Functioneel:** build faalt in allerlaatste stap "Finishing app": `Unknown device type input, valid types are: dri, all, kvm, shm`.
- **Technisch:** HC55 draait flatpak 1.14.6; het fijnmazige `--device=input` bestaat pas sinds 1.15.6. Fix: `--device=input` + `--device=dri` → `--device=all` (superset, overal ondersteund).
- **Architectonisch:** finish-args worden door de bouwende flatpak-versie gevalideerd; manifest moet op de oudste ondersteunde flatpak (builder én SteamOS-runtime) mikken. P-SDM-06 (gamepad-passthrough) blijft gedekt via `all`.

### BUG-013 (geel) — KDE-SDK 6.7 levert geen Qt6CorePrivate/GuiPrivate cmake-packages — v0.2.1-KingsValley 2026-07-25 ✅ OPGELOST (build groen 25-7)
- **Functioneel:** steamdeckmsx-ui-module faalt op configure: `find_package(Qt6 REQUIRED COMPONENTS CorePrivate GuiPrivate)` → Qt6_FOUND=FALSE.
- **Technisch:** de Flatpak-SDK bouwt Qt zónder de losse *Private cmake-packages; `qzipreader_p.h` zit er wél in (`/usr/include/QtCore/6.7.3/QtCore/private/`). Fix: private components via `OPTIONAL_COMPONENTS`, targets alleen linken als ze bestaan, anders fallback op `${Qt6Core_PRIVATE_INCLUDE_DIRS}` + `${Qt6Gui_PRIVATE_INCLUDE_DIRS}` (QZip-symbolen zitten in Qt6::Core zelf).
- **Architectonisch:** zelfde wortel als BUG-009 — private-Qt-API-afhankelijkheid (QZipReader) is build-omgeving-fragiel; nu afgedekt voor beide varianten (los package vs alleen include-dirs). Mac-smoke na fix: 5/5 groen.

### BUG-012 (geel) — Flatpak install-stap: bindist-pad bestaat niet — v0.2.1-KingsValley 2026-07-25 ✅ OPGELOST (build groen 25-7, sandbox-smoke geverifieerd)
- **Functioneel:** openMSX compileert + linkt in de Flatpak-sandbox, maar module faalt op installeren van machine-XML's → geen bundle.
- **Technisch:** manifest verwees naar `derived/*-linux-opt/bindist/share/machines/*.xml`; `bindist/` ontstaat alleen bij `make bindist`, wij draaien kale `make`. Bron staat in `share/` (hele tree) + `Contrib/cbios/` (C-BIOS XML's + ROM's).
- **Architectonisch:** (1) handgerolde install kopieerde alleen machines+skins terwijl openMSX op runtime de héle share-tree nodig heeft (init.tcl, scripts/, settings.xml); (2) binary heeft DATADIR=/opt/openMSX/share hardcoded — Flatpak vergt `OPENMSX_SYSTEM_DATA`-override; (3) layout moet matchen met OpenmsxLocator kandidaat 2 (`/app/share/openmsx` met `machines/` direct eronder, BUG-004-fix).
- **Fix:** manifest — volledige `share/.` → `/app/share/openmsx/`, Contrib/cbios XML's → `machines/` + ROM's → `systemroms/`, finish-arg `--env=OPENMSX_SYSTEM_DATA=/app/share/openmsx`.

## Opgelost

### BUG-011 (geel) — Flatpak-build: openMSX-probe faalt in KDE-SDK-sandbox — v0.2.1-KingsValley 2026-07-24
- **Functioneel:** 5 eerdere Flatpak-builds (op de Deck) + eerste HC55-pogingen faalden vóór of tijdens openMSX-compile.
- **Technisch:** ketting van sandbox-gaten: (1) fd.o-runtime had geen Qt6 → org.kde.Platform 6.7; (2) KDE-SDK mist Tcl → eigen tcl 8.6.16-module + `TCL_CONFIG=/app/lib`; (3) KDE-SDK mist GLEW terwijl openMSX v21 `GL/glew.h` onvoorwaardelijk include't → glew 2.2.0-module + `GLEW_NO_GLU` (SDK mist GL/glu.h) + dev-symlinks; (4) probe linkt zonder `-L/app/lib` → `LIBRARY_PATH=/app/lib` + GLEW-linkpad als fork-patch in `build/libraries.py` (3RDPARTY_INSTALL_DIR=/app kaapte de SDL2/PNG-config-scripts); (5) sed-commando met ` #` werd door YAML als comment gekapt → quoting.
- **Architectonisch:** openMSX' homemade probe-systeem kent geen pkg-config-prefix-injectie; Flatpak-prefix `/app` moet per kanaal (TCL_CONFIG, LIBRARY_PATH, fork-patch) worden aangereikt.
- **Resultaat:** buildlog 2026-07-24 08:36 UTC: volledige compile + "Linking openmsx..." ✅ — faalt pas op install-stap (= BUG-012).

### BUG-009 (geel) — Qt 6.11: qzipreader_p.h verhuisd QtGui→QtCore — v0.2.1-KingsValley 2026-07-24
- **Functioneel:** BiosZipExtractor.cc compileerde niet ("file not found") → hele core-lib + app onbuildbaar.
- **Technisch:** Qt 6.11 verplaatste de private QZipReader-header naar QtCore; ook ontbrak `#include <QRegularExpression>` (nooit gebouwd in v0.2.0). Fix: `__has_include`-keuze + CorePrivate én GuiPrivate linken.
- **Architectonisch:** private-Qt-API-afhankelijkheid (bewuste keuze v0.2.0 i.p.v. QuaZip-dep) is versie-fragiel; gemitigeerd met feature-detectie i.p.v. harde pad-aanname.

### BUG-010 (groen) — flaky msxcore_smoke onder buildload — v0.2.1-KingsValley 2026-07-24
- **Functioneel:** testsuite faalde incidenteel (logSpy leeg) direct na parallelle build; solo altijd groen.
- **Technisch:** wachtlus 10×100ms = 1s te krap voor mock-proces onder load; nu 50×100ms (4 plekken).
- **Architectonisch:** timing-gevoelige asserts horen ruime bovengrens + conditie-check te hebben, geen krappe vaste budgetten.

### BUG-004 — Geel (runtime) — OPENMSX_SYSTEM_DATA env-var (✅ FIXED v0.0.5)
- **Geopend:** 2026-05-31 (v0.0.4-Aleste)
- **Symptoom:** `Couldn't find machines/C-BIOS_MSX2+.xml in any of: ~/.openMSX/share, <repo>/externals/openmsx/share`
- **RCA:** Mac-dev-binary heeft geen impliciete machines-zoekpad zoals Flatpak; standalone bin/openmsx zoekt enkel ~/.openMSX/share + relative paths die niet matchen
- **Fix v0.0.5:** `OpenmsxLocator.dataPath` property + auto-discovery (Mac bindist + Linux Flatpak + Linux dev). `MsxCore.setDataPath()` zet `OPENMSX_SYSTEM_DATA` env-var via `QProcessEnvironment` op spawn
- **Live-verified:** `OPENMSX_SYSTEM_DATA=<share> openmsx --version` → "openMSX 21.0\nflavour: opt\ncomponents: CORE GL LASERDISC" ✅



### BUG-005 — Geel (build) — test_msxcore mist Qt6::Qml link
- **Datum:** 2026-05-31 (v0.0.4-Aleste)
- **Symptoom:** `'qqmlregistration.h' file not found` in test_msxcore.cc build
- **RCA:** MsxCore.h gebruikt `QML_ELEMENT` macro die qqmlregistration.h header vereist; test linkt alleen Qt6::Core+Test
- **Fix:** `target_link_libraries(test_msxcore PRIVATE Qt6::Core Qt6::Qml Qt6::Test)` in tests/CMakeLists.txt

### BUG-003 — Geel (runtime) — `openmsx -version` is unknown option
- **Datum:** 2026-05-31 (v0.0.4-Aleste)
- **Symptoom:** Probe-call faalt: `Error parsing command line: -version`
- **RCA:** openMSX CLI gebruikt GNU-style `--version` (double-dash), niet `-version`
- **Fix:** MsxCore.cc `probeVersion()`: `-version` → `--version`

### BUG-001 — Geel (build) — `qt_add_qml_module` dir-conflict met exe-output
- **Datum:** 2026-05-31 (v0.0.3-Castlevania)
- **Symptoom:** Linker EISDIR (errno=21) op `src/steamdeckmsx` tijdens link-stap
- **RCA functioneel:** qt_add_qml_module schreef QML-resources naar `src/steamdeckmsx/` (afgeleid van target-name) op exact pad waar CMake het exe wilde schrijven
- **RCA technisch:** Default CMAKE_RUNTIME_OUTPUT_DIRECTORY ontbreekt → exe komt in source-tree-relative `src/`
- **RCA architectonisch:** Qt6 QML-module-systeem reserveert target-name als dir; convention-conflict niet expliciet gedocumenteerd in upstream
- **Fix:** Top-level CMakeLists `set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")` + target rename naar `steamdeckmsx_app` met `OUTPUT_NAME=steamdeckmsx`

### BUG-002 — Geel (runtime QML) — `Keys.onPressed` op ApplicationWindow werkt niet
- **Datum:** 2026-05-31 (v0.0.3-Castlevania)
- **Symptoom:** "Could not attach Keys property to: Main is not an Item"
- **RCA functioneel:** Esc/B-toets globaal voor quit werkt niet
- **RCA technisch:** ApplicationWindow erft van `Window` niet van `Item`; `Keys` is `attached property` op `Item`
- **RCA architectonisch:** Qt6 design-keuze; eigen P-SDM-02 (gamepad-first) eist global key-handling
- **Fix:** Vervangen door `Shortcut { sequences: ["Escape","B"]; onActivated: Qt.quit() }` — werkt cross-Window-types

## Verwacht v0.0.3 (waar te letten bij eerste Mac-smoke + eerste Deck-build)

### Build-tijd

- **Groen** — Qt6 niet gevonden door CMake → check `find_package(Qt6 6.6 REQUIRED ...)` PATH; oplossing: `cmake --preset native-debug -DCMAKE_PREFIX_PATH=$(brew --prefix qt)`
- **Geel** — `qt_add_qml_module` herkent Tokens.qml niet als singleton → check `QT_QML_SINGLETON_TYPE` property + `pragma Singleton` in Tokens.qml
- **Geel** — `QML_ELEMENT` macro vereist `qqmlregistration.h` + Qt6.5+ — als oudere Qt: degradeer naar `qmlRegisterType` in main.cc

### Runtime (Mac smoke)

- **Geel** — Tokens singleton-import faalt → "module SteamDeckMSX not found" → moc-generated qmldir te checken in build-dir
- **Geel** — `KeyNavigationEnabled: true` op ListView werkt niet met items zonder explicit focus-handling — momentaneel via `ListView.isCurrentItem` op delegate
- **Groen** — `Qt.UserRole + 1` hard-coded in Main.qml console.log — gebruik role-naam string i.p.v. enum

### Runtime (Deck-Flatpak, v0.0.4 verwacht)

- **Rood** — Flatpak runtime 23.08 heeft mogelijk Qt 6.5 i.p.v. 6.6 → CMake requirement bump pessimistisch
- **Geel** — `--device=input` portal blokkeert mogelijk Steam Input rebindings → moet getest op Deck
- **Rood** — openMSX `make` in Flatpak-sandbox kan falen door tcl-detectie → mogelijk apart module met tcl als source

## Cross-repo bug-referenties

Zie `Meta_Master/BUGS_GLOBAL.md`. Specifiek voor Native:
- **CACHE/QML** — Qt6 QML-compiler cache `.qmlc` kan stale zijn bij CMake-only rebuilds; rebuild `build/` bij module-wijzigingen
- **DEPLOY/Flatpak** — submodule openMSX moet ge-add zijn vóór flatpak-builder run

## Terugkerende patronen

_(in te vullen vanaf eerste echte bugs in v0.0.3/v0.0.4)_

### BUG-017 (geel) — galerij blijft leeg: sandbox ziet de scanmappen niet — v0.3.1-MazeOfGalious 2026-07-25 ✅ OPGELOST
- **Functioneel:** melding gebruiker na Deck-install: "scan levert geen spellen op". De automatische scan bij opstarten liep wel, maar vond nooit iets.
- **Technisch:** binnen de Flatpak-sandbox is de home-map leeg op wat expliciet gemount is. Geverifieerd met `flatpak run --command=ls` in de sandbox: `/root` bevatte alleen `.local` en `.var`; `~/ROMs` en `~/Downloads` bestonden er niet. `--filesystem=xdg-download:ro` bood geen soelaas — dat resolveert naar niets zonder XDG-user-dirs — en `/run/media` bestond ook niet. Alle scanroots faalden dus stil op `QFileInfo::exists()`. Fix: `--filesystem=home:ro` + de home-map zelf als scanroot.
- **Architectonisch:** rechten zo krap mogelijk houden is goed, maar een functie die op het bestandssysteem leunt moet geverifieerd worden *binnen* de sandbox, niet ernaast. De v0.3.0-verificatie gaf `--filesystem=/root/Downloads:ro` handmatig mee bij `flatpak run` en testte daarmee een omgeving die de gebruiker nooit krijgt — de test bewees de scanner, niet het product. Tweede les: een lege staat moet tonen wáár gezocht is, anders is "niets gevonden" niet te onderscheiden van "verkeerde plek".

### BUG-018 (geel) — tick-budget gold niet binnen één map — v0.3.1-MazeOfGalious 2026-07-25 ✅ OPGELOST (test)
- **Functioneel:** de "incrementele" scan kon de UI alsnog seconden vastzetten.
- **Technisch:** `scanTick()` controleerde het budget alleen tússen mappen; de lus over de bestanden in één map liep altijd volledig door en hashte ze allemaal. Bij een collectie in één map (het normale geval voor MSX-dumps: duizenden ROMs naast elkaar) is dat één tick met duizenden SHA-1-berekeningen. Fix: wachtrij die per tick wordt afgewerkt, budget geldt ook binnen een map.
- **Architectonisch:** "incrementeel" moet op de duurste stap slaan, niet op de goedkoopste. Gevonden doordat een testcase met 80 bestanden in één map faalde op de assertie dat rijen vóór `scanFinished` binnenkomen — zonder die test was het pas op een echte collectie opgevallen.

### BUG-019 (geel) — systeem-ROMs stonden als "spel" in de galerij — v0.3.1-MazeOfGalious 2026-07-25 ✅ OPGELOST
- **Functioneel:** na de home-scan-fix vulde de galerij zich met 19 C-BIOS-bestanden (`cbios_main_msx2.rom`, `cbios_logo_msx1.rom`, …) uit een build-map. Die zijn niet speelbaar; ze horen niet als tegel te verschijnen.
- **Technisch:** de scanner accepteerde elke `.rom`. Fix: twee signalen, want geen van beide volstaat alleen. (1) Cartridge-header: een MSX-cartridge begint met `AB` op offset 0 of 0x4000, systeem-ROMs niet — gemeten op de C-BIOS-set filtert dit 17 van de 19 weg. (2) Naam bevat "bios" — nodig voor `cbios_music.rom` en `cbios_disk.rom`, die wél een `AB`-header hebben omdat het cartridge-achtige uitbreidings-ROMs zijn. De check draait vóór de cache-lookup, anders blijven ze na een upgrade in de opgeslagen bibliotheek staan. Alleen `.rom` wordt zo beoordeeld: `.dsk`/`.cas` hebben geen ROM-header.
- **Architectonisch:** heuristieken op bestandsinhoud horen gemeten te worden op echt materiaal vóór ze worden vastgelegd — de aanname "geen AB-header = geen spel" was voor 17 van de 19 gevallen juist en zou zonder meting stilzwijgend twee bestanden hebben doorgelaten. Bijvangst: de testhelper schreef dummy-ROMs zónder header, waardoor 10 bestaande tests terecht omvielen; die schrijven nu geldige cartridges.

### BUG-020 (groen) — app meldde verkeerde codenaam (0.3.2-KingsValley) — v0.3.2-MazeOfGalious 2026-07-25 ✅ OPGELOST
- **Functioneel:** de gebouwde v0.3.2-bundle presenteerde zich bij het starten als `0.3.2-KingsValley`, terwijl de bundelnaam en de repo `MazeOfGalious` zeggen.
- **Technisch:** `-DSTEAMDECKMSX_VERSION_CODENAME=KingsValley` stond hard in het Flatpak-manifest en overrulede daarmee de waarde uit `CMakeLists.txt`. Fix: de optie verwijderd; CMakeLists is de enige bron. Geverifieerd met `strings` op de binary in de geïnstalleerde bundle: `0.3.2-MazeOfGalious`.
- **Architectonisch:** herhaling van BUG-008 (versie/codenaam-drift) op een tweede plek. De les van toen — bump op één plek — hield geen stand omdat het manifest een eigen kopie van dezelfde waarde had. Een waarde die in twee bestanden staat, loopt uiteen; het manifest hoort de buildconfiguratie niet te dupliceren.

### BUG-022 (rood) — spel "start" maar er is geen beeld op de Deck — v0.3.3-TreasureOfUsas 2026-07-26 ✅ OPGELOST
- **Functioneel:** een spel kiezen toont de melding "Start: <titel>", maar daarna is er niets te zien. De emulator lijkt te draaien; er verschijnt alleen geen beeld.
- **Technisch (RCA, gemeten):** met `-control stdio` laat openMSX het opzetten van de weergave over aan de aansturende partij. Het start dan **zonder renderer** (geen venster) én met de **machine uit** (geen emulatie), terwijl het proces gewoon draait en netjes XML terugstuurt. `MsxCore::start()` gaf alleen `-control stdio`, `-machine` en `-carta` mee en zette geen van beide aan. Fix: `-command "set renderer SDLGL-PP ; set power on ; set fullscreen on ; bind F12 quit"` — renderer vóór fullscreen, want fullscreen slaat nergens op zonder venster. Faalt SDLGL-PP, dan zoekt openMSX zelf een andere renderer.
- **De oorspronkelijke hypothese was fout, en de voorgestelde fix bestond niet.** De vorige sessie noteerde "venster hangt achter de galerij, geef `-fullscreen` mee". Maar (a) openMSX kent geen `-fullscreen`-vlag — die staat niet in `openmsx --help`, het is een Tcl-setting, en (b) op HC55 gemeten met vier startvarianten: `-control stdio` alleen → 0 vensters, zwart scherm (3392 bytes); `-control stdio` + fullscreen-setting → nog steeds 0 vensters, byte-identiek zwart; `-control stdio` + renderer → venster met beeld (708 KB); renderer + power on → Nemesis 2 speelt zichtbaar. Er was dus nooit een venster om achter de galerij te hangen.
- **Waarom vier gates dit misten:** alle bestaande verificatie startte openMSX **zonder** `-control stdio` (rechtstreeks met `-command "screenshot"`), precies de route waarin openMSX zijn renderer wél zelf aanzet. Ze toetsten bovendien of openMSX een screenshot van zichzelf produceerde — dat lukt ook zonder zichtbaar venster. Ze bewezen dat de emulator draait, niet dat de speler iets ziet. Ook `ThumbnailGenerator` gebruikt de route zonder `-control` en werkte daarom altijd.
- **Meegenomen, want de fix creëerde ze zelf:** het galerijvenster verbergen tijdens het spelen (anders vechten twee vensters om de voorgrond) vereist `setQuitOnLastWindowClosed(false)` — anders sluit dat verbergen het laatste venster en daarmee de app plus het spel. En een verborgen galerij vangt geen toetsen meer, dus legt openMSX zelf `F12` vast als uitgang; de footer toont die toets vóór het starten.
- **Architectonisch:** de subprocess-koppeling maakt de emulator een eigen toepassing naast de UI. Zolang dat zo is, is niet alleen "zichtbaarheid" maar de héle weergavetoestand — renderer, power, fullscreen — een expliciete verantwoordelijkheid van deze app. Een controlekanaal dat de weergave stil laat staan tot je erom vraagt, is geen storing maar een ontwerpkeuze van openMSX; de aannames daarover horen in de gate vastgelegd, niet in het hoofd van de volgende sessie.
- **Verificatie:** `deploy/verify-visible-hc55.sh` (nieuw) draait mét window manager — zonder WM honoreert niemand het fullscreen-verzoek van SDL en meet je een omgeving die de gebruiker nooit krijgt (zelfde soort fout als BUG-017). De gate meet de map-state van het galerijvenster en telt de heldere pixels op het scherm zelf, in plaats van openMSX om een screenshot te vragen.

### BUG-032 (geel) — alleen muis-aansturing op de Deck: Steam paste de keyboard-layout niet toe — v0.6.0-SpaceManbow 2026-07-26 ✅ FIX GEBOUWD (hardware-herbewijs open)
- **Functioneel:** melding gebruiker na v0.5.1-deploy: "nu heb ik een muis aansturing. dat wil ik niet." Alle knoppen deden niets zinvols; alleen de trackpad-muis werkte.
- **Technisch:** de BUG-023-aanpak (controller_neptune.vdf in Steam Controller Configs zetten) is door Steam kennelijk niet opgepikt — het VDF-formaat en de activatieregels voor non-Steam shortcuts zijn ongedocumenteerd, en het default-sjabloon geeft gamepad-signalen (die de app negeerde) + trackpad-muis. Precies het risico dat bij BUG-023 als "nog te bewijzen op hardware" stond.
- **Fix (structureel):** de app leest de controller nu zélf — nieuwe core-component `GamepadInput` (SDL2 uit de KDE-runtime; `--device=all` stond al aan): A=Return, B=Escape, X/Y, Select=F12, L1=R, R1=I, D-pad ∪ linker stick = pijltjes met eigen auto-repeat (400/140 ms) en hysterese op de stick. Injectie via `QWindowSystemInterface::handleKeyEvent` — het enige pad dat ook de QShortcutMap passeert; `postEvent` had elk QML `Shortcut`-element stil overgeslagen (BUG-027-familie). In-game passief: zonder focusWindow wordt niets geïnjecteerd en leest openMSX de controller via zijn eigen SDL. Zonder SDL2 compileert een no-op (Mac-smokebuild). De deploy draait eerder geplaatste keyboard-layouts terug (`.bak` terug of verwijderen).
- **Architectonisch:** bediening hoort een capability van de app te zijn, niet van een extern, onobserveerbaar configuratiemechanisme. De VDF-route had geen enkele verifieerbare terugkoppeling ("is de layout actief?") — precies het soort schakel dat BUG-030 ook al liet zien.

### BUG-033 (geel) — geïmporteerde BIOS was nergens "als BIOS te kiezen" — v0.6.0-SpaceManbow 2026-07-26 ✅ FIX GEBOUWD
- **Functioneel:** melding gebruiker: lokaal geladen BIOS-rom is niet als BIOS te kiezen.
- **Technisch:** BiosManager bewaarde imports alleen in `AppDataLocation/bios/` — een map die openMSX nooit leest. openMSX zoekt machine-roms (op SHA-1, bestandsnaam irrelevant) in `OPENMSX_HOME/share/systemroms`; zonder rom daar is elke echte machine in de machine-kiezer onbootbaar en "bestond" de import functioneel niet.
- **Fix:** import (lokaal én URL) spiegelt naar `OPENMSX_HOME/share/systemroms/`; constructor spiegelt bestaande entries idempotent bij (migratie voor de Deck); verwijderen/clearAll ruimt de spiegel mee op. Route voor de gebruiker: BIOS importeren → machine kiezen (bv. Philips NMS 8245) → boot met echte BIOS.
- **Architectonisch:** een "beheer"-scherm dat artefacten opslaat waar de consument ze niet zoekt, beheert niets. De opslagplaats van een artefact hoort uit de consument (openMSX) afgeleid te worden, niet uit de app-conventie.

### BUG-034 (geel) — galerij oogde dood: basis-thumbnail was het zwarte bootframe — v0.6.0-SpaceManbow 2026-07-26 ✅ OPGELOST (gemeten op HC55)
- **Functioneel:** melding gebruiker: "screenshots van roms zijn niet geanimeerd". Op HC55-gate-screenshots stonden de tegels er volledig zwart bij.
- **Technisch (gemeten):** ThumbnailGenerator schreef het éérste bruikbare frame als basis-PNG — frame 0 = t≈0 = bootmoment = zwart (basis: 0 heldere pixels; frame 05: vol beeld; 12 frames per ROM stonden gewoon op schijf). Niet-gefocuste tegels tonen alleen de basis, en animatie draait bewust alleen op de gefocuste tegel — zonder werkende navigatie (BUG-032) kreeg níets focus, dus bewoog én toonde niets.
- **Fix:** grootste frame wordt de basis (zwart PNG comprimeert naar bijna niets — bestandsgrootte is een betrouwbare helderheids-proxy zonder decoderen); `repairBase()` migreert bestaande installaties (alleen wanneer basis byte-gelijk aan frame 0 én er een groter frame bestaat). Focus-animatie ongewijzigd by design.
- **Architectonisch:** "frame 0" was een impliciete aanname over representativiteit. Een reeks heeft een expliciet gekozen representant nodig, met een meetbaar criterium.

### BUG-035 (groen) — header-elementen linksboven over elkaar — v0.6.0-SpaceManbow 2026-07-26 ✅ OPGELOST
- **Functioneel:** melding gebruiker: "items linksboven in menu staan over elkaar heen" — titel en knoppen door elkaar. Reproduceerbaar op de HC55-gate-screenshots (zelfde 1280×800).
- **Technisch:** de header is links een titel en rechts een groeiende Row (4 knoppen + sneltoets-strip + scan-voortgang + versie + status); bij 1280px schoof de rechts-verankerde Row onder de titel. Fix: de redundante sneltoets-strip (~350px; sinds v0.5.0 bestaan de knoppen en toont de footer de controller-hints) is verwijderd en de titel verbergt zichzelf zodra de Row eroverheen zou komen — decoratie wijkt, bediening nooit.
- **Architectonisch:** twee onafhankelijk verankerde elementen zonder onderlinge ruimte-afspraak overlappen per definitie een keer. De zwakste partij moet expliciet weten wanneer hij moet wijken.

### BUG-030 (ROOD) — het hele stdin-commandokanaal naar openMSX is nooit werkend geweest — v0.5.1-Hinotori 2026-07-26 ✅ OPGELOST
- **Functioneel:** elk commando dat de app tijdens een sessie naar openMSX stuurde — savestate, loadstate, pause, cartb/slot B, diskb, quit, thumbnail-verzoeken op de lopende instantie — werd stilzwijgend genegeerd. Zichtbaar geworden doordat de nieuwe roundtrip-gate (BUG-029) een save-state eiste en er nooit één verscheen; drie gate-runs faalden identiek. Ook het pauzemenu heeft dus nooit werkelijk gepauzeerd — het spel liep achter het menu door.
- **Technisch (gemeten, geïsoleerd op HC55 met openMSX 19.1 én de eigen 21.0-fork):** een `<command id="5">…</command>` krijgt géén reply en wordt niet uitgevoerd; exact hetzelfde commando **zonder id-attribuut** krijgt `<reply result="ok">` en het state-bestand verschijnt. Bovendien verstuurde de app nooit de `<openmsx-control>`-roottag die het protocol van de cliënt verwacht, en stuurt openMSX in zijn replies géén id terug — het id-gebaseerde correlatie-ontwerp van MsxCore kon dus nooit kloppen.
- **Waarom dit drie releases onzichtbaar bleef:** alles wat aantoonbaar werkte liep buiten dit kanaal om — machinekeuze/ROM/renderer/power via launch-argumenten (`-command`, `-carta`), F12-pauzemenu via openMSX→app (stdout `message`), thumbnails via losse headless processen. De enige stdin-afhankelijke functies (save-states, slot B, pause, nette quit) waren precies de functies die nooit een release-gate hadden. Extra verwarring tijdens de diagnose: de lege proceslog leek op onderdrukte `qWarning`-output, maar er víel niets te loggen — zonder reply vuurt geen enkel foutpad. Ná de fix verschijnen zowel de C++- als de QML-logregel gewoon (gate-run 4); de gate toetst op de QML-regel.
- **Fix:** (1) `<openmsx-control>\n` direct na processtart (QProcess buffert tot het kanaal open is); (2) commando's zonder id-attribuut, inhoud XML-escaped (`&`, `<`, `>` — een ROM-naam met `&` brak anders de parser aan de openMSX-kant); (3) reply-correlatie via FIFO-queue (`m_replyIdQueue`) — openMSX antwoordt strikt in verzendvolgorde; queue geleegd bij elke processtart.
- **Architectonisch:** een protocol-aanname (id-attribuut) die nooit tegen de werkelijke tegenpartij is gevalideerd, plus een commandokanaal zonder enige gate erop. De les is dezelfde als BUG-022/BUG-024 maar dan op protocolniveau: elke integratie-schakel heeft een eigen bewijs nodig — "het proces draait en er komt XML terug" bewijst alleen de stdout-richting. De roundtrip-gate van BUG-029 dekt dit nu structureel: zonder werkend stdin-kanaal komt er geen save-state en faalt de release.

### BUG-031 (geel, OPEN) — eigen BIOS-dumps verschijnen als speelbare tegel — waargenomen 2026-07-26
- **Functioneel:** met verse app-data startte de gate niet Nemesis maar `nms8245_disk_1.06` (een machine-disk-ROM uit de eigen dumps in de home-map) als "spel".
- **Technisch:** het BUG-019-filter kent twee signalen — cartridge-header `AB` of "bios" in de naam. Een disk-/machine-ROM zonder AB-header en zonder "bios" in de bestandsnaam glipt er doorheen. Bijkomend: met koude cache verschijnen tegels in ontdek-volgorde, dus "de eerste tegel" is niet deterministisch — de gate kan een willekeurig "spel" starten.
- **Fix-richting:** filter uitbreiden (bekende machine-ROM-naamvormen + heuristiek op grootte/extensie), en de gate een expliciete titel laten kiezen i.p.v. blind de eerste tegel.
- **Bijvangst gate-run 4 (2026-07-26):** de save-fase (koude cache) savede `nms8245_disk_1.06`, de herstart-fase (warme cache, gesorteerd) startte Nemesis en laadde die state er stilzwijgend overheen — `loadFrom` toetst niet of het slot bij de actieve ROM hoort. De load zelf is correct (openMSX herstelt de volledige machine uit de state), maar (a) de visuele 05-vs-07-vergelijking van de gate is hierdoor zwak zolang de tegelkeuze niet deterministisch is, en (b) een speler die per ongeluk een state van een ander spel laadt krijgt geen waarschuwing. Beide meenemen bij de fix.

### BUG-029 (geel) — save-states onbereikbaar op de Deck én resultaat nooit gecontroleerd — v0.5.1-Hinotori 2026-07-26
- **Functioneel:** op de Deck kon een speler nooit een save-state maken of terugladen. De X-toets staat in de footer, maar tijdens het spelen is het galerijvenster verborgen (BUG-022-fix) en vangt die sneltoets dus niets; het pauzemenu — de enige plek waar de speler nog komt — bood geen save-states aan. De functie bestond op de Deck simpelweg niet.
- **Technisch (tweede laag):** `SaveStateModel::saveTo/loadFrom` waren fire-and-forget: het `replyReceived`-signaal van MsxCore werd nooit uitgelezen. Een mislukte load toonde een succes-toast ("Load slot N"), en een mislukte save liet het slot als "bezet" achter terwijl er geen state-bestand bestond — elke latere load daarop faalt dan stil.
- **Fix:** (1) Save-states-knop in het pauzemenu (toets X werkt daar ook), overlay houdt het spel gepauzeerd en hervat bij sluiten; (2) SaveStateModel volgt command-ids en meldt het échte openMSX-resultaat via `operationFinished` — toast toont pas succes ná bevestiging, mislukte save draait de bezet-markering terug, en de logregel `[SaveState] <op> slot <N> ok` is het bewijs waar de gate op toetst.
- **Verificatie:** `verify-visible-hc55.sh` doet nu de volledige spelersroute: save via pauzemenu → state-bestand aantoonbaar op schijf → **app volledig herstarten** → zelfde spel starten → load via pauzemenu → bevestigde load in de log + beeld op het scherm. "Kan schrijven" (BUG-024-check) was tot nu een proxy; terugladen ná herstart was nooit gemeten.
- **Architectonisch:** een UI-route die alleen in de ontwikkelsituatie bestaat (galerij zichtbaar naast het spel) is geen route. En een commando-API met reply-kanaal dat niemand uitleest, degradeert elke bevestiging in de UI tot fictie — resultaat-terugkoppeling hoort bij het versturen van het commando ontworpen te worden, niet erna.

### BUG-025 (geel) — elke build hercompileert openMSX: cache-miss is structureel — v0.5.1-Hinotori 2026-07-26 ✅ OPGELOST
- **Echte root cause (de analyse hieronder zat er nét naast):** flatpak-builder geeft een `type: dir`-source niet "geen betrouwbare checksum" maar **bewust een willekeurige**: `builder_source_dir_checksum()` bevat letterlijk `/* We can't realistically checksum a directory, so always rebuild */` gevolgd door `builder_cache_checksum_random (cache)` (broncode 1.4.2, regel 274-275). Elke build is dus per definitie een miss; `skip:` en de inhoud van `derived/` stonden er volledig buiten.
- **Fix:** de openmsx-module bouwt uit de GitHub-fork `cpaglebbeek/openMSX-steamdeckmsx`, branch `steamdeckmsx-flatpak`, gepind op commit `dbf9612` — een git-source checksumt op commit-hash. build-all.sh stap 1 blokkeert op (a) drift tussen manifest-pin en submodule-HEAD en (b) een vuile submodule; negatief getest (verminkte pin → "AFGEBROKEN: manifest pint openmsx-commit 000… maar submodule staat op dbf961…").
- **Gemeten bewijs (2026-07-26):** build 1 ná de manifest-wijziging: verwachte miss (nieuwe source-definitie) + eenmalige git-mirror-fetch. Build 2 zonder wijzigingen: **`Cache hit for openmsx, skipping build`** — alleen de UI-module (bewust `type: dir`, verandert elke release) wordt nog gebouwd. De ~8 min openMSX-hercompilatie per ronde is weg; `derived/` (70 MB) wordt ook niet meer meegersynct.
- **Architectonisch (bevestigd):** een build-cache kan alleen werken als de bron identificeerbaar is; de fork had al een remote + commit en het manifest hoorde die te gebruiken. De cache-key ligt nu bij het versiebeheer, en de drift-guard maakt de koppeling afdwingbaar in plaats van afspraak.

### BUG-025-oorspronkelijke-analyse (historie, grotendeels correct — root-cause-regel aangescherpt hierboven)
- **Functioneel:** elke release-ronde kost ~8,5 minuten, ook als er niets aan de emulator is veranderd. Dat maakt elke verificatie-iteratie traag en verleidt tot minder vaak verifiëren.
- **Gemeten (het ontbrekende stuk):** twee builds ná elkaar gedraaid **zonder ook maar één wijziging** — beide melden `Cache hit for tcl` / `Cache hit for glew` / **`Cache miss`** op de openmsx-module. De miss hangt dus niet af van wat er gewijzigd is; die module krijgt structureel nooit een hit. Verder: in `externals/openmsx` is buiten `derived/` niets aangeraakt in de uren rond de builds, dus de bron zelf verandert niet.
- **Wat het níét is (uitgesloten):** de eerdere aanname dat het aan de rsync of aan gewijzigde app-code lag. Een build waarin alleen `finish-args` veranderden was juist wél snel — wat de indruk wekte dat de cache "soms" werkt; die build kwam simpelweg niet aan de openmsx-module toe.
- **`derived/` uitgesloten als oorzaak:** de map (70 MB build-output) is opzij gezet en er is opnieuw gebouwd — nog steeds een cache-miss. Daarmee is het spoor uit de projectmemory ("skip: derived hielp NIET") definitief dicht: niet de inhoud van die map, en ook niet het bestaan ervan.
- **Wat wél opvalt:** tcl en glew zijn `type: archive`-sources met een `sha256`; openmsx is een `type: dir`. Een archive is inhoudelijk vastgepind, een lokale map niet — flatpak-builder kan die niet op inhoud vergelijken en beschouwt hem als gewijzigd. Dat verklaart waarom precies deze ene module nooit een hit krijgt, ongeacht wat er verandert.
- **Fix-richting (nog niet uitgevoerd):** openMSX opnemen als `type: git` met een vaste commit, of als tarball met `sha256`, in plaats van `type: dir`. Dan is de bron vastgepind en kan de cache zijn werk doen. Impact: de submodule blijft voor ontwikkeling, alleen de manifest-source verandert — te verifiëren met twee opeenvolgende builds (tweede moet een hit geven).
- **Architectonisch:** een build-cache kan alleen werken als de bron identificeerbaar is. Een lokale map is dat niet; een commit-hash of checksum wel. `skip:` helpt daar niet bij — dat beperkt wat gekopieerd wordt, niet waaraan de bron herkend wordt.

### BUG-024 (geel) — openMSX mag nergens schrijven: waarschuwing over het spel heen, save-states stuk — v0.3.5-Pippols 2026-07-26 ✅ OPGELOST
- **Functioneel:** zichtbaar op het verificatie-screenshot van v0.3.3 (`docs/verification/v0.3.3/02-spel.png`): over het Nemesis-titelscherm staat een openMSX-dialoog "Couldn't save SRAM cbios-msx2.cmos (Error creating dir /root/.openMSX/persistent/C-BIOS_MSX2)". Elke speelsessie begon met een foutmelding, en voortgang in SRAM ging verloren.
- **Technisch:** `--filesystem=home:ro` (BUG-017-fix, sinds v0.3.1) mount de echte home read-only ín de sandbox, en HOME wijst daarheen — dus `~/.openMSX/` was onschrijfbaar. Dat raakte SRAM, `settings.xml` én de save-states achter de X-toets.
- **Eerste poging werkte niet, en dat is gemeten:** `--persist=.openMSX` als finish-arg leek de aangewezen route, maar de gate bleef falen met "can't create directory /root/.openMSX/savestates: read-only file system". De read-only home-mount wint van die bind. Manifest-vlaggen zijn hier dus niet het juiste gereedschap.
- **Fix:** openMSX kent `OPENMSX_HOME` als expliciete override (`FileOperations.cc:395`). `MsxCore::userDataDir()` wijst die naar `AppDataLocation/openmsx` — altijd schrijfbaar, ook in de sandbox, en portabel omdat Qt het pad per platform bepaalt in plaats van een hard pad in het manifest. `ThumbnailGenerator` zet dezelfde variabele, anders levert elke tegel een foutmelding op.
- **De gate was óók fout, op precies de manier die BUG-022 al had aangetoond:** het eerste `verify-persist-hc55.sh` riep openMSX rechtstreeks aan. Dan zet niemand `OPENMSX_HOME` — dat doet de app — dus die gate faalde op een correcte fix. Nu zit de controle in `verify-visible-hc55.sh`, ná een echte spelsessie via de app: geen read-only-klachten in de app-log, en de user-map bestaat en is gevuld (8 bestanden gemeten). Het losse script is verwijderd; het bewees niets en zou een volgende sessie op het verkeerde been zetten.
- **Architectonisch:** de BUG-017-fix koos read-only toegang tot de héle home om de scanner te bedienen. Voor lezen is dat juist, maar de emulator heeft óók een eigen schrijfplek nodig. Lees- en schrijfpaden horen apart belegd; en de schrijfplek hoort door de app te worden aangewezen, niet door een sandbox-vlag die met andere mounts kan botsen.

### BUG-023 (geel) — knoptoewijzing Deck nog niet logisch — gemeld 2026-07-25 → ✅ AFGESLOTEN via BUG-032 (v0.6.0)
> De VDF-route hieronder bleek op hardware niet door Steam toegepast te worden (melding 2026-07-26: alleen muis). De structurele oplossing is native gamepad-invoer in de app zelf — zie BUG-032. Historie hieronder ongewijzigd.
- **Functioneel:** de knoppen van de Steam Deck doen niet wat je verwacht in de galerij.
- **Technisch:** er is nog geen Steam Input-preset; de app luistert op toetsenbordtoetsen (A/Enter = start, R = scan, M = map, O = openen, Y = stop, X = save-state, B/Esc = terug). Zonder preset stuurt de Deck standaard muis/joystick-signalen die daar niet op aansluiten.
- **Fix (v0.5.1-Hinotori, 2026-07-26):** eigen layout `presets/controller_neptune.vdf` — D-pad/linker stick = pijltjes, A = Return, B = Escape, X = save-states, Y = stop, Select = F12-pauzemenu, L1 = R (scan), R1 = I (BIOS), rechter stick = muis, R2/L2 = klik. `run-build-hc55.sh` publiceert hem naast de bundle; `deck-deploy.sh` zet hem in `Steam Controller Configs/<account>/config/steamdeckmsx/` (de plek waar Steam de actieve layout van een non-Steam shortcut leest) met backup van een bestaande. Knoptoewijzing staat nu ook in de galerij-footer.
- **Nog te bewijzen op hardware:** of Steam de layout accepteert zoals geschreven (VDF-formaat is niet formeel gedocumenteerd) en of hij zonder handmatige keuze actief wordt. Terugvalroute staat in de deploy-output: layout of sjabloon handmatig kiezen via de Steam-knop. Test via /bugcheck-terugkoppeling.

### BUG-021 (geel) — lege galerij op de Deck was geen bug in de scanner — v0.3.2-MazeOfGalious 2026-07-25 ✅ OPGELOST (startpakket)
- **Functioneel:** "geen spellen gevonden, ook niet na rescan" op de Deck, terwijl dezelfde bundle op HC55 wél spellen vond.
- **Technisch:** op de Deck stonden simpelweg geen losse `.rom/.dsk/.cas`-bestanden in de persoonlijke map; de ROM's stonden alleen op HC55 (`/var/lib/steamdeckmsx/roms/` en `/srv/steamweb/konami/`). De scan was correct, het resultaat leeg. Fix: `msx-startpakket.zip` naast de bundle gehost en de installatie pakt die uit in `~/ROMs` (uitpakken, niet de zip laten staan — een zip telt alleen mee in een map die zelf `rom`/`msx`/`games` heet).
- **Architectonisch:** correct gedrag dat als storing overkomt is een ontwerpfout in de terugkoppeling, niet in de logica. Twee maatregelen: de lege staat toont sinds v0.3.1 in welke mappen is gezocht, en een verse installatie levert nu inhoud mee zodat de eerste indruk nooit een leeg scherm is.

### BUG-026 (geel) — tegel-animaties verschenen nooit op een geüpgradede installatie — v0.4.1-Quarth 2026-07-26 ✅ OPGELOST
- **Functioneel:** melding gebruiker na Deck-upgrade naar v0.4.0: "ik zie nog geen geanimeerde voorbeeldbeelden van de roms".
- **Technisch:** `ThumbnailGenerator::enqueue()` sloeg elk spel over waarvan de basis-thumbnail al bestond — en op een geüpgradede installatie bestond die altijd, als één los v0.3.x-beeld. De frame-reeks werd dus nooit gemaakt; alleen een schone installatie (zoals de HC55-gate) toonde animaties. Fix: bestaat de basis maar zijn er minder dan 2 frames terwijl `frameCount > 1` → alsnog genereren.
- **Architectonisch:** een cache-conditie hoort te toetsen of het gewenste eindresultaat er is, niet of er ooit íéts gemaakt is. Zelfde familie als BUG-017: de verificatie-omgeving (vers) verschilde stilzwijgend van de gebruikersomgeving (geüpgraded met oude data).

### BUG-027 (geel) — Keys op een Popup wordt stilzwijgend genegeerd — v0.4.1-Quarth 2026-07-26 ✅ OPGELOST
- **Functioneel:** pauzemenu en online-zoekscherm reageerden niet op Escape/B; joystick-navigatie voelde "dood" in die schermen.
- **Technisch:** `Keys` is een attached property voor `Item`s; `Popup` is geen `Item`. De handler op de Popup-root gaf alleen een runtime-waarschuwing ("Could not attach Keys property … is not an Item") en deed verder niets. Fix: handlers naar het `contentItem` verplaatst. Bijvangst: `Tokens.fontSizeTitle` bestond niet ("Unable to assign [undefined] to int" 2×) — token toegevoegd.
- **Architectonisch:** de app-launch-gate (stap 4 van build-all.sh) grepte alleen op `is not a type|Cannot assign|SyntaxError` en was blind voor precies deze twee foutklassen. Filter uitgebreid met `Unable to assign|Could not attach` — gevonden doordat `/loopuntilverified` de bewering "geen QML-fouten" tegen de ruwe run.log hield.

### BUG-028 (geel) — online download sloeg zips op als "<naam>.zip.rom" — v0.4.1-Quarth 2026-07-26 ✅ OPGELOST
- **Functioneel:** melding gebruiker: "als ik een rom download via zoeken (.zip) wordt het spel niet zichtbaar in de library".
- **Technisch:** `resolveDestPath()` plakte blind `.rom` achter elke naam zonder die extensie — een gedownloade zip werd `spel.zip.rom`. De galerij-headercheck (BUG-019) ziet dan `PK…` in plaats van `AB` en filtert het bestand terecht weg; openMSX zou het archief ook niet booten. Fix: bekende extensies (.rom/.zip/.dsk/.cas) blijven staan; een gedownloade zip wordt via de (nu configureerbare) zip-extractor uitgepakt naar storage en de inhoud geregistreerd; eenmalige migratie ruimt bestaande `*.zip.rom`-spookbestanden op.
- **Architectonisch:** een aanname uit v0.1.0 ("alles wat we ophalen is een ruwe rom") overleefde stilzwijgend de komst van een nieuw aanvoerkanaal. De extractor is bewust hergebruikt (BiosZipExtractor met instelbare whitelist/cap) in plaats van een tweede zip-pad te bouwen.
