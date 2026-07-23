add_executable(unit_tests

    test_debug_log.cpp
    test_math.cpp
    test_structs.cpp
    test_vector.cpp
    test_chunked_array.cpp
    test_list.cpp
    test_static_free_list.cpp
    test_free_list.cpp
    test_stack.cpp
    test_queue.cpp
)
target_link_libraries(unit_tests PRIVATE mylib Catch2::Catch2WithMain)

target_include_directories(unit_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include)


include(Catch)
catch_discover_tests(unit_tests)
