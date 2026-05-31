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
)

set_source_files_properties(qml/Tokens.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

# Quick.Dialogs voor FileDialog op alle platformen (Flatpak portal binnen sandbox)
find_package(Qt6 REQUIRED COMPONENTS QuickDialogs2)

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
)

target_link_libraries(steamdeckmsx_app PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickDialogs2
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
