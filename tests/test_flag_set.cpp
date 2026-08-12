#include <catch2/catch_all.hpp>   // или <catch2/catch.hpp> для старой версии

#include "mylib/mylib.h"

using namespace mylib;

TEST_CASE("FlagSet construction", "[FlagSet]") {
    SECTION("Default state: all flags false") {
        FlagSet fs(8);
        for (int i = 0; i < 8; ++i) {
            REQUIRE_FALSE(fs.test(i));
        }
        REQUIRE(fs.toInteger() == 0);
    }

    SECTION("Invalid number of flags throws") {
        REQUIRE_THROWS_AS(FlagSet(0), std::out_of_range);
        REQUIRE_THROWS_AS(FlagSet(-1), std::out_of_range);
        REQUIRE_THROWS_AS(FlagSet(65), std::out_of_range);
    }

    SECTION("Works with 1 flag") {
        FlagSet fs(1);
        fs.set(0);
        REQUIRE(fs.test(0));
        REQUIRE(fs.toInteger() == 1);
    }

    SECTION("Works with 64 flags") {
        FlagSet fs(64);
        fs.set(63);
        REQUIRE(fs.test(63));
        REQUIRE(fs.toInteger() == (1ULL << 63));
    }
}

TEST_CASE("FlagSet set / test / clear / toggle", "[FlagSet]") {
    FlagSet fs(8);

    SECTION("set and test") {
        fs.set(3);
        REQUIRE(fs.test(3));
        REQUIRE_FALSE(fs.test(0));
    }

    SECTION("set with value false") {
        fs.set(3, false);   // уже false, ничего не меняет
        REQUIRE_FALSE(fs.test(3));
        fs.set(3, true);
        REQUIRE(fs.test(3));
    }

    SECTION("clear") {
        fs.set(2);
        REQUIRE(fs.test(2));
        fs.clear(2);
        REQUIRE_FALSE(fs.test(2));
    }

    SECTION("toggle") {
        fs.set(5);
        REQUIRE(fs.test(5));
        fs.toggle(5);
        REQUIRE_FALSE(fs.test(5));
        fs.toggle(5);
        REQUIRE(fs.test(5));
    }

    SECTION("Out-of-range index throws") {
        REQUIRE_THROWS_AS(fs.set(8), std::out_of_range);
        REQUIRE_THROWS_AS(fs.clear(-1), std::out_of_range);
        REQUIRE_THROWS_AS(fs.toggle(8), std::out_of_range);
        REQUIRE_THROWS_AS(fs.test(8), std::out_of_range);
    }
}

TEST_CASE("FlagSet bulk operations", "[FlagSet]") {
    FlagSet fs(8);

    SECTION("setAll true") {
        fs.setAll(true);
        for (int i = 0; i < 8; ++i) {
            REQUIRE(fs.test(i));
        }
        REQUIRE(fs.toInteger() == 0xFF);
    }

    SECTION("setAll false") {
        fs.setAll(true);
        fs.setAll(false);
        for (int i = 0; i < 8; ++i) {
            REQUIRE_FALSE(fs.test(i));
        }
        REQUIRE(fs.toInteger() == 0);
    }

    SECTION("clearAll") {
        fs.setAll(true);
        fs.clearAll();
        for (int i = 0; i < 8; ++i) {
            REQUIRE_FALSE(fs.test(i));
        }
        REQUIRE(fs.toInteger() == 0);
    }
}

TEST_CASE("FlagSet integer serialization", "[FlagSet]") {
    FlagSet fs(8);

    SECTION("toInteger returns correct mask") {
        fs.set(0);
        fs.set(7);
        REQUIRE(fs.toInteger() == 0x81);    // 10000001
    }

    SECTION("fromInteger sets flags and masks extra bits") {
        fs.fromInteger(0xFF);
        for (int i = 0; i < 8; ++i) {
            REQUIRE(fs.test(i));
        }
        // Биты выше 7 должны игнорироваться
        fs.fromInteger(0x1FF);  // 9 бит
        for (int i = 0; i < 8; ++i) {
            REQUIRE(fs.test(i));  // все младшие 8 бит = 1
        }
        REQUIRE(fs.toInteger() == 0xFF);
    }

    SECTION("round-trip integer") {
        uint64_t original = 0b10101010;
        fs.fromInteger(original);
        REQUIRE(fs.toInteger() == original);
    }

    SECTION("fromInteger with zero") {
        fs.setAll(true);
        fs.fromInteger(0);
        for (int i = 0; i < 8; ++i) {
            REQUIRE_FALSE(fs.test(i));
        }
    }
}

TEST_CASE("FlagSet byte serialization (little-endian)", "[FlagSet]") {
    FlagSet fs(8);

    SECTION("toBytes returns correct array") {
        fs.set(0);
        fs.set(7);
        auto bytes = fs.toBytes();
        // Ожидаем: байт 0 = 0x81, остальные 0
        REQUIRE(bytes[0] == 0x81);
        for (int i = 1; i < 8; ++i) {
            REQUIRE(bytes[i] == 0);
        }
    }

    SECTION("fromBytes sets flags correctly") {
        std::array<uint8_t, 8> bytes{};
        bytes[0] = 0x81;   // биты 0 и 7 установлены
        fs.fromBytes(bytes);
        REQUIRE(fs.test(0));
        REQUIRE(fs.test(7));
        for (int i = 1; i < 7; ++i) {
            REQUIRE_FALSE(fs.test(i));
        }
    }

    SECTION("fromBytes ignores bits beyond numFlags") {
        // Для 8 флагов байт 0 содержит все, остальные байты игнорируются
        std::array<uint8_t, 8> bytes{};
        bytes[1] = 0xFF;  // эти биты не должны повлиять
        fs.fromBytes(bytes);
        for (int i = 0; i < 8; ++i) {
            REQUIRE_FALSE(fs.test(i));
        }
        // Но если numFlags > 8, то байт 1 влияет на флаги 8-15
        FlagSet fs16(16);
        bytes[1] = 0x01;  // бит 8 (индекс 8) установлен
        fs16.fromBytes(bytes);
        REQUIRE(fs16.test(8));
        REQUIRE_FALSE(fs16.test(0));
    }

    SECTION("round-trip bytes") {
        fs.set(2);
        fs.set(5);
        auto bytes = fs.toBytes();
        FlagSet fs2(8);
        fs2.fromBytes(bytes);
        REQUIRE(fs2.test(2));
        REQUIRE(fs2.test(5));
        REQUIRE_FALSE(fs2.test(0));
    }
}

TEST_CASE("FlagSet combined operations", "[FlagSet]") {
    FlagSet fs(8);

    SECTION("Mixed set/clear/toggle and integer") {
        fs.set(0);
        fs.set(2);
        fs.toggle(2);   // теперь 2 = false
        fs.set(4, true);
        fs.clear(0);
        // Ожидаем только бит 4 = 1
        REQUIRE(fs.toInteger() == 0x10);
        REQUIRE(fs.test(4));
        REQUIRE_FALSE(fs.test(0));
        REQUIRE_FALSE(fs.test(2));
    }

    SECTION("fromInteger then modify") {
        fs.fromInteger(0b101010);
        fs.toggle(1);   // инвертируем бит 1
        REQUIRE_FALSE(fs.test(1));  // стал 0
        fs.clear(3);
        REQUIRE_FALSE(fs.test(3));
        REQUIRE(fs.toInteger() == 0x20);
    }

    SECTION("fromInteger with bits beyond numFlags") {
        fs.fromInteger(0xFFFFFFFFFFFFFFFF); // все 64 бита
        // для 8-флагового набора должны быть установлены только первые 8 бит
        for (int i = 0; i < 8; ++i) {
            REQUIRE(fs.test(i));
        }
        // биты выше 7 игнорируются
        REQUIRE(fs.toInteger() == 0xFF);
    }

    SECTION("fromBytes with non-zero extra bytes") {
        std::array<uint8_t, 8> bytes{};
        bytes[1] = 0x01; // этот байт должен игнорироваться для 8 флагов
        fs.fromBytes(bytes);
        for (int i = 0; i < 8; ++i) {
            REQUIRE_FALSE(fs.test(i));
        }
    }
}

