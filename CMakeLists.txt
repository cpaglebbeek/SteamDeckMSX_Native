cmake_minimum_required(VERSION 3.28)

project(SteamDeckMSX
    VERSION 0.5.1
    DESCRIPTION "SteamDeckMSX_Native — Qt6 Deck-UI rondom openMSX-fork"
    HOMEPAGE_URL "https://github.com/cpaglebbeek/SteamDeckMSX_Native"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

set(STEAMDECKMSX_VERSION_CODENAME "Hinotori" CACHE STRING "MSX-game-heroes codename")
set(STEAMDECKMSX_UI_TOOLKIT "qt6" CACHE STRING "UI toolkit (qt6|gtk4) — qt6 only in v0.x")
set(STEAMDECKMSX_OPENMSX_LINK "subprocess" CACHE STRING "openMSX coupling (subprocess|embedded) — subprocess only in v0.x")
set(STEAMDECKMSX_TARGET "steamdeck" CACHE STRING "Build target (steamdeck|native)")
set(STEAMDECKMSX_SKIP_FLATPAK OFF CACHE BOOL "Skip Flatpak-specific config (for Mac smoke-build)")

find_package(Qt6 6.6 REQUIRED COMPONENTS Core Gui Qml Quick Network)

qt_standard_project_setup(REQUIRES 6.6)

add_subdirectory(src)

option(STEAMDECKMSX_BUILD_TESTS "Build tests" OFF)
if(STEAMDECKMSX_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

message(STATUS "SteamDeckMSX v${PROJECT_VERSION}-${STEAMDECKMSX_VERSION_CODENAME}")
message(STATUS "  Target:        ${STEAMDECKMSX_TARGET}")
message(STATUS "  UI toolkit:    ${STEAMDECKMSX_UI_TOOLKIT}")
message(STATUS "  openMSX link:  ${STEAMDECKMSX_OPENMSX_LINK}")
message(STATUS "  Qt6 version:   ${Qt6_VERSION}")
