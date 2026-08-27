#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cstdint>
#include <limits>
#include <type_traits>

// Предполагается, что заголовочный файл библиотеки доступен
#include "mylib/mylib.h"


// -----------------------------------------------------------------------------
// Константы
// -----------------------------------------------------------------------------
TEST_CASE("Константы", "[constants]")
{
    REQUIRE(mylib::bit::ZERO == 0ULL);
    REQUIRE(mylib::bit::FULL == ~0ULL);
}

// -----------------------------------------------------------------------------
// twoPower
// -----------------------------------------------------------------------------
TEST_CASE("twoPower", "[twopower]")
{
    SECTION("Компиляционная версия")
    {
        REQUIRE(mylib::bit::twoPower<0>() == 1ULL);
        REQUIRE(mylib::bit::twoPower<1>() == 2ULL);
        REQUIRE(mylib::bit::twoPower<5>() == 32ULL);
        REQUIRE(mylib::bit::twoPower<10>() == 1024ULL);
        REQUIRE(mylib::bit::twoPower<20>() == 1ULL << 20);
        REQUIRE(mylib::bit::twoPower<30>() == 1ULL << 30);
        REQUIRE(mylib::bit::twoPower<62>() == 1ULL << 62);
        REQUIRE(mylib::bit::twoPower<63>() == 1ULL << 63);
    }

    SECTION("Рантайм-версия")
    {
        REQUIRE(mylib::bit::twoPower(0) == 1ULL);
        REQUIRE(mylib::bit::twoPower(1) == 2ULL);
        REQUIRE(mylib::bit::twoPower(5) == 32ULL);
        REQUIRE(mylib::bit::twoPower(10) == 1024ULL);
        REQUIRE(mylib::bit::twoPower(20) == 1ULL << 20);
        REQUIRE(mylib::bit::twoPower(30) == 1ULL << 30);
        REQUIRE(mylib::bit::twoPower(62) == 1ULL << 62);
        REQUIRE(mylib::bit::twoPower(63) == 1ULL << 63);
    }
}

// -----------------------------------------------------------------------------
// isPowerOfTwo
// -----------------------------------------------------------------------------
TEST_CASE("isPowerOfTwo", "[ispoweroftwo]")
{
    REQUIRE(mylib::bit::isPowerOfTwo(0ULL) == false);
    REQUIRE(mylib::bit::isPowerOfTwo(1ULL) == true);
    REQUIRE(mylib::bit::isPowerOfTwo(2ULL) == true);
    REQUIRE(mylib::bit::isPowerOfTwo(3ULL) == false);
    REQUIRE(mylib::bit::isPowerOfTwo(4ULL) == true);
    REQUIRE(mylib::bit::isPowerOfTwo(8ULL) == true);
    REQUIRE(mylib::bit::isPowerOfTwo(16ULL) == true);
    REQUIRE(mylib::bit::isPowerOfTwo(1ULL << 63) == true);
    REQUIRE(mylib::bit::isPowerOfTwo((1ULL << 63) + 1) == false);
    REQUIRE(mylib::bit::isPowerOfTwo(mylib::bit::FULL) == false);
}

// -----------------------------------------------------------------------------
// Логарифмы
// -----------------------------------------------------------------------------
TEST_CASE("log2floor и log2ceiling", "[log2]")
{
    SECTION("Компиляционные версии")
    {
        static_assert(mylib::bit::log2floor<2ULL>() == 1);
        static_assert(mylib::bit::log2floor<3ULL>() == 1);
        static_assert(mylib::bit::log2floor<4ULL>() == 2);
        static_assert(mylib::bit::log2floor<8ULL>() == 3);
        static_assert(mylib::bit::log2floor<16ULL>() == 4);
        static_assert(mylib::bit::log2floor<1ULL << 63>() == 63);

        static_assert(mylib::bit::log2ceiling<1>() == 0);
        static_assert(mylib::bit::log2ceiling<2>() == 1);
        static_assert(mylib::bit::log2ceiling<3>() == 2);
        static_assert(mylib::bit::log2ceiling<4>() == 2);
        static_assert(mylib::bit::log2ceiling<5>() == 3);
        static_assert(mylib::bit::log2ceiling<8>() == 3);
        static_assert(mylib::bit::log2ceiling<1ULL << 63>() == 63);
    }

    SECTION("Рантайм-версии")
    {
        REQUIRE(mylib::bit::log2floor(1) == 0);
        REQUIRE(mylib::bit::log2floor(2) == 1);
        REQUIRE(mylib::bit::log2floor(3) == 1);
        REQUIRE(mylib::bit::log2floor(4) == 2);
        REQUIRE(mylib::bit::log2floor(8) == 3);
        REQUIRE(mylib::bit::log2floor(1ULL << 63) == 63);

        REQUIRE(mylib::bit::log2ceiling(1) == 0);
        REQUIRE(mylib::bit::log2ceiling(2) == 1);
        REQUIRE(mylib::bit::log2ceiling(3) == 2);
        REQUIRE(mylib::bit::log2ceiling(4) == 2);
        REQUIRE(mylib::bit::log2ceiling(5) == 3);
        REQUIRE(mylib::bit::log2ceiling(8) == 3);
        REQUIRE(mylib::bit::log2ceiling(1ULL << 63) == 63);
    }
}

// -----------------------------------------------------------------------------
// nextPowerOfTwo
// -----------------------------------------------------------------------------
TEST_CASE("nextPowerOfTwo", "[nextpower]")
{
    REQUIRE(mylib::bit::nextPowerOfTwo(0) == 0);
    REQUIRE(mylib::bit::nextPowerOfTwo(1) == 1);
    REQUIRE(mylib::bit::nextPowerOfTwo(2) == 2);
    REQUIRE(mylib::bit::nextPowerOfTwo(3) == 4);
    REQUIRE(mylib::bit::nextPowerOfTwo(4) == 4);
    REQUIRE(mylib::bit::nextPowerOfTwo(5) == 8);
    REQUIRE(mylib::bit::nextPowerOfTwo(7) == 8);
    REQUIRE(mylib::bit::nextPowerOfTwo(8) == 8);
    REQUIRE(mylib::bit::nextPowerOfTwo(1ULL << 62) == 1ULL << 62);
    REQUIRE(mylib::bit::nextPowerOfTwo((1ULL << 62) + 1) == 1ULL << 63);
    REQUIRE(mylib::bit::nextPowerOfTwo(1ULL << 63) == 1ULL << 63);
}

// -----------------------------------------------------------------------------
// get, flip, set
// -----------------------------------------------------------------------------
TEST_CASE("Операции с битами: get, flip, set", "[bits]")
{
    uint64_t x{ 0b1010'1010'1010'1010ULL };

    SECTION("get")
    {
        REQUIRE(mylib::bit::get(x, 0) == 0);
        REQUIRE(mylib::bit::get(x, 1) == 1);
        REQUIRE(mylib::bit::get(x, 2) == 0);
        REQUIRE(mylib::bit::get(x, 3) == 1);
        REQUIRE(mylib::bit::get(x, 63) == 0); // старший бит 0
    }

    SECTION("flip")
    {
        uint64_t y{ mylib::bit::flip(x, 0) }; // инвертируем бит 0 (0 -> 1)
        REQUIRE(mylib::bit::get(y, 0) == 1);
        REQUIRE(mylib::bit::get(y, 1) == 1); // остальные не изменились
        REQUIRE(mylib::bit::get(y, 2) == 0);
        REQUIRE(y == x + 1);

        y = mylib::bit::flip(x, 1); // бит 1 (1 -> 0)
        REQUIRE(mylib::bit::get(y, 1) == 0);
        REQUIRE(mylib::bit::get(y, 0) == 0);
        REQUIRE(y == (x & ~(1ULL << 1)));
    }

    SECTION("set")
    {
        uint64_t y{ x };
        mylib::bit::set(y, 0, true);
        REQUIRE(mylib::bit::get(y, 0) == 1);
        REQUIRE(y == x + 1);

        mylib::bit::set(y, 1, false);
        REQUIRE(mylib::bit::get(y, 1) == 0);
        REQUIRE(y == ((x + 1) & ~(1ULL << 1)));
    }

    // Проверка для разных беззнаковых типов
    SECTION("set для uint32_t")
    {
        uint32_t z{ 0xAAAAAAAA };
        mylib::bit::set(z, 0, true);
        REQUIRE(z == 0xAAAAAAAB);
        mylib::bit::set(z, 31, false);
        REQUIRE(z == 0x2AAAAAAB);
    }
}

// -----------------------------------------------------------------------------
// Маски
// -----------------------------------------------------------------------------
TEST_CASE("Маски: upperMask, lowerMask, middleMask", "[masks]")
{
    SECTION("upperMask")
    {
        REQUIRE(mylib::bit::upperMask(0) == mylib::bit::FULL);
        REQUIRE(mylib::bit::upperMask(1) == 0xFFFFFFFFFFFFFFFEULL);
        REQUIRE(mylib::bit::upperMask(4) == 0xFFFFFFFFFFFFFFF0ULL);
        REQUIRE(mylib::bit::upperMask(63) == 0x8000000000000000ULL);
        REQUIRE(mylib::bit::upperMask(64) == 0ULL);
    }

    SECTION("lowerMask")
    {
        REQUIRE(mylib::bit::lowerMask(0) == 0ULL);
        REQUIRE(mylib::bit::lowerMask(1) == 1ULL);
        REQUIRE(mylib::bit::lowerMask(4) == 0xFULL);
        REQUIRE(mylib::bit::lowerMask(32) == 0xFFFFFFFFULL);
        REQUIRE(mylib::bit::lowerMask(64) == mylib::bit::FULL);
    }

    SECTION("middleMask")
    {
        REQUIRE(mylib::bit::middleMask(0, 0) == 0ULL);
        REQUIRE(mylib::bit::middleMask(0, 1) == 1ULL);
        REQUIRE(mylib::bit::middleMask(4, 2) == 0b110000ULL);
        REQUIRE(mylib::bit::middleMask(2, 3) == 0b11100ULL);
        REQUIRE(mylib::bit::middleMask(60, 4) == 0xF000000000000000ULL);
        REQUIRE(mylib::bit::middleMask(63, 1) == 0x8000000000000000ULL);
    }
}

// -----------------------------------------------------------------------------
// getValue и setValue
// -----------------------------------------------------------------------------
TEST_CASE("getValue и setValue", "[field]")
{
    uint64_t x{ 0xAAAAAAAAAAAAAAAAULL };

    SECTION("getValue")
    {
        REQUIRE(mylib::bit::getValue(x, 0, 4) == 0xAULL);
        REQUIRE(mylib::bit::getValue(x, 4, 4) == 0xAULL);
        REQUIRE(mylib::bit::getValue(x, 8, 4) == 0xAULL);
        REQUIRE(mylib::bit::getValue(x, 60, 4) == 0xAULL);
        REQUIRE(mylib::bit::getValue(x, 62, 2) == 0x2ULL); // биты 62,63 = 10
    }

    SECTION("setValue")
    {
        uint64_t y{ x };
        mylib::bit::setValue(y, 0x5ULL, 0, 4);   // записываем 0x5 в младшие 4 бита
        REQUIRE(mylib::bit::getValue(y, 0, 4) == 0x5);
        REQUIRE((y & 0xF) == 0x5);
        // остальные биты не изменились
        REQUIRE((y >> 4) == (x >> 4));

        mylib::bit::setValue(y, 0x3ULL, 4, 2);   // записываем 0x3 в биты 4..5
        REQUIRE(mylib::bit::getValue(y, 4, 2) == 0x3);
        // Проверяем, что биты 6..63 не тронуты
        REQUIRE((y >> 6) == (x >> 6));
    }

    // Тест для других типов
    SECTION("setValue для uint32_t")
    {
        uint32_t z{ 0xFFFFFFFF };
        mylib::bit::setValue(z, 0x0U, 8, 8);
        REQUIRE(z == 0xFFFF00FF);
        mylib::bit::setValue(z, 0xAAU, 0, 8);
        REQUIRE(z == 0xFFFF00AA);
    }
}

// -----------------------------------------------------------------------------
// Табличные классы PopCount8 и ReverseBits8
// -----------------------------------------------------------------------------
TEST_CASE("Табличные классы PopCount8 и ReverseBits8", "[tables]")
{
    mylib::bit::PopCount8 pop8;
    mylib::bit::ReverseBits8 rev8;

    SECTION("PopCount8")
    {
        REQUIRE(pop8(0x00) == 0);
        REQUIRE(pop8(0x01) == 1);
        REQUIRE(pop8(0x0F) == 4);
        REQUIRE(pop8(0xFF) == 8);
        REQUIRE(pop8(0x55) == 4);
        REQUIRE(pop8(0xAA) == 4);
        REQUIRE(pop8(0x80) == 1);
    }

    SECTION("ReverseBits8")
    {
        REQUIRE(rev8(0x00) == 0x00);
        REQUIRE(rev8(0x01) == 0x80);
        REQUIRE(rev8(0x80) == 0x01);
        REQUIRE(rev8(0x0F) == 0xF0);
        REQUIRE(rev8(0xF0) == 0x0F);
        REQUIRE(rev8(0x55) == 0xAA);
        REQUIRE(rev8(0xAA) == 0x55);
        REQUIRE(rev8(0xFF) == 0xFF);
    }
}

// -----------------------------------------------------------------------------
// reverseAllBits и reverseBits
// -----------------------------------------------------------------------------
TEST_CASE("reverseAllBits и reverseBits", "[reverse]")
{
    SECTION("reverseAllBits")
    {
        // uint8_t
        uint8_t a8 = 0b11001010;
        uint8_t r8 = mylib::bit::reverseAllBits(a8);
        REQUIRE(r8 == 0b01010011);

        // uint16_t
        uint16_t a16 = 0b1110'0000'1000'0001;
        uint16_t r16 = mylib::bit::reverseAllBits(a16);
        REQUIRE(r16 == 0b1000'0001'0000'0111);

        // uint32_t
        uint32_t a32 = 0x12345678;
        uint32_t r32 = mylib::bit::reverseAllBits(a32);
        REQUIRE(r32 == 0x1E6A2C48); // известное значение

        // uint64_t
        uint64_t a64 = 0x0123456789ABCDEFULL;
        uint64_t r64 = mylib::bit::reverseAllBits(a64);
        REQUIRE(r64 == 0xF7B3D591E6A2C480ULL);

        uint64_t r64R{ mylib::bit::reverseBits(a64) };
        REQUIRE(r64R == 0xF7B3D591E6A2C480ULL);
    }

    SECTION("reverseBits с параметром n")
    {
        uint64_t x = 0b1010'1010'1010'1010ULL;

        // n=0 – ничего не меняется
        REQUIRE(mylib::bit::reverseBits(x, 0) == x);

        // n=1 – младший бит остаётся на месте
        REQUIRE(mylib::bit::reverseBits(x, 1) == x);

        // n=2 – меняем местами биты 0 и 1
        // x = ...10 (0xAAAA...AA) младшие 2 бита = 10, после реверса станет 01
        uint64_t expected = (x & ~0x3ULL) | 0x1ULL;
        REQUIRE(mylib::bit::reverseBits(x, 2) == expected);

        // n=4 – реверс младших 4 бит: 1010 -> 0101
        expected = (x & ~0xFULL) | 0x5ULL;
        REQUIRE(mylib::bit::reverseBits(x, 4) == expected);

        // тест для uint8_t
        uint8_t y = 0b10100101;
        REQUIRE(mylib::bit::reverseBits(y, 4) == 0b10101010);
        REQUIRE(mylib::bit::reverseBits(y, 8) == 0b10100101); // полный реверс: 10100101 -> 10100101 (палиндром)
        // другой пример: 0b11010010 -> реверс всех 8 бит = 0b01001011
        uint8_t y2 = 0b11010010;
        REQUIRE(mylib::bit::reverseBits(y2, 8) == 0b01001011);
    }
}

// -----------------------------------------------------------------------------
// popcount
// -----------------------------------------------------------------------------
TEST_CASE("popcount", "[popcount]")
{
    SECTION("uint8_t")
    {
        REQUIRE(mylib::bit::popcount(uint8_t{0}) == 0);
        REQUIRE(mylib::bit::popcount(uint8_t{1}) == 1);
        REQUIRE(mylib::bit::popcount(uint8_t{0xFF}) == 8);
        REQUIRE(mylib::bit::popcount(uint8_t{0x55}) == 4);
        REQUIRE(mylib::bit::popcount(uint8_t{0xAA}) == 4);
        REQUIRE(mylib::bit::popcount(uint8_t{0x0F}) == 4);
        REQUIRE(mylib::bit::popcount(uint8_t{0xF0}) == 4);
    }

    SECTION("uint16_t")
    {
        REQUIRE(mylib::bit::popcount(uint16_t{0}) == 0);
        REQUIRE(mylib::bit::popcount(uint16_t{0xFFFF}) == 16);
        REQUIRE(mylib::bit::popcount(uint16_t{0xAAAA}) == 8);
        REQUIRE(mylib::bit::popcount(uint16_t{0x5555}) == 8);
        REQUIRE(mylib::bit::popcount(uint16_t{0x1234}) == 5); // 0x1234 = 0b0001 0010 0011 0100 -> popcount=1+1+2+1=5
    }

    SECTION("uint32_t")
    {
        REQUIRE(mylib::bit::popcount(uint32_t{0}) == 0);
        REQUIRE(mylib::bit::popcount(uint32_t{0xFFFFFFFF}) == 32);
        REQUIRE(mylib::bit::popcount(uint32_t{0xAAAAAAAA}) == 16);
        REQUIRE(mylib::bit::popcount(uint32_t{0x55555555}) == 16);
        REQUIRE(mylib::bit::popcount(uint32_t{0x12345678}) == 13); // подсчитано
    }

    SECTION("uint64_t")
    {
        REQUIRE(mylib::bit::popcount(uint64_t{0}) == 0);
        REQUIRE(mylib::bit::popcount(uint64_t{0xFFFFFFFFFFFFFFFF}) == 64);
        REQUIRE(mylib::bit::popcount(uint64_t{0xAAAAAAAAAAAAAAAA}) == 32);
        REQUIRE(mylib::bit::popcount(uint64_t{0x5555555555555555}) == 32);
        REQUIRE(mylib::bit::popcount(uint64_t{0x0123456789ABCDEF}) == 32); // известное значение
    }
}

// -----------------------------------------------------------------------------
// rightmostNullCount
// -----------------------------------------------------------------------------
TEST_CASE("rightmostNullCount", "[trailingzeros]")
{
    REQUIRE(mylib::bit::rightmostNullCount(0ULL) == 64);
    REQUIRE(mylib::bit::rightmostNullCount(1) == 0);
    REQUIRE(mylib::bit::rightmostNullCount(2) == 1);   // 2 = 10, младший бит 0, следующий 1 -> один ноль
    REQUIRE(mylib::bit::rightmostNullCount(3) == 0);
    REQUIRE(mylib::bit::rightmostNullCount(4) == 2);   // 100 -> два нуля
    REQUIRE(mylib::bit::rightmostNullCount(5) == 0);   // 101 -> младший бит 1
    REQUIRE(mylib::bit::rightmostNullCount(8) == 3);   // 1000
    REQUIRE(mylib::bit::rightmostNullCount(0x8000000000000000ULL) == 63);
    REQUIRE(mylib::bit::rightmostNullCount(0xFFFFFFFFFFFFFFFEULL) == 1); // все единицы, кроме младшего бита
    REQUIRE(mylib::bit::rightmostNullCount(mylib::bit::FULL) == 0);
}

