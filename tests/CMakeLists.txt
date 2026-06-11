# Подключаем Catch2
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.6.0
)
FetchContent_MakeAvailable(Catch2)

# Тест для helloFromMyLib
add_executable(test_hello test_hello.cpp)
target_link_libraries(test_hello PRIVATE mylib Catch2::Catch2WithMain)

# Тест для debug макросов
add_executable(test_debug_log test_debug_log.cpp)
target_link_libraries(test_debug_log PRIVATE mylib Catch2::Catch2WithMain)
