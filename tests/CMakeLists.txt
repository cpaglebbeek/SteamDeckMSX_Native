add_executable(test_placeholder test_placeholder.cc)
target_link_libraries(test_placeholder PRIVATE Qt6::Core)
add_test(NAME placeholder COMMAND test_placeholder)

# MsxCore state-machine smoke
find_package(Qt6 REQUIRED COMPONENTS Test)
add_executable(test_msxcore
    test_msxcore.cc
    ../src/MsxCore.cc
    ../src/MsxCore.h
)
set_target_properties(test_msxcore PROPERTIES AUTOMOC ON)
target_include_directories(test_msxcore PRIVATE ../src)
target_link_libraries(test_msxcore PRIVATE Qt6::Core Qt6::Qml Qt6::Test)
add_test(NAME msxcore_smoke COMMAND test_msxcore)

# SaveStateModel persistence + state-machine smoke
add_executable(test_savestatemodel
    test_savestatemodel.cc
    ../src/SaveStateModel.cc
    ../src/SaveStateModel.h
    ../src/MsxCore.cc
    ../src/MsxCore.h
)
set_target_properties(test_savestatemodel PROPERTIES AUTOMOC ON)
target_include_directories(test_savestatemodel PRIVATE ../src)
target_link_libraries(test_savestatemodel PRIVATE Qt6::Core Qt6::Qml Qt6::Test)
add_test(NAME savestatemodel_smoke COMMAND test_savestatemodel)

# RomTypeDetector — heuristische BIOS-detect per ROM-bytes (v0.0.8-Snatcher)
add_executable(test_romtypedetector
    test_romtypedetector.cc
    ../src/RomTypeDetector.cc
    ../src/RomTypeDetector.h
)
target_include_directories(test_romtypedetector PRIVATE ../src)
target_link_libraries(test_romtypedetector PRIVATE Qt6::Core Qt6::Test)
add_test(NAME romtypedetector_smoke COMMAND test_romtypedetector)
