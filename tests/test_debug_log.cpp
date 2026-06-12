#include <catch2/catch_all.hpp>

#include "mylib/mylib.h"

TEST_CASE("SHOW_DEBUG_LOG_INFO macro works with expression", "[debug]")
{
    int x{ 42 };

    REQUIRE_NOTHROW(SHOW_DEBUG_LOG_INFO(x));
}

TEST_CASE("SHOW_DEBUG_LOG_INFO_MSG macro works with message", "[debug]")
 {
    int a{ 10 };
    double b{ 20.5 };
    REQUIRE_NOTHROW(SHOW_DEBUG_LOG_INFO_MSG(a + b, "Sum check"));
}

TEST_CASE("Macro works with complex expressions", "[debug]")
{
    std::string s{ "hello" };
    REQUIRE_NOTHROW(SHOW_DEBUG_LOG_INFO(s.length()));
}
