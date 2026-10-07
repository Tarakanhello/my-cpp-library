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
        size_t expectedPop = 0;
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
        size_t expected{ std::popcount(0b1010U) + std::popcount(0b11110000U) };
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




TEST_CASE("Bitset size modification (prepend, prepend, removeLast)", "[bitset][modifiers]")
{
    // ------------------------------------------------------------------------
    // 1. appendMSB(bool value)
    // ------------------------------------------------------------------------
    SECTION("prepend single bit")
    {
        // Пустой
        Bitset b;
        b.appendMSB(true);
        REQUIRE(b.size() == 1);
        REQUIRE(b.wordsSize() == 1);
        REQUIRE(b[0] == true);
        REQUIRE(b.popcount() == 1);
        REQUIRE(!b.isZero());

        b.appendMSB(false);
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
            b2.appendMSB(i % 2 == 0); // чередуем
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
        b2.appendMSB(true);
        REQUIRE(b2.size() == WORD_BITS + 1);
        REQUIRE(b2.wordsSize() == 2);
        REQUIRE(b2.lastWordBits() == 1);
        REQUIRE(b2.garbageBits() == WORD_BITS - 1);
        REQUIRE(b2[WORD_BITS] == true);
        REQUIRE(b2.popcount() == WORD_BITS / 2 + 1);
    }

    // ------------------------------------------------------------------------
    // 2. appendMSB(WORD value, size_t size)
    // ------------------------------------------------------------------------
    SECTION("prepend WORD value with specified number of bits")
    {
        Bitset b;

        // Добавляем 1 бит из значения
        b.appendMSB(0x1, 1);
        REQUIRE(b.size() == 1);
        REQUIRE(b[0] == true);
        REQUIRE(b.popcount() == 1);

        // Добавляем 3 бита из значения 0b100 (5) – берём младшие 3 бита: 100
        b.appendMSB(0b100, 3);
        REQUIRE(b.size() == 4);

        REQUIRE(b[0] == true);
        REQUIRE(b[1] == false);
        REQUIRE(b[2] == false);
        REQUIRE(b[3] == true);
        REQUIRE(b.popcount() == 2);;

        // Добавляем 0 бит
        b.appendMSB(0, 0);
        REQUIRE(b[0] == true);
        REQUIRE(b[1] == false);
        REQUIRE(b[2] == false);
        REQUIRE(b[3] == true);
        REQUIRE(b.popcount() == 2);;

        // Добавляем больше чем WORD_BITS – исключение
        REQUIRE_THROWS_AS(b.appendMSB(0, WORD_BITS + 1), std::out_of_range);

        // Добавляем максимальное количество бит (WORD_BITS) – должны быть взяты все биты значения
        Word val{ 0xFFFFFFFFFFFFFFFF };
        b.appendMSB(val, WORD_BITS);
        REQUIRE(b.size() == WORD_BITS + 4);
        REQUIRE(b.wordsSize() == 2);
        for(size_t i{ 4 }; i < WORD_BITS; ++i)
        {
            REQUIRE(b[i] == true);
        }

        REQUIRE(b.popcount() == WORD_BITS + 2);

        // Добавляем значение, у которого старшие биты за пределами size игнорируются
        Bitset b3;
        b3.appendMSB(0b1111, 2); // берём только младшие 2 бита (оба 1)
        REQUIRE(b3.size() == 2);
        REQUIRE(b3[0] == true);
        REQUIRE(b3[1] == true);
        REQUIRE(b3.popcount() == 2);
    }

    // ------------------------------------------------------------------------
    // 3. appendLSB(const Bitset& other)
    // ------------------------------------------------------------------------
    SECTION("prepend another Bitset")
    {
        // Простое добавление
        Bitset a{ "100" };
        Bitset b{ "01" };

        REQUIRE(a.equals("100"));
        a.appendLSB(b);
        REQUIRE(a.size() == 5);
        REQUIRE(a.equals("10001")); // 100 + 01 = 10001
        REQUIRE(b.size() == 2); // исходный не изменился
        REQUIRE(b.equals("01"));

        Bitset c{ "111" };
        Bitset empty;
        c.appendLSB(empty);
        REQUIRE(c.size() == 3);
        REQUIRE(c.equals("111"));
        REQUIRE(empty.size() == 0);

        // Self-append – должно работать (делается копия)
        Bitset d{ "10" };
        d.appendLSB(d);
        REQUIRE(d.size() == 4);
        d.equals("1010");

        // Добавление большого битсета
        Bitset big1(100);
        for(size_t i{}; i < big1.size(); i += 2)
        {
            big1.set(i);
        }

        Bitset bigCheck{ big1 };
        REQUIRE(big1 == bigCheck);

        Bitset big2(50);
        for(size_t i{}; i < big2.size(); i += 3)
        {
            big2.set(i);
        }

        size_t oldSize{ big1.size() };
        big1.appendLSB(big2);
        REQUIRE(big1.size() == oldSize + big2.size());

        // Проверяем, что добавленные биты соответствуют big2
        for(size_t i{}; i < big2.size(); ++i)
        {
            REQUIRE(big1[i] == big2[i]);
        }

        for(size_t i{ big2.size()}; i < big1.size(); ++i)
        {
            REQUIRE(big1[i] == bigCheck[i - big2.size()]);
        }
    }

    // ------------------------------------------------------------------------
    // 4. removeLast() и pop_back()
    // ------------------------------------------------------------------------
    SECTION("removeLast and pop_back")
    {
        // Создаём битсет с несколькими битами
        Bitset b{ "1101" };
        REQUIRE(b.size() == 4);

        b.removeMSB();
        REQUIRE(b.size() == 3);
        b.equals("101"); // 101

        b.popMSB(); // синоним removeLast
        REQUIRE(b.size() == 2);
        b.equals("01"); //
        // Удаляем до пустого состояния
        b.removeMSB();
        REQUIRE(b.size() == 1);
        b.equals("1"); //

        b.removeMSB();
        REQUIRE(b.size() == 0);
        REQUIRE(b.wordsSize() == 0);
        REQUIRE(b.isZero());

        // После удаления всех битов, пробуем добавить новые
        b.appendLSB(true);
        REQUIRE(b.size() == 1);
        REQUIRE(b[0] == true);


        // Удаление последнего бита, когда он находится не на границе слова
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
        // Проверим, что бит 97 не тронут
        big.set(97, true);
        REQUIRE(big[97] == true);

        // Проверка, что мусорные биты остаются нулевыми после удаления
        Bitset c(WORD_BITS + 5);
        c.set(WORD_BITS + 3, true); // устанавливаем бит во втором слове
        c.removeMSB(); // удаляем последний бит (индекс WORD_BITS+4) – он был нулевой
        REQUIRE(c.size() == WORD_BITS + 4);
        // Проверим, что бит WORD_BITS+3 всё ещё true
        REQUIRE(c[WORD_BITS + 3] == true);
        // Удалим ещё несколько, чтобы перейти границу слова
        for (int i = 0; i < 4; ++i)
            c.removeMSB();
        REQUIRE(c.size() == WORD_BITS);
        REQUIRE(c.wordsSize() == 1);
        // Бит WORD_BITS+3 теперь должен отсутствовать, но мы можем проверить, что последнее слово содержит только значимые биты, а мусорные – нули
        // Проверим, что бит 63 (последний в первом слове) не установлен
        REQUIRE(c[WORD_BITS - 1] == false);
        // Проверим через getData, что мусорные биты в последнем слове нулевые
        const Word* data = c.getData();
        // В последнем слове (единственном) должны быть нули, т.к. мы не устанавливали биты в первом слове
        REQUIRE(data[0] == 0);
    }
}

TEST_CASE("Bitset comparison operators", "[bitset][comparison]")
{
    // ------------------------------------------------------------------------
    // 1. operator== and operator!=
    // ------------------------------------------------------------------------
    SECTION("Equality and inequality")
    {
        // Одинаковые размеры и биты
        Bitset a{ "1010" };
        Bitset b{ "1010" };
        REQUIRE(a == b);
        REQUIRE_FALSE(a != b);

        // Одинаковые размеры, разные биты
        Bitset c{ "1011" };
        REQUIRE_FALSE(a == c);
        REQUIRE(a != c);

        // Разные размеры, но биты совпадают до меньшего размера
        Bitset d{ "10" };
        REQUIRE_FALSE(a == d);
        REQUIRE(a != d);

        // Пустые
        Bitset e1, e2;
        REQUIRE(e1 == e2);
        REQUIRE_FALSE(e1 != e2);

        // Пустой и непустой
        REQUIRE_FALSE(e1 == a);
        REQUIRE(e1 != a);

        // Самосравнение
        REQUIRE(a == a);
        REQUIRE_FALSE(a != a);

        // Все нули vs все единицы (одинаковый размер)
        Bitset zeros(10);
        Bitset ones(10);
        ones.setAll(true);
        REQUIRE_FALSE(zeros == ones);
        REQUIRE(zeros != ones);

        // Более сложный случай: разные слова, но одинаковый размер
        Bitset big1(std::vector<Word>{0x1234567890ABCDEFull, 0xFEDCBA9876543210ull});
        Bitset big2(std::vector<Word>{0x1234567890ABCDEFull, 0xFEDCBA9876543210ull});
        REQUIRE(big1 == big2);

        Bitset big3(std::vector<Word>{0x1234567890ABCDEFull, 0xFEDCBA9876543211ull});
        REQUIRE(big1 != big3);

        Bitset bs1(1);
        Bitset bs2(2);
        bs1.set(0);
        bs2.set(0);

        REQUIRE_FALSE(bs1 == bs2);
        REQUIRE_FALSE(bs1 == bs2);

        mylib::Bitset<std::uint8_t> bs{ std::vector<std::uint8_t>{ 0xFF, 0xFF, 0xFF } };
        REQUIRE(bs.size() == 24);
        REQUIRE(bs.toString() == "111111111111111111111111");

        bs <<= 16;
        REQUIRE(bs.toString() == "111111110000000000000000");

    }

    // ------------------------------------------------------------------------
    // 2. Three-way comparison (operator<=>) and generated relational operators
    // ------------------------------------------------------------------------
    SECTION("Three-way comparison and relational operators")
    {
        // 2.1 Сравнение по размеру: меньший размер всегда меньше
        Bitset small{ "111" };   // size 3
        Bitset large{ "0000" };  // size 4
        REQUIRE(small < large);
        REQUIRE(small <= large);
        REQUIRE(large > small);
        REQUIRE(large >= small);
        REQUIRE_FALSE(small > large);
        REQUIRE_FALSE(small >= large);
        REQUIRE_FALSE(large < small);
        REQUIRE_FALSE(large <= small);

        // 2.2 Одинаковый размер, лексикографическое сравнение по словам (от младшего слова к старшему)
        // Сравнение происходит сначала по первому слову (младшие биты), затем по второму и т.д.
        // Пример: size 2, a = 01 (bit0=1, bit1=0) => значение 1
        //          b = 10 (bit0=0, bit1=1) => значение 2
        Bitset a{ "01" }; // bit0=1, bit1=0
        Bitset b{ "10" }; // bit0=0, bit1=1
        REQUIRE(a < b);
        REQUIRE(a <= b);
        REQUIRE(b > a);
        REQUIRE(b >= a);

        // Пример: a = 11 (3), b = 10 (2)
        Bitset c{ "11" };
        Bitset d{ "10" };
        REQUIRE(c > d);
        REQUIRE(c >= d);
        REQUIRE(d < c);
        REQUIRE(d <= c);

        // Равенство
        REQUIRE(a == a);
        REQUIRE(a <= a);
        REQUIRE(a >= a);
        REQUIRE_FALSE(a < a);
        REQUIRE_FALSE(a > a);

        // 2.3 Сравнение на границе слов
        // Создадим два битсета размера WORD_BITS + 1
        Bitset big1(WORD_BITS + 1);
        Bitset big2(WORD_BITS + 1);
        // Установим бит в первом слове (младшее слово) у big1, а у big2 - во втором слове
        big1.set(0, true);               // первое слово = 1
        big2.set(WORD_BITS, true);       // второе слово = 1
        // Сравниваем: сначала первое слово: big1=1, big2=0 => big1 > big2
        REQUIRE(big1 < big2);
        REQUIRE(big1 <= big2);
        REQUIRE(big2 > big1);
        REQUIRE(big2 >= big1);

        // Теперь сделаем big2 с тем же первым словом, но большим вторым
        Bitset big3(WORD_BITS + 1);
        big3.set(0, true);               // первое слово = 1
        big3.set(WORD_BITS, true);       // второе слово = 1
        // Сравнение big1 (0,1) и big3 (1,1): первое слово 0 < 1 => big1 < big3
        REQUIRE(big1 < big3);
        REQUIRE(big3 > big1);

        // 2.4 Пустые битсеты
        Bitset empty1, empty2;
        REQUIRE(empty1 == empty2);
        REQUIRE(empty1 <= empty2);
        REQUIRE(empty1 >= empty2);
        REQUIRE_FALSE(empty1 < empty2);
        REQUIRE_FALSE(empty1 > empty2);

        // Пустой и непустой
        REQUIRE(empty1 < a);   // a имеет размер 2 > 0
        REQUIRE(a > empty1);
        REQUIRE(empty1 <= a);
        REQUIRE(a >= empty1);

        // 2.5 Большие битсеты, сравнение по словам
        // Создадим три битсета размера 2 * WORD_BITS
        Bitset x(2 * WORD_BITS);
        Bitset y(2 * WORD_BITS);
        Bitset z(2 * WORD_BITS);

        // x: первое слово = 5, второе = 10
        x.setValue(5, 0, WORD_BITS);
        x.setValue(10, WORD_BITS, WORD_BITS);

        // y: первое слово = 5, второе = 20
        y.setValue(5, 0, WORD_BITS);
        y.setValue(20, WORD_BITS, WORD_BITS);

        // z: первое слово = 6, второе = 0
        z.setValue(6, 0, WORD_BITS);
        z.setValue(0, WORD_BITS, WORD_BITS);

        // Сравнение: x и y: первые слова равны (5), вторые: 10 < 20 => x < y
        REQUIRE(x < y);
        REQUIRE(x <= y);
        REQUIRE(y > x);
        REQUIRE(y >= x);

        // Сравнение x и z:  старшие слова 10 > 0 ⇒ x > z (несмотря на младшие)
        REQUIRE(x > z);
        REQUIRE(z < x);

        // Сравнение y и z: старшие слова 20 > 0 ⇒ y > z
        REQUIRE(y > z);
        REQUIRE(z < y);
    }

    // ------------------------------------------------------------------------
    // 3. Дополнительные проверки для оператора <=>
    // ------------------------------------------------------------------------
    SECTION("Direct use of spaceship operator")
    {
        Bitset a{ "101" };
        Bitset b{ "110" };
        Bitset c{ "1010" };

        // Прямое использование <=>
        REQUIRE((a <=> b) < 0);
        REQUIRE((a <=> b) <= 0);
        REQUIRE_FALSE((a <=> b) > 0);
        REQUIRE_FALSE((a <=> b) >= 0);
        REQUIRE((a <=> a) == 0);
        REQUIRE((a <=> c) < 0);   // размер 3 < 4
        REQUIRE((c <=> a) > 0);
    }

    // ------------------------------------------------------------------------
    // 4. Сравнение с мусорными битами (убедиться, что они не влияют)
    // ------------------------------------------------------------------------
    SECTION("Garbage bits do not affect comparison")
    {
        // Создаём битсет с размером, не кратным слову
        Bitset a(WORD_BITS + 5);
        Bitset b(WORD_BITS + 5);

        // Устанавливаем одинаковые значимые биты
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
