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
