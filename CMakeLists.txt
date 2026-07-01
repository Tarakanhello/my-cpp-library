cmake_minimum_required(VERSION 3.15)
project(mylib VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Подключение тестов
option(BUILD_TESTING "Build unit tests" ON)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(third_party/catch2)
    add_subdirectory(tests)
endif()

# Библиотека mylib
add_library(mylib STATIC
    src/mylib.cpp
    include/mylib/structs.h
    include/mylib/vector.h
    include/mylib/chunked_array.h
    include/mylib/list.h

)
target_include_directories(mylib PUBLIC include)
target_sources(mylib PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES
        include/mylib/mylib.h
        include/mylib/show_debug_log_info.h
        include/mylib/math.h
        include/mylib/memory.h
)
