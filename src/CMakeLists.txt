qt_add_executable(steamdeckmsx_app
    main.cc
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
)

set_source_files_properties(qml/Tokens.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

# Quick.Dialogs voor FileDialog op alle platformen (Flatpak portal binnen sandbox)
find_package(Qt6 REQUIRED COMPONENTS QuickDialogs2 Svg)

qt_add_qml_module(steamdeckmsx_app
    URI SteamDeckMSX
    VERSION 1.0
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
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickDialogs2
    Qt6::Svg
    Qt6::Network
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
