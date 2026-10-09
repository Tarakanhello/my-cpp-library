#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <array>
#include <bit>
#include <cstdint>
#include <numeric>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "mylib/mylib.h"

namespace
{

using Word = uint64_t;
using Bitset = mylib::Bitset<Word>;
[[maybe_unused]] constexpr size_t WORD_BITS{ std::numeric_limits<Word>::digits };

// ========================================================================
//  [NEW] Helpers: инварианты и ожидания
// ========================================================================

/**
     * @brief Проверяет инварианты Bitset<Word>.
     *        Вызывать после каждой мутации.
     */
void requireInvariants(const Bitset& b, const char* context = "")
{
    INFO("Invariant context: " << context);
    INFO("size()         = " << b.size());
    INFO("wordsSize()    = " << b.wordsSize());
    INFO("garbageBits()  = " << b.garbageBits());

    const size_t n{ b.size() };
    const size_t w{ b.wordsSize() };

    // ---- Пустой bitset -------------------------------------------------
    if (n == 0)
    {
        REQUIRE(w == 0);
        REQUIRE(b.getData() == nullptr);
        REQUIRE(b.garbageBits() == 0);
        REQUIRE(b.popcount() == 0);
        REQUIRE(b.isZero());
        REQUIRE(b.toString().empty());
        return;
    }

    // ---- Непустой ------------------------------------------------------
    const size_t expectedWords{ (n + WORD_BITS - 1) / WORD_BITS };
    REQUIRE(w == expectedWords);
    REQUIRE(b.getData() != nullptr);
    REQUIRE(b.get().size() == w);
    REQUIRE(b.get().data() == b.getData());

    const size_t lastBits{ (n % WORD_BITS == 0) ? WORD_BITS : (n % WORD_BITS) };
    REQUIRE(b.lastWordBits() == lastBits);
    REQUIRE(b.garbageBits() == WORD_BITS - lastBits);
    REQUIRE(b.lastWordBits() + b.garbageBits() == WORD_BITS);

    // Мусорные биты последнего слова обязаны быть нулевыми
    const Word lastWord{ b.getData()[w - 1] };
    const Word mask{ (lastBits == WORD_BITS)
                        ? static_cast<Word>(~Word{ 0 })
                        : static_cast<Word>((Word{ 1 } << lastBits) - Word{ 1 }) };
    REQUIRE((lastWord & static_cast<Word>(~mask)) == 0);

    // Согласованность popcount / isZero / operator bool
    size_t pc{ 0 };
    for (size_t i{ 0 }; i < w; ++i)
    {
        pc += std::popcount(b.getData()[i]);
    }
    REQUIRE(b.popcount() == pc);
    REQUIRE(b.isZero() == (pc == 0));
    REQUIRE(static_cast<bool>(b) == (pc != 0));
}

/**
     * @brief Проверяет size, toString() и equals() одновременно.
     */
[[maybe_unused]] void expectString(const Bitset& b, std::string_view expected)
{
    INFO("expected = " << expected);
    INFO("actual   = " << b.toString());
    REQUIRE(b.size() == expected.size());
    REQUIRE(b.toString() == expected);
    REQUIRE(b.equals(expected));
    REQUIRE(b.equals(b.toString()));
}

/**
     * @brief Создаёт Bitset из строки (явно через std::string,
     *        чтобы не поймать другой конструктор).
     */
[[maybe_unused]] Bitset makeBitset(std::string_view s)
{
    return Bitset(std::string(s));
}

} // end namespace




TEST_CASE("Bitset construction and basic properties", "[bitset][construction]")
{
    // ------------------------------------------------------------------------
    // 1. Конструктор по умолчанию
    // ------------------------------------------------------------------------
    SECTION("Default constructor")
    {
        Bitset b;
        REQUIRE(b.size() == 0);
        REQUIRE(b.wordsSize() == 0);
        REQUIRE(b.getData() == nullptr);
        REQUIRE(b.isZero() == true);
        REQUIRE(b.operator bool() == false);
        REQUIRE(b.popcount() == 0);
        REQUIRE(b.garbageBits() == 0);
        requireInvariants(b, "default ctor");                    // [NEW]
    }

    // ------------------------------------------------------------------------
    // 2. Конструктор с указанием количества бит
    // ------------------------------------------------------------------------
    SECTION("Constructor with initial size")
    {
        // размер 0
        Bitset b0{ 0 };
        REQUIRE(b0.size() == 0);
        REQUIRE(b0.wordsSize() == 0);
        REQUIRE(b0.isZero());
        REQUIRE(b0.popcount() == 0);
        REQUIRE(b0.garbageBits() == 0);
        requireInvariants(b0, "size=0");                          // [NEW]

        // размер 1
        Bitset b1{ 1 };
        REQUIRE(b1.size() == 1);
        REQUIRE(b1.wordsSize() == 1);
        REQUIRE(b1.lastWordBits() == 1);
        REQUIRE(b1.garbageBits() == WORD_BITS - 1);
        REQUIRE(b1.isZero());
        REQUIRE(b1.popcount() == 0);
        requireInvariants(b1, "size=1");                          // [NEW]

        // размер ровно одно слово
        Bitset bfull{ WORD_BITS };
        REQUIRE(bfull.size() == WORD_BITS);
        REQUIRE(bfull.wordsSize() == 1);
        REQUIRE(bfull.lastWordBits() == WORD_BITS);
        REQUIRE(bfull.garbageBits() == 0);
        REQUIRE(bfull.isZero());
        requireInvariants(bfull, "size=WORD_BITS");               // [NEW]

        // размер одно слово + 1 бит
        Bitset bfull1{ WORD_BITS + 1 };
        REQUIRE(bfull1.size() == WORD_BITS + 1);
        REQUIRE(bfull1.wordsSize() == 2);
        REQUIRE(bfull1.lastWordBits() == 1);
        REQUIRE(bfull1.garbageBits() == WORD_BITS - 1);
        REQUIRE(bfull1.isZero());
        requireInvariants(bfull1, "size=WORD_BITS+1");            // [NEW]

        // размер ровно два слова
        Bitset b2full{ 2 * WORD_BITS };
        REQUIRE(b2full.size() == 2 * WORD_BITS);
        REQUIRE(b2full.wordsSize() == 2);
        REQUIRE(b2full.lastWordBits() == WORD_BITS);
        REQUIRE(b2full.garbageBits() == 0);
        REQUIRE(b2full.isZero());
        requireInvariants(b2full, "size=2*WORD_BITS");            // [NEW]

        // произвольный размер (например, 100 бит)
        const size_t sz{ 100 };
        Bitset b100{ sz };
        REQUIRE(b100.size() == sz);
        size_t expectedWords{ (sz + WORD_BITS - 1) / WORD_BITS };
        REQUIRE(b100.wordsSize() == expectedWords);
        size_t lastBits{ sz % WORD_BITS };
        if (lastBits == 0)
        {
            lastBits = WORD_BITS;
        }
        REQUIRE(b100.lastWordBits() == lastBits);
        REQUIRE(b100.garbageBits() == WORD_BITS - lastBits);
        REQUIRE(b100.isZero());
        REQUIRE(b100.popcount() == 0);
        requireInvariants(b100, "size=100");                      // [NEW]
    }

    // ------------------------------------------------------------------------
    // 3. Конструктор из контейнера WORD
    // ------------------------------------------------------------------------
    SECTION("Constructor from container of WORDs")
    {
        // 3.1 Обычный вектор с двумя словами
        std::vector<Word> vec{ 0x1234567890ABCDEFull, 0xFEDCBA9876543210ull };
        Bitset b{ vec };
        REQUIRE(b.size() == vec.size() * WORD_BITS);
        REQUIRE(b.wordsSize() == vec.size());
        const Word* data{ b.getData() };
        REQUIRE(data != nullptr);
        for (size_t i{ 0 }; i < vec.size(); ++i)
        {
            REQUIRE(data[i] == vec[i]);
        }
        REQUIRE(b.lastWordBits() == WORD_BITS);
        REQUIRE(b.garbageBits() == 0);
        REQUIRE(b.isZero() == false);
        REQUIRE(b.operator bool() == true);
        requireInvariants(b, "ctor from vector(2 words)");        // [NEW]

        size_t expectedPop = 0;
        for (Word w : vec)
        {
            expectedPop += std::popcount(w);
        }
        REQUIRE(b.popcount() == expectedPop);

        // 3.2 Вектор с одним словом, все биты установлены
        std::vector<Word> vecAll{ 0xFFFFFFFFFFFFFFFFull };
        Bitset bAll{ vecAll };
        REQUIRE(bAll.size() == WORD_BITS);
        REQUIRE(bAll.wordsSize() == 1);
        REQUIRE(bAll.lastWordBits() == WORD_BITS);
        REQUIRE(bAll.garbageBits() == 0);
        REQUIRE(bAll.isZero() == false);
        REQUIRE(bAll.popcount() == WORD_BITS);
        requireInvariants(bAll, "ctor from vector(1 full word)"); // [NEW]

        // 3.3 Пустой контейнер
        std::vector<Word> empty;
        Bitset bEmpty{ empty };
        REQUIRE(bEmpty.size() == 0);
        REQUIRE(bEmpty.wordsSize() == 0);
        REQUIRE(bEmpty.getData() == nullptr);
        REQUIRE(bEmpty.isZero());
        REQUIRE(bEmpty.garbageBits() == 0);
        requireInvariants(bEmpty, "ctor from empty vector");      // [NEW]

        // 3.4 Контейнер другого типа (std::array)
        std::array<Word, 2> arr{ 0x1111, 0x2222 };
        Bitset bArr{ arr };
        REQUIRE(bArr.size() == 2 * WORD_BITS);
        REQUIRE(bArr.wordsSize() == 2);
        const Word* dataArr{ bArr.getData() };
        REQUIRE(dataArr[0] == 0x1111);
        REQUIRE(dataArr[1] == 0x2222);
        requireInvariants(bArr, "ctor from std::array");          // [NEW]
    }

    // ------------------------------------------------------------------------
    // 4. Метод get() – возвращает константную ссылку на внутренний контейнер
    // ------------------------------------------------------------------------
    SECTION("get() returns const reference to underlying container")
    {
        std::vector<Word> vec{ 1, 2, 3 };
        Bitset b{ vec };
        const auto& container{ b.get() };
        REQUIRE(container.size() == vec.size());
        for (size_t i{ 0 }; i < container.size(); ++i)
        {
            REQUIRE(container[i] == vec[i]);
        }
        REQUIRE(container.data() == b.getData());
        requireInvariants(b, "get()");                            // [NEW]
    }

    // ------------------------------------------------------------------------
    // 5. isZero() и operator bool
    // ------------------------------------------------------------------------
    SECTION("isZero and operator bool")
    {
        Bitset b1;
        REQUIRE(b1.isZero());
        REQUIRE(!b1.operator bool());

        Bitset b2{ 5 };
        REQUIRE(b2.isZero());
        REQUIRE(!b2.operator bool());

        Bitset b3{ std::vector<Word>{ 1 } };
        REQUIRE(!b3.isZero());
        REQUIRE(b3.operator bool());

        Bitset b4{ 10 };
        b4.set(0, true);
        REQUIRE(!b4.isZero());
        REQUIRE(b4.operator bool());
        requireInvariants(b4, "isZero/operator bool");            // [NEW]
    }

    // ------------------------------------------------------------------------
    // 6. popcount() – подсчёт установленных бит
    // ------------------------------------------------------------------------
    SECTION("popcount")
    {
        Bitset b1;
        REQUIRE(b1.popcount() == 0);

        Bitset b2{ 100 };
        REQUIRE(b2.popcount() == 0);

        std::vector<Word> vec{ 0b1010, 0b11110000 };
        Bitset b3(vec);
        size_t expected{ std::popcount(0b1010U) + std::popcount(0b11110000U) };
        REQUIRE(b3.popcount() == expected);

        Bitset b4{ 64 };
        b4.set(0, true);
        REQUIRE(b4.popcount() == 1);
        b4.set(63, true);
        REQUIRE(b4.popcount() == 2);
        b4.set(0, false);
        REQUIRE(b4.popcount() == 1);
        requireInvariants(b4, "popcount");                        // [NEW]
    }

    // ------------------------------------------------------------------------
    // 7. lastWordBits() и garbageBits() – детальные проверки
    // ------------------------------------------------------------------------
    SECTION("lastWordBits and garbageBits edge cases")
    {
        Bitset b0{ 0 };
        REQUIRE(b0.garbageBits() == 0);

        Bitset b1{ 1 };
        REQUIRE(b1.lastWordBits() == 1);
        REQUIRE(b1.garbageBits() == WORD_BITS - 1);

        Bitset bfull{ WORD_BITS };
        REQUIRE(bfull.lastWordBits() == WORD_BITS);
        REQUIRE(bfull.garbageBits() == 0);

        Bitset bfull1{ WORD_BITS + 1 };
        REQUIRE(bfull1.lastWordBits() == 1);
        REQUIRE(bfull1.garbageBits() == WORD_BITS - 1);

        Bitset b2full{ 2 * WORD_BITS };
        REQUIRE(b2full.lastWordBits() == WORD_BITS);
        REQUIRE(b2full.garbageBits() == 0);

        requireInvariants(b0, "lastWordBits edge b0");            // [NEW]
        requireInvariants(b1, "lastWordBits edge b1");            // [NEW]
        requireInvariants(bfull, "lastWordBits edge bfull");      // [NEW]
        requireInvariants(bfull1, "lastWordBits edge bfull1");    // [NEW]
        requireInvariants(b2full, "lastWordBits edge b2full");    // [NEW]
    }

    // ------------------------------------------------------------------------
    // 8. getData() для пустого и непустого
    // ------------------------------------------------------------------------
    SECTION("getData returns nullptr for empty, non-null otherwise")
    {
        Bitset b;
        REQUIRE(b.getData() == nullptr);

        Bitset b2{ 10 };
        REQUIRE(b2.getData() != nullptr);
        requireInvariants(b2, "getData");                         // [NEW]
    }
}




TEST_CASE("Bitset bit access and BitReference", "[bitset][access]")
{
    // ------------------------------------------------------------------------
    // 1. operator[] (non-const) – возвращает BitReference
    // ------------------------------------------------------------------------
    SECTION("operator[] non-const allows read and write")
    {
        Bitset b{ 10 };
        for (size_t i{ 0 }; i < 10; ++i)
        {
            REQUIRE(b[i] == false);
        }

        b[0] = true;
        b[3] = true;
        b[9] = true;
        REQUIRE(b[0] == true);
        REQUIRE(b[3] == true);
        REQUIRE(b[9] == true);
        REQUIRE(b[1] == false);
        REQUIRE(b[2] == false);

        auto ref{ b[0] };
        ref = false;
        REQUIRE(b[0] == false);
        ref = true;
        REQUIRE(b[0] == true);

        auto ref2{ b[9] };
        ref2 = b[1];
        REQUIRE(b[9] == false);
        ref2 = true;

        const Word* data{ b.getData() };
        REQUIRE(data != nullptr);
        Word expected{ (1ULL << 0) | (1ULL << 3) | (1ULL << 9) };
        REQUIRE(data[0] == expected);

        REQUIRE(!b.isZero());
        requireInvariants(b, "operator[] non-const");             // [NEW]

        REQUIRE_THROWS_AS(b[10], std::out_of_range);
        REQUIRE_THROWS_AS(b[100], std::out_of_range);
    }

    // ------------------------------------------------------------------------
    // 2. operator[] (const) – возвращает bool
    // ------------------------------------------------------------------------
    SECTION("operator[] const returns bool")
    {
        Bitset b{ 5 };
        b.set(2, true);
        b.set(4, true);

        const Bitset& cb{ b };
        REQUIRE(cb[0] == false);
        REQUIRE(cb[2] == true);
        REQUIRE(cb[4] == true);
        requireInvariants(b, "operator[] const");                 // [NEW]

        REQUIRE_THROWS_AS(cb[5], std::out_of_range);
        REQUIRE_THROWS_AS(cb[10], std::out_of_range);
    }

    // ------------------------------------------------------------------------
    // 3. set(size_t i, bool value) – установка бита
    // ------------------------------------------------------------------------
    SECTION("set() sets bit to given value")
    {
        Bitset b{ 8 };
        b.set(1);
        b.set(5);
        REQUIRE(b[1] == true);
        REQUIRE(b[5] == true);
        REQUIRE(b[0] == false);

        b.set(1, false);
        REQUIRE(b[1] == false);
        b.set(5, true);
        REQUIRE(b[5] == true);

        b.set(0, true);
        b.set(7, true);
        REQUIRE(b[0] == true);
        REQUIRE(b[7] == true);
        requireInvariants(b, "set()");                            // [NEW]

        REQUIRE_THROWS_AS(b.set(8, true), std::out_of_range);
        REQUIRE_THROWS_AS(b.set(100), std::out_of_range);

        Bitset big{ 100 };
        big.set(63, true);
        big.set(64, true);
        REQUIRE(big[63] == true);
        REQUIRE(big[64] == true);
        REQUIRE(big[62] == false);
        REQUIRE(big[65] == false);
        requireInvariants(big, "set() multi-word");               // [NEW]
    }

    // ------------------------------------------------------------------------
    // 4. BitReference – детальное тестирование
    // ------------------------------------------------------------------------
    SECTION("BitReference construction and operations")
    {
        Bitset b{ WORD_BITS };
        Word* ptr{ const_cast<Word*>(b.getData()) };

        // 4.1 Корректный offset
        {
            mylib::Bitset<Word>::BitReference ref{ ptr, 0 };
            REQUIRE(ref == false);
            ref = true;
            REQUIRE(ref == true);
            REQUIRE(b[0] == true);

            mylib::Bitset<Word>::BitReference ref2{ ptr, 63 };
            REQUIRE(ref2 == false);
            ref2 = true;
            REQUIRE(ref2 == true);
            REQUIRE(b[63] == true);
        }

        // 4.2 Присваивание от другого BitReference
        {
            mylib::Bitset<Word>::BitReference refA{ ptr, 5 };
            mylib::Bitset<Word>::BitReference refB{ ptr, 10 };
            refA = true;
            refB = false;
            refB = refA;
            REQUIRE(refB == true);
            REQUIRE(b[10] == true);
            REQUIRE(refA == true);
        }

        // 4.3 Исключение при offset >= WORD_BITS
        {
            REQUIRE_THROWS_AS((mylib::Bitset<Word>::BitReference(ptr, WORD_BITS)), std::out_of_range);
            REQUIRE_THROWS_AS((mylib::Bitset<Word>::BitReference(ptr, WORD_BITS + 1)), std::out_of_range);
        }

        // 4.4 BitReference живёт после clear()
        {
            mylib::Bitset<Word>::BitReference ref{ ptr, 20 };
            ref = true;
            REQUIRE(b[20] == true);
            b.clear();
            REQUIRE(ref == false);
            REQUIRE(b[20] == false);
        }

        requireInvariants(b, "BitReference");                     // [NEW]
    }
}




TEST_CASE("Bitset size modification (prepend, prepend, removeLast)", "[bitset][modifiers]")
{
    // ------------------------------------------------------------------------
    // 1. appendMSB(bool value)
    // ------------------------------------------------------------------------
    SECTION("prepend single bit")
    {
        Bitset b;
        b.appendMSB(true);
        REQUIRE(b.size() == 1);
        REQUIRE(b.wordsSize() == 1);
        REQUIRE(b[0] == true);
        REQUIRE(b.popcount() == 1);
        REQUIRE(!b.isZero());
        requireInvariants(b, "appendMSB(true) empty");            // [NEW]

        b.appendMSB(false);
        REQUIRE(b.size() == 2);
        REQUIRE(b.wordsSize() == 1);
        REQUIRE(b[0] == true);
        REQUIRE(b[1] == false);
        REQUIRE(b.popcount() == 1);
        REQUIRE(!b.isZero());
        requireInvariants(b, "appendMSB(false)");                 // [NEW]

        Bitset b2;
        for (size_t i{ 0 }; i < WORD_BITS; ++i)
        {
            b2.appendMSB(i % 2 == 0);
        }
        REQUIRE(b2.size() == WORD_BITS);
        REQUIRE(b2.wordsSize() == 1);
        REQUIRE(b2.lastWordBits() == WORD_BITS);
        REQUIRE(b2.garbageBits() == 0);
        for (size_t i{ 0 }; i < WORD_BITS; ++i)
        {
            REQUIRE(b2[i] == (i % 2 == 0));
        }
        REQUIRE(b2.popcount() == WORD_BITS / 2);
        requireInvariants(b2, "appendMSB x WORD_BITS");           // [NEW]

        b2.appendMSB(true);
        REQUIRE(b2.size() == WORD_BITS + 1);
        REQUIRE(b2.wordsSize() == 2);
        REQUIRE(b2.lastWordBits() == 1);
        REQUIRE(b2.garbageBits() == WORD_BITS - 1);
        REQUIRE(b2[WORD_BITS] == true);
        REQUIRE(b2.popcount() == WORD_BITS / 2 + 1);
        requireInvariants(b2, "appendMSB cross word");            // [NEW]
    }

    // ------------------------------------------------------------------------
    // 2. appendMSB(WORD value, size_t size)
    // ------------------------------------------------------------------------
    SECTION("prepend WORD value with specified number of bits")
    {
        Bitset b;

        b.appendMSB(0x1, 1);
        REQUIRE(b.size() == 1);
        REQUIRE(b[0] == true);
        REQUIRE(b.popcount() == 1);

        b.appendMSB(0b100, 3);
        REQUIRE(b.size() == 4);
        REQUIRE(b[0] == true);
        REQUIRE(b[1] == false);
        REQUIRE(b[2] == false);
        REQUIRE(b[3] == true);
        REQUIRE(b.popcount() == 2);
        requireInvariants(b, "appendMSB(WORD,size) #1");          // [NEW]

        // size == 0 → по доке std::out_of_range. Тест в audit-секции.

        REQUIRE_THROWS_AS(b.appendMSB(0, WORD_BITS + 1), std::out_of_range);
        requireInvariants(b, "after throw appendMSB(size>WB)");   // [NEW]

        Word val{ 0xFFFFFFFFFFFFFFFF };
        b.appendMSB(val, WORD_BITS);
        REQUIRE(b.size() == WORD_BITS + 4);
        REQUIRE(b.wordsSize() == 2);
        for (size_t i{ 4 }; i < WORD_BITS; ++i)
        {
            REQUIRE(b[i] == true);
        }
        REQUIRE(b.popcount() == WORD_BITS + 2);
        requireInvariants(b, "appendMSB full word");              // [NEW]

        Bitset b3;
        b3.appendMSB(0b1111, 2);
        REQUIRE(b3.size() == 2);
        REQUIRE(b3[0] == true);
        REQUIRE(b3[1] == true);
        REQUIRE(b3.popcount() == 2);
        requireInvariants(b3, "appendMSB mask");                  // [NEW]
    }

    // ------------------------------------------------------------------------
    // 3. appendLSB(const Bitset& other)
    // ------------------------------------------------------------------------
    SECTION("prepend another Bitset")
    {
        Bitset a{ "100" };
        Bitset b{ "01" };

        REQUIRE(a.equals("100"));
        a.appendLSB(b);
        REQUIRE(a.size() == 5);
        REQUIRE(a.equals("10001"));
        REQUIRE(b.size() == 2);
        REQUIRE(b.equals("01"));
        requireInvariants(a, "appendLSB(bitset)");                // [NEW]

        Bitset c{ "111" };
        Bitset empty;
        c.appendLSB(empty);
        REQUIRE(c.size() == 3);
        REQUIRE(c.equals("111"));
        REQUIRE(empty.size() == 0);
        requireInvariants(c, "appendLSB(empty)");                 // [NEW]

        // Self-append
        Bitset d{ "10" };
        d.appendLSB(d);
        REQUIRE(d.size() == 4);
        REQUIRE(d.equals("1010"));                                // [FIX] было без REQUIRE
        requireInvariants(d, "self-appendLSB");                   // [NEW]

        Bitset big1(100);
        for (size_t i{}; i < big1.size(); i += 2)
        {
            big1.set(i);
        }

        Bitset bigCheck{ big1 };
        REQUIRE(big1 == bigCheck);

        Bitset big2(50);
        for (size_t i{}; i < big2.size(); i += 3)
        {
            big2.set(i);
        }

        size_t oldSize{ big1.size() };
        big1.appendLSB(big2);
        REQUIRE(big1.size() == oldSize + big2.size());

        for (size_t i{}; i < big2.size(); ++i)
        {
            REQUIRE(big1[i] == big2[i]);
        }

        for (size_t i{ big2.size() }; i < big1.size(); ++i)
        {
            REQUIRE(big1[i] == bigCheck[i - big2.size()]);
        }
        requireInvariants(big1, "appendLSB(big2)");               // [NEW]
    }

    // ------------------------------------------------------------------------
    // 4. removeMSB / popMSB
    // ------------------------------------------------------------------------
    SECTION("removeLast and pop_back")
    {
        Bitset b{ "1101" };
        REQUIRE(b.size() == 4);

        b.removeMSB();
        REQUIRE(b.size() == 3);
        REQUIRE(b.equals("101"));                                 // [FIX] было без REQUIRE
        requireInvariants(b, "after removeMSB");                  // [NEW]

        b.popMSB();
        REQUIRE(b.size() == 2);
        REQUIRE(b.equals("01"));                                  // [FIX] было без REQUIRE

        b.removeMSB();
        REQUIRE(b.size() == 1);
        REQUIRE(b.equals("1"));                                   // [FIX] было без REQUIRE

        b.removeMSB();
        REQUIRE(b.size() == 0);
        REQUIRE(b.wordsSize() == 0);
        REQUIRE(b.isZero());
        requireInvariants(b, "empty after removes");              // [NEW]

        b.appendLSB(true);
        REQUIRE(b.size() == 1);
        REQUIRE(b[0] == true);
        requireInvariants(b, "appendLSB after empty");            // [NEW]

        Bitset big(100);
        big.set(99, true);
        REQUIRE(big.size() == 100);
        big.removeMSB();
        REQUIRE(big.size() == 99);
        REQUIRE_THROWS_AS(big[99], std::out_of_range);
        big.set(98, true);
        REQUIRE(big[98] == true);
        big.removeMSB();
        REQUIRE(big.size() == 98);
        big.set(97, true);
        REQUIRE(big[97] == true);
        requireInvariants(big, "removeMSB multi-word");           // [NEW]

        Bitset c(WORD_BITS + 5);
        c.set(WORD_BITS + 3, true);
        c.removeMSB();
        REQUIRE(c.size() == WORD_BITS + 4);
        REQUIRE(c[WORD_BITS + 3] == true);

        for (int i = 0; i < 4; ++i)
            c.removeMSB();
        REQUIRE(c.size() == WORD_BITS);
        REQUIRE(c.wordsSize() == 1);
        REQUIRE(c[WORD_BITS - 1] == false);
        const Word* data = c.getData();
        REQUIRE(data[0] == 0);
        requireInvariants(c, "removeMSB shrink word");            // [NEW]
    }
}




TEST_CASE("Bitset comparison operators", "[bitset][comparison]")
{
    // ------------------------------------------------------------------------
    // 1. operator== and operator!=
    // ------------------------------------------------------------------------
    SECTION("Equality and inequality")
    {
        Bitset a{ "1010" };
        Bitset b{ "1010" };
        REQUIRE(a == b);
        REQUIRE_FALSE(a != b);

        Bitset c{ "1011" };
        REQUIRE_FALSE(a == c);
        REQUIRE(a != c);

        Bitset d{ "10" };
        REQUIRE_FALSE(a == d);
        REQUIRE(a != d);

        Bitset e1, e2;
        REQUIRE(e1 == e2);
        REQUIRE_FALSE(e1 != e2);

        REQUIRE_FALSE(e1 == a);
        REQUIRE(e1 != a);

        REQUIRE(a == a);
        REQUIRE_FALSE(a != a);

        Bitset zeros(10);
        Bitset ones(10);
        ones.setAll(true);
        REQUIRE_FALSE(zeros == ones);
        REQUIRE(zeros != ones);

        Bitset big1(std::vector<Word>{ 0x1234567890ABCDEFull, 0xFEDCBA9876543210ull });
        Bitset big2(std::vector<Word>{ 0x1234567890ABCDEFull, 0xFEDCBA9876543210ull });
        REQUIRE(big1 == big2);

        Bitset big3(std::vector<Word>{ 0x1234567890ABCDEFull, 0xFEDCBA9876543211ull });
        REQUIRE(big1 != big3);

        Bitset bs1(1);
        Bitset bs2(2);
        bs1.set(0);
        bs2.set(0);
        REQUIRE_FALSE(bs1 == bs2);

        mylib::Bitset<std::uint8_t> bs{ std::vector<std::uint8_t>{ 0xFF, 0xFF, 0xFF } };
        REQUIRE(bs.size() == 24);
        REQUIRE(bs.toString() == "111111111111111111111111");

        bs <<= 16;
        REQUIRE(bs.toString() == "111111110000000000000000");
    }

    // ------------------------------------------------------------------------
    // 2. Three-way comparison
    // ------------------------------------------------------------------------
    SECTION("Three-way comparison and relational operators")
    {
        Bitset small{ "111" };
        Bitset large{ "0000" };
        REQUIRE(small < large);
        REQUIRE(small <= large);
        REQUIRE(large > small);
        REQUIRE(large >= small);
        REQUIRE_FALSE(small > large);
        REQUIRE_FALSE(small >= large);
        REQUIRE_FALSE(large < small);
        REQUIRE_FALSE(large <= small);

        Bitset a{ "01" };
        Bitset b{ "10" };
        REQUIRE(a < b);
        REQUIRE(a <= b);
        REQUIRE(b > a);
        REQUIRE(b >= a);

        Bitset c{ "11" };
        Bitset d{ "10" };
        REQUIRE(c > d);
        REQUIRE(c >= d);
        REQUIRE(d < c);
        REQUIRE(d <= c);

        REQUIRE(a == a);
        REQUIRE(a <= a);
        REQUIRE(a >= a);
        REQUIRE_FALSE(a < a);
        REQUIRE_FALSE(a > a);

        Bitset big1(WORD_BITS + 1);
        Bitset big2(WORD_BITS + 1);
        big1.set(0, true);
        big2.set(WORD_BITS, true);
        REQUIRE(big1 < big2);
        REQUIRE(big1 <= big2);
        REQUIRE(big2 > big1);
        REQUIRE(big2 >= big1);

        Bitset big3(WORD_BITS + 1);
        big3.set(0, true);
        big3.set(WORD_BITS, true);
        REQUIRE(big1 < big3);
        REQUIRE(big3 > big1);

        Bitset empty1, empty2;
        REQUIRE(empty1 == empty2);
        REQUIRE(empty1 <= empty2);
        REQUIRE(empty1 >= empty2);
        REQUIRE_FALSE(empty1 < empty2);
        REQUIRE_FALSE(empty1 > empty2);

        REQUIRE(empty1 < a);
        REQUIRE(a > empty1);
        REQUIRE(empty1 <= a);
        REQUIRE(a >= empty1);

        Bitset x(2 * WORD_BITS);
        Bitset y(2 * WORD_BITS);
        Bitset z(2 * WORD_BITS);

        x.setValue(5, 0, WORD_BITS);
        x.setValue(10, WORD_BITS, WORD_BITS);

        y.setValue(5, 0, WORD_BITS);
        y.setValue(20, WORD_BITS, WORD_BITS);

        z.setValue(6, 0, WORD_BITS);
        z.setValue(0, WORD_BITS, WORD_BITS);

        REQUIRE(x < y);
        REQUIRE(x <= y);
        REQUIRE(y > x);
        REQUIRE(y >= x);

        REQUIRE(x > z);
        REQUIRE(z < x);

        REQUIRE(y > z);
        REQUIRE(z < y);
    }

    // ------------------------------------------------------------------------
    // 3. Direct spaceship
    // ------------------------------------------------------------------------
    SECTION("Direct use of spaceship operator")
    {
        Bitset a{ "101" };
        Bitset b{ "110" };
        Bitset c{ "1010" };

        REQUIRE((a <=> b) < 0);
        REQUIRE((a <=> b) <= 0);
        REQUIRE_FALSE((a <=> b) > 0);
        REQUIRE_FALSE((a <=> b) >= 0);
        REQUIRE((a <=> a) == 0);
        REQUIRE((a <=> c) < 0);
        REQUIRE((c <=> a) > 0);
    }

    // ------------------------------------------------------------------------
    // 4. Garbage bits
    // ------------------------------------------------------------------------
    SECTION("Garbage bits do not affect comparison")
    {
        Bitset a(WORD_BITS + 5);
        Bitset b(WORD_BITS + 5);

        a.set(0, true);
        a.set(WORD_BITS + 3, true);
        b.set(0, true);
        b.set(WORD_BITS + 3, true);

        REQUIRE(a == b);

        b.set(WORD_BITS + 4, true);

        REQUIRE(a != b);
        REQUIRE(a < b);
        REQUIRE(b > a);
    }
}




// ============================================================================
//  [NEW] AUDIT: контракты, которые нужно явно зафиксировать тестами
// ============================================================================
TEST_CASE("Bitset contract audit", "[bitset][audit]")
{
    SECTION("appendMSB(WORD, 0) throws out_of_range")
    {
        Bitset b;
        REQUIRE_THROWS_AS(b.appendMSB(Word{ 1 }, 0), std::out_of_range);
        REQUIRE(b.size() == 0);
        requireInvariants(b, "after throw appendMSB(size=0)");
    }

    SECTION("appendLSB(WORD, 0) is a no-op")
    {
        Bitset b{ "101" };
        const std::string before{ b.toString() };
        REQUIRE_NOTHROW(b.appendLSB(Word{ 1 }, 0));
        REQUIRE(b.toString() == before);
        requireInvariants(b, "after appendLSB(_,0)");
    }

    SECTION("getValue with n==0 returns 0 and does not throw")
    {
        Bitset b{ "1010" };
        REQUIRE(b.getValue(0, 0) == 0);
        REQUIRE(b.getValue(b.size(), 0) == 0);
        REQUIRE_THROWS_AS(b.getValue(0, WORD_BITS + 1), std::out_of_range);
        requireInvariants(b, "after getValue n=0");
    }

    SECTION("setValue with n==0 is a no-op")
    {
        Bitset b{ "1010" };
        const std::string before{ b.toString() };
        REQUIRE_NOTHROW(b.setValue(Word{ 0xFF }, 0, 0));
        REQUIRE(b.toString() == before);
        requireInvariants(b, "after setValue n=0");
    }

    SECTION("shifts on empty bitset are safe")
    {
        Bitset b;
        REQUIRE_NOTHROW(b <<= 5);
        REQUIRE_NOTHROW(b >>= 5);
        REQUIRE(b.size() == 0);
        requireInvariants(b, "after shift of empty");
    }

    SECTION("appendMSB(empty bitset) is a no-op")
    {
        Bitset a{ "1011" };
        Bitset e;
        const std::string before{ a.toString() };
        a.appendMSB(e);
        REQUIRE(a.toString() == before);
        requireInvariants(a, "after appendMSB(empty)");
    }

    SECTION("appendLSB(empty bitset) is a no-op")
    {
        Bitset a{ "1011" };
        Bitset e;
        const std::string before{ a.toString() };
        a.appendLSB(e);
        REQUIRE(a.toString() == before);
        requireInvariants(a, "after appendLSB(empty)");
    }

    SECTION("appendLSB(empty string_view) is a no-op")
    {
        Bitset a{ "1011" };
        const std::string before{ a.toString() };
        REQUIRE_NOTHROW(a.appendLSB(std::string_view{ }));
        REQUIRE(a.toString() == before);
        requireInvariants(a, "after appendLSB(\"\")");
    }

    SECTION("setFromString(\"\") is a no-op")
    {
        Bitset a{ "1011" };
        const std::string before{ a.toString() };
        REQUIRE_NOTHROW(a.setFromString(std::string_view{ }));
        REQUIRE(a.toString() == before);
        requireInvariants(a, "after setFromString(\"\")");
    }

    SECTION("reverse on empty / single bit")
    {
        Bitset e;
        REQUIRE_NOTHROW(e.reverse());
        requireInvariants(e, "reverse empty");

        Bitset one{ "1" };
        one.reverse();
        REQUIRE(one.equals("1"));
        requireInvariants(one, "reverse single");
    }
}

// ============================================================================
//  ЭТАП 1. Конструктор из строки
// ============================================================================
TEST_CASE("Bitset string constructor", "[bitset][construction][string]")
{
    // ------------------------------------------------------------------------
    // 1. Пустая строка -> пустой bitset
    // ------------------------------------------------------------------------
    SECTION("Empty string produces empty bitset")
    {
        Bitset b{ std::string{} };
        REQUIRE(b.size() == 0);
        REQUIRE(b.wordsSize() == 0);
        REQUIRE(b.getData() == nullptr);
        REQUIRE(b.isZero());
        REQUIRE(b.toString() == "");
        requireInvariants(b, "string ctor empty");
    }

    // ------------------------------------------------------------------------
    // 2. Один символ
    // ------------------------------------------------------------------------
    SECTION("Single character")
    {
        Bitset b0{ std::string("0") };
        REQUIRE(b0.size() == 1);
        REQUIRE(b0[0] == false);
        REQUIRE(b0.toString() == "0");
        requireInvariants(b0, "string ctor '0'");

        Bitset b1{ std::string("1") };
        REQUIRE(b1.size() == 1);
        REQUIRE(b1[0] == true);
        REQUIRE(b1.popcount() == 1);
        REQUIRE(b1.toString() == "1");
        requireInvariants(b1, "string ctor '1'");
    }

    // ------------------------------------------------------------------------
    // 3. Различные валидные строки, MSB-first
    // ------------------------------------------------------------------------
    SECTION("Various valid strings (MSB first)")
    {
        for (auto s : { "0", "1", "10", "01", "1010", "11110000",
                       "00000001", "10000000", "11111111" })
        {
            const std::string_view sv{ s };
            Bitset b{ std::string(sv) };
            INFO("input = " << s);
            REQUIRE(b.size() == sv.size());
            REQUIRE(b.toString() == sv);
            REQUIRE(b.equals(sv));
            requireInvariants(b, "string ctor various");
        }
    }

    // ------------------------------------------------------------------------
    // 4. Соответствие бит -> индексов: строка MSB-first
    //    Последний символ строки = бит 0, первый символ = бит size()-1
    // ------------------------------------------------------------------------
    SECTION("MSB-first mapping of characters to bit indices")
    {
        Bitset b{ std::string("1001") };
        REQUIRE(b.size() == 4);
        REQUIRE(b[3] == true);    // '1' первый
        REQUIRE(b[2] == false);
        REQUIRE(b[1] == false);
        REQUIRE(b[0] == true);    // '1' последний
        requireInvariants(b, "MSB-first mapping");
    }

    // ------------------------------------------------------------------------
    // 5. Длинная строка, пересекающая границы слов
    // ------------------------------------------------------------------------
    SECTION("Long string spanning multiple words")
    {
        const size_t len{ WORD_BITS * 3 + 7 };
        std::string s;
        s.reserve(len);
        for (size_t i = 0; i < len; ++i)
        {
            s += (i % 3 == 0) ? '1' : '0';
        }
        Bitset b{ s };
        REQUIRE(b.size() == len);
        REQUIRE(b.toString() == s);
        requireInvariants(b, "string ctor long");
    }

    // ------------------------------------------------------------------------
    // 6. Все единицы
    // ------------------------------------------------------------------------
    SECTION("All ones string")
    {
        std::string s(100, '1');
        Bitset b{ s };
        REQUIRE(b.size() == 100);
        REQUIRE(b.popcount() == 100);
        REQUIRE(b.toString() == s);
        requireInvariants(b, "string ctor all ones");
    }

    // ------------------------------------------------------------------------
    // 7. Невалидные символы -> std::invalid_argument
    // ------------------------------------------------------------------------
    SECTION("Invalid characters throw std::invalid_argument")
    {
        for (auto s : { "2", "102", "abc", "  ", "10 1", "1.0", "one", "-1" })
        {
            INFO("input = " << s);
            REQUIRE_THROWS_AS(Bitset(std::string(s)), std::invalid_argument);
        }
    }

    // ------------------------------------------------------------------------
    // 8. Round-trip: string -> bitset -> string
    // ------------------------------------------------------------------------
    SECTION("Round-trip string -> bitset -> string")
    {
        for (auto s : { "0", "1", "10", "111", "1000", "10101",
                       "1111111111", "0", "0000", "10101010" })
        {
            Bitset b{ std::string(s) };
            REQUIRE(b.toString() == s);
            REQUIRE(b.equals(s));
        }
    }

    // ------------------------------------------------------------------------
    // 9. Разные контейнеры-обёртки строки
    //    (std::string vs std::string_view через явное преобразование)
    // ------------------------------------------------------------------------
    SECTION("std::string and std::string_view produce identical bitsets")
    {
        const char* raw{ "110100101" };
        Bitset a{ std::string(raw) };
        Bitset b{ std::string(std::string_view(raw)) };
        REQUIRE(a == b);
        REQUIRE(a.toString() == raw);
        requireInvariants(a, "string vs string_view");
    }
}

// ============================================================================
//  ЭТАП 1. Rule of Five
// ============================================================================
TEST_CASE("Bitset Rule of Five", "[bitset][rule_of_five]")
{
    // ------------------------------------------------------------------------
    // 1. Copy constructor — глубокая копия
    // ------------------------------------------------------------------------
    SECTION("Copy constructor performs deep copy")
    {
        Bitset original{ "101101" };
        Bitset copy{ original };

        REQUIRE(copy == original);
        REQUIRE(copy.size() == original.size());
        REQUIRE(copy.wordsSize() == original.wordsSize());
        REQUIRE(copy.getData() != original.getData());    // разные буферы
        requireInvariants(copy, "copy ctor");

        // Изменяем копию — оригинал не меняется
        copy.set(0, false);       // LSB
        copy.set(5, false);      // MSB
        REQUIRE(original.equals("101101"));
        REQUIRE(copy.equals("001100"));
        requireInvariants(original, "original after copy mutation");
        requireInvariants(copy, "copy after mutation");
    }

    // ------------------------------------------------------------------------
    // 2. Copy assignment — глубокая копия
    // ------------------------------------------------------------------------
    SECTION("Copy assignment performs deep copy")
    {
        Bitset src{ "1000" };
        Bitset dst{ "1111" };
        REQUIRE(dst.getData() != src.getData());

        dst = src;
        REQUIRE(dst == src);
        REQUIRE(dst.equals("1000"));
        REQUIRE(dst.getData() != src.getData());

        dst.set(0, true);
        REQUIRE(dst.equals("1001"));
        REQUIRE(src.equals("1000"));   // src не изменился
        requireInvariants(src, "src after copy assign");
        requireInvariants(dst, "dst after copy assign");
    }

    // ------------------------------------------------------------------------
    // 3. Copy assignment на разные размеры
    // ------------------------------------------------------------------------
    SECTION("Copy assignment from different size")
    {
        Bitset small{ "10" };
        Bitset big{ 200 };   // 200 нулей
        REQUIRE(big.size() == 200);

        big = small;
        REQUIRE(big.size() == 2);
        REQUIRE(big.equals("10"));
        REQUIRE(big.wordsSize() == 1);
        requireInvariants(big, "assign small to big");
    }

    // ------------------------------------------------------------------------
    // 4. Self copy-assignment
    // ------------------------------------------------------------------------
    SECTION("Self copy-assignment is safe")
    {
        Bitset a{ "1010" };
        Bitset& alias = a;
        a = alias;                 // через reference, чтобы не ловить -Wself-assign
        REQUIRE(a.equals("1010"));
        requireInvariants(a, "self copy assign");
    }

    // ------------------------------------------------------------------------
    // 5. Move constructor
    // ------------------------------------------------------------------------
    SECTION("Move constructor transfers data")
    {
        Bitset source{ "101101" };
        [[maybe_unused]] const Word* sourceData{ source.getData() };
        [[maybe_unused]] const size_t sourceSize{ source.size() };
        [[maybe_unused]] const size_t sourceWords{ source.wordsSize() };

        Bitset moved{ std::move(source) };

        REQUIRE(moved.size() == sourceSize);
        REQUIRE(moved.wordsSize() == sourceWords);
        REQUIRE(moved.equals("101101"));
        REQUIRE(moved.getData() == sourceData);   // данные переехали без realloc
        requireInvariants(moved, "move ctor dest");

        // source — валидный, но unspecified. Главное — можно переиспользовать.
        REQUIRE_NOTHROW(source.clear());
        source = makeBitset("11");
        REQUIRE(source.equals("11"));
        requireInvariants(source, "source reused after move");
    }

    // ------------------------------------------------------------------------
    // 6. Move assignment
    // ------------------------------------------------------------------------
    SECTION("Move assignment transfers data")
    {
        Bitset source{ "110010" };
        Bitset dest{ "1111" };
        const Word* sourceData{ source.getData() };

        dest = std::move(source);
        REQUIRE(dest.equals("110010"));
        REQUIRE(dest.getData() == sourceData);
        requireInvariants(dest, "move assign dest");

        REQUIRE_NOTHROW(source.clear());
        source = makeBitset("0");
        REQUIRE(source.equals("0"));
        requireInvariants(source, "source reused after move assign");
    }

    // ------------------------------------------------------------------------
    // 7. Self move-assignment (valid, не падает)
    // ------------------------------------------------------------------------
    SECTION("Self move-assignment leaves object valid")
    {
        Bitset a{ "1010" };
        Bitset& alias = a;
        a = std::move(alias);
        // valid but unspecified: достаточно, что объект пригоден к использованию
        REQUIRE_NOTHROW(a.clear());
        a = makeBitset("01");
        REQUIRE(a.equals("01"));
        requireInvariants(a, "after self move assign");
    }

    // ------------------------------------------------------------------------
    // 8. Цепочка копий: изменения не «протекают»
    // ------------------------------------------------------------------------
    SECTION("Chain of copies: no aliasing")
    {
        Bitset a{ "1000" };
        Bitset b{ a };
        Bitset c{ b };

        c.set(0, true);   // только c
        b.set(1, true);   // только b

        REQUIRE(a.equals("1000"));
        REQUIRE(b.equals("1010"));
        REQUIRE(c.equals("1001"));
        requireInvariants(a, "chain a");
        requireInvariants(b, "chain b");
        requireInvariants(c, "chain c");
    }

    // ------------------------------------------------------------------------
    // 9. Интеграция с std::vector (move при реаллокации)
    // ------------------------------------------------------------------------
    SECTION("Bitset works inside std::vector with reallocation")
    {
        std::vector<Bitset> v;
        v.reserve(1);
        v.push_back(Bitset{ std::string("1010") });
        for (int i = 0; i < 20; ++i)
        {
            v.push_back(Bitset{ std::string("110") });
        }
        REQUIRE(v.front().equals("1010"));
        for (size_t i = 1; i < v.size(); ++i)
        {
            REQUIRE(v[i].equals("110"));
        }
    }

    // ------------------------------------------------------------------------
    // 10. Проверка noexcept / type traits
    // ------------------------------------------------------------------------
    SECTION("Type traits")
    {
        STATIC_REQUIRE(std::is_default_constructible_v<Bitset>);
        STATIC_REQUIRE(std::is_copy_constructible_v<Bitset>);
        STATIC_REQUIRE(std::is_copy_assignable_v<Bitset>);
        STATIC_REQUIRE(std::is_nothrow_move_constructible_v<Bitset>);
        STATIC_REQUIRE(std::is_nothrow_move_assignable_v<Bitset>);
        STATIC_REQUIRE(std::is_nothrow_destructible_v<Bitset>);
    }
}

// ============================================================================
//  ЭТАП 2. Размеры, capacity и инварианты
// ============================================================================
TEST_CASE("Bitset sizes, capacity and invariants", "[bitset][sizes]")
{
    // ------------------------------------------------------------------------
    // 1. Формула wordsSize = ceil(size / WORD_BITS) на всём диапазоне
    // ------------------------------------------------------------------------
    SECTION("wordsSize formula across sizes")
    {
        for (size_t n : std::initializer_list<size_t>{ 0u, 1u, 2u, WORD_BITS - 1, WORD_BITS,
                         WORD_BITS + 1, WORD_BITS + 2,
                         2 * WORD_BITS - 1, 2 * WORD_BITS, 2 * WORD_BITS + 1,
                         3 * WORD_BITS, 3 * WORD_BITS + 5, 100u, 1000u })
        {
            INFO("n = " << n);
            Bitset b{ n };
            const size_t expectedWords{ (n + WORD_BITS - 1) / WORD_BITS };
            REQUIRE(b.size() == n);
            REQUIRE(b.wordsSize() == expectedWords);
            requireInvariants(b, "wordsSize formula");
        }
    }

    // ------------------------------------------------------------------------
    // 2. lastWordBits + garbageBits == WORD_BITS всегда (кроме пустого)
    // ------------------------------------------------------------------------
    SECTION("lastWordBits + garbageBits == WORD_BITS")
    {
        for (size_t n : std::initializer_list<size_t>{ 1u, WORD_BITS - 1, WORD_BITS, WORD_BITS + 1,
                         WORD_BITS + 7, 2 * WORD_BITS, 2 * WORD_BITS + 3 })
        {
            INFO("n = " << n);
            Bitset b{ n };
            REQUIRE(b.lastWordBits() + b.garbageBits() == WORD_BITS);

            const size_t expectedLast{ (n % WORD_BITS == 0) ? WORD_BITS : (n % WORD_BITS) };
            REQUIRE(b.lastWordBits() == expectedLast);
            REQUIRE(b.garbageBits() == WORD_BITS - expectedLast);
        }
    }

    // ------------------------------------------------------------------------
    // 3. lastWordBits для кратных WORD_BITS == WORD_BITS, garbage == 0
    // ------------------------------------------------------------------------
    SECTION("Multiples of WORD_BITS have zero garbage")
    {
        for (size_t mult : { 1u, 2u, 3u, 5u, 10u })
        {
            const size_t n{ mult * WORD_BITS };
            INFO("n = " << n);
            Bitset b{ n };
            REQUIRE(b.lastWordBits() == WORD_BITS);
            REQUIRE(b.garbageBits() == 0);
            requireInvariants(b, "multiple of WORD_BITS");
        }
    }

    // ------------------------------------------------------------------------
    // 4. Пустой bitset: специальные значения
    // ------------------------------------------------------------------------
    SECTION("Empty bitset special values")
    {
        Bitset b;
        REQUIRE(b.size() == 0);
        REQUIRE(b.wordsSize() == 0);
        REQUIRE(b.getData() == nullptr);
        REQUIRE(b.garbageBits() == 0);
        REQUIRE(b.popcount() == 0);
        REQUIRE(b.isZero());
        REQUIRE(b.toString().empty());
        requireInvariants(b, "empty special values");
    }

    // ------------------------------------------------------------------------
    // 5. get() и getData() возвращают согласованные данные
    // ------------------------------------------------------------------------
    SECTION("get() and getData() are consistent")
    {
        for (size_t n : std::initializer_list<size_t>{ 1u, WORD_BITS, WORD_BITS + 1, 3 * WORD_BITS + 7 })
        {
            INFO("n = " << n);
            Bitset b{ n };
            b.setAll(true);

            const auto& c{ b.get() };
            REQUIRE(c.size() == b.wordsSize());
            REQUIRE(c.data() == b.getData());

            // Данные через get() и getData() идентичны
            for (size_t i = 0; i < c.size(); ++i)
            {
                REQUIRE(c[i] == b.getData()[i]);
            }
            requireInvariants(b, "get/getData consistency");
        }
    }

    // ------------------------------------------------------------------------
    // 6. Изменение битов не меняет size/wordsSize/lastWordBits/garbageBits
    // ------------------------------------------------------------------------
    SECTION("Setting bits does not change size metadata")
    {
        Bitset b{ 100 };
        const size_t n0{ b.size() };
        const size_t w0{ b.wordsSize() };
        const size_t lw0{ b.lastWordBits() };
        const size_t gb0{ b.garbageBits() };

        b.set(0, true);
        b.set(99, true);
        b.setAll(true);
        b.flip();
        b.reverse();

        REQUIRE(b.size() == n0);
        REQUIRE(b.wordsSize() == w0);
        REQUIRE(b.lastWordBits() == lw0);
        REQUIRE(b.garbageBits() == gb0);
        requireInvariants(b, "metadata invariant under bit ops");
    }

    // ------------------------------------------------------------------------
    // 7. clear() не меняет размер, только обнуляет биты
    // ------------------------------------------------------------------------
    SECTION("clear() preserves size metadata")
    {
        Bitset b{ WORD_BITS + 5 };
        b.setAll(true);
        const size_t n0{ b.size() };
        const size_t w0{ b.wordsSize() };
        const size_t lw0{ b.lastWordBits() };
        const size_t gb0{ b.garbageBits() };

        b.clear();

        REQUIRE(b.size() == n0);
        REQUIRE(b.wordsSize() == w0);
        REQUIRE(b.lastWordBits() == lw0);
        REQUIRE(b.garbageBits() == gb0);
        REQUIRE(b.isZero());
        REQUIRE(b.popcount() == 0);
        requireInvariants(b, "clear preserves metadata");
    }

    // ------------------------------------------------------------------------
    // 8. Мусорные биты последнего слова всегда нулевые после setAll
    // ------------------------------------------------------------------------
    SECTION("Garbage bits stay zero after setAll")
    {
        for (size_t n : std::initializer_list<size_t>{ 1u, WORD_BITS - 1, WORD_BITS + 3, 2 * WORD_BITS + 7 })
        {
            INFO("n = " << n);
            Bitset b{ n };
            b.setAll(true);

            const size_t w{ b.wordsSize() };
            const size_t lastBits{ b.lastWordBits() };
            const Word lastWord{ b.getData()[w - 1] };

            // Верхние (garbageBits) биты последнего слова должны быть 0
            if (lastBits < WORD_BITS)
            {
                const Word mask{ static_cast<Word>((Word{ 1 } << lastBits) - Word{ 1 }) };
                REQUIRE((lastWord & static_cast<Word>(~mask)) == 0);
            }
            // popcount == size, т.к. все валидные биты == 1
            REQUIRE(b.popcount() == n);
        }
    }

    // ------------------------------------------------------------------------
    // 9. Заполнение нулями через setAll(false) не портит валидные биты
    // ------------------------------------------------------------------------
    SECTION("setAll(false) zeros all valid bits")
    {
        Bitset b{ 130 };
        b.setAll(true);
        REQUIRE(b.popcount() == 130);
        b.setAll(false);
        REQUIRE(b.isZero());
        REQUIRE(b.popcount() == 0);
        requireInvariants(b, "setAll(false)");
    }

    // ------------------------------------------------------------------------
    // 10. Порядок бит MSB-first сохраняется при разных размерах
    //     (последний символ toString == младший индекс)
    // ------------------------------------------------------------------------
    SECTION("Bit-index mapping is stable across word boundaries")
    {
        {
            const size_t n{ 1 };
            Bitset b{ n };
            b.set(0, true);
            REQUIRE(b[0] == true);
            REQUIRE(b.popcount() == 1);
            REQUIRE(b.toString() == "1");
            requireInvariants(b, "bit index mapping n=1");
        }

        for (size_t n : std::initializer_list<size_t>{ 2u, WORD_BITS, WORD_BITS + 1, 2 * WORD_BITS + 3 })
        {
            INFO("n = " << n);
            Bitset b{ n };
            b.set(0, true);
            b.set(n - 1, true);
            REQUIRE(b[0] == true);
            REQUIRE(b[n - 1] == true);
            REQUIRE(b.popcount() == 2);
            REQUIRE(b.toString().front() == '1');
            REQUIRE(b.toString().back() == '1');
            requireInvariants(b, "bit index mapping");
        }
    }

    // ------------------------------------------------------------------------
    // 11. Согласованность popcount / isZero / operator bool / getData
    // ------------------------------------------------------------------------
    SECTION("popcount, isZero, operator bool consistency")
    {
        Bitset b{ 200 };
        REQUIRE(b.popcount() == 0);
        REQUIRE(b.isZero());
        REQUIRE_FALSE(static_cast<bool>(b));

        b.set(0, true);
        REQUIRE(b.popcount() == 1);
        REQUIRE_FALSE(b.isZero());
        REQUIRE(static_cast<bool>(b));

        b.set(199, true);
        REQUIRE(b.popcount() == 2);

        b.clear();
        REQUIRE(b.popcount() == 0);
        REQUIRE(b.isZero());
        requireInvariants(b, "query consistency");
    }

    // ------------------------------------------------------------------------
    // 12. size() и wordsSize() — noexcept
    // ------------------------------------------------------------------------
    SECTION("Query methods are noexcept")
    {
        Bitset b{ 37 };
        STATIC_REQUIRE(noexcept(std::declval<const Bitset&>().size()));
        STATIC_REQUIRE(noexcept(std::declval<const Bitset&>().wordsSize()));
        STATIC_REQUIRE(noexcept(std::declval<const Bitset&>().lastWordBits()));
        STATIC_REQUIRE(noexcept(std::declval<const Bitset&>().garbageBits()));
        STATIC_REQUIRE(noexcept(std::declval<const Bitset&>().get()));
        STATIC_REQUIRE(noexcept(std::declval<const Bitset&>().getData()));
        STATIC_REQUIRE(noexcept(std::declval<const Bitset&>().popcount()));
        STATIC_REQUIRE(noexcept(std::declval<const Bitset&>().isZero()));
        STATIC_REQUIRE(noexcept(static_cast<bool>(std::declval<const Bitset&>())));
        REQUIRE(b.size() == 37);
    }
}

// ============================================================================
//  ЭТАП 3. Доступ к битам и BitReference
// ============================================================================
TEST_CASE("Bitset bit access and BitReference (full)", "[bitset][access]")
{
    using Ref = Bitset::BitReference;

    // ------------------------------------------------------------------------
    // 1. operator[] const: возвращает bool, границы
    // ------------------------------------------------------------------------
    SECTION("operator[] const returns bool, checks bounds")
    {
        Bitset b{ 5 };
        b.set(2, true);
        b.set(4, true);

        const Bitset& cb{ b };
        REQUIRE(cb[0] == false);
        REQUIRE(cb[1] == false);
        REQUIRE(cb[2] == true);
        REQUIRE(cb[3] == false);
        REQUIRE(cb[4] == true);

        REQUIRE_THROWS_AS(cb[5], std::out_of_range);
        REQUIRE_THROWS_AS(cb[100], std::out_of_range);

        // Пустой bitset — любое обращение бросает
        const Bitset e;
        REQUIRE_THROWS_AS(e[0], std::out_of_range);
    }

    // ------------------------------------------------------------------------
    // 2. operator[] non-const: чтение и запись через BitReference
    // ------------------------------------------------------------------------
    SECTION("operator[] non-const allows write and read")
    {
        Bitset b{ 10 };

        // Чтение по умолчанию
        for (size_t i = 0; i < 10; ++i)
        {
            REQUIRE(b[i] == false);
        }

        // Запись
        b[0] = true;
        b[3] = true;
        b[9] = true;
        REQUIRE(b[0] == true);
        REQUIRE(b[1] == false);
        REQUIRE(b[2] == false);
        REQUIRE(b[3] == true);
        REQUIRE(b[9] == true);
        REQUIRE(b.popcount() == 3);

        // Через ref
        auto ref{ b[0] };
        ref = false;
        REQUIRE(b[0] == false);
        ref = true;
        REQUIRE(b[0] == true);

        // Границы
        REQUIRE_THROWS_AS(b[10], std::out_of_range);
        REQUIRE_THROWS_AS(b[1000], std::out_of_range);

        requireInvariants(b, "operator[] non-const");
    }

    // ------------------------------------------------------------------------
    // 3. Многозначный bitset: корректный индекс слова и offset
    // ------------------------------------------------------------------------
    SECTION("operator[] across word boundary")
    {
        Bitset b{ 3 * WORD_BITS };

        for (size_t i : std::initializer_list<size_t>{ 0u, WORD_BITS - 1, WORD_BITS, WORD_BITS + 1,
                         2 * WORD_BITS - 1, 2 * WORD_BITS, 3 * WORD_BITS - 1 })
        {
            INFO("i = " << i);
            b.set(i, true);
            REQUIRE(b[i] == true);
            REQUIRE(b.popcount() == 1);

            b.set(i, false);
            REQUIRE(b[i] == false);
            REQUIRE(b.popcount() == 0);
        }
        requireInvariants(b, "operator[] cross word");
    }

    // ------------------------------------------------------------------------
    // 4. set(i, value): значения и границы
    // ------------------------------------------------------------------------
    SECTION("set(i, value) sets to given value")
    {
        Bitset b{ 8 };

        b.set(1);            // по умолчанию true
        b.set(5);
        REQUIRE(b[1] == true);
        REQUIRE(b[5] == true);
        REQUIRE(b.popcount() == 2);

        b.set(1, false);
        REQUIRE(b[1] == false);
        REQUIRE(b.popcount() == 1);

        b.set(0, true);
        b.set(7, true);
        REQUIRE(b[0] == true);
        REQUIRE(b[7] == true);
        REQUIRE(b.popcount() == 3);

        // Идемпотентность
        b.set(0, true);
        REQUIRE(b.popcount() == 3);
        b.set(0, false);
        b.set(0, false);
        REQUIRE(b.popcount() == 2);

        // Границы
        REQUIRE_THROWS_AS(b.set(8, true), std::out_of_range);
        REQUIRE_THROWS_AS(b.set(8, false), std::out_of_range);
        REQUIRE_THROWS_AS(b.set(100), std::out_of_range);

        requireInvariants(b, "set(i,value)");
    }

    // ------------------------------------------------------------------------
    // 5. BitReference: конструктор и границы offset
    // ------------------------------------------------------------------------
    SECTION("BitReference ctor validates offset")
    {
        Bitset b{ WORD_BITS };
        Word* ptr{ const_cast<Word*>(b.getData()) };

        // Корректные offset
        REQUIRE_NOTHROW(Ref(ptr, 0));
        REQUIRE_NOTHROW(Ref(ptr, WORD_BITS - 1));

        // Некорректные offset
        REQUIRE_THROWS_AS(Ref(ptr, WORD_BITS), std::out_of_range);
        REQUIRE_THROWS_AS(Ref(ptr, WORD_BITS + 1), std::out_of_range);
        REQUIRE_THROWS_AS(Ref(ptr, 1000), std::out_of_range);
    }

    // ------------------------------------------------------------------------
    // 6. BitReference: operator bool / operator!
    // ------------------------------------------------------------------------
    SECTION("BitReference bool conversion and negation")
    {
        Bitset b{ WORD_BITS };
        Word* ptr{ const_cast<Word*>(b.getData()) };

        Ref ref0{ ptr, 0 };
        Ref ref7{ ptr, 7 };

        REQUIRE(static_cast<bool>(ref0) == false);
        REQUIRE(!ref0 == true);

        ref0 = true;
        REQUIRE(static_cast<bool>(ref0) == true);
        REQUIRE(!ref0 == false);

        ref7 = true;
        REQUIRE(static_cast<bool>(ref7) == true);
        REQUIRE(b[7] == true);
        REQUIRE(b[0] == true);
    }

    // ------------------------------------------------------------------------
    // 7. BitReference: присваивание от bool и от другого BitReference
    // ------------------------------------------------------------------------
    SECTION("BitReference assignment copies value, does not rebind")
    {
        Bitset b{ WORD_BITS };
        Word* ptr{ const_cast<Word*>(b.getData()) };

        Ref a{ ptr, 3 };
        Ref c{ ptr, 10 };

        a = true;
        c = false;
        REQUIRE(b[3] == true);
        REQUIRE(b[10] == false);

        // c = a: копирует значение, не перепривязывает
        c = a;
        REQUIRE(b[10] == true);
        REQUIRE(b[3] == true);

        // Изменение a не должно менять c
        a = false;
        REQUIRE(b[3] == false);
        REQUIRE(b[10] == true);   // c всё ещё указывает на бит 10
    }

    // ------------------------------------------------------------------------
    // 8. BitReference: сравнения с bool (с обеих сторон)
    // ------------------------------------------------------------------------
    SECTION("BitReference equality with bool")
    {
        Bitset b{ WORD_BITS };
        Word* ptr{ const_cast<Word*>(b.getData()) };

        Ref ref{ ptr, 5 };

        REQUIRE(ref == false);
        REQUIRE(ref != true);
        REQUIRE(false == ref);
        REQUIRE(true != ref);

        ref = true;
        REQUIRE(ref == true);
        REQUIRE(ref != false);
        REQUIRE(true == ref);
        REQUIRE(false != ref);
    }

    // ------------------------------------------------------------------------
    // 9. BitReference: сравнение двух BitReference
    // ------------------------------------------------------------------------
    SECTION("BitReference equality with another BitReference")
    {
        Bitset b{ WORD_BITS };
        Word* ptr{ const_cast<Word*>(b.getData()) };

        Ref r0{ ptr, 0 };
        Ref r1{ ptr, 1 };
        Ref r0b{ ptr, 0 };

        REQUIRE(r0 == r1);          // оба false
        REQUIRE(r0 == r0b);

        r0 = true;
        REQUIRE(r0 != r1);
        REQUIRE(r0 == r0b);
        REQUIRE(r0 == r0);

        r1 = true;
        REQUIRE(r0 == r1);
        REQUIRE(r0 == r0b);
    }

    // ------------------------------------------------------------------------
    // 10. BitReference отражает изменения через другие API
    // ------------------------------------------------------------------------
    SECTION("BitReference observes external modifications")
    {
        Bitset b{ WORD_BITS };
        Word* ptr{ const_cast<Word*>(b.getData()) };

        Ref ref{ ptr, 20 };
        ref = true;
        REQUIRE(b[20] == true);

        // Изменение через set
        b.set(20, false);
        REQUIRE(ref == false);

        // Через flip / clear / setAll
        b.set(20, true);
        REQUIRE(ref == true);
        b.flip();
        REQUIRE(ref == false);
        b.clear();
        REQUIRE(ref == false);

        b.setAll(true);
        REQUIRE(ref == true);
    }

    // ------------------------------------------------------------------------
    // 11. BitReference на границах слова
    // ------------------------------------------------------------------------
    SECTION("BitReference at word boundaries")
    {
        Bitset b{ 2 * WORD_BITS };

        // Первый бит первого слова и последний бит первого слова
        b[0] = true;
        b[WORD_BITS - 1] = true;
        // Первый бит второго слова и последний
        b[WORD_BITS] = true;
        b[2 * WORD_BITS - 1] = true;

        REQUIRE(b.popcount() == 4);
        REQUIRE(b.getData()[0] == (Word{ 1 } | (Word{ 1 } << (WORD_BITS - 1))));
        REQUIRE(b.getData()[1] == (Word{ 1 } | (Word{ 1 } << (WORD_BITS - 1))));
        requireInvariants(b, "BitReference boundaries");
    }

    // ------------------------------------------------------------------------
    // 12. Мусорные биты никогда не выставляются через operator[]
    // ------------------------------------------------------------------------
    SECTION("operator[] never touches garbage bits")
    {
        // size кратен WORD_BITS не всегда — берём с запасом
        for (size_t n : std::initializer_list<size_t>{ 1u, WORD_BITS - 1, WORD_BITS, WORD_BITS + 1, 100u })
        {
            INFO("n = " << n);
            Bitset b{ n };
            b.setAll(true);

            const size_t w{ b.wordsSize() };
            const size_t lastBits{ b.lastWordBits() };
            const Word lastWord{ b.getData()[w - 1] };

            // Мусорные биты последнего слова == 0
            if (lastBits < WORD_BITS)
            {
                const Word mask{ static_cast<Word>((Word{ 1 } << lastBits) - Word{ 1 }) };
                REQUIRE((lastWord & static_cast<Word>(~mask)) == 0);
            }
        }
    }
}

// ============================================================================
//  ЭТАП 4. Модификация битов
// ============================================================================
TEST_CASE("Bitset bit modification (clear, flip, reverse, setAll)",
          "[bitset][modifiers][bits]")
{
    // ------------------------------------------------------------------------
    // 1. clear()
    // ------------------------------------------------------------------------
    SECTION("clear() zeros all bits, preserves size")
    {
        Bitset b{ 100 };
        b.setAll(true);
        REQUIRE(b.popcount() == 100);

        const size_t n0{ b.size() };
        const size_t w0{ b.wordsSize() };

        b.clear();

        REQUIRE(b.size() == n0);
        REQUIRE(b.wordsSize() == w0);
        REQUIRE(b.popcount() == 0);
        REQUIRE(b.isZero());
        REQUIRE_FALSE(static_cast<bool>(b));
        REQUIRE(b.toString() == std::string(100, '0'));
        requireInvariants(b, "clear");

        // повторный вызов — идемпотентно
        REQUIRE_NOTHROW(b.clear());
        REQUIRE(b.popcount() == 0);
    }

    // ------------------------------------------------------------------------
    // 2. flip() — инверсия всех битов
    // ------------------------------------------------------------------------
    SECTION("flip() inverts all valid bits")
    {
        for (size_t n : std::initializer_list<size_t>{ 1u, 2u, WORD_BITS - 1, WORD_BITS, WORD_BITS + 1,
                         2 * WORD_BITS + 5, 100u })
        {
            INFO("n = " << n);
            Bitset b{ n };
            REQUIRE(b.popcount() == 0);

            b.flip();
            REQUIRE(b.popcount() == n);      // все валидные биты == 1
            requireInvariants(b, "flip once");

            b.flip();
            REQUIRE(b.popcount() == 0);      // вернулись к нулю
            requireInvariants(b, "flip twice");
        }
    }

    SECTION("flip() leaves garbage bits zero")
    {
        for (size_t n : std::initializer_list<size_t>{ 1u, WORD_BITS - 1, WORD_BITS + 3, 2 * WORD_BITS + 7 })
        {
            INFO("n = " << n);
            Bitset b{ n };
            b.flip();

            const size_t w{ b.wordsSize() };
            const size_t lastBits{ b.lastWordBits() };
            const Word lastWord{ b.getData()[w - 1] };

            if (lastBits < WORD_BITS)
            {
                const Word mask{ static_cast<Word>((Word{ 1 } << lastBits) - Word{ 1 }) };
                REQUIRE((lastWord & static_cast<Word>(~mask)) == 0);
            }
            requireInvariants(b, "flip garbage");
        }
    }

    // ------------------------------------------------------------------------
    // 3. reverse()
    // ------------------------------------------------------------------------
    SECTION("reverse() reverses bit order (string view)")
    {
        // "1101" -> "1011"
        Bitset b{ "1101" };
        b.reverse();
        REQUIRE(b.equals("1011"));
        REQUIRE(b.size() == 4);
        requireInvariants(b, "reverse 1101");

        // "1001" -> "1001" (палиндром)
        Bitset p{ "1001" };
        p.reverse();
        REQUIRE(p.equals("1001"));

        // "1" -> "1"
        Bitset one{ "1" };
        one.reverse();
        REQUIRE(one.equals("1"));

        // "0" -> "0"
        Bitset zero{ "0" };
        zero.reverse();
        REQUIRE(zero.equals("0"));

        // пустой — no-op
        Bitset e;
        REQUIRE_NOTHROW(e.reverse());
        REQUIRE(e.size() == 0);
        requireInvariants(e, "reverse empty");

        // двойной reverse — исходное
        Bitset orig{ "1011010" };
        Bitset copy{ orig };
        copy.reverse();
        copy.reverse();
        REQUIRE(copy == orig);
    }

    SECTION("reverse() across word boundary")
    {
        const size_t n{ 2 * WORD_BITS + 5 };
        Bitset b{ n };
        b.set(0, true);
        b.set(n - 1, true);
        b.set(WORD_BITS, true);
        b.set(WORD_BITS + 3, true);

        Bitset copy{ b };
        copy.reverse();
        // После reverse: биты 0 и n-1 меняются местами, WORD_BITS <-> n-1-WORD_BITS и т.д.
        REQUIRE(copy[0] == b[n - 1]);
        REQUIRE(copy[n - 1] == b[0]);
        REQUIRE(copy[WORD_BITS] == b[n - 1 - WORD_BITS]);
        REQUIRE(copy[n - 1 - WORD_BITS] == b[WORD_BITS]);
        REQUIRE(copy.popcount() == b.popcount());
        requireInvariants(copy, "reverse cross word");
    }

    // ------------------------------------------------------------------------
    // 4. setAll()
    // ------------------------------------------------------------------------
    SECTION("setAll(true) sets every valid bit, garbage stays zero")
    {
        for (size_t n : std::initializer_list<size_t>{ 1u, WORD_BITS - 1, WORD_BITS, WORD_BITS + 1,
                         2 * WORD_BITS + 5, 130u })
        {
            INFO("n = " << n);
            Bitset b{ n };
            b.setAll(true);

            REQUIRE(b.popcount() == n);
            REQUIRE(b.toString() == std::string(n, '1'));
            REQUIRE_FALSE(b.isZero());
            requireInvariants(b, "setAll(true)");
        }
    }

    SECTION("setAll(false) zeroes everything")
    {
        Bitset b{ 137 };
        b.setAll(true);
        REQUIRE(b.popcount() == 137);

        b.setAll(false);
        REQUIRE(b.popcount() == 0);
        REQUIRE(b.isZero());
        REQUIRE(b.toString() == std::string(137, '0'));
        requireInvariants(b, "setAll(false)");
    }

    SECTION("setAll() default argument is true")
    {
        Bitset b{ 20 };
        b.setAll();
        REQUIRE(b.popcount() == 20);
    }

    SECTION("setAll on empty is safe no-op")
    {
        Bitset e;
        REQUIRE_NOTHROW(e.setAll(true));
        REQUIRE_NOTHROW(e.setAll(false));
        REQUIRE(e.size() == 0);
    }

    // ------------------------------------------------------------------------
    // 5. getValue()
    // ------------------------------------------------------------------------
    SECTION("getValue basic single-word cases")
    {
        Bitset b{ "101101" };        // MSB->LSB: 1 0 1 1 0 1
        // bit0=1, bit1=0, bit2=1, bit3=1, bit4=0, bit5=1

        REQUIRE(b.getValue(0, 1) == 1);      // bit0
        REQUIRE(b.getValue(1, 1) == 0);      // bit1
        REQUIRE(b.getValue(2, 1) == 1);
        REQUIRE(b.getValue(5, 1) == 1);

        // Поле из 3 бит: bit0=1,bit1=0,bit2=1 -> LSB-first = 0b101 = 5
        REQUIRE(b.getValue(0, 3) == 0b101);
        // bit2=1,bit3=1,bit4=0 -> 0b011 = 3
        REQUIRE(b.getValue(2, 3) == 0b011);
        // bit1=0,bit2=1,bit3=1 -> 0b110 = 6
        REQUIRE(b.getValue(1, 3) == 0b110);

        // Всё поле
        REQUIRE(b.getValue(0, 6) == 0b101101);
    }

    SECTION("getValue at size boundary")
    {
        Bitset b{ 8 };
        b.setAll(true);
        // i + n == size
        REQUIRE(b.getValue(0, 8) == 0xFF);
        REQUIRE(b.getValue(4, 4) == 0xF);
        REQUIRE(b.getValue(7, 1) == 0x1);

        // i + n > size -> out_of_range
        REQUIRE_THROWS_AS(b.getValue(4, 5), std::out_of_range);
        REQUIRE_THROWS_AS(b.getValue(8, 1), std::out_of_range);
        REQUIRE_THROWS_AS(b.getValue(0, WORD_BITS + 1), std::out_of_range);
    }

    SECTION("getValue across word boundary")
    {
        Bitset b{ 2 * WORD_BITS };
        // Установим известные биты
        b.set(WORD_BITS - 2, true);
        b.set(WORD_BITS - 1, true);
        b.set(WORD_BITS, true);
        b.set(WORD_BITS + 1, true);

        // Поле из 4 бит начиная с WORD_BITS-2: LSB-first = bit[WB-2, WB-1, WB, WB+1] = 1,1,1,1 -> 0b1111
        REQUIRE(b.getValue(WORD_BITS - 2, 4) == 0b1111);

        // Поле из 2 бит начиная с WORD_BITS-1 = 1,1 -> 0b11
        REQUIRE(b.getValue(WORD_BITS - 1, 2) == 0b11);

        // Поле из 2 бит начиная с WORD_BITS = 1,1 -> 0b11
        REQUIRE(b.getValue(WORD_BITS, 2) == 0b11);

        requireInvariants(b, "getValue cross word");
    }

    SECTION("getValue n == 0 returns 0 (audit)")
    {
        Bitset b{ "1010" };
        REQUIRE(b.getValue(0, 0) == 0);
        REQUIRE(b.getValue(b.size(), 0) == 0);
        REQUIRE_THROWS_AS(b.getValue(0, WORD_BITS + 1), std::out_of_range);
    }

    // ------------------------------------------------------------------------
    // 6. setValue()
    // ------------------------------------------------------------------------
    SECTION("setValue basic write and read back")
    {
        Bitset b{ 16 };
        b.setValue(0b1010, 0, 4);        // bits 0..3 = 0b1010
        REQUIRE(b.getValue(0, 4) == 0b1010);
        REQUIRE(b[0] == false);
        REQUIRE(b[1] == true);
        REQUIRE(b[2] == false);
        REQUIRE(b[3] == true);

        b.setValue(0b11, 4, 2);          // bits 4..5 = 0b11
        REQUIRE(b.getValue(4, 2) == 0b11);

        REQUIRE(b.popcount() == 4);
        requireInvariants(b, "setValue basic");
    }

    SECTION("setValue masks high bits of value")
    {
        Bitset b{ 8 };
        // value имеет больше бит, чем n -> берутся младшие n бит
        b.setValue(0xFFFF, 0, 4);
        REQUIRE(b.getValue(0, 4) == 0xF);
        REQUIRE(b.popcount() == 4);
        requireInvariants(b, "setValue masking");
    }

    SECTION("setValue across word boundary")
    {
        Bitset b{ 2 * WORD_BITS };
        const Word v{ 0b1111 };
        b.setValue(v, WORD_BITS - 2, 4);

        REQUIRE(b[WORD_BITS - 2] == true);
        REQUIRE(b[WORD_BITS - 1] == true);
        REQUIRE(b[WORD_BITS] == true);
        REQUIRE(b[WORD_BITS + 1] == true);
        REQUIRE(b.popcount() == 4);
        REQUIRE(b.getValue(WORD_BITS - 2, 4) == 0b1111);
        requireInvariants(b, "setValue cross word");
    }

    SECTION("setValue overwrites previous bits, keeps neighbors intact")
    {
        Bitset b{ 16 };
        b.setAll(true);
        // Перезапишем поле [4,8) значением 0
        b.setValue(0, 4, 4);

        REQUIRE(b.getValue(0, 4) == 0xF);       // соседи слева целы
        REQUIRE(b.getValue(4, 4) == 0x0);       // поле обнулено
        REQUIRE(b.getValue(8, 8) == 0xFF);      // соседи справа целы
        REQUIRE(b.popcount() == 12);
        requireInvariants(b, "setValue overwrite");
    }

    SECTION("setValue n == 0 is a no-op (audit)")
    {
        Bitset b{ "1010" };
        const std::string before{ b.toString() };
        REQUIRE_NOTHROW(b.setValue(Word{ 0xFF }, 0, 0));
        REQUIRE(b.toString() == before);
        requireInvariants(b, "setValue n=0");
    }

    SECTION("setValue bounds")
    {
        Bitset b{ 8 };
        REQUIRE_THROWS_AS(b.setValue(0, 0, WORD_BITS + 1), std::out_of_range);
        REQUIRE_THROWS_AS(b.setValue(0, 4, 5), std::out_of_range);
        REQUIRE_THROWS_AS(b.setValue(0, 8, 1), std::out_of_range);
    }

    // ------------------------------------------------------------------------
    // 7. getValue / setValue — round-trip
    // ------------------------------------------------------------------------
    SECTION("setValue + getValue round-trip")
    {
        Bitset b{ 2 * WORD_BITS };
        for (size_t n : std::initializer_list<size_t>{ 1u, 2u, 4u, 8u, WORD_BITS })
        {
            for (size_t i : std::initializer_list<size_t>{ 0u, 3u, WORD_BITS - 1, WORD_BITS, WORD_BITS + 5 })
            {
                if (i + n > b.size()) continue;

                const Word v{ static_cast<Word>((Word{ 1 } << (n - 1)) | Word{ 1 }) };
                b.setAll(false);
                b.setValue(v, i, n);

                INFO("i = " << i << ", n = " << n);
                REQUIRE(b.getValue(i, n) == v);
                REQUIRE(b.popcount() == static_cast<size_t>(std::popcount(v)));
                requireInvariants(b, "round-trip");
            }
        }
    }

    // ------------------------------------------------------------------------
    // 8. Согласованность с operator[] и toString
    // ------------------------------------------------------------------------
    SECTION("flip equals manual inversion via operator[]")
    {
        Bitset a{ "1010011" };
        Bitset b{ a };

        a.flip();
        for (size_t i = 0; i < b.size(); ++i)
        {
            b[i] = !b[i];
        }
        REQUIRE(a == b);
        requireInvariants(a, "flip vs manual");
    }

    SECTION("reverse equals manual swap")
    {
        Bitset a{ "10011" };
        Bitset b{ a };

        REQUIRE(a == b);

        const size_t n{ b.size() };

        for (size_t i = 0; i < n; ++i)
        {
            b[i] = a[n - 1 - i];
        }

        a.reverse();

        REQUIRE(a == b);   // a уже reversed, b собран вручную
        requireInvariants(a, "reverse vs manual");
    }
}

// ============================================================================
//  ЭТАП 5. Строковые преобразования
// ============================================================================
TEST_CASE("Bitset string conversions (toString, equals, setFromString, prependFromString)",
          "[bitset][string]")
{
    // ------------------------------------------------------------------------
    // 1. toString()
    // ------------------------------------------------------------------------
    SECTION("toString MSB-first")
    {
        REQUIRE(Bitset{}.toString() == "");
        REQUIRE(Bitset{ std::string("0") }.toString() == "0");
        REQUIRE(Bitset{ std::string("1") }.toString() == "1");
        REQUIRE(Bitset{ std::string("1010") }.toString() == "1010");
        REQUIRE(Bitset{ std::string("00000001") }.toString() == "00000001");
    }

    SECTION("toString is const and does not modify bitset")
    {
        Bitset b{ "101101" };
        const Bitset& cb{ b };
        const std::string s1{ cb.toString() };
        const std::string s2{ cb.toString() };
        REQUIRE(s1 == s2);
        REQUIRE(b.equals("101101"));
        requireInvariants(b, "toString const");
    }

    SECTION("toString length == size()")
    {
        for (size_t n : std::initializer_list<size_t>{ 0u, 1u, WORD_BITS - 1, WORD_BITS, WORD_BITS + 1,
                         2 * WORD_BITS + 7, 200u })
        {
            INFO("n = " << n);
            Bitset b{ n };
            REQUIRE(b.toString().size() == n);
            b.setAll(true);
            REQUIRE(b.toString().size() == n);
            REQUIRE(b.toString() == std::string(n, '1'));
            requireInvariants(b, "toString length");
        }
    }

    SECTION("toString across word boundary")
    {
        Bitset b(WORD_BITS + 4);
        b.set(0, true);
        b.set(WORD_BITS, true);

        const std::string s{ b.toString() };
        REQUIRE(s.size() == WORD_BITS + 4);

        const std::string expectedStr =
            std::string("0001") + std::string(WORD_BITS - 1, '0') + "1";
        REQUIRE(s == expectedStr);

        // Дополнительные точечные проверки
        REQUIRE(s.front() == '0');    // bit size-1
        REQUIRE(s.back()  == '1');    // bit 0
        REQUIRE(s[WORD_BITS + 3] == '1'); // bit WORD_BITS

        requireInvariants(b, "toString cross word");
    }

    // ------------------------------------------------------------------------
    // 2. equals()
    // ------------------------------------------------------------------------
    SECTION("equals returns true for identical string")
    {
        for (auto s : { "", "0", "1", "1010", "11110000", "00000001" })
        {
            INFO("s = " << s);
            Bitset b{ std::string(s) };
            REQUIRE(b.equals(s));
            REQUIRE(b.equals(std::string_view(s)));
        }
    }

    SECTION("equals returns false on size mismatch")
    {
        Bitset b{ "1010" };
        REQUIRE_FALSE(b.equals("101"));
        REQUIRE_FALSE(b.equals("10101"));
        REQUIRE_FALSE(b.equals(""));
    }

    SECTION("equals returns false on bit mismatch")
    {
        Bitset b{ "1010" };
        REQUIRE_FALSE(b.equals("1011"));
        REQUIRE_FALSE(b.equals("0010"));
        REQUIRE_FALSE(b.equals("1111"));
    }

    SECTION("equals does not throw on invalid chars, returns false")
    {
        Bitset b{ "1010" };
        REQUIRE_FALSE(b.equals("10a0"));
        REQUIRE_FALSE(b.equals("1020"));
        REQUIRE_FALSE(b.equals("abcd"));
        REQUIRE_FALSE(b.equals("    "));
        REQUIRE_FALSE(b.equals("10 0"));
    }

    SECTION("equals on empty")
    {
        Bitset e;
        REQUIRE(e.equals(""));
        REQUIRE_FALSE(e.equals("0"));
        REQUIRE_FALSE(e.equals("1"));
    }

    SECTION("equals is noexcept")
    {
        STATIC_REQUIRE(noexcept(std::declval<const Bitset&>().equals(std::declval<std::string_view>())));
    }

    // ------------------------------------------------------------------------
    // 3. setFromString(str)
    // ------------------------------------------------------------------------
    SECTION("setFromString at position 0 on empty bitset")
    {
        Bitset b;
        b.setFromString("1010");
        REQUIRE(b.size() == 4);
        REQUIRE(b.equals("1010"));
        requireInvariants(b, "setFromString(0) on empty");
    }

    SECTION("setFromString overwrites existing bits in range, keeps neighbors")
    {
        Bitset b{ "11111111" };
        b.setFromString("00", 2);      // биты 2,3 := 0,0
        // MSB-first: строка "00", позиция 2 (LSB), значит bit3=0, bit2=0
        REQUIRE(b[2] == false);
        REQUIRE(b[3] == false);
        REQUIRE(b[0] == true);
        REQUIRE(b[1] == true);
        REQUIRE(b[4] == true);
        REQUIRE(b[7] == true);
        requireInvariants(b, "setFromString overwrite");
    }

    SECTION("setFromString MSB-first mapping")
    {
        Bitset b{ 8 };
        // строка "10" -> bit(position+1)=1, bit(position)=0
        b.setFromString("10", 3);
        REQUIRE(b[3] == false);   // младший символ '0'
        REQUIRE(b[4] == true);    // старший символ '1'
        REQUIRE(b.popcount() == 1);
        requireInvariants(b, "setFromString MSB-first");
    }

    SECTION("setFromString extends bitset when needed")
    {
        Bitset b{ 3 };                 // size == 3
        b.setFromString("1111", 0);    // нужно 4 бита
        REQUIRE(b.size() == 4);
        REQUIRE(b.equals("1111"));
        requireInvariants(b, "setFromString extends");

        Bitset c{ 3 };
        c.setFromString("10", 5);      // нужно 7 бит
        REQUIRE(c.size() == 7);
        REQUIRE(c[5] == false);
        REQUIRE(c[6] == true);
        REQUIRE(c.popcount() == 1);
        requireInvariants(c, "setFromString extends at offset");
    }

    SECTION("setFromString on empty string is a no-op")
    {
        Bitset b{ "1011" };
        const std::string before{ b.toString() };
        REQUIRE_NOTHROW(b.setFromString(""));
        REQUIRE(b.toString() == before);
        requireInvariants(b, "setFromString empty str");
    }

    SECTION("setFromString default position is 0")
    {
        Bitset b{ 4 };
        b.setFromString("1010");       // position == 0 по умолчанию
        REQUIRE(b.equals("1010"));
    }

    SECTION("setFromString throws invalid_argument on invalid chars")
    {
        Bitset b{ 8 };
        REQUIRE_THROWS_AS(b.setFromString("10a0"), std::invalid_argument);
        REQUIRE_THROWS_AS(b.setFromString("2"), std::invalid_argument);
        REQUIRE_THROWS_AS(b.setFromString(" "), std::invalid_argument);
        // Биты не изменились (strong guarantee)
        requireInvariants(b, "setFromString invalid");
    }

    SECTION("setFromString strong guarantee on invalid input")
    {
        Bitset b{ "101101" };
        const Bitset copy{ b };
        REQUIRE_THROWS_AS(b.setFromString("10x"), std::invalid_argument);
        REQUIRE(b == copy);
    }

    // ------------------------------------------------------------------------
    // 4. prependFromString(str)
    // ------------------------------------------------------------------------
    SECTION("prependFromString prepends to string representation")
    {
        Bitset b{ "101" };
        b.prependFromString("11");
        // toString == "11" + "101" == "11101"
        REQUIRE(b.size() == 5);
        REQUIRE(b.equals("11101"));
        requireInvariants(b, "prependFromString");
    }

    SECTION("prependFromString on empty")
    {
        Bitset b;
        b.prependFromString("1001");
        REQUIRE(b.size() == 4);
        REQUIRE(b.equals("1001"));
        requireInvariants(b, "prependFromString empty");
    }

    SECTION("prependFromString with empty string is a no-op")
    {
        Bitset b{ "1011" };
        const std::string before{ b.toString() };
        REQUIRE_NOTHROW(b.prependFromString(""));
        REQUIRE(b.toString() == before);
        requireInvariants(b, "prependFromString empty str");
    }

    SECTION("prependFromString throws on invalid chars")
    {
        Bitset b{ "101" };
        const Bitset copy{ b };
        REQUIRE_THROWS_AS(b.prependFromString("1x1"), std::invalid_argument);
        REQUIRE(b == copy);
        requireInvariants(b, "prependFromString invalid");
    }

    SECTION("prependFromString equivalence with setFromString(str, size())")
    {
        for (auto s : { "1", "10", "111", "1010", "0001" })
        {
            INFO("s = " << s);
            Bitset a{ "110" };
            Bitset b{ "110" };

            a.prependFromString(s);
            b.setFromString(s, b.size());

            REQUIRE(a == b);
            REQUIRE(a.toString() == std::string(s) + "110");
            requireInvariants(a, "prepend vs setFromString equivalence");
        }
    }

    SECTION("prependFromString with long string crossing word boundaries")
    {
        const size_t m{ WORD_BITS * 2 + 5 };
        std::string s(m, '1');
        for (size_t i = 0; i < m; i += 3) s[i] = '0';

        Bitset b{ "101" };
        b.prependFromString(s);

        REQUIRE(b.size() == m + 3);
        REQUIRE(b.toString() == s + "101");
        requireInvariants(b, "prependFromString long");
    }

    // ------------------------------------------------------------------------
    // 5. Round-trip и согласованность
    // ------------------------------------------------------------------------
    SECTION("Round-trip toString -> setFromString")
    {
        for (auto s : { "1", "10", "111", "1010", "101101",
                       "1111000011110000", "00000001" })
        {
            INFO("s = " << s);
            Bitset src{ std::string(s) };
            Bitset dst{ src.size() };
            dst.setFromString(src.toString());
            REQUIRE(dst == src);
            requireInvariants(dst, "round-trip");
        }
    }

    SECTION("setFromString and operator[] agree on bit values")
    {
        Bitset b{ 8 };
        b.setFromString("10110010", 0);
        REQUIRE(b[0] == false);
        REQUIRE(b[1] == true);
        REQUIRE(b[2] == false);
        REQUIRE(b[3] == false);
        REQUIRE(b[4] == true);
        REQUIRE(b[5] == true);
        REQUIRE(b[6] == false);
        REQUIRE(b[7] == true);
        requireInvariants(b, "setFromString vs operator[]");
    }

    SECTION("setFromString with all zeros")
    {
        Bitset b{ 8 };
        b.setAll(true);
        b.setFromString("00000000");
        REQUIRE(b.isZero());
        REQUIRE(b.popcount() == 0);
        requireInvariants(b, "setFromString all zeros");
    }

    SECTION("setFromString with all ones")
    {
        Bitset b{ 8 };
        b.setFromString("11111111");
        REQUIRE(b.popcount() == 8);
        REQUIRE(b.toString() == "11111111");
        requireInvariants(b, "setFromString all ones");
    }

    SECTION("setFromString does not touch garbage bits beyond size")
    {
        Bitset b{ 5 };                // 5 бит, 59 бит мусора
        b.setFromString("11111", 0);
        REQUIRE(b.popcount() == 5);
        const Word lastWord{ b.getData()[0] };
        REQUIRE(lastWord == 0b11111);
        requireInvariants(b, "setFromString garbage");
    }
}

// ============================================================================
//  ЭТАП 6. Вставка и удаление
// ============================================================================
TEST_CASE("Bitset insertion and removal", "[bitset][insert-remove]")
{
    // ------------------------------------------------------------------------
    // 1. appendMSB(bool)
    // ------------------------------------------------------------------------
    SECTION("appendMSB(bool) on empty and single calls")
    {
        Bitset b;
        b.appendMSB(true);
        REQUIRE(b.size() == 1);
        REQUIRE(b.equals("1"));
        requireInvariants(b, "appendMSB(true) empty");

        b.appendMSB(false);
        REQUIRE(b.size() == 2);
        REQUIRE(b.equals("01"));   // новый бит MSB
        requireInvariants(b, "appendMSB(false)");

        b.appendMSB(true);
        REQUIRE(b.size() == 3);
        REQUIRE(b.equals("101"));
        requireInvariants(b, "appendMSB(true) again");
    }

    SECTION("appendMSB(bool) crosses word boundary")
    {
        Bitset b;
        for (size_t i = 0; i < WORD_BITS; ++i)
            b.appendMSB(true);
        REQUIRE(b.size() == WORD_BITS);
        REQUIRE(b.wordsSize() == 1);
        REQUIRE(b.equals(std::string(WORD_BITS, '1')));
        requireInvariants(b, "appendMSB fill word");

        b.appendMSB(true);
        REQUIRE(b.size() == WORD_BITS + 1);
        REQUIRE(b.wordsSize() == 2);
        REQUIRE(b.lastWordBits() == 1);
        REQUIRE(b.equals(std::string(WORD_BITS + 1, '1')));
        requireInvariants(b, "appendMSB cross word");
    }

    SECTION("appendMSB(bool) preserves existing string")
    {
        Bitset b{ "101" };
        b.appendMSB(true);
        REQUIRE(b.equals("1101"));   // '1' + "101"
        b.appendMSB(false);
        REQUIRE(b.equals("01101"));  // '0' + "1101"
        requireInvariants(b, "appendMSB preserves");
    }

    // ------------------------------------------------------------------------
    // 2. appendMSB(WORD, size_t)
    // ------------------------------------------------------------------------
    SECTION("appendMSB(WORD, size) basic")
    {
        Bitset b;
        b.appendMSB(0b1, 1);
        REQUIRE(b.equals("1"));

        // Добавляем 3 младших бита 0b100 = "100"
        b.appendMSB(0b100, 3);
        // Результат: "100" + "1" = "1001"
        REQUIRE(b.size() == 4);
        REQUIRE(b.equals("1001"));
        requireInvariants(b, "appendMSB(WORD,3)");

        b.appendMSB(0xFFFF, WORD_BITS);
        REQUIRE(b.size() == WORD_BITS + 4);
        REQUIRE(b.wordsSize() == 2);
        requireInvariants(b, "appendMSB full word");
    }

    SECTION("appendMSB(WORD, size) masks high bits")
    {
        Bitset b;
        b.appendMSB(0b1111, 2);   // младшие 2 бита = 0b11 = "11"
        REQUIRE(b.size() == 2);
        REQUIRE(b.equals("11"));
        REQUIRE(b.popcount() == 2);

        Bitset c;
        c.appendMSB(0b1000, 3);   // младшие 3 бита 0b000 = "000"
        REQUIRE(c.size() == 3);
        REQUIRE(c.equals("000"));
        requireInvariants(c, "appendMSB mask");
    }

    SECTION("appendMSB(WORD, size) throws on size > WORD_BITS")
    {
        Bitset b{ "101" };
        const Bitset copy{ b };
        REQUIRE_THROWS_AS(b.appendMSB(Word{ 0 }, WORD_BITS + 1), std::out_of_range);
        REQUIRE(b == copy);
    }

    // ------------------------------------------------------------------------
    // 3. appendMSB(const Bitset&)
    // ------------------------------------------------------------------------
    SECTION("appendMSB(Bitset) basic")
    {
        Bitset a{ "101" };
        Bitset b{ "11" };
        a.appendMSB(b);
        REQUIRE(a.equals("11101"));   // "11" + "101"
        REQUIRE(b.equals("11"));      // b не изменился
        requireInvariants(a, "appendMSB(Bitset)");
    }

    SECTION("appendMSB(empty Bitset) is a no-op")
    {
        Bitset a{ "1011" };
        Bitset e;
        a.appendMSB(e);
        REQUIRE(a.equals("1011"));
        requireInvariants(a, "appendMSB(empty)");
    }

    SECTION("appendMSB self-append works via copy")
    {
        Bitset d{ "10" };
        d.appendMSB(d);
        REQUIRE(d.size() == 4);
        REQUIRE(d.equals("1010"));
        requireInvariants(d, "self appendMSB");
    }

    SECTION("appendMSB onto empty equals copy")
    {
        Bitset src{ "101101" };
        Bitset dst;
        dst.appendMSB(src);
        REQUIRE(dst == src);
        requireInvariants(dst, "appendMSB onto empty");
    }

    SECTION("appendMSB(WORD) large with multi-word other")
    {
        Bitset src(2 * WORD_BITS + 3);
        src.setAll(true);
        Bitset dst{ "101" };
        dst.appendMSB(src);
        REQUIRE(dst.size() == src.size() + 3);
        REQUIRE(dst.toString() == src.toString() + "101");
        requireInvariants(dst, "appendMSB big");
    }

    // ------------------------------------------------------------------------
    // 4. appendLSB(bool)
    // ------------------------------------------------------------------------
    SECTION("appendLSB(bool) appends at LSB side")
    {
        Bitset b;
        b.appendLSB(true);
        REQUIRE(b.equals("1"));

        b.appendLSB(false);
        REQUIRE(b.equals("10"));

        b.appendLSB(true);
        REQUIRE(b.equals("101"));
        requireInvariants(b, "appendLSB(bool)");
    }

    SECTION("appendLSB(bool) preserves existing string")
    {
        Bitset b{ "101" };
        b.appendLSB(true);
        REQUIRE(b.equals("1011"));   // "101" + '1'
        b.appendLSB(false);
        REQUIRE(b.equals("10110"));  // "1011" + '0'
        requireInvariants(b, "appendLSB preserves");
    }

    SECTION("appendLSB(bool) crosses word boundary")
    {
        Bitset b;
        for (size_t i = 0; i < WORD_BITS; ++i)
            b.appendLSB(true);
        REQUIRE(b.size() == WORD_BITS);
        REQUIRE(b.wordsSize() == 1);

        b.appendLSB(true);
        REQUIRE(b.size() == WORD_BITS + 1);
        REQUIRE(b.wordsSize() == 2);
        REQUIRE(b.lastWordBits() == 1);
        REQUIRE(b.equals(std::string(WORD_BITS + 1, '1')));
        requireInvariants(b, "appendLSB cross word");
    }

    // ------------------------------------------------------------------------
    // 5. appendLSB(std::string_view)
    // ------------------------------------------------------------------------
    SECTION("appendLSB(string) appends to right")
    {
        Bitset b{ "101" };
        b.appendLSB("11");

        REQUIRE(b.equals("10111"));   // "101" + "11"
        requireInvariants(b, "appendLSB(string)");

        Bitset e;
        e.appendLSB("1001");
        REQUIRE(e.equals("1001"));
        requireInvariants(e, "appendLSB(string) empty");
    }

    SECTION("appendLSB(string) invalid chars throws invalid_argument")
    {
        Bitset b{ "101" };
        const Bitset copy{ b };
        REQUIRE_THROWS_AS(b.appendLSB("10x1"), std::invalid_argument);
        REQUIRE(b == copy);
    }

    SECTION("appendLSB(string) empty is a no-op")
    {
        Bitset b{ "101" };
        const std::string before{ b.toString() };
        REQUIRE_NOTHROW(b.appendLSB(std::string_view{ }));
        REQUIRE(b.toString() == before);
        requireInvariants(b, "appendLSB(empty string)");
    }

    SECTION("appendLSB(string) across word boundary")
    {
        Bitset b(WORD_BITS);
        b.setAll(true);
        b.appendLSB("0000");
        REQUIRE(b.size() == WORD_BITS + 4);
        REQUIRE(b.toString() == std::string(WORD_BITS, '1') + "0000");
        requireInvariants(b, "appendLSB(string) cross word");
    }

    // ------------------------------------------------------------------------
    // 6. appendLSB(WORD, size_t)
    // ------------------------------------------------------------------------
    SECTION("appendLSB(WORD, size) appends low bits")
    {
        Bitset b;
        b.appendLSB(Word{ 0b101 }, 3);
        REQUIRE(b.equals("101"));

        Bitset c{ "11" };
        c.appendLSB(Word{ 0b01 }, 2);
        // toString == "11" + "01" == "1101"
        REQUIRE(c.equals("1101"));
        requireInvariants(c, "appendLSB(WORD,2)");
    }

    SECTION("appendLSB(WORD, 0) is a no-op")
    {
        Bitset b{ "101" };
        const std::string before{ b.toString() };
        REQUIRE_NOTHROW(b.appendLSB(Word{ 0xFF }, 0));
        REQUIRE(b.toString() == before);
        requireInvariants(b, "appendLSB(_,0)");
    }

    SECTION("appendLSB(WORD, size) throws on size > WORD_BITS")
    {
        Bitset b{ "101" };
        const Bitset copy{ b };
        REQUIRE_THROWS_AS(b.appendLSB(Word{ 0 }, WORD_BITS + 1), std::out_of_range);
        REQUIRE(b == copy);
    }

    // ------------------------------------------------------------------------
    // 7. appendLSB(const Bitset&)
    // ------------------------------------------------------------------------
    SECTION("appendLSB(Bitset) basic")
    {
        Bitset a{ "100" };
        Bitset b{ "01" };
        a.appendLSB(b);
        REQUIRE(a.equals("10001"));
        REQUIRE(b.equals("01"));
        requireInvariants(a, "appendLSB(Bitset)");
    }

    SECTION("appendLSB(empty Bitset) is a no-op")
    {
        Bitset a{ "1011" };
        Bitset e;
        a.appendLSB(e);
        REQUIRE(a.equals("1011"));
        requireInvariants(a, "appendLSB(empty)");
    }

    SECTION("appendLSB self-append works via copy")
    {
        Bitset d{ "10" };
        d.appendLSB(d);
        REQUIRE(d.size() == 4);
        REQUIRE(d.equals("1010"));
        requireInvariants(d, "self appendLSB");
    }

    // ------------------------------------------------------------------------
    // 8. removeMSB / popMSB
    // ------------------------------------------------------------------------
    SECTION("removeMSB removes leading character")
    {
        Bitset b{ "1101" };
        b.removeMSB();
        REQUIRE(b.equals("101"));
        b.removeMSB();
        REQUIRE(b.equals("01"));
        b.removeMSB();
        REQUIRE(b.equals("1"));
        b.removeMSB();
        REQUIRE(b.size() == 0);
        REQUIRE(b.wordsSize() == 0);
        REQUIRE(b.isZero());
        requireInvariants(b, "removeMSB to empty");
    }

    SECTION("popMSB is equivalent to removeMSB")
    {
        Bitset a{ "1011" };
        Bitset b{ a };
        a.removeMSB();
        b.popMSB();
        REQUIRE(a == b);
    }

    SECTION("removeMSB crosses word boundary")
    {
        Bitset b(WORD_BITS + 3);
        b.setAll(true);
        REQUIRE(b.equals(std::string(WORD_BITS + 3, '1')));

        b.removeMSB();
        REQUIRE(b.size() == WORD_BITS + 2);
        REQUIRE(b.equals(std::string(WORD_BITS + 2, '1')));
        requireInvariants(b, "removeMSB cross word");

        // Удалим до границы слова
        for (size_t i = 0; i < 2; ++i)
            b.removeMSB();
        REQUIRE(b.size() == WORD_BITS);
        REQUIRE(b.wordsSize() == 1);
        REQUIRE(b.equals(std::string(WORD_BITS, '1')));
        requireInvariants(b, "removeMSB shrink to one word");
    }

    SECTION("removeMSB keeps specific bits")
    {
        Bitset b(WORD_BITS + 2);
        b.set(WORD_BITS, true);   // будет удалено, если снять MSB
        b.set(0, true);           // должно остаться

        b.removeMSB();            // удаляем bit WORD_BITS+1 (был 0)
        REQUIRE(b.size() == WORD_BITS + 1);
        REQUIRE(b[WORD_BITS] == true);
        REQUIRE(b[0] == true);
        requireInvariants(b, "removeMSB keep bits");
    }

    // ------------------------------------------------------------------------
    // 9. removeLSB
    // ------------------------------------------------------------------------
    SECTION("removeLSB removes trailing character")
    {
        Bitset b{ "1101" };
        b.removeLSB();
        REQUIRE(b.equals("110"));
        b.removeLSB();
        REQUIRE(b.equals("11"));
        b.removeLSB();
        REQUIRE(b.equals("1"));
        b.removeLSB();
        REQUIRE(b.size() == 0);
        REQUIRE(b.isZero());
        requireInvariants(b, "removeLSB to empty");
    }

    SECTION("removeLSB crosses word boundary")
    {
        Bitset b(WORD_BITS + 3);
        b.setAll(true);
        b.removeLSB();
        REQUIRE(b.size() == WORD_BITS + 2);
        REQUIRE(b.equals(std::string(WORD_BITS + 2, '1')));
        requireInvariants(b, "removeLSB cross word");
    }

    // ------------------------------------------------------------------------
    // 10. Согласованность: appendLSB / removeLSB, appendMSB / removeMSB
    // ------------------------------------------------------------------------
    SECTION("appendLSB then removeLSB returns to original")
    {
        for (auto s : { "1", "10", "101", "101101", "1" })
        {
            INFO("s = " << s);
            Bitset b{ std::string(s) };
            const Bitset copy{ b };

            b.appendLSB(true);
            b.removeLSB();
            REQUIRE(b == copy);
            requireInvariants(b, "appendLSB/removeLSB round-trip");
        }
    }

    SECTION("appendMSB then removeMSB returns to original")
    {
        for (auto s : { "1", "10", "101", "101101", "0" })
        {
            INFO("s = " << s);
            Bitset b{ std::string(s) };
            const Bitset copy{ b };

            b.appendMSB(true);
            b.removeMSB();
            REQUIRE(b == copy);
            requireInvariants(b, "appendMSB/removeMSB round-trip");
        }
    }

    SECTION("appendLSB(Bitset) then removeLSB n times returns to original")
    {
        Bitset a{ "1011" };
        Bitset b{ "110" };
        const Bitset a0{ a };

        a.appendLSB(b);
        REQUIRE(a.size() == a0.size() + b.size());

        for (size_t i = 0; i < b.size(); ++i)
            a.removeLSB();

        REQUIRE(a == a0);
        requireInvariants(a, "appendLSB/removeLSB x N");
    }

    // ------------------------------------------------------------------------
    // 11. Интеграция с toString
    // ------------------------------------------------------------------------
    SECTION("appendLSB(string) == concat on right")
    {
        for (auto s : { "1", "01", "110", "1011" })
        {
            Bitset b{ "101" };
            b.appendLSB(s);
            REQUIRE(b.toString() == std::string("101") + s);
        }
    }

    SECTION("appendMSB(string via Bitset) == concat on left")
    {
        for (auto s : { "1", "01", "110", "1011" })
        {
            Bitset b{ "101" };
            Bitset other{ std::string(s) };
            b.appendMSB(other);
            REQUIRE(b.toString() == std::string(s) + "101");
        }
    }

    // ------------------------------------------------------------------------
    // 12. Управление памятью: растущие и убывающие последовательности
    // ------------------------------------------------------------------------
    SECTION("grow then shrink keeps invariants")
    {
        Bitset b;
        for (size_t i = 0; i < 200; ++i)
            b.appendMSB(true);
        REQUIRE(b.size() == 200);
        requireInvariants(b, "grow 200");

        for (size_t i = 0; i < 200; ++i)
            b.removeMSB();
        REQUIRE(b.size() == 0);
        REQUIRE(b.wordsSize() == 0);
        requireInvariants(b, "shrink to 0");
    }

    SECTION("removeMSB with alternating removal from both ends")
    {
        Bitset b{ "101010101010" };
        while (b.size() > 0)
        {
            b.removeMSB();
            if (b.size() > 0)
                b.removeLSB();
        }
        REQUIRE(b.size() == 0);
        REQUIRE(b.wordsSize() == 0);
        requireInvariants(b, "alternating remove");
    }
}

// ============================================================================
//  ЭТАП 7. Операторы сравнения
// ============================================================================
TEST_CASE("Bitset comparison operators (full)", "[bitset][comparison]")
{
    // ------------------------------------------------------------------------
    // 1. operator== / operator!= — базовые случаи
    // ------------------------------------------------------------------------
    SECTION("== and != on same size")
    {
        for (auto s : { "", "0", "1", "10", "1010", "11110000",
                       "00000001", "11111111" })
        {
            INFO("s = " << s);
            Bitset a{ std::string(s) };
            Bitset b{ std::string(s) };
            REQUIRE(a == b);
            REQUIRE_FALSE(a != b);
            REQUIRE(b == a);
            REQUIRE_FALSE(b != a);
        }
    }

    SECTION("== and != on same size, different bits")
    {
        for (auto pair : std::initializer_list<std::pair<const char*, const char*>>{
                                                                                      { "0", "1" },
                                                                                      { "10", "11" },
                                                                                      { "1010", "1011" },
                                                                                      { "0000", "1000" },
                                                                                      { "11110000", "11110001" } })
        {
            INFO("a = " << pair.first << ", b = " << pair.second);
            Bitset a{ std::string(pair.first) };
            Bitset b{ std::string(pair.second) };
            REQUIRE_FALSE(a == b);
            REQUIRE(a != b);
            REQUIRE_FALSE(b == a);
            REQUIRE(b != a);
        }
    }

    SECTION("== on different sizes is always false")
    {
        Bitset a{ "1010" };
        for (auto s : std::initializer_list<std::string>{ "", "0", "1", "10", "101", "10101", "1010" + std::string(1, '0') })
        {
            INFO("s = " << s);
            Bitset b{ s };
            if (a.size() != b.size())
            {
                REQUIRE_FALSE(a == b);
                REQUIRE(a != b);
            }
        }
    }

    SECTION("empty bitsets are equal")
    {
        Bitset a, b;
        REQUIRE(a == b);
        REQUIRE_FALSE(a != b);
    }

    SECTION("empty vs non-empty is not equal")
    {
        Bitset e;
        for (auto s : { "0", "1", "10", "111" })
        {
            Bitset b{ std::string(s) };
            REQUIRE_FALSE(e == b);
            REQUIRE_FALSE(b == e);
            REQUIRE(e != b);
            REQUIRE(b != e);
        }
    }

    SECTION("self comparison")
    {
        Bitset a{ "101101" };
        REQUIRE(a == a);
        REQUIRE_FALSE(a != a);
    }

    // ------------------------------------------------------------------------
    // 2. == / != на границе слов и при разном мусоре
    // ------------------------------------------------------------------------
    SECTION("== across word boundaries")
    {
        Bitset a(WORD_BITS + 5);
        Bitset b(WORD_BITS + 5);
        a.set(0, true);
        a.set(WORD_BITS + 3, true);
        b.set(0, true);
        b.set(WORD_BITS + 3, true);
        REQUIRE(a == b);

        b.set(WORD_BITS + 4, true);
        REQUIRE_FALSE(a == b);
        REQUIRE(a != b);
    }

    SECTION("== ignores garbage bits (invariant guarantees zero garbage)")
    {
        // После любых операций мусорные биты == 0; равенство зависит только
        // от валидных битов [0, size)
        Bitset a(WORD_BITS + 3);
        Bitset b(WORD_BITS + 3);
        a.setAll(true);
        b.setAll(true);
        REQUIRE(a == b);
        REQUIRE(a.toString() == b.toString());
    }

    // ------------------------------------------------------------------------
    // 3. operator<=> — базовые свойства
    // ------------------------------------------------------------------------
    SECTION("<=> returns strong_ordering")
    {
        STATIC_REQUIRE(
            std::is_same_v<decltype(std::declval<const Bitset&>() <=> std::declval<const Bitset&>()),
                           std::strong_ordering>);
    }

    SECTION("<=> on equal bitsets")
    {
        Bitset a{ "1010" };
        Bitset b{ "1010" };
        REQUIRE((a <=> b) == std::strong_ordering::equal);
        REQUIRE_FALSE((a <=> b) < 0);
        REQUIRE_FALSE((a <=> b) > 0);
    }

    SECTION("<=> compares sizes first")
    {
        Bitset a{ "111" };         // size 3
        Bitset b{ "0000" };        // size 4
        REQUIRE((a <=> b) == std::strong_ordering::less);
        REQUIRE((b <=> a) == std::strong_ordering::greater);

        Bitset e;
        REQUIRE((e <=> a) == std::strong_ordering::less);
        REQUIRE((a <=> e) == std::strong_ordering::greater);
    }

    SECTION("<=> on same size compares MSB-first (numeric order)")
    {
        // "01" == value 1 (bit0=1, bit1=0)
        // "10" == value 2 (bit0=0, bit1=1)
        Bitset a{ "01" };
        Bitset b{ "10" };
        REQUIRE((a <=> b) < 0);
        REQUIRE((b <=> a) > 0);
        REQUIRE((a <=> a) == 0);

        // "10" (2) < "11" (3)
        Bitset c{ "10" };
        Bitset d{ "11" };
        REQUIRE(c < d);
        REQUIRE(d > c);
    }

    SECTION("<=> multi-word: high word decides")
    {
        Bitset x(2 * WORD_BITS);
        Bitset y(2 * WORD_BITS);

        x.setValue(0, 0, WORD_BITS);            // low  = 0
        x.setValue(1, WORD_BITS, WORD_BITS);    // high = 1

        y.setValue(~Word{ 0 }, 0, WORD_BITS);   // low  = all ones
        y.setValue(0, WORD_BITS, WORD_BITS);    // high = 0

        // Старшее слово у x > y, значит x > y, несмотря на low
        REQUIRE((x <=> y) > 0);
        REQUIRE((y <=> x) < 0);
        REQUIRE(x > y);
        REQUIRE(y < x);
    }

    SECTION("<=> is consistent with == on equal sizes")
    {
        for (auto s : { "1", "10", "1011", "10000001", "11111111" })
        {
            Bitset a{ std::string(s) };
            Bitset b{ std::string(s) };
            REQUIRE((a <=> b) == std::strong_ordering::equal);
            REQUIRE(a == b);
        }
    }

    // ------------------------------------------------------------------------
    // 4. Сгенерированные операторы <, <=, >, >=
    // ------------------------------------------------------------------------
    SECTION("relational operators on same size")
    {
        Bitset a{ "0100" }; // value 4? посмотрим: MSB->LSB = 0,1,0,0 -> bit2=1 -> value 4
        Bitset b{ "1000" }; // bit3=1 -> value 8
        Bitset c{ a };

        REQUIRE(a < b);
        REQUIRE(a <= b);
        REQUIRE(b > a);
        REQUIRE(b >= a);
        REQUIRE_FALSE(a > b);
        REQUIRE_FALSE(a >= b);
        REQUIRE_FALSE(b < a);
        REQUIRE_FALSE(b <= a);

        REQUIRE(a <= c);
        REQUIRE(a >= c);
        REQUIRE_FALSE(a < c);
        REQUIRE_FALSE(a > c);
    }

    SECTION("relational operators on different sizes")
    {
        Bitset s{ "111" };     // size 3
        Bitset l{ "0000" };    // size 4

        REQUIRE(s < l);
        REQUIRE(s <= l);
        REQUIRE(l > s);
        REQUIRE(l >= s);
        REQUIRE_FALSE(s > l);
        REQUIRE_FALSE(s >= l);
        REQUIRE_FALSE(l < s);
        REQUIRE_FALSE(l <= s);
    }

    SECTION("all relational operators agree with <=>")
    {
        std::vector<std::string> strings{
            "", "0", "1", "10", "11", "100", "101",
            "1000", "1010", "1111", "0000", "00000001"
        };

        for (const auto& sa : strings)
        {
            for (const auto& sb : strings)
            {
                Bitset a{ sa };
                Bitset b{ sb };
                const auto cmp{ a <=> b };

                INFO("a = " << sa << ", b = " << sb);
                REQUIRE((a == b) == (cmp == 0));
                REQUIRE((a != b) == (cmp != 0));
                REQUIRE((a <  b) == (cmp <  0));
                REQUIRE((a <= b) == (cmp <= 0));
                REQUIRE((a >  b) == (cmp >  0));
                REQUIRE((a >= b) == (cmp >= 0));
            }
        }
    }

    // ------------------------------------------------------------------------
    // 5. Транзитивность и антисимметричность
    // ------------------------------------------------------------------------
    SECTION("transitivity of <=")
    {
        std::vector<std::string> strings{
            "", "0", "1", "10", "11", "100", "1000", "10000"
        };
        for (const auto& sa : strings)
            for (const auto& sb : strings)
                for (const auto& sc : strings)
                {
                    Bitset a{ sa }, b{ sb }, c{ sc };
                    if (a <= b && b <= c)
                    {
                        INFO(sa << " <= " << sb << " <= " << sc);
                        REQUIRE(a <= c);
                    }
                }
    }

    SECTION("antisymmetry: a <= b && b <= a implies a == b")
    {
        std::vector<std::string> strings{
            "", "0", "1", "10", "11", "101", "1010"
        };
        for (const auto& sa : strings)
            for (const auto& sb : strings)
            {
                Bitset a{ sa }, b{ sb };
                if (a <= b && b <= a)
                {
                    INFO(sa << " <=> " << sb);
                    REQUIRE(a == b);
                }
                if (a < b)
                {
                    REQUIRE_FALSE(b < a);
                }
            }
    }


    // ------------------------------------------------------------------------
    // 6. noexcept
    // ------------------------------------------------------------------------
    SECTION("comparison operators are noexcept")
    {
        STATIC_REQUIRE(noexcept(std::declval<const Bitset&>() == std::declval<const Bitset&>()));
        STATIC_REQUIRE(noexcept(std::declval<const Bitset&>() != std::declval<const Bitset&>()));
        STATIC_REQUIRE(noexcept(std::declval<const Bitset&>() <=> std::declval<const Bitset&>()));
        STATIC_REQUIRE(noexcept(std::declval<const Bitset&>() < std::declval<const Bitset&>()));
        STATIC_REQUIRE(noexcept(std::declval<const Bitset&>() <= std::declval<const Bitset&>()));
        STATIC_REQUIRE(noexcept(std::declval<const Bitset&>() > std::declval<const Bitset&>()));
        STATIC_REQUIRE(noexcept(std::declval<const Bitset&>() >= std::declval<const Bitset&>()));
    }


    // ------------------------------------------------------------------------
    // 7. Согласованность с числовым значением (для size <= 64)
    // ------------------------------------------------------------------------
    SECTION("ordering matches numeric value for small sizes (exhaustive)")
    {
        // Полный перебор разумен только для маленьких n.
        for (size_t n : { 1u, 2u, 3u, 4u, 5u, 8u })
        {
            INFO("n = " << n);
            const size_t limit{ size_t{ 1 } << n };   // 2^n, n <= 8
            for (size_t x = 0; x < limit; ++x)
            {
                for (size_t y = 0; y < limit; ++y)
                {
                    Bitset bx{ n };
                    Bitset by{ n };
                    bx.setValue(static_cast<Word>(x), 0, n);
                    by.setValue(static_cast<Word>(y), 0, n);

                    INFO("n = " << n << ", x = " << x << ", y = " << y);
                    REQUIRE((bx == by) == (x == y));
                    REQUIRE((bx != by) == (x != y));
                    REQUIRE((bx <  by) == (x <  y));
                    REQUIRE((bx <= by) == (x <= y));
                    REQUIRE((bx >  by) == (x >  y));
                    REQUIRE((bx >= by) == (x >= y));
                }
            }
        }
    }

    SECTION("ordering matches numeric value for medium sizes (sampled)")
    {
        // Для больших n — детерминированная выборка значений вокруг
        // границ и «интересных» точек.
        for (size_t n : { 16u, 32u, 63u })
        {
            INFO("n = " << n);
            const Word maxBit{ static_cast<Word>(Word{ 1 } << (n - 1)) };
            std::vector<Word> samples{
                Word{ 0 },
                Word{ 1 },
                Word{ 2 },
                Word{ 3 },
                maxBit,
                maxBit - 1,
                maxBit + 1,
                static_cast<Word>(maxBit | Word{ 1 }),
                static_cast<Word>(maxBit - 2),
                static_cast<Word>(~Word{ 0 } >> (64 - n)),  // все валидные биты = 1
            };
            // Уберём дубликаты и всё, что не влезает в n бит
            std::sort(samples.begin(), samples.end());
            samples.erase(std::unique(samples.begin(), samples.end()), samples.end());
            samples.erase(std::remove_if(samples.begin(), samples.end(),
                                         [n](Word v)
                                         {
                                             if (n == 64) return false;
                                             return v >= (Word{ 1 } << n);
                                         }),
                          samples.end());

            for (Word x : samples)
            {
                for (Word y : samples)
                {
                    Bitset bx{ n };
                    Bitset by{ n };
                    bx.setValue(x, 0, n);
                    by.setValue(y, 0, n);

                    INFO("n = " << n << ", x = " << x << ", y = " << y);
                    REQUIRE((bx == by) == (x == y));
                    REQUIRE((bx <  by) == (x <  y));
                    REQUIRE((bx <= by) == (x <= y));
                    REQUIRE((bx >  by) == (x >  y));
                    REQUIRE((bx >= by) == (x >= y));
                }
            }
        }
    }

    // ------------------------------------------------------------------------
    // 8. Согласованность == с toString
    // ------------------------------------------------------------------------
    SECTION("== agrees with toString equality")
    {
        std::vector<std::string> strings{
            "", "0", "1", "10", "11", "100", "101", "1010",
            "11110000", "00000001", "10000000"
        };
        for (const auto& sa : strings)
            for (const auto& sb : strings)
            {
                Bitset a{ sa };
                Bitset b{ sb };
                INFO("a = " << sa << ", b = " << sb);
                REQUIRE((a == b) == (a.toString() == b.toString()));
            }
    }
}

// ============================================================================
//  ЭТАП 8. Побитовые операции
// ============================================================================
TEST_CASE("Bitset bitwise operations", "[bitset][bitwise]")
{
    // ------------------------------------------------------------------------
    // 1. operator&= — базовые случаи
    // ------------------------------------------------------------------------
    SECTION("&= basic truth table on single bits")
    {
        // Все 4 комбинации
        struct Case { const char* a; const char* b; const char* r; };
        for (auto c : std::initializer_list<Case>{
                                                  { "0", "0", "0" },
                                                  { "0", "1", "0" },
                                                  { "1", "0", "0" },
                                                  { "1", "1", "1" } })
        {
            INFO("a = " << c.a << ", b = " << c.b);
            Bitset a{ std::string(c.a) };
            Bitset b{ std::string(c.b) };
            a &= b;
            REQUIRE(a.equals(c.r));
            requireInvariants(a, "&= single");
        }
    }

    SECTION("&= on longer bitsets")
    {
        Bitset a{ "1010" };
        Bitset b{ "1100" };
        a &= b;
        REQUIRE(a.equals("1000"));
        requireInvariants(a, "&= longer");

        Bitset c{ "11110000" };
        Bitset d{ "10101010" };
        c &= d;
        REQUIRE(c.equals("10100000"));
        requireInvariants(c, "&= longer 2");
    }

    SECTION("&= with all zeros / all ones")
    {
        Bitset x{ "101101" };
        Bitset zero{ "000000" };
        Bitset ones{ "111111" };

        Bitset a{ x };
        a &= zero;
        REQUIRE(a.isZero());
        REQUIRE(a.popcount() == 0);

        Bitset b{ x };
        b &= ones;
        REQUIRE(b == x);
        requireInvariants(b, "&= all ones");
    }

    // ------------------------------------------------------------------------
    // 2. operator|= — базовые случаи
    // ------------------------------------------------------------------------
    SECTION("|= basic truth table on single bits")
    {
        struct Case { const char* a; const char* b; const char* r; };
        for (auto c : std::initializer_list<Case>{
                                                  { "0", "0", "0" },
                                                  { "0", "1", "1" },
                                                  { "1", "0", "1" },
                                                  { "1", "1", "1" } })
        {
            INFO("a = " << c.a << ", b = " << c.b);
            Bitset a{ std::string(c.a) };
            Bitset b{ std::string(c.b) };
            a |= b;
            REQUIRE(a.equals(c.r));
            requireInvariants(a, "|= single");
        }
    }

    SECTION("|= on longer bitsets")
    {
        Bitset a{ "1010" };
        Bitset b{ "1100" };
        a |= b;
        REQUIRE(a.equals("1110"));
        requireInvariants(a, "|= longer");
    }

    SECTION("|= with all zeros / all ones")
    {
        Bitset x{ "101101" };
        Bitset zero{ "000000" };
        Bitset ones{ "111111" };

        Bitset a{ x };
        a |= zero;
        REQUIRE(a == x);

        Bitset b{ x };
        b |= ones;
        REQUIRE(b.equals("111111"));
        REQUIRE(b.popcount() == 6);
        requireInvariants(b, "|= all ones");
    }

    // ------------------------------------------------------------------------
    // 3. operator^= — базовые случаи
    // ------------------------------------------------------------------------
    SECTION("^= basic truth table on single bits")
    {
        struct Case { const char* a; const char* b; const char* r; };
        for (auto c : std::initializer_list<Case>{
                                                  { "0", "0", "0" },
                                                  { "0", "1", "1" },
                                                  { "1", "0", "1" },
                                                  { "1", "1", "0" } })
        {
            INFO("a = " << c.a << ", b = " << c.b);
            Bitset a{ std::string(c.a) };
            Bitset b{ std::string(c.b) };
            a ^= b;
            REQUIRE(a.equals(c.r));
            requireInvariants(a, "^= single");
        }
    }

    SECTION("^= on longer bitsets")
    {
        Bitset a{ "1010" };
        Bitset b{ "1100" };
        a ^= b;
        REQUIRE(a.equals("0110"));
        requireInvariants(a, "^= longer");
    }

    SECTION("^= with self produces zero")
    {
        Bitset a{ "101101" };
        Bitset b{ a };
        a ^= b;
        REQUIRE(a.isZero());
        REQUIRE(a.popcount() == 0);
        requireInvariants(a, "^= self");
    }

    // ------------------------------------------------------------------------
    // 4. Возвращаемое значение — ссылка на *this
    // ------------------------------------------------------------------------
    SECTION("operators return *this reference")
    {
        Bitset a{ "1010" };
        Bitset b{ "1100" };

        Bitset& refA = (a &= b);
        REQUIRE(&refA == &a);

        Bitset c{ "1010" };
        Bitset& refC = (c |= b);
        REQUIRE(&refC == &c);

        Bitset d{ "1010" };
        Bitset& refD = (d ^= b);
        REQUIRE(&refD == &d);
    }

    SECTION("chaining works")
    {
        Bitset a{ "1111" };
        Bitset b{ "1010" };
        Bitset c{ "1100" };
        a &= b;
        a |= c;
        // a = ((1111 & 1010) | 1100) = (1010 | 1100) = 1110
        REQUIRE(a.equals("1110"));
        requireInvariants(a, "chain");
    }

    // ------------------------------------------------------------------------
    // 5. Self-присваивание (a &= a, a |= a, a ^= a)
    // ------------------------------------------------------------------------
    SECTION("self &=")
    {
        Bitset a{ "101101" };
        Bitset copy{ a };
        a &= copy;   // через копию, чтобы не ловить -Wself-assign
        REQUIRE(a == copy);
    }

    SECTION("self |=")
    {
        Bitset a{ "101101" };
        Bitset copy{ a };
        a |= copy;
        REQUIRE(a == copy);
    }

    SECTION("self ^=")
    {
        Bitset a{ "101101" };
        Bitset copy{ a };
        a ^= copy;
        REQUIRE(a.isZero());
    }

    // ------------------------------------------------------------------------
    // 6. Мусорные биты: результат тоже не должен их иметь
    // ------------------------------------------------------------------------
    SECTION("garbage bits stay zero after bitwise ops")
    {
        for (size_t n : std::initializer_list<size_t>{ 1u, WORD_BITS - 1, WORD_BITS + 1,
                         WORD_BITS + 5, 2 * WORD_BITS + 3 })
        {
            INFO("n = " << n);
            Bitset a{ n };
            Bitset b{ n };
            a.setAll(true);
            b.setAll(true);

            Bitset c{ a };
            c &= b;
            requireInvariants(c, "&= garbage");

            Bitset d{ a };
            d |= b;
            requireInvariants(d, "|= garbage");

            Bitset e{ a };
            e ^= b;
            requireInvariants(e, "^= garbage");
        }
    }

    // ------------------------------------------------------------------------
    // 7. Разный размер — std::length_error
    // ------------------------------------------------------------------------
    SECTION("&= on different sizes throws length_error")
    {
        Bitset a{ "1010" };   // size 4
        Bitset b{ "101" };    // size 3
        const Bitset copy{ a };
        REQUIRE_THROWS_AS(a &= b, std::length_error);
        REQUIRE(a == copy);   // strong guarantee
    }

    SECTION("|= on different sizes throws length_error")
    {
        Bitset a{ "1010" };
        Bitset b{ "101" };
        const Bitset copy{ a };
        REQUIRE_THROWS_AS(a |= b, std::length_error);
        REQUIRE(a == copy);
    }

    SECTION("^= on different sizes throws length_error")
    {
        Bitset a{ "1010" };
        Bitset b{ "101" };
        const Bitset copy{ a };
        REQUIRE_THROWS_AS(a ^= b, std::length_error);
        REQUIRE(a == copy);
    }

    SECTION("empty vs non-empty throws length_error")
    {
        Bitset e;
        Bitset b{ "1" };
        REQUIRE_THROWS_AS(e &= b, std::length_error);
        REQUIRE_THROWS_AS(e |= b, std::length_error);
        REQUIRE_THROWS_AS(e ^= b, std::length_error);

        Bitset e2;
        Bitset b2{ "1" };
        REQUIRE_THROWS_AS(b2 &= e2, std::length_error);
        REQUIRE_THROWS_AS(b2 |= e2, std::length_error);
        REQUIRE_THROWS_AS(b2 ^= e2, std::length_error);
    }

    SECTION("empty &= empty is a no-op")
    {
        Bitset a, b;
        REQUIRE_NOTHROW(a &= b);
        REQUIRE_NOTHROW(a |= b);
        REQUIRE_NOTHROW(a ^= b);
        REQUIRE(a.size() == 0);
        requireInvariants(a, "empty bitwise");
    }

    // ------------------------------------------------------------------------
    // 8. Алгебраические свойства
    // ------------------------------------------------------------------------
    SECTION("commutativity: a & b == b & a")
    {
        for (auto pair : std::initializer_list<std::pair<const char*, const char*>>{
                                                                                      { "1010", "1100" },
                                                                                      { "0000", "1111" },
                                                                                      { "101101", "011010" },
                                                                                      { "1111000011110000", "1010101010101010" } })
        {
            Bitset a{ std::string(pair.first) };
            Bitset b{ std::string(pair.second) };

            Bitset ab{ a }, ba{ b };
            ab &= b;
            ba &= a;
            REQUIRE(ab == ba);

            Bitset ao{ a }, bo{ b };
            ao |= b;
            bo |= a;
            REQUIRE(ao == bo);

            Bitset ax{ a }, bx{ b };
            ax ^= b;
            bx ^= a;
            REQUIRE(ax == bx);
        }
    }

    SECTION("De Morgan's laws")
    {
        Bitset a{ "101101" };
        Bitset b{ "110011" };

        // NOT (a & b) == (NOT a) | (NOT b)
        Bitset left{ a };
        left &= b;
        left.flip();

        Bitset na{ a }; na.flip();
        Bitset nb{ b }; nb.flip();
        na |= nb;

        REQUIRE(left == na);
    }

    SECTION("a & (b | c) == (a & b) | (a & c)")
    {
        Bitset a{ "101010" };
        Bitset b{ "110011" };
        Bitset c{ "001111" };

        Bitset left{ b };
        left |= c;
        left &= a;

        Bitset ab{ a }; ab &= b;
        Bitset ac{ a }; ac &= c;
        ab |= ac;

        REQUIRE(left == ab);
    }

    SECTION("a ^ b == (a | b) & ~(a & b)")
    {
        Bitset a{ "101101" };
        Bitset b{ "110011" };

        Bitset left{ a }; left ^= b;

        Bitset orAB{ a }; orAB |= b;
        Bitset andAB{ a }; andAB &= b;
        andAB.flip();
        orAB &= andAB;

        REQUIRE(left == orAB);
    }

    SECTION("a ^ b == (a | b) & (~a | ~b)")
    {
        Bitset a{ "101101" };
        Bitset b{ "110011" };

        Bitset left{ a }; left ^= b;

        Bitset orAB{ a }; orAB |= b;
        Bitset na{ a }; na.flip();
        Bitset nb{ b }; nb.flip();
        na |= nb;
        orAB &= na;

        REQUIRE(left == orAB);
    }

    // ------------------------------------------------------------------------
    // 9. Согласованность с popcount / ==
    // ------------------------------------------------------------------------
    SECTION("popcount relationships")
    {
        for (auto pair : std::initializer_list<std::pair<const char*, const char*>>{
                                                                                      { "1010", "1100" },
                                                                                      { "11110000", "10101010" } })
        {
            Bitset a{ std::string(pair.first) };
            Bitset b{ std::string(pair.second) };

            Bitset andAB{ a }; andAB &= b;
            Bitset orAB{ a }; orAB |= b;
            Bitset xorAB{ a }; xorAB ^= b;

            // |a| + |b| == |a&b| + |a|b|
            REQUIRE(a.popcount() + b.popcount() ==
                    andAB.popcount() + orAB.popcount());
            // |a^b| == |a|b| - |a&b|
            REQUIRE(xorAB.popcount() == orAB.popcount() - andAB.popcount());
        }
    }

    // ------------------------------------------------------------------------
    // 10. Round-trip / инволюция
    // ------------------------------------------------------------------------
    SECTION("^= is its own inverse")
    {
        Bitset a{ "101101" };
        Bitset b{ "110011" };
        const Bitset a0{ a };

        a ^= b;
        a ^= b;
        REQUIRE(a == a0);
        requireInvariants(a, "^= involution");
    }

    SECTION("&= is idempotent")
    {
        Bitset a{ "101101" };
        const Bitset copy{ a };
        a &= copy;
        a &= copy;
        a &= copy;
        REQUIRE(a == copy);
    }

    SECTION("|= is idempotent")
    {
        Bitset a{ "101101" };
        const Bitset copy{ a };
        a |= copy;
        a |= copy;
        a |= copy;
        REQUIRE(a == copy);
    }
}

// ============================================================================
//  ЭТАП 9. Операторы сдвига
// ============================================================================
TEST_CASE("Bitset shift operations", "[bitset][shifts]")
{
    // ------------------------------------------------------------------------
    // 1. Сдвиг на 0 — no-op
    // ------------------------------------------------------------------------
    SECTION("shift by 0 is a no-op")
    {
        for (auto s : { "", "0", "1", "10", "1010", "10110101" })
        {
            INFO("s = " << s);
            Bitset a{ std::string(s) };
            const Bitset b{ a };

            REQUIRE_NOTHROW(a <<= 0);
            REQUIRE(a == b);

            REQUIRE_NOTHROW(a >>= 0);
            REQUIRE(a == b);
        }
    }

    // ------------------------------------------------------------------------
    // 2. <<= 1 на маленьких bitset'ах
    // ------------------------------------------------------------------------
    SECTION("<<= 1 on small bitsets")
    {
        Bitset a{ "1010" };
        a <<= 1;
        REQUIRE(a.equals("0100"));
        REQUIRE(a.size() == 4);
        requireInvariants(a, "<<=1 1010");

        Bitset b{ "1001" };
        b <<= 1;
        REQUIRE(b.equals("0010"));

        Bitset c{ "1111" };
        c <<= 1;
        REQUIRE(c.equals("1110"));

        Bitset d{ "0001" };
        d <<= 1;
        REQUIRE(d.equals("0010"));

        Bitset e{ "1000" };
        e <<= 1;
        REQUIRE(e.equals("0000"));

        Bitset one{ "1" };
        one <<= 1;
        REQUIRE(one.equals("0"));

        Bitset zero{ "0" };
        zero <<= 1;
        REQUIRE(zero.equals("0"));
    }

    // ------------------------------------------------------------------------
    // 3. >>= 1 на маленьких bitset'ах
    // ------------------------------------------------------------------------
    SECTION(">>= 1 on small bitsets")
    {
        Bitset a{ "1010" };
        a >>= 1;
        REQUIRE(a.equals("0101"));
        requireInvariants(a, ">>=1 1010");

        Bitset b{ "1001" };
        b >>= 1;
        REQUIRE(b.equals("0100"));

        Bitset c{ "1111" };
        c >>= 1;
        REQUIRE(c.equals("0111"));

        Bitset d{ "1000" };
        d >>= 1;
        REQUIRE(d.equals("0100"));

        Bitset e{ "0001" };
        e >>= 1;
        REQUIRE(e.equals("0000"));

        Bitset one{ "1" };
        one >>= 1;
        REQUIRE(one.equals("0"));
    }

    // ------------------------------------------------------------------------
    // 4. Соответствие ручному сдвигу: один бит
    // ------------------------------------------------------------------------
    SECTION("<<= k moves bit 0 to bit k")
    {
        for (size_t n : std::initializer_list<size_t>{ 4u, 8u, WORD_BITS })
        {
            for (size_t k : std::initializer_list<size_t>{ 0u, 1u, 2u, n - 1 })
            {
                INFO("n = " << n << ", k = " << k);
                Bitset a{ n };
                a.set(0, true);
                a <<= k;

                Bitset expected{ n };
                expected.set(k, true);
                REQUIRE(a == expected);
                REQUIRE(a.popcount() == 1);
                requireInvariants(a, "<<= k manual");
            }
        }
    }

    SECTION(">>= k moves top bit down by k")
    {
        for (size_t n : std::initializer_list<size_t>{ 4u, 8u, WORD_BITS })
        {
            for (size_t k : std::initializer_list<size_t>{ 0u, 1u, 2u, n - 1 })
            {
                INFO("n = " << n << ", k = " << k);
                Bitset a{ n };
                a.set(n - 1, true);
                a >>= k;

                Bitset expected{ n };
                expected.set(n - 1 - k, true);
                REQUIRE(a == expected);
                REQUIRE(a.popcount() == 1);
                requireInvariants(a, ">>= k manual");
            }
        }
    }

    // ------------------------------------------------------------------------
    // 5. Сдвиги, пересекающие границу слов
    // ------------------------------------------------------------------------
    SECTION("<<= crossing word boundary")
    {
        Bitset b(2 * WORD_BITS);
        b.set(0, true);
        b <<= WORD_BITS;
        REQUIRE(b[WORD_BITS] == true);
        REQUIRE(b.popcount() == 1);
        requireInvariants(b, "<<= cross word full");

        Bitset c(2 * WORD_BITS);
        c.set(0, true);
        c <<= WORD_BITS - 1;
        REQUIRE(c[WORD_BITS - 1] == true);
        REQUIRE(c.popcount() == 1);
        requireInvariants(c, "<<= cross word -1");

        Bitset d(2 * WORD_BITS);
        d.set(0, true);
        d <<= WORD_BITS + 1;
        REQUIRE(d[WORD_BITS + 1] == true);
        REQUIRE(d.popcount() == 1);
        requireInvariants(d, "<<= cross word +1");
    }

    SECTION(">>= crossing word boundary")
    {
        Bitset b(2 * WORD_BITS);
        b.set(2 * WORD_BITS - 1, true);
        b >>= WORD_BITS;
        REQUIRE(b[WORD_BITS - 1] == true);
        REQUIRE(b.popcount() == 1);
        requireInvariants(b, ">>= cross word");

        Bitset c(2 * WORD_BITS);
        c.set(WORD_BITS, true);
        c >>= WORD_BITS - 1;
        REQUIRE(c[1] == true);
        REQUIRE(c.popcount() == 1);
        requireInvariants(c, ">>= cross word -1");
    }

    // ------------------------------------------------------------------------
    // 6. Размер сохраняется
    // ------------------------------------------------------------------------
    SECTION("shifts preserve size and wordsSize")
    {
        for (size_t n : std::initializer_list<size_t>{ 1u, WORD_BITS - 1, WORD_BITS, WORD_BITS + 1,
                         2 * WORD_BITS + 3 })
        {
            INFO("n = " << n);
            Bitset a{ n };
            const size_t n0{ a.size() };
            const size_t w0{ a.wordsSize() };
            a <<= 1;
            REQUIRE(a.size() == n0);
            REQUIRE(a.wordsSize() == w0);
            a >>= 3;
            REQUIRE(a.size() == n0);
            REQUIRE(a.wordsSize() == w0);
            requireInvariants(a, "shift size preserve");
        }
    }

    // ------------------------------------------------------------------------
    // 7. <<= n и >>= n на all-ones
    // ------------------------------------------------------------------------
    SECTION("<<= k on all-ones zeroes lower k bits")
    {
        Bitset b{ "11111111" };
        b <<= 3;
        REQUIRE(b.equals("11111000"));
        requireInvariants(b, "<<=3 all-ones");

        Bitset c{ "1111" };
        c <<= 2;
        REQUIRE(c.equals("1100"));

        Bitset d{ "1111" };
        d <<= 3;
        REQUIRE(d.equals("1000"));

        Bitset e{ "1111" };
        e <<= 4;
        REQUIRE(e.equals("0000"));   // если mod: должно быть "1111"
        // ВНИМАНИЕ: этот REQUIRE отражает modulo-семантику
        // (<<=4 на size 4 == <<=0). Если падает — см. audit-секцию.
    }

    SECTION(">>= k on all-ones zeroes upper k bits")
    {
        Bitset b{ "11111111" };
        b >>= 3;
        REQUIRE(b.equals("00011111"));

        Bitset c{ "1111" };
        c >>= 2;
        REQUIRE(c.equals("0011"));

        Bitset d{ "1111" };
        d >>= 3;
        REQUIRE(d.equals("0001"));

        Bitset e{ "1111" };
        e >>= 4;
        REQUIRE(e.equals("0000"));
    }

    // ------------------------------------------------------------------------
    // 8. Пустой bitset безопасен
    // ------------------------------------------------------------------------
    SECTION("shifts on empty bitset are safe")
    {
        Bitset e;
        REQUIRE_NOTHROW(e <<= 1);
        REQUIRE_NOTHROW(e >>= 1);
        REQUIRE_NOTHROW(e <<= 100);
        REQUIRE_NOTHROW(e >>= 100);
        REQUIRE_NOTHROW(e <<= 0);
        REQUIRE_NOTHROW(e >>= 0);
        REQUIRE(e.size() == 0);
        REQUIRE(e.wordsSize() == 0);
        requireInvariants(e, "empty shifts");
    }

    // ------------------------------------------------------------------------
    // 9. Мусорные биты остаются нулевыми
    // ------------------------------------------------------------------------
    SECTION("garbage bits stay zero after shifts")
    {
        for (size_t n : std::initializer_list<size_t>{ 1u, WORD_BITS - 1, WORD_BITS + 1,
                         WORD_BITS + 5, 2 * WORD_BITS + 3 })
        {
            INFO("n = " << n);
            Bitset a{ n };
            a.setAll(true);
            a <<= 1;
            requireInvariants(a, "<<= garbage");

            Bitset b{ n };
            b.setAll(true);
            b >>= 1;
            requireInvariants(b, ">>= garbage");

            Bitset c{ n };
            c.setAll(true);
            c <<= 5;
            requireInvariants(c, "<<=5 garbage");

            Bitset d{ n };
            d.setAll(true);
            d >>= 5;
            requireInvariants(d, ">>=5 garbage");
        }
    }

    SECTION("shift by size zeroes all bits")
    {
        for (auto s : { "1", "10", "1010", "101101" })
        {
            Bitset a{ std::string(s) };
            a <<= a.size();
            REQUIRE(a.isZero());

            Bitset b{ std::string(s) };
            b >>= b.size();
            REQUIRE(b.isZero());
        }
    }

    // ------------------------------------------------------------------------
    // 10. Композиция сдвигов
    // ------------------------------------------------------------------------
    SECTION("<<= p then <<= q equals <<= (p+q) for small p+q")
    {
        for (auto s : { "1000", "1001", "1010", "1100" })
        {
            INFO("s = " << s);
            for (size_t p : { 0u, 1u, 2u })
                for (size_t q : { 0u, 1u, 2u })
                {
                    if (p + q >= 4) continue;
                    INFO("p = " << p << ", q = " << q);

                    Bitset a{ std::string(s) };
                    Bitset b{ std::string(s) };

                    a <<= p;
                    a <<= q;

                    b <<= (p + q);

                    REQUIRE(a == b);
                }
        }
    }

    SECTION("<<= and >>= are inverses when no bit lost")
    {
        Bitset a{ "0011" };
        const Bitset copy{ a };
        a <<= 2;
        a >>= 2;
        REQUIRE(a == copy);
        requireInvariants(a, "<<=k then >>=k inverse");
    }

    SECTION("double flip via shifts for symmetric input")
    {
        Bitset a{ "1111" };
        Bitset b{ a };
        b <<= 2;
        b >>= 2;
        // a = "1111", b после <<=2 = "1100", после >>=2 = "0011"
        REQUIRE(b.equals("0011"));
    }

    // ------------------------------------------------------------------------
    // 12. Пример из существующих тестов (24-битный)
    // ------------------------------------------------------------------------
    SECTION("24-bit example with uint8_t words")
    {
        mylib::Bitset<std::uint8_t> bs{
                                       std::vector<std::uint8_t>{ 0xFF, 0xFF, 0xFF } };
        REQUIRE(bs.size() == 24);
        REQUIRE(bs.toString() == "111111111111111111111111");

        bs <<= 16;
        REQUIRE(bs.toString() == "111111110000000000000000");

        bs >>= 8;
        // "111111110000000000000000" >>= 8:
        // верхние 8 бит становятся 0, остальное сдвигается вниз
        REQUIRE(bs.toString() == "000000001111111100000000");
    }
}
