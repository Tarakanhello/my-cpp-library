#include <catch2/catch_all.hpp>

#include "include/mylib.h"

TEST_CASE("Целочисленное деление с округлением вверх", "[math::ceiling]")
{
    SECTION("Простое деление (с остатком)")
    {
        REQUIRE(math::ceiling(10, 3) == 4);   // 10/3 = 3.33 → 4
        REQUIRE(math::ceiling(1, 2) == 1);    // 1/2 = 0.5 → 1
        REQUIRE(math::ceiling(7, 5) == 2);    // 7/5 = 1.4 → 2
    }

    SECTION("Деление без остатка")
    {
        REQUIRE(math::ceiling(8, 2) == 4);    // 8/2 = 4 → 4
        REQUIRE(math::ceiling(100, 10) == 10);
        REQUIRE(math::ceiling(0, 5) == 0);    // 0/5 = 0 → 0
    }

    SECTION("Деление на 0 (выброс исключения)")
    {
        REQUIRE_THROWS_AS(math::ceiling(42, 0), std::invalid_argument);
        REQUIRE_THROWS_WITH(math::ceiling(100, 0),
                            Catch::Matchers::ContainsSubstring("division by zero"));
    }
}
