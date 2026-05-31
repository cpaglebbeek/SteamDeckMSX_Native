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
