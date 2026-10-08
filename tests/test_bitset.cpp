#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <array>
#include <bit>
#include <cstdint>
#include <numeric>
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
