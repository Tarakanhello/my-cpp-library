add_executable(unit_tests
    test_hello.cpp
    test_debug_log.cpp
)
target_link_libraries(unit_tests PRIVATE mylib Catch2::Catch2WithMain)

target_include_directories(unit_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include)


include(Catch)
catch_discover_tests(unit_tests)
