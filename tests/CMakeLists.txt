add_executable(test_placeholder test_placeholder.cc)
target_link_libraries(test_placeholder PRIVATE Qt6::Core)
add_test(NAME placeholder COMMAND test_placeholder)
