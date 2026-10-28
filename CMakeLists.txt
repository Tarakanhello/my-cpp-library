cmake_minimum_required(VERSION 3.15)
project(mylib VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Определим функцию, чтобы не дублировать код для каждой цели
function(target_set_warnings TARGET)
    if(MSVC)
        target_compile_options(${TARGET} PRIVATE /W4 /WX)
        # если нужно /WX - раскомментируйте следующую строку
        # target_compile_options(${TARGET} PRIVATE /WX)
    else()
        target_compile_options(${TARGET} PRIVATE -Wall -Wextra -Wpedantic -Werror)
        # если нужно -Werror - раскомментируйте
        # target_compile_options(${TARGET} PRIVATE -Werror)
    endif()
endfunction()

# Подключение тестов
option(BUILD_TESTING "Build unit tests" ON)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(third_party/catch2)
    add_subdirectory(tests)
    target_set_warnings(unit_tests)
endif()

# Библиотека mylib
add_library(mylib STATIC
    src/mylib.cpp
    src/union_find.cpp

    include/mylib/core/structs.h
    include/mylib/core/concepts.h
    include/mylib/core/overflow.h
    include/mylib/core/show_debug_log_info.h

    include/mylib/containers/vector.h
    include/mylib/containers/chunked_array.h
    include/mylib/containers/list.h
    include/mylib/containers/free_list.h
    include/mylib/containers/static_free_list.h
    include/mylib/containers/stack.h
    include/mylib/containers/queue.h
    include/mylib/containers/flag_set.h
    include/mylib/containers/bitset.h
    include/mylib/containers/union_find.h

    include/mylib/math/bit_operations.h
    include/mylib/math/math.h

    include/mylib/memory/memory.h

    include/mylib/random/engines.h
    include/mylib/containers.h
    include/mylib/random.h
    include/mylib/core.h
    include/mylib/math.h
    include/mylib/random/distributions.h
    include/mylib/random/algorithms.h
    include/mylib/random/weighted.h


)
target_include_directories(mylib PUBLIC include)

target_sources(mylib PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES
        include/mylib/mylib.h
)

target_set_warnings(mylib)
