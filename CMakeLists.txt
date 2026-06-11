cmake_minimum_required(VERSION 3.15)
project(mylib VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Добавляем папку с тестами (если она есть)
add_subdirectory(tests)

# Библиотека mylib
add_library(mylib STATIC src/mylib.cpp)
target_include_directories(mylib PUBLIC include)