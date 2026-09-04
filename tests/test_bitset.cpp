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

        // Утилита для проверки, что все биты битсета равны заданному значению
    template<typename Bitset>
    void checkBitsViaIndex(const Bitset& bs, const std::initializer_list<bool>& expected)
    {
        REQUIRE(bs.size() == expected.size());
        size_t i = 0;
        for(bool val : expected)
        {
            REQUIRE(bs[i] == val);   // используется константный operator[]
            ++i;
        }
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

        // размер 1
        Bitset b1{ 1 };
        REQUIRE(b1.size() == 1);
        REQUIRE(b1.wordsSize() == 1);
        REQUIRE(b1.lastWordBits() == 1);
        REQUIRE(b1.garbageBits() == WORD_BITS - 1);
        REQUIRE(b1.isZero());
        REQUIRE(b1.popcount() == 0);

        // размер ровно одно слово
        Bitset bfull{ WORD_BITS };
        REQUIRE(bfull.size() == WORD_BITS);
        REQUIRE(bfull.wordsSize() == 1);
        REQUIRE(bfull.lastWordBits() == WORD_BITS);
        REQUIRE(bfull.garbageBits() == 0);
        REQUIRE(bfull.isZero());

        // размер одно слово + 1 бит
        Bitset bfull1{ WORD_BITS + 1 };
        REQUIRE(bfull1.size() == WORD_BITS + 1);
        REQUIRE(bfull1.wordsSize() == 2);
        REQUIRE(bfull1.lastWordBits() == 1);
        REQUIRE(bfull1.garbageBits() == WORD_BITS - 1);
        REQUIRE(bfull1.isZero());

        // размер ровно два слова
        Bitset b2full{ 2 * WORD_BITS };
        REQUIRE(b2full.size() == 2 * WORD_BITS);
        REQUIRE(b2full.wordsSize() == 2);
        REQUIRE(b2full.lastWordBits() == WORD_BITS);
        REQUIRE(b2full.garbageBits() == 0);
        REQUIRE(b2full.isZero());

        // произвольный размер (например, 100 бит)
        const size_t sz{ 100 };
        Bitset b100{ sz };
        REQUIRE(b100.size() == sz);
        size_t expectedWords{ (sz + WORD_BITS - 1) / WORD_BITS };
        REQUIRE(b100.wordsSize() == expectedWords);
        size_t lastBits{ sz % WORD_BITS };
        if(lastBits == 0)
        {
            lastBits = WORD_BITS;
        }
        REQUIRE(b100.lastWordBits() == lastBits);
        REQUIRE(b100.garbageBits() == WORD_BITS - lastBits);
        REQUIRE(b100.isZero());
        REQUIRE(b100.popcount() == 0);
    }

    // ------------------------------------------------------------------------
    // 3. Конструктор из контейнера WORD
    // ------------------------------------------------------------------------
    SECTION("Constructor from container of WORDs")
    {
        // 3.1 Обычный вектор с двумя словами
        std::vector<Word> vec { 0x1234567890ABCDEFull, 0xFEDCBA9876543210ull };
        Bitset b{ vec };
        REQUIRE(b.size() == vec.size() * WORD_BITS);
        REQUIRE(b.wordsSize() == vec.size());
        const Word* data{ b.getData() };
        REQUIRE(data != nullptr);
        for(size_t i{ 0 }; i < vec.size(); ++i)
        {
            REQUIRE(data[i] == vec[i]);
        }
        // последнее слово полное → garbage = 0
        REQUIRE(b.lastWordBits() == WORD_BITS);
        REQUIRE(b.garbageBits() == 0);
        REQUIRE(b.isZero() == false);
        REQUIRE(b.operator bool() == true);

        // popcount
        int expectedPop = 0;
        for(Word w : vec)
        {
            expectedPop += std::popcount(w);    // C++20
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

        // 3.3 Пустой контейнер
        std::vector<Word> empty;
        Bitset bEmpty{ empty };
        REQUIRE(bEmpty.size() == 0);
        REQUIRE(bEmpty.wordsSize() == 0);
        REQUIRE(bEmpty.getData() == nullptr);
        REQUIRE(bEmpty.isZero());
        REQUIRE(bEmpty.garbageBits() == 0);

        // 3.4 Контейнер другого типа (std::array)
        std::array<Word, 2> arr{ 0x1111, 0x2222 };
        Bitset bArr{ arr };
        REQUIRE(bArr.size() == 2 * WORD_BITS);
        REQUIRE(bArr.wordsSize() == 2);
        const Word* dataArr{ bArr.getData() };
        REQUIRE(dataArr[0] == 0x1111);
        REQUIRE(dataArr[1] == 0x2222);
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
        // Проверяем, что данные совпадают
        for(size_t i{ 0 }; i < container.size(); ++i)
        {
            REQUIRE(container[i] == vec[i]);
        }
        // getData() должен указывать на те же данные
        REQUIRE(container.data() == b.getData());
    }

    // ------------------------------------------------------------------------
    // 5. isZero() и operator bool
    // ------------------------------------------------------------------------
    SECTION("isZero and operator bool")
    {
        Bitset b1;                // пустой
        REQUIRE(b1.isZero());
        REQUIRE(!b1.operator bool());

        Bitset b2{ 5 };             // нулевой
        REQUIRE(b2.isZero());
        REQUIRE(!b2.operator bool());

        Bitset b3{ std::vector<Word>{ 1 } };   // ненулевой
        REQUIRE(!b3.isZero());
        REQUIRE(b3.operator bool());

        Bitset b4{ 10 };
        b4.set(0, true);          // устанавливаем бит
        REQUIRE(!b4.isZero());
        REQUIRE(b4.operator bool());
    }

    // ------------------------------------------------------------------------
    // 6. popcount() – подсчёт установленных бит
    // ------------------------------------------------------------------------
    SECTION("popcount")
    {
        Bitset b1;                // пустой
        REQUIRE(b1.popcount() == 0);

        Bitset b2{ 100 };           // все нули
        REQUIRE(b2.popcount() == 0);

        // создаём с известными словами
        std::vector<Word> vec{ 0b1010, 0b11110000 };
        Bitset b3(vec);
        int expected{ std::popcount(0b1010U) + std::popcount(0b11110000U) };
        REQUIRE(b3.popcount() == expected);

        // после установки битов
        Bitset b4{ 64 };
        b4.set(0, true);
        REQUIRE(b4.popcount() == 1);
        b4.set(63, true);
        REQUIRE(b4.popcount() == 2);
        b4.set(0, false);        // сбрасываем
        REQUIRE(b4.popcount() == 1);
    }

    // ------------------------------------------------------------------------
    // 7. lastWordBits() и garbageBits() – детальные проверки
    // ------------------------------------------------------------------------
    SECTION("lastWordBits and garbageBits edge cases")
    {
        // для пустого garbageBits = 0, lastWordBits не вызываем
        Bitset b0{ 0 };
        REQUIRE(b0.garbageBits() == 0);

        // размер 1
        Bitset b1{ 1 };
        REQUIRE(b1.lastWordBits() == 1);
        REQUIRE(b1.garbageBits() == WORD_BITS - 1);

        // размер ровно WORD_BITS
        Bitset bfull{ WORD_BITS };
        REQUIRE(bfull.lastWordBits() == WORD_BITS);
        REQUIRE(bfull.garbageBits() == 0);

        // размер WORD_BITS + 1
        Bitset bfull1{ WORD_BITS + 1 };
        REQUIRE(bfull1.lastWordBits() == 1);
        REQUIRE(bfull1.garbageBits() == WORD_BITS - 1);

        // размер 2 * WORD_BITS
        Bitset b2full{ 2 * WORD_BITS };
        REQUIRE(b2full.lastWordBits() == WORD_BITS);
        REQUIRE(b2full.garbageBits() == 0);
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
        // Изначально все нули
        for (size_t i{ 0 }; i < 10; ++i)
        {
            REQUIRE(b[i] == false);   // operator[] const
        }

        // Запись через BitReference
        b[0] = true;
        b[3] = true;
        b[9] = true;
        REQUIRE(b[0] == true);
        REQUIRE(b[3] == true);
        REQUIRE(b[9] == true);
        REQUIRE(b[1] == false);
        REQUIRE(b[2] == false);

        // Изменение через ссылку
        auto ref{ b[0] };
        ref = false;
        REQUIRE(b[0] == false);
        ref = true;
        REQUIRE(b[0] == true);

        // Присваивание BitReference от другого BitReference
        auto ref2{ b[9] };
        ref2 = b[1];   // копирование значения бита 1 (false)
        REQUIRE(b[9] == false);
        ref2 = true;  // меняем обратно

        // Проверка, что изменения отражаются в getData
        const Word* data{ b.getData() };
        REQUIRE(data != nullptr);
        // Первое слово должно иметь биты 0 и 3 установлены
        // Ожидаем: бит 0=1, бит 3=1, бит 9=1
        Word expected{ (1ULL << 0) | (1ULL << 3) | (1ULL << 9) };
        REQUIRE(data[0] == expected);

        // isZero должно быть false
        REQUIRE(!b.isZero());

        // Выход за границы
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

        // Выход за границы
        REQUIRE_THROWS_AS(cb[5], std::out_of_range);
        REQUIRE_THROWS_AS(cb[10], std::out_of_range);
    }

    // ------------------------------------------------------------------------
    // 3. set(size_t i, bool value) – установка бита
    // ------------------------------------------------------------------------
    SECTION("set() sets bit to given value")
    {
        Bitset b{ 8 };
        // Установка по умолчанию (true)
        b.set(1);
        b.set(5);
        REQUIRE(b[1] == true);
        REQUIRE(b[5] == true);
        REQUIRE(b[0] == false);

        // Установка false
        b.set(1, false);
        REQUIRE(b[1] == false);
        b.set(5, true);
        REQUIRE(b[5] == true);

        // Граничные индексы
        b.set(0, true);
        b.set(7, true);
        REQUIRE(b[0] == true);
        REQUIRE(b[7] == true);

        // Выход за границы
        REQUIRE_THROWS_AS(b.set(8, true), std::out_of_range);
        REQUIRE_THROWS_AS(b.set(100), std::out_of_range);

        // Проверка на большом битсете (несколько слов)
        Bitset big{ 100 };
        big.set(63, true);   // последний бит первого слова
        big.set(64, true);   // первый бит второго слова
        REQUIRE(big[63] == true);
        REQUIRE(big[64] == true);
        REQUIRE(big[62] == false);
        REQUIRE(big[65] == false);
    }

    // ------------------------------------------------------------------------
    // 4. BitReference – детальное тестирование публичного прокси-класса
    // ------------------------------------------------------------------------
    SECTION("BitReference construction and operations")
    {
        // Создаём Bitset с одним словом, чтобы получить указатель на слово
        Bitset b{ WORD_BITS };
        Word* ptr{ const_cast<Word*>(b.getData()) }; // неконстантный указатель (мы можем изменять)

        // 4.1 Создание BitReference с корректным offset
        {
            mylib::Bitset<Word>::BitReference ref{ ptr, 0 };
            REQUIRE(ref == false); // бит изначально 0
            ref = true;
            REQUIRE(ref == true);
            REQUIRE(b[0] == true);

            // Другой offset
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
            // Копируем refA в refB
            refB = refA;
            REQUIRE(refB == true);
            REQUIRE(b[10] == true);
            // Проверяем, что refA не изменился
            REQUIRE(refA == true);
        }

        // 4.3 Исключение при offset >= WORD_BITS
        {
            // offset = WORD_BITS (равен numberOfDigits) -> должно выбросить
            REQUIRE_THROWS_AS((mylib::Bitset<Word>::BitReference(ptr, WORD_BITS)), std::out_of_range);
            // offset = WORD_BITS+1
            REQUIRE_THROWS_AS((mylib::Bitset<Word>::BitReference(ptr, WORD_BITS + 1)), std::out_of_range);
        }

        // 4.4 Проверка, что BitReference работает даже при изменении слова через другие операции
        {
            mylib::Bitset<Word>::BitReference ref{ ptr, 20 };
            ref = true;
            REQUIRE(b[20] == true);
            // Очищаем всё через clear()
            b.clear();
            REQUIRE(ref == false); // бит стал нулевым
            // ref всё ещё указывает на то же место, но теперь бит сброшен
            REQUIRE(b[20] == false);
        }
    }
}




TEST_CASE("Bitset size modification (append, push_back, removeLast)", "[bitset][modifiers]")
{
    // ------------------------------------------------------------------------
    // 1. append(bool value)
    // ------------------------------------------------------------------------
    SECTION("append single bit")
    {
        // Пустой
        Bitset b;
        b.append(true);
        REQUIRE(b.size() == 1);
        REQUIRE(b.wordsSize() == 1);
        REQUIRE(b[0] == true);
        REQUIRE(b.popcount() == 1);
        REQUIRE(!b.isZero());

        b.append(false);
        REQUIRE(b.size() == 2);
        REQUIRE(b.wordsSize() == 1);
        REQUIRE(b[0] == true);
        REQUIRE(b[1] == false);
        REQUIRE(b.popcount() == 1);
        REQUIRE(!b.isZero());

        // Добавляем много бит, чтобы перейти через границу слова
        Bitset b2;
        for(size_t i{ 0 }; i < WORD_BITS; ++i)
        {
            b2.append(i % 2 == 0); // чередуем
        }
        REQUIRE(b2.size() == WORD_BITS);
        REQUIRE(b2.wordsSize() == 1);
        REQUIRE(b2.lastWordBits() == WORD_BITS);
        REQUIRE(b2.garbageBits() == 0);
        // Проверяем значения
        for(size_t i{ 0 }; i < WORD_BITS; ++i)
        {
            REQUIRE(b2[i] == (i % 2 == 0));
        }
        REQUIRE(b2.popcount() == WORD_BITS / 2);

        // Добавляем ещё один бит – создаётся новое слово
        b2.append(true);
        REQUIRE(b2.size() == WORD_BITS + 1);
        REQUIRE(b2.wordsSize() == 2);
        REQUIRE(b2.lastWordBits() == 1);
        REQUIRE(b2.garbageBits() == WORD_BITS - 1);
        REQUIRE(b2[WORD_BITS] == true);
        REQUIRE(b2.popcount() == WORD_BITS / 2 + 1);
    }

    // ------------------------------------------------------------------------
    // 2. push_back(WORD value, size_t size)
    // ------------------------------------------------------------------------
    SECTION("push_back WORD value with specified number of bits")
    {
        Bitset b;

        // Добавляем 1 бит из значения
        b.push_back(0x1, 1);
        REQUIRE(b.size() == 1);
        REQUIRE(b[0] == true);
        REQUIRE(b.popcount() == 1);

        // Добавляем 3 бита из значения 0b101 (5) – берём младшие 3 бита: 101
        b.push_back(0b101, 3);
        REQUIRE(b.size() == 4);

        REQUIRE(b[0] == true);
        REQUIRE(b[1] == true);
        REQUIRE(b[2] == false);
        REQUIRE(b[3] == true);
        REQUIRE(b.popcount() == 3);;

        // Добавляем 0 бит
        b.push_back(0, 0);
        REQUIRE(b[0] == true);
        REQUIRE(b[1] == true);
        REQUIRE(b[2] == false);
        REQUIRE(b[3] == true);
        REQUIRE(b.popcount() == 3);;

        // Добавляем больше чем WORD_BITS – исключение
        REQUIRE_THROWS_AS(b.push_back(0, WORD_BITS + 1), std::out_of_range);

        // Добавляем максимальное количество бит (WORD_BITS) – должны быть взяты все биты значения
        Word val{ 0xFFFFFFFFFFFFFFFF };
        b.push_back(val, WORD_BITS);
        REQUIRE(b.size() == WORD_BITS + 4);
        REQUIRE(b.wordsSize() == 2);
        for(size_t i{ 4 }; i < WORD_BITS; ++i)
        {
            REQUIRE(b[i] == true);
        }

        REQUIRE(b.popcount() == WORD_BITS + 3);

        // Добавляем значение, у которого старшие биты за пределами size игнорируются
        Bitset b3;
        b3.push_back(0b1111, 2); // берём только младшие 2 бита (оба 1)
        REQUIRE(b3.size() == 2);
        REQUIRE(b3[0] == true);
        REQUIRE(b3[1] == true);
        REQUIRE(b3.popcount() == 2);
    }

    // ------------------------------------------------------------------------
    // 3. push_back(const Bitset& other)
    // ------------------------------------------------------------------------
    SECTION("push_back another Bitset")
    {
        // Простое добавление
        Bitset a{ "101" };
        Bitset b{ "01" };
        a.push_back(b);
        REQUIRE(a.size() == 5);
        checkBits(a, {true, false, true, false, true}); // 101 + 01 = 10101
        REQUIRE(b.size() == 2); // исходный не изменился
        checkBits(b, {false, true});
    }
}
