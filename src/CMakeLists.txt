# v0.0.9-YS: extract core (alle non-UI logica) naar static lib.
# RCA van BUG-007 (v0.0.8): tests linkten direct naar individuele .cc-files
# i.p.v. via 1 library-target → transitive dep RomTypeDetector raakte verloren.
# Static lib voorkomt herhaling: tests + app linken `steamdeckmsx_core`,
# nieuwe core-bestanden landen automatisch in beide.
add_library(steamdeckmsx_core STATIC
    MsxCore.h
    MsxCore.cc
    CartridgeModel.h
    CartridgeModel.cc
    OpenmsxLocator.h
    OpenmsxLocator.cc
    MachineModel.h
    MachineModel.cc
    SaveStateModel.h
    SaveStateModel.cc
    RomTypeDetector.h
    RomTypeDetector.cc
    # v0.1.0-Xanadu
    FileDownloader.h
    FileDownloader.cc
    BiosManager.h
    BiosManager.cc
    # v0.2.0-TreasureOfUsas
    SoftwareDb.h
    SoftwareDb.cc
    BiosZipExtractor.h
    BiosZipExtractor.cc
    # v0.3.0-MazeOfGalious
    RomLibrary.h
    RomLibrary.cc
    ThumbnailGenerator.h
    OnlineIndex.h
    ThumbnailGenerator.cc
    OnlineIndex.cc
)
set_target_properties(steamdeckmsx_core PROPERTIES AUTOMOC ON)
target_include_directories(steamdeckmsx_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# QZipReader private header: locatie wisselt per Qt-versie/-build (BUG-009);
# include kiest via __has_include. KDE-Flatpak-SDK 6.7 levert GEEN
# Qt6CorePrivate/Qt6GuiPrivate cmake-packages (BUG-013) — daarom optioneel
# zoeken en anders terugvallen op <Module>_PRIVATE_INCLUDE_DIRS (header zit
# daar in /usr/include/QtCore/<ver>/; QZip-symbolen zitten in Qt6::Core zelf).
find_package(Qt6 REQUIRED COMPONENTS Gui OPTIONAL_COMPONENTS CorePrivate GuiPrivate)
target_link_libraries(steamdeckmsx_core PUBLIC
    Qt6::Core
    Qt6::Qml
    Qt6::Network
)
if(TARGET Qt6::CorePrivate)
    target_link_libraries(steamdeckmsx_core PUBLIC Qt6::CorePrivate)
endif()
if(TARGET Qt6::GuiPrivate)
    target_link_libraries(steamdeckmsx_core PUBLIC Qt6::GuiPrivate)
endif()
if(NOT TARGET Qt6::CorePrivate AND NOT TARGET Qt6::GuiPrivate)
    target_include_directories(steamdeckmsx_core PUBLIC
        ${Qt6Core_PRIVATE_INCLUDE_DIRS}
        ${Qt6Gui_PRIVATE_INCLUDE_DIRS}
    )
    target_link_libraries(steamdeckmsx_core PUBLIC Qt6::Gui)
endif()

# BUG-016: QML_ELEMENT-types in een gelinkte static lib registreren NIET
# betrouwbaar mee in de qml-module van de app (Qt 6.7 Flatpak: geen enkele;
# Qt 6.11 Mac: slechts een deel — app-launch was in v0.2.x stil kapot).
# Canonieke fix: core-lib is zelf een QML-module; app-module importeert hem
# (IMPORTS hieronder) zodat `import SteamDeckMSX` alle core-types blijft
# leveren en nieuwe QML_ELEMENT-types automatisch mee-registreren.
qt_add_qml_module(steamdeckmsx_core
    URI SteamDeckMSX.Core
    VERSION 1.0
)

qt_add_executable(steamdeckmsx_app
    main.cc
)
target_link_libraries(steamdeckmsx_app PRIVATE steamdeckmsx_core steamdeckmsx_coreplugin)

set_source_files_properties(qml/Tokens.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

# Quick.Dialogs voor FileDialog op alle platformen (Flatpak portal binnen sandbox)
find_package(Qt6 REQUIRED COMPONENTS QuickDialogs2 Svg)

qt_add_qml_module(steamdeckmsx_app
    URI SteamDeckMSX
    VERSION 1.0
    IMPORTS SteamDeckMSX.Core
    QML_FILES
        qml/Tokens.qml
        qml/Main.qml
        qml/CartridgeBrowser.qml
        qml/CartridgeCard.qml
        qml/AddRomCard.qml
        qml/SettingsRow.qml
        qml/Toast.qml
        qml/MachineSelector.qml
        qml/SaveStateOverlay.qml
        qml/SaveStateCard.qml
        # v0.1.0-Xanadu
        qml/UrlImportDialog.qml
        qml/SlotPickerDialog.qml
        qml/BiosManagerScreen.qml
        # v0.3.0-MazeOfGalious
        qml/GameGrid.qml
        qml/GameTile.qml
        # v0.4.0-Vampire — bedienbaar zonder toetsenbord
        qml/MenuButton.qml
        qml/EmulatorMenu.qml
        # v0.5.0-Quarth — online zoeken zonder toetsenbord
        qml/OnScreenKeyboard.qml
        qml/OnlineBrowser.qml
    RESOURCES
        assets/icons/dpad/up.svg
        assets/icons/dpad/down.svg
        assets/icons/dpad/left.svg
        assets/icons/dpad/right.svg
        assets/icons/btn/a.svg
        assets/icons/btn/b.svg
        assets/icons/btn/x.svg
        assets/icons/btn/y.svg
        assets/icons/bumper/l1.svg
        assets/icons/bumper/r1.svg
        assets/icons/trigger/l2.svg
        assets/icons/trigger/r2.svg
)

target_link_libraries(steamdeckmsx_app PRIVATE
    # Core/Qml/Network al via steamdeckmsx_core (PUBLIC).
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickDialogs2
    Qt6::Svg
)

target_compile_definitions(steamdeckmsx_app PRIVATE
    STEAMDECKMSX_VERSION="${PROJECT_VERSION}-${STEAMDECKMSX_VERSION_CODENAME}"
    STEAMDECKMSX_TARGET="${STEAMDECKMSX_TARGET}"
)

if(STEAMDECKMSX_TARGET STREQUAL "steamdeck")
    target_compile_definitions(steamdeckmsx_app PRIVATE STEAMDECKMSX_DECK)
endif()

set_target_properties(steamdeckmsx_app PROPERTIES OUTPUT_NAME "steamdeckmsx")

install(TARGETS steamdeckmsx_app
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)
