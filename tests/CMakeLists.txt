find_package(Qt6 REQUIRED COMPONENTS Test)

# v0.0.9-YS: alle tests linken `steamdeckmsx_core` (zie src/CMakeLists.txt RCA-comment).
# Nieuwe core-bestanden landen automatisch in alle tests — geen herhaling van BUG-007.

add_executable(test_placeholder test_placeholder.cc)
target_link_libraries(test_placeholder PRIVATE Qt6::Core)
add_test(NAME placeholder COMMAND test_placeholder)

# MsxCore state-machine smoke
add_executable(test_msxcore test_msxcore.cc)
set_target_properties(test_msxcore PROPERTIES AUTOMOC ON)
target_link_libraries(test_msxcore PRIVATE steamdeckmsx_core Qt6::Test)
add_test(NAME msxcore_smoke COMMAND test_msxcore)

# SaveStateModel persistence + state-machine smoke
add_executable(test_savestatemodel test_savestatemodel.cc)
set_target_properties(test_savestatemodel PROPERTIES AUTOMOC ON)
target_link_libraries(test_savestatemodel PRIVATE steamdeckmsx_core Qt6::Test)
add_test(NAME savestatemodel_smoke COMMAND test_savestatemodel)

# RomTypeDetector — heuristische BIOS-detect per ROM-bytes (v0.0.8-Snatcher → v0.0.9-YS)
add_executable(test_romtypedetector test_romtypedetector.cc)
target_link_libraries(test_romtypedetector PRIVATE steamdeckmsx_core Qt6::Test)
add_test(NAME romtypedetector_smoke COMMAND test_romtypedetector)
