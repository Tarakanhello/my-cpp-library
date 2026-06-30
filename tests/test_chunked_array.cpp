#include <catch2/catch_all.hpp>

#include <array>
#include <algorithm>
#include <list>
#include <numeric>
#include <string>
#include <vector>

#include "mylib/mylib.h"

namespace
{

    // Вспомогательная функция для проверки содержимого ChunkedArray
    template<typename T, size_t CHUNK, typename ALLOC>
    void checkChunkedArray(const mylib::ChunkedArray<T, CHUNK, ALLOC>& arr,
                           const std::initializer_list<T>& expected)
    {
        REQUIRE(arr.size() == expected.size());
        auto it{ expected.begin() };

        for(size_t i = 0; i < expected.size(); ++i, ++it)
        {
            REQUIRE(arr[i] == *it);
        }
    }

    // Структура для проверки нетривиальных типов
    struct TestStruct
    {
        int a;
        std::string s;
        TestStruct(int theA = 0, const std::string& theS = "")
            : a{ theA }
            , s{ theS }
        {}

        auto operator<=>(const TestStruct&) const = default;
    };



    template<typename T>
    struct CountingAllocator : mylib::MySimpleAllocator<T>
    {
        static inline size_t allocateCount = 0;
        static inline size_t deallocateCount = 0;

        T* allocate(size_t size)
        {
            ++allocateCount;
            return mylib::MySimpleAllocator<T>::allocate(size);
        }

        void deallocate(T* array, size_t size)
        {
            ++deallocateCount;
            mylib::MySimpleAllocator<T>::deallocate(array, size);
        }

        static void reset()
        {
            allocateCount = 0;
            deallocateCount = 0;
        }
    };

    constexpr const size_t defaultChunk{ 32 };


    struct ThrowingCopy
    {
        int value;
        static int copyCounter;
        static int throwAfter;

        ThrowingCopy(int v = 0) : value(v) {}

        ThrowingCopy(const ThrowingCopy& other) : value(other.value)
        {
            ++copyCounter;
            if (copyCounter >= throwAfter && throwAfter >= 0)
            {
                throw std::runtime_error("copy failed");
            }
        }

        ThrowingCopy(ThrowingCopy&& other) noexcept : value(other.value) {}

        ThrowingCopy& operator=(const ThrowingCopy& other)
        {
            ++copyCounter;
            if (copyCounter >= throwAfter && throwAfter >= 0)
            {
                throw std::runtime_error("copy failed");
            }
            value = other.value;
            return *this;
        }

        ThrowingCopy& operator=(ThrowingCopy&& other) noexcept
        {
            value = other.value;
            return *this;
        }

        static void reset(int throwAt = -1)
        {
            copyCounter = 0;
            throwAfter = throwAt;
        }

        auto operator<=>(const ThrowingCopy&) const = default;
    };

    int ThrowingCopy::copyCounter = 0;
    int ThrowingCopy::throwAfter = -1;

} // end namespace



TEST_CASE("ChunkedArray construction and basic properties", "[chunked_array][construction]")
{
    using ChunkedInt = mylib::ChunkedArray<int>;

    SECTION("1.1 Default constructor")
    {
        ChunkedInt arr;
        REQUIRE(arr.size() == 0);
        REQUIRE(arr.empty());
        REQUIRE(arr.blockCount() == 0);
        REQUIRE(!arr.operator bool()); // explicit operator bool
    }

    SECTION("1.2 Constructor with size and value")
    {
        const size_t size = 5;
        ChunkedInt arr(size, 42);
        REQUIRE(arr.size() == size);
        for(size_t i = 0; i < size; ++i)
        {
            REQUIRE(arr[i] == 42);
        }

        size_t expectedBlocks{ (size + defaultChunk - 1) / defaultChunk };
        REQUIRE(arr.blockCount() == expectedBlocks);

        // Проверка size == 0
        ChunkedInt empty(0, 100);
        REQUIRE(empty.size() == 0);
        REQUIRE(empty.empty());
        REQUIRE(empty.blockCount() == 0);
    }

    SECTION("1.3 Constructor from initializer_list")
    {
        ChunkedInt arr{ 1, 2, 3, 4, 5 };
        checkChunkedArray(arr, { 1, 2, 3, 4, 5 });
        REQUIRE(arr.blockCount() == 1); // 5 <= 32

        // Больше одного блока
        std::initializer_list<int> many{  1,  2,  3,  4,
                                          5,  6,  7,  8,
                                          9, 10, 11, 12,
                                         13, 14, 15, 16,
                                         17, 18, 19, 20,
                                         21, 22, 23, 24,
                                         25, 26, 27, 28,
                                         29, 30, 31, 32, 33};
        ChunkedInt large{ many };
        REQUIRE(large.size() == 33);
        REQUIRE(large.blockCount() == 2);
        // Проверим последний элемент
        REQUIRE(large[32] == 33);
    }

    SECTION("1.4 Constructor from arbitrary container")
    {
        // Используем std::vector
        std::vector<int> vec{ 10, 20, 30, 40 };
        ChunkedInt fromVec{ vec };

        checkChunkedArray(fromVec, { 10, 20, 30, 40 });

        REQUIRE(fromVec.blockCount() == 1);

        // Используем std::list
        std::list<int> lst{ 5, 6, 7 };
        ChunkedInt fromList{ lst };
        checkChunkedArray(fromList, { 5, 6, 7 });

        // Используем std::array
        std::array<int, 4> arrData{ 1, 2, 3, 4 };
        ChunkedInt fromArray(arrData);
        checkChunkedArray(fromArray, { 1, 2, 3, 4 });

        // Проверим, что конструктор не конфликтует с копирующим (передаём другой тип)
        // Убедимся, что компилируется – если бы конфликтовал, была бы ошибка.
        // Дополнительно проверим, что конструктор не вызывается для копирования самого ChunkedArray
        ChunkedInt original{ 9, 8, 7 };
        ChunkedInt copy{ original }; // должен вызваться копирующий конструктор
        checkChunkedArray(copy, { 9, 8, 7 });
    }

    SECTION("1.5 Copy constructor")
    {
        ChunkedInt original{  1,  2,  3,  4,
                              5,  6,  7,  8,
                              9, 10, 11, 12,
                             13, 14, 15, 16,
                             17, 18, 19, 20,
                             21, 22, 23, 24,
                             25, 26, 27, 28,
                             29, 30, 31, 32, 33};
        ChunkedInt copy{ original };
        // Проверяем содержимое
        for(size_t i{}; i < original.size(); ++i)
        {
            REQUIRE(copy[i] == original[i]);
        }
        REQUIRE(copy.size() == original.size());
        REQUIRE(copy.blockCount() == original.blockCount());
        // Оригинал не изменился
        REQUIRE(original.size() == 33);
        REQUIRE(original.blockCount() == 2);
    }

    SECTION("1.6 Move constructor")
    {
        ChunkedInt source{ 1, 2, 3, 4, 5 };
        size_t oldSize{ source.size() };
        size_t oldBlocks{ source.blockCount() };

        ChunkedInt target{ std::move(source) };
        REQUIRE(target.size() == oldSize);
        REQUIRE(target.blockCount() == oldBlocks);
        checkChunkedArray(target, { 1, 2, 3, 4, 5 });

        // Source должен быть пустым
        REQUIRE(source.size() == 0);
        REQUIRE(source.empty());
        REQUIRE(source.blockCount() == 0);
    }

    SECTION("1.7 Copy and move assignment operators")
    {
        ChunkedInt a{ 1, 2, 3 };
        ChunkedInt b{ 4, 5, 6, 7 };

        // Copy assignment
        a = b;
        checkChunkedArray(a, { 4, 5, 6, 7 });
        // b не изменился
        checkChunkedArray(b, { 4, 5, 6, 7 });

        // Self-assignment copy
        a = a; // должно работать корректно
        checkChunkedArray(a, { 4, 5, 6, 7 });

        // Move assignment
        ChunkedInt c{ 10, 20 };
        c = std::move(a);
        checkChunkedArray(c, { 4, 5, 6, 7 });
        // a должен быть пустым
        REQUIRE(a.size() == 0);
        REQUIRE(a.empty());
        REQUIRE(a.blockCount() == 0);

        // Self-assignment move
        c = std::move(c); // должно работать корректно
        checkChunkedArray(c, { 4, 5, 6, 7 });
    }

    SECTION("1.8 Methods size(), empty(), blockCount(), operator bool")
    {
        ChunkedInt empty;
        REQUIRE(empty.size() == 0);
        REQUIRE(empty.empty());
        REQUIRE(empty.blockCount() == 0);
        REQUIRE(!empty.operator bool());

        ChunkedInt nonEmpty{ 1, 2, 3 };
        REQUIRE(nonEmpty.size() == 3);
        REQUIRE(!nonEmpty.empty());
        REQUIRE(nonEmpty.blockCount() == 1);
        REQUIRE(nonEmpty.operator bool());
    }
}



TEST_CASE("ChunkedArray element access", "[chunked_array][access]")
{
    using ChunkedInt = mylib::ChunkedArray<int>;

    SECTION("operator[] non-const")
    {
        ChunkedInt arr{ 10, 20, 30, 40, 50 };
        REQUIRE(arr[0] == 10);
        REQUIRE(arr[2] == 30);
        arr[1] = 99;
        REQUIRE(arr[1] == 99);
        // Проверка на последнем элементе блока и переходе через границу
        ChunkedInt large(33, 0); // 33 элемента, 2 блока
        large[31] = 777;
        large[32] = 888;
        REQUIRE(large[31] == 777);
        REQUIRE(large[32] == 888);
    }

    SECTION("operator[] const")
    {
        const ChunkedInt arr{ 1, 2, 3 };
        REQUIRE(arr[0] == 1);
        REQUIRE(arr[2] == 3);
        // arr[1] = 5; // ошибка компиляции
    }

    SECTION("at() valid indices")
    {
        ChunkedInt arr{ 5, 6, 7, 8 };
        REQUIRE(arr.at(0) == 5);
        REQUIRE(arr.at(2) == 7);
        arr.at(1) = 42;
        REQUIRE(arr.at(1) == 42);

        const ChunkedInt carr{ 9, 10, 11 };
        REQUIRE(carr.at(2) == 11);
    }

    SECTION("at() out of range throws")
    {
        ChunkedInt arr{ 1, 2, 3 };
        REQUIRE_THROWS_AS(arr.at(3), std::out_of_range);
        REQUIRE_THROWS_AS(arr.at(100), std::out_of_range);
        REQUIRE_THROWS_AS(arr.at(arr.size()), std::out_of_range);
        REQUIRE_THROWS_AS(arr.at(-1), std::out_of_range);

        const ChunkedInt carr{ 4, 5 };
        REQUIRE_THROWS_AS(carr.at(2), std::out_of_range);
    }

    SECTION("at() on empty throws")
    {
        ChunkedInt empty;
        REQUIRE_THROWS_AS(empty.at(0), std::out_of_range);
    }

    SECTION("front() and back() non-const")
    {
        ChunkedInt arr{ 10, 20, 30, 40 };
        REQUIRE(arr.front() == 10);
        REQUIRE(arr.back() == 40);
        arr.front() = 100;
        arr.back() = 400;
        REQUIRE(arr[0] == 100);
        REQUIRE(arr[3] == 400);
    }

    SECTION("front() and back() const")
    {
        const ChunkedInt arr{ 1, 2, 3, 4 };
        REQUIRE(arr.front() == 1);
        REQUIRE(arr.back() == 4);
    }
}



TEST_CASE("ChunkedArray insertion (push_back, emplace_back)", "[chunked_array][insertion]")
{
    using ChunkedInt = mylib::ChunkedArray<int>;
    using ChunkedTest = mylib::ChunkedArray<TestStruct>;

    SECTION("push_back lvalue")
    {
        ChunkedInt arr;
        int x{ 5 };
        arr.push_back(x);
        arr.push_back(x + 1);
        REQUIRE(arr.size() == 2);
        checkChunkedArray(arr, { 5, 6 });
    }

    SECTION("push_back rvalue")
    {
        ChunkedInt arr;
        arr.push_back(10);
        arr.push_back(20);
        checkChunkedArray(arr, { 10, 20 });

        // Строки
        mylib::ChunkedArray<std::string> sarr;
        std::string s{ "hello" };
        sarr.push_back(s);               // lvalue
        sarr.push_back(std::string{ "world" }); // rvalue
        REQUIRE(sarr.size() == 2);
        REQUIRE(sarr[0] == "hello");
        REQUIRE(sarr[1] == "world");
    }

    SECTION("emplace_back constructs in-place")
    {
        ChunkedTest arr;
        arr.emplace_back(1, "one");
        arr.emplace_back(2, "two");
        REQUIRE(arr.size() == 2);
        REQUIRE(arr[0].a == 1);
        REQUIRE(arr[0].s == "one");
        REQUIRE(arr[1].a == 2);
        REQUIRE(arr[1].s == "two");

        // Для int
        ChunkedInt iarr;
        iarr.emplace_back(42);
        REQUIRE(iarr[0] == 42);
    }

    SECTION("Push/emplace triggering new chunks")
    {
        const size_t CHUNK{ 32 };
        ChunkedInt arr;
        // Заполняем первый блок полностью
        for(size_t i{}; i < CHUNK; ++i)
        {
            arr.push_back(static_cast<int>(i));
        }
        REQUIRE(arr.size() == CHUNK);
        REQUIRE(arr.blockCount() == 1);
        // Добавляем ещё один элемент – должен появиться новый блок
        arr.push_back(100);
        REQUIRE(arr.size() == CHUNK + 1);
        REQUIRE(arr.blockCount() == 2);
        REQUIRE(arr[CHUNK] == 100);

        // Проверяем, что элементы в первом блоке не повреждены
        for(size_t i{}; i < CHUNK; ++i)
        {
            REQUIRE(arr[i] == static_cast<int>(i));
        }

        // Аналогично с emplace_back
        ChunkedInt arr2;
        for(size_t i{}; i < CHUNK + 1; ++i)
        {
            arr2.emplace_back(static_cast<int>(i));
        }
        REQUIRE(arr2.size() == CHUNK + 1);
        REQUIRE(arr2.blockCount() == 2);
        REQUIRE(arr2[CHUNK] == static_cast<int>(CHUNK));
    }
}



TEST_CASE("ChunkedArray element removal", "[chunked_array][removal]")
{
    using ChunkedInt = mylib::ChunkedArray<int>;

    SECTION("pop_back() removes last element and returns it")
    {
        ChunkedInt arr{ 1, 2, 3, 4, 5 };
        int val{ arr.pop_back() };
        REQUIRE(val == 5);
        REQUIRE(arr.size() == 4);
        checkChunkedArray(arr, { 1, 2, 3, 4 });
        // blockCount не должен измениться (был 1, остался 1)
        REQUIRE(arr.blockCount() == 1);
    }

    SECTION("pop_back() removes last element and frees empty chunk")
    {
        // Создаём ровно CHUNK_SIZE + 1 элементов (2 блока: первый полный, второй с 1 элементом)
        ChunkedInt arr;
        for(size_t i{}; i < defaultChunk + 1; ++i)
        {
            arr.push_back(static_cast<int>(i));
        }
        REQUIRE(arr.size() == defaultChunk + 1);
        REQUIRE(arr.blockCount() == 2);

        // Удаляем последний элемент (единственный во втором блоке)
        int val{ arr.pop_back() };
        REQUIRE(val == static_cast<int>(defaultChunk));
        REQUIRE(arr.size() == defaultChunk);
        // Второй блок должен быть освобождён
        REQUIRE(arr.blockCount() == 1);
        // Проверим последний элемент (теперь это последний элемент первого блока)
        REQUIRE(arr[defaultChunk - 1] == static_cast<int>(defaultChunk - 1));
    }

    SECTION("pop_back() on single element makes array empty")
    {
        ChunkedInt arr{ 42 };
        int val{ arr.pop_back() };
        REQUIRE(val == 42);
        REQUIRE(arr.size() == 0);
        REQUIRE(arr.empty());
        REQUIRE(arr.blockCount() == 0);
    }

    SECTION("pop_back() on multiple elements crossing chunk boundaries")
    {
        ChunkedInt arr;
        // Заполняем 2 полных блока (2 * CH)
        for(size_t i{}; i < 2 * defaultChunk; ++i)
        {
            arr.push_back(static_cast<int>(i));
        }
        REQUIRE(arr.blockCount() == 2);

        // Удаляем один элемент – блоков всё ещё 2, т.к. второй блок не пуст
        arr.pop_back();
        REQUIRE(arr.size() == 2 * defaultChunk - 1);
        REQUIRE(arr.blockCount() == 2);
        // Проверим последний элемент теперь (индекс 2*CH-2)
        REQUIRE(arr[2 * defaultChunk - 2] == static_cast<int>(2 * defaultChunk - 2));

        // Удаляем ещё (CH - 1) элементов, чтобы во втором блоке остался 1 элемент
        for(size_t i{}; i < defaultChunk - 1; ++i)
        {
            arr.pop_back();
        }
        REQUIRE(arr.size() == defaultChunk); // остался один полный блок
        REQUIRE(arr.blockCount() == 1); // второй блок должен был освободиться, когда стал пустым
        // Последний элемент теперь последний в первом блоке
        REQUIRE(arr[defaultChunk - 1] == static_cast<int>(defaultChunk - 1));
    }

    SECTION("clear() removes all elements and frees all chunks")
    {
        ChunkedInt arr;
        for(size_t i{ 0 }; i < 100; ++i)
        {
            arr.push_back(static_cast<int>(i));
        }
        REQUIRE(arr.size() == 100);
        REQUIRE(arr.blockCount() == 4); // 100 / 32 = 3.125 => 4 блока

        arr.clear();
        REQUIRE(arr.size() == 0);
        REQUIRE(arr.empty());
        REQUIRE(arr.blockCount() == 0);

        // Проверим, что можно снова добавлять элементы
        arr.push_back(10);
        REQUIRE(arr.size() == 1);
        REQUIRE(arr[0] == 10);
        REQUIRE(arr.blockCount() == 1);
    }

    SECTION("clear() on empty array is no-op")
    {
        ChunkedInt arr;
        arr.clear();
        REQUIRE(arr.size() == 0);
        REQUIRE(arr.blockCount() == 0);
    }
}



TEST_CASE("ChunkedArray resize, reserve, shrink_to_fit", "[chunked_array][capacity]")
{
    using ChunkedInt = mylib::ChunkedArray<int>;

    SECTION("resize() increasing size")
    {
        ChunkedInt arr{ 1, 2, 3 };
        arr.resize(5, 42);
        REQUIRE(arr.size() == 5);
        checkChunkedArray(arr, { 1, 2, 3, 42, 42 });
        REQUIRE(arr.blockCount() == 1); // всё ещё 1 блок (5 <= 32)
    }

    SECTION("resize() increasing beyond one chunk")
    {
        ChunkedInt arr{ 1, 2, 3 };
        arr.resize(defaultChunk + 5, 99);
        REQUIRE(arr.size() == defaultChunk + 5);
        // Проверим первые три
        REQUIRE(arr[0] == 1);
        REQUIRE(arr[1] == 2);
        REQUIRE(arr[2] == 3);
        // Проверим последние 5 элементов (должны быть 99)
        for(size_t i{ defaultChunk }; i < defaultChunk + 5; ++i)
        {
            REQUIRE(arr[i] == 99);
        }
        // Количество блоков должно быть 2
        REQUIRE(arr.blockCount() == 2);
    }

    SECTION("resize() decreasing size")
    {
        ChunkedInt arr;
        for(size_t i{}; i < defaultChunk + 5; ++i)
        {
            arr.push_back(static_cast<int>(i));
        }
        REQUIRE(arr.size() == defaultChunk + 5);
        REQUIRE(arr.blockCount() == 2);

        arr.resize(defaultChunk); // остаётся ровно один блок
        REQUIRE(arr.size() == defaultChunk);
        REQUIRE(arr.blockCount() == 1);
        // Проверим последний элемент
        REQUIRE(arr[defaultChunk - 1] == static_cast<int>(defaultChunk - 1));

        arr.resize(10);
        REQUIRE(arr.size() == 10);
        REQUIRE(arr.blockCount() == 1);
        REQUIRE(arr[9] == 9);
    }

    SECTION("resize() to zero")
    {
        ChunkedInt arr{ 1, 2, 3 };
        arr.resize(0);
        REQUIRE(arr.size() == 0);
        REQUIRE(arr.empty());
        REQUIRE(arr.blockCount() == 0);
    }

    SECTION("resize() to same size does nothing")
    {
        ChunkedInt arr{ 1, 2, 3 };
        arr.resize(3, 100);
        checkChunkedArray(arr, { 1, 2, 3 }); // значения не изменились
    }

    SECTION("reserve() allocates chunks without changing size")
    {
        ChunkedInt arr{ 1, 2, 3 };
        arr.shrink_to_fit();
        size_t oldSize{ arr.size() };
        REQUIRE(arr.capacity() <= 32);
        arr.reserve(100);
        REQUIRE(arr.size() == oldSize);
        REQUIRE(arr.capacity() >= 100);
        // Проверим, что элементы сохранились
        checkChunkedArray(arr, { 1, 2, 3 });
    }

    SECTION("reserve() with size less than current does nothing")
    {
        ChunkedInt arr;
        for(size_t i{}; i < defaultChunk; ++i)
        {
            arr.push_back(static_cast<int>(i));
        }
        size_t oldBlocks{ arr.blockCount() };
        arr.reserve(10); // меньше текущего размера (32)
        REQUIRE(arr.blockCount() == oldBlocks);
        REQUIRE(arr.size() == defaultChunk);
    }

    SECTION("reserve() with capacity equal to current does nothing")
    {
        ChunkedInt arr;
        REQUIRE(arr.capacity() == 0);
        arr.reserve(10); // сейчас 0 блоков, резервируем 1 блок
        REQUIRE(arr.capacity() > 0);
        size_t blocks{ arr.blockCount() };
        arr.reserve(10);
        REQUIRE(arr.blockCount() == blocks); // не должно увеличиться
    }

    SECTION("shrink_to_fit() reduces chunks to minimum needed")
    {
        ChunkedInt arr;
        for(size_t i{}; i < defaultChunk + 5; ++i)
        {
            arr.push_back(static_cast<int>(i));
        }
        REQUIRE(arr.blockCount() == 2);

        REQUIRE(arr.capacity() == 256);

        // Удаляем элементы, чтобы остался 1 блок с небольшим количеством
        arr.resize(10);
        REQUIRE(arr.size() == 10);
        REQUIRE(arr.blockCount() == 1);
        REQUIRE(arr.capacity() == 256);

        // shrink_to_fit должен уменьшить выделенную память до 1 блока
        arr.shrink_to_fit();
        REQUIRE(arr.blockCount() == 1);
        REQUIRE(arr.size() == 10);
        REQUIRE(arr.capacity() == 32);

        checkChunkedArray(arr, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 });
    }

    SECTION("shrink_to_fit() on empty array clears all")
    {
        ChunkedInt arr{ 1, 2, 3 };
        arr.clear();
        REQUIRE(arr.blockCount() == 0);
        arr.shrink_to_fit(); // ничего не делает
        REQUIRE(arr.blockCount() == 0);
    }

    SECTION("shrink_to_fit() with exactly full chunks")
    {
        ChunkedInt arr;
        for(size_t i{}; i < 2 * defaultChunk; ++i)
        {
            arr.push_back(static_cast<int>(i));
        }
        REQUIRE(arr.blockCount() == 2);
        arr.shrink_to_fit(); // уже минимально
        REQUIRE(arr.blockCount() == 2);
        REQUIRE(arr.size() == 2 * defaultChunk);
    }
}



TEST_CASE("ChunkedArray iterators", "[chunked_array][iterators]")
{
    using ChunkedInt = mylib::ChunkedArray<int>;

    // Создаём массив размером больше одного блока для проверки переходов
    ChunkedInt arr;
    for(size_t i = 0; i < defaultChunk + 5; ++i)
    {
        arr.push_back(static_cast<int>(i));
    }

    SECTION("begin() and end()")
    {
        auto it{ arr.begin() };
        auto end{ arr.end() };
        REQUIRE(it != end);
        REQUIRE(*it == 0);
        ++it;
        REQUIRE(*it == 1);

        // Проверка константных версий
        const auto& carr{ arr };
        auto cit{ carr.begin() };
        REQUIRE(*cit == 0);
        REQUIRE(cit == arr.cbegin());
    }

    SECTION("increment and decrement")
    {
        auto it{ arr.begin() };
        // prefix increment
        ++it;
        REQUIRE(*it == 1);
        // postfix increment
        auto it2 = it++;
        REQUIRE(*it2 == 1);
        REQUIRE(*it == 2);

        // prefix decrement
        --it;
        REQUIRE(*it == 1);
        // postfix decrement
        auto it3{ it-- };
        REQUIRE(*it3 == 1);
        REQUIRE(*it == 0);
    }

    SECTION("arithmetic + and -")
    {
        auto it{ arr.begin() };
        auto it2{ it + 5 };
        REQUIRE(*it2 == 5);
        auto it3{ it2 - 3 };
        REQUIRE(*it3 == 2);

        it += 10;
        REQUIRE(*it == 10);
        it -= 7;
        REQUIRE(*it == 3);

        // Разность
        std::ptrdiff_t diff{ it2 - it };
        REQUIRE(diff == 2); // 5 - 3
    }

    SECTION("comparison operators")
    {
        auto it1{ arr.begin() };
        auto it2{ arr.begin() + 5 };
        REQUIRE(it1 < it2);
        REQUIRE(it1 <= it2);
        REQUIRE(it2 > it1);
        REQUIRE(it2 >= it1);
        REQUIRE(it1 == arr.begin());
        REQUIRE(it1 != it2);
    }

    SECTION("operator[]")
    {
        auto it{ arr.begin() };
        REQUIRE(it[0] == 0);
        REQUIRE(it[5] == 5);
        REQUIRE(it[defaultChunk + 2] == static_cast<int>(defaultChunk + 2)); // переход через блок
        it[3] = 999;
        REQUIRE(arr[3] == 999);
    }

    SECTION("reverse iterators")
    {
        auto rit{ arr.rbegin() };
        REQUIRE(*rit == static_cast<int>(defaultChunk + 4)); // последний элемент
        ++rit;
        REQUIRE(*rit == static_cast<int>(defaultChunk + 3));
        --rit;
        REQUIRE(*rit == static_cast<int>(defaultChunk + 4));

        // Проверка обратного обхода
        std::vector<int> reversed;
        for(auto it{ arr.rbegin() }; it != arr.rend(); ++it)
        {
            reversed.push_back(*it);
        }
        REQUIRE(reversed.size() == arr.size());
        // проверяем, что reversed содержит элементы в обратном порядке
        for(size_t i{}; i < arr.size(); ++i)
        {
            REQUIRE(reversed[i] == static_cast<int>(arr.size() - 1 - i));
        }
    }

    SECTION("const iterators")
    {
        const auto& carr{ arr };
        auto cit{ carr.cbegin() };
        REQUIRE(*cit == 0);
        ++cit;
        REQUIRE(*cit == 1);
    }

    SECTION("std algorithms with iterators")
    {
        // reverse
        ChunkedInt arr2{ arr }; // копия
        std::reverse(arr2.begin(), arr2.end());
        for(size_t i{}; i < arr2.size(); ++i)
        {
            REQUIRE(arr2[i] == static_cast<int>(arr.size() - 1 - i));
        }

        // sort
        ChunkedInt arr3;
        for(int i{ 10 }; i > 0; --i)
        {
            arr3.push_back(i);
        }

        std::sort(arr3.begin(), arr3.end());
        for(size_t i{}; i < arr3.size(); ++i)
        {
            REQUIRE(arr3[i] == static_cast<int>(i + 1));
        }

        // find
        auto found{ std::find(arr.begin(), arr.end(), 10) };
        REQUIRE(found != arr.end());
        REQUIRE(*found == 10);
        auto notFound{ std::find(arr.begin(), arr.end(), 999) };
        REQUIRE(notFound == arr.end());

        // accumulate
        int sum{ std::accumulate(arr.begin(), arr.end(), 0) };
        int expected{ 0 };
        for(size_t i{}; i < arr.size(); ++i)
        {
            expected += static_cast<int>(i);
        }
        REQUIRE(sum == expected);
    }

    SECTION("iterator difference_type and distance")
    {
        auto begin{ arr.begin() };
        auto end{ arr.end() };
        std::ptrdiff_t dist{ std::distance(begin, end) };
        REQUIRE(dist == static_cast<std::ptrdiff_t>(arr.size()));
        REQUIRE((end - begin) == static_cast<std::ptrdiff_t>(arr.size()));
    }
}



TEST_CASE("ChunkedArray overflow and huge sizes", "[chunked_array][overflow]")
{
    using ChunkedInt = mylib::ChunkedArray<int>;
    const size_t huge = std::numeric_limits<size_t>::max() / 2;

    SECTION("constructor with huge size throws std::bad_alloc or std::length_error")
    {
        // Попытка создать массив с размером, близким к максимальному size_t
        // Ожидаем, что аллокатор не сможет выделить память и выбросит std::bad_alloc
        REQUIRE_THROWS_AS(ChunkedInt(huge, 42), std::bad_alloc);
    }

    SECTION("reserve with huge capacity throws std::bad_alloc")
    {
        ChunkedInt arr;
        // reserve должен попытаться выделить память, что приведёт к std::bad_alloc
        REQUIRE_THROWS_AS(arr.reserve(huge), std::bad_alloc);
    }

    SECTION("resize to huge size throws std::bad_alloc")
    {
        ChunkedInt arr;
        REQUIRE_THROWS_AS(arr.resize(huge, 42), std::bad_alloc);
    }
}



TEST_CASE("ChunkedArray with non-trivial type (TestStruct)", "[chunked_array][custom]")
{
    using ChunkedTest = mylib::ChunkedArray<TestStruct>;

    SECTION("Default constructor")
    {
        ChunkedTest arr;
        REQUIRE(arr.empty());
        REQUIRE(arr.size() == 0);
        REQUIRE(arr.blockCount() == 0);
    }

    SECTION("Constructor with size and value")
    {
        TestStruct val{ 42, "forty-two" };
        ChunkedTest arr(5, val);
        REQUIRE(arr.size() == 5);
        for(size_t i{}; i < 5; ++i)
        {
            REQUIRE(arr[i].a == 42);
            REQUIRE(arr[i].s == "forty-two");
        }
        REQUIRE(arr.blockCount() == 1);
    }

    SECTION("Constructor from initializer_list")
    {
        ChunkedTest arr{ { 1, "one" }, { 2, "two" }, { 3, "three" } };
        checkChunkedArray(arr, { {1, "one"}, {2, "two"}, {3, "three"} });
        REQUIRE(arr.blockCount() == 1);
    }

    SECTION("Constructor from container (std::vector)")
    {
        std::vector<TestStruct> vec{ {10, "ten"}, {20, "twenty"} };
        ChunkedTest arr(vec);
        checkChunkedArray(arr, { {10, "ten"}, {20, "twenty"} });
    }

    SECTION("Copy constructor")
    {
        ChunkedTest original{ {1, "a"}, {2, "b"}, {3, "c"} };
        ChunkedTest copy{ original };

        checkChunkedArray(copy, { {1, "a"}, {2, "b"}, {3, "c"} });
        // Оригинал не изменился
        checkChunkedArray(original, { {1, "a"}, {2, "b"}, {3, "c"} });
    }

    SECTION("Move constructor")
    {
        ChunkedTest source{ {1, "one"}, {2, "two"} };
        size_t oldSize{ source.size() };
        ChunkedTest target{ std::move(source) };

        REQUIRE(target.size() == oldSize);
        checkChunkedArray(target, { {1, "one"}, {2, "two"} });
        REQUIRE(source.empty());
        REQUIRE(source.blockCount() == 0);
    }

    SECTION("Copy assignment")
    {
        ChunkedTest a{ {1, "a"}, {2, "b"} };
        ChunkedTest b{ {3, "c"}, {4, "d"}, {5, "e"} };
        a = b;
        checkChunkedArray(a, { {3, "c"}, {4, "d"}, {5, "e"} });
        checkChunkedArray(b, { {3, "c"}, {4, "d"}, {5, "e"} });
        // self-assignment
        a = a;
        checkChunkedArray(a, { {3, "c"}, {4, "d"}, {5, "e"} });
    }

    SECTION("Move assignment")
    {
        ChunkedTest a{ {1, "one"}, {2, "two"} };
        ChunkedTest b{ {3, "three"} };
        b = std::move(a);
        checkChunkedArray(b, { {1, "one"}, {2, "two"} });
        REQUIRE(a.empty());
        REQUIRE(a.blockCount() == 0);
        // self-move
        b = std::move(b);
        checkChunkedArray(b, { {1, "one"}, {2, "two"} });
    }

    SECTION("push_back and emplace_back")
    {
        ChunkedTest arr;
        TestStruct val{ 10, "ten" };
        arr.push_back(val); // lvalue
        arr.push_back(TestStruct{ 20, "twenty" }); // rvalue
        arr.emplace_back(30, "thirty");
        checkChunkedArray(arr, { {10, "ten"}, {20, "twenty"}, {30, "thirty"} });
    }

    SECTION("pop_back")
    {
        ChunkedTest arr{ {1, "a"}, {2, "b"}, {3, "c"} };
        TestStruct popped{ arr.pop_back() };
        REQUIRE(popped.a == 3);
        REQUIRE(popped.s == "c");
        checkChunkedArray(arr, { {1, "a"}, {2, "b"} });
        // Удаляем ещё один и проверяем блоки
        arr.pop_back();
        checkChunkedArray(arr, { {1, "a"} });
        arr.pop_back();
        REQUIRE(arr.empty());
        REQUIRE(arr.blockCount() == 0);
    }

    SECTION("clear")
    {
        ChunkedTest arr{ {1, "one"}, {2, "two"}, {3, "three"} };
        arr.clear();
        REQUIRE(arr.empty());
        REQUIRE(arr.blockCount() == 0);
        // Можно снова добавлять
        arr.push_back(TestStruct{ 4, "four" });
        checkChunkedArray(arr, { {4, "four"} });
    }

    SECTION("resize")
    {
        ChunkedTest arr{ {1, "a"}, {2, "b"} };
        // Увеличение
        arr.resize(5, TestStruct{ 99, "default" });
        REQUIRE(arr.size() == 5);
        checkChunkedArray(arr, { {1, "a"}, {2, "b"}, {99, "default"}, {99, "default"}, {99, "default"} });
        // Уменьшение
        arr.resize(2);
        checkChunkedArray(arr, { {1, "a"}, {2, "b"} });
        // Уменьшение до нуля
        arr.resize(0);
        REQUIRE(arr.empty());
        REQUIRE(arr.blockCount() == 0);
    }

    SECTION("reserve and shrink_to_fit")
    {
        ChunkedTest arr;
        arr.reserve(300); // резервируем 4 блока (100 / 32 + 1)
        REQUIRE(arr.blockCount() == 0); // reserve не создаёт блоки, только резервирует память для указателей
        // добавляем элементы
        for(int i{}; i < 100; ++i)
        {
            arr.push_back(TestStruct(i, std::to_string(i)));
        }
        REQUIRE(arr.size() == 100);
        REQUIRE(arr.blockCount() == 4); // 50 элементов -> 2 блока
        // shrink_to_fit не должен уменьшать блоки, т.к. они заняты
        arr.shrink_to_fit();
        REQUIRE(arr.blockCount() == 4);
        // Удаляем часть элементов, чтобы освободить блок
        arr.resize(10);
        REQUIRE(arr.blockCount() == 1);
        arr.shrink_to_fit(); // должен освободить второй блок
        REQUIRE(arr.blockCount() == 1);
        REQUIRE(arr.size() == 10);
    }

    SECTION("access operators")
    {
        ChunkedTest arr { {1, "one"}, {2, "two"}, {3, "three"} };
        // operator[]
        REQUIRE(arr[0].s == "one");
        arr[1] = TestStruct{ 20, "twenty" };
        REQUIRE(arr[1].a == 20);
        // const version
        const ChunkedTest& carr{ arr };
        REQUIRE(carr[2].s == "three");
        // at()
        REQUIRE(arr.at(0).a == 1);
        REQUIRE_THROWS_AS(arr.at(10), std::out_of_range);
        // front/back
        REQUIRE(arr.front().s == "one");
        REQUIRE(arr.back().s == "three");
        arr.front() = TestStruct(100, "hundred");
        REQUIRE(arr[0].a == 100);
    }

    SECTION("iterators")
    {
        ChunkedTest arr{ {1, "a"}, {2, "b"}, {3, "c"}, {4, "d"} };
        // begin/end
        auto it{ arr.begin() };
        REQUIRE(it->a == 1);
        ++it;
        REQUIRE(it->s == "b");
        // reverse
        auto rit{ arr.rbegin() };
        REQUIRE(rit->a == 4);
        ++rit;
        REQUIRE(rit->s == "c");
        // алгоритмы
        std::reverse(arr.begin(), arr.end());
        checkChunkedArray(arr, { {4, "d"}, {3, "c"}, {2, "b"}, {1, "a"} });
        std::sort(arr.begin(), arr.end(), [](const TestStruct& x, const TestStruct& y)
        {
            return x.a < y.a;
        });
        checkChunkedArray(arr, { {1, "a"}, {2, "b"}, {3, "c"}, {4, "d"} });
    }

    SECTION("appendVector-like? – нет такого метода в ChunkedArray, но можно проверить emplace_back с большим числом")
    {
        ChunkedTest arr;
        for(int i{}; i < 100; ++i)
        {
            arr.emplace_back(i, std::to_string(i));
        }

        REQUIRE(arr.size() == 100);
        REQUIRE(arr.blockCount() == 4); // 100/32 = 3.125 -> 4
        // проверим случайный элемент
        REQUIRE(arr[50].a == 50);
        REQUIRE(arr[50].s == "50");
    }
}



TEST_CASE("ChunkedArray with custom allocator", "[chunked_array][allocator]")
{
    using Alloc = CountingAllocator<int>;
    using ChunkedIntAlloc = mylib::ChunkedArray<int, defaultChunk, Alloc>;

    SECTION("default constructor does not allocate")
    {
        Alloc::reset();
        ChunkedIntAlloc arr;
        REQUIRE(arr.size() == 0);
        REQUIRE(arr.blockCount() == 0);
        REQUIRE(Alloc::allocateCount == 0);
        REQUIRE(Alloc::deallocateCount == 0);
    }

    SECTION("constructor with size allocates chunks")
    {
        Alloc::reset();
        ChunkedIntAlloc arr(10, 42);
        REQUIRE(arr.size() == 10);
        REQUIRE(arr.blockCount() == 1);
        REQUIRE(Alloc::allocateCount == 1);
        REQUIRE(Alloc::deallocateCount == 0);
    }

    SECTION("initializer_list constructor allocates chunks")
    {
        Alloc::reset();
        ChunkedIntAlloc arr{1, 2, 3, 4, 5};
        REQUIRE(arr.blockCount() == 1);
        REQUIRE(Alloc::allocateCount == 1);
        REQUIRE(Alloc::deallocateCount == 0);
    }

    SECTION("copy constructor allocates new chunks")
    {
        ChunkedIntAlloc original(5, 42);
        Alloc::reset();
        ChunkedIntAlloc copy{ original };
        REQUIRE(copy.size() == 5);
        REQUIRE(copy.blockCount() == 1);
        REQUIRE(Alloc::allocateCount == 1);
        REQUIRE(Alloc::deallocateCount == 0);
    }

    SECTION("move constructor does not allocate")
    {
        ChunkedIntAlloc source(5, 42);
        Alloc::reset();
        ChunkedIntAlloc target{ std::move(source) };
        REQUIRE(target.size() == 5);
        REQUIRE(target.blockCount() == 1);
        REQUIRE(Alloc::allocateCount == 0);
        REQUIRE(Alloc::deallocateCount == 0);
    }

    SECTION("copy assignment allocates and deallocates")
    {
        ChunkedIntAlloc a(5, 42);
        ChunkedIntAlloc b(3, 99);
        Alloc::reset();
        a = b;
        REQUIRE(a.size() == 3);
        REQUIRE(a.blockCount() == 1);
        REQUIRE(Alloc::allocateCount == 1); // новый блок для a
        REQUIRE(Alloc::deallocateCount == 1); // освобождение старого блока a
    }

    SECTION("move assignment does not allocate but deallocates old resources")
    {
        ChunkedIntAlloc a(5, 42);
        ChunkedIntAlloc b(3, 99);
        Alloc::reset();
        a = std::move(b);
        REQUIRE(a.size() == 3);
        REQUIRE(a.blockCount() == 1);
        REQUIRE(Alloc::allocateCount == 0);
        REQUIRE(Alloc::deallocateCount == 1);
    }

    SECTION("push_back allocates new blocks as needed")
    {
        Alloc::reset();
        ChunkedIntAlloc arr;
        for(size_t i{}; i < defaultChunk + 1; ++i)
        {
            arr.push_back(static_cast<int>(i));
        }
        REQUIRE(Alloc::allocateCount == 2);
        REQUIRE(Alloc::deallocateCount == 0);
        REQUIRE(arr.size() == defaultChunk + 1);
        REQUIRE(arr.blockCount() == 2);
    }

    SECTION("pop_back deallocates empty chunk")
    {
        ChunkedIntAlloc arr;
        for(size_t i{}; i < defaultChunk + 1; ++i)
        {
            arr.push_back(static_cast<int>(i));
        }
        Alloc::reset();
        arr.pop_back();
        REQUIRE(Alloc::allocateCount == 0);
        REQUIRE(Alloc::deallocateCount == 1);
        REQUIRE(arr.size() == defaultChunk);
        REQUIRE(arr.blockCount() == 1);
    }

    SECTION("clear deallocates all chunks")
    {
        ChunkedIntAlloc arr;
        for(size_t i{}; i < defaultChunk + 1; ++i)
        {
            arr.push_back(static_cast<int>(i));
        }
        Alloc::reset();
        arr.clear();
        REQUIRE(Alloc::deallocateCount == 2);
        REQUIRE(arr.blockCount() == 0);
        REQUIRE(Alloc::allocateCount == 0);
    }

    SECTION("resize decreasing deallocates chunks")
    {
        ChunkedIntAlloc arr;
        for(size_t i{}; i < 2 * defaultChunk; ++i)
        {
            arr.push_back(static_cast<int>(i));
        }
        Alloc::reset();
        arr.resize(defaultChunk);
        REQUIRE(Alloc::deallocateCount == 1);
        REQUIRE(arr.blockCount() == 1);
    }

    SECTION("resize increasing allocates chunks")
    {
        ChunkedIntAlloc arr;
        arr.resize(defaultChunk, 42);
        Alloc::reset();
        arr.resize(2 * defaultChunk + 5, 99);
        REQUIRE(Alloc::allocateCount == 2);
        REQUIRE(arr.blockCount() == 3);
    }

    SECTION("destructor deallocates all chunks")
    {
        Alloc::reset();
        {
            ChunkedIntAlloc arr(5, 42);
            REQUIRE(Alloc::allocateCount == 1);
        }
        REQUIRE(Alloc::deallocateCount == 1);
    }

    SECTION("with std::allocator works")
    {
        using ChunkedIntStd = mylib::ChunkedArray<int, defaultChunk, std::allocator<int>>;
        ChunkedIntStd arr;
        arr.push_back(10);
        arr.push_back(20);
        REQUIRE(arr.size() == 2);
        REQUIRE(arr[0] == 10);
        REQUIRE(arr[1] == 20);
        arr.resize(5, 42);
        REQUIRE(arr.size() == 5);
        REQUIRE(arr[4] == 42);
        ChunkedIntStd copy{ arr };
        REQUIRE(copy.size() == 5);
        ChunkedIntStd moved{ std::move(arr) };
        REQUIRE(moved.size() == 5);
        REQUIRE(arr.size() == 0);
    }
}



TEST_CASE("ChunkedArray exception safety", "[chunked_array][exceptions]")
{
    using ChunkedThrow = mylib::ChunkedArray<ThrowingCopy>;

    SECTION("push_back copy throws, object remains unchanged")
    {
        ChunkedThrow arr;
        arr.push_back(ThrowingCopy{ 10 }); // перемещение
        ThrowingCopy val{ 20 };
        ThrowingCopy::reset(1); // бросить при первом копировании
        REQUIRE_THROWS_AS(arr.push_back(val), std::runtime_error);
        REQUIRE(arr.size() == 1);
        REQUIRE(arr[0].value == 10);
        REQUIRE(arr.blockCount() == 1);
    }

    SECTION("resize increase throws during copy, object unchanged")
    {
        ChunkedThrow arr;
        arr.push_back(ThrowingCopy{ 1 });
        arr.push_back(ThrowingCopy{ 2 });
        ThrowingCopy::reset(2); // бросить на втором копировании
        REQUIRE_THROWS_AS(arr.resize(5, ThrowingCopy(99)), std::runtime_error);
        REQUIRE(arr.size() == 2);
        REQUIRE(arr[0].value == 1);
        REQUIRE(arr[1].value == 2);
        REQUIRE(arr.blockCount() == 1);
    }

    SECTION("resize decrease does not throw")
    {
        ChunkedThrow arr;
        for(int i{}; i < 10; ++i)
        {
            arr.push_back(ThrowingCopy{ i });
        }

        REQUIRE_NOTHROW(arr.resize(3));
        REQUIRE(arr.size() == 3);
        REQUIRE(arr[0].value == 0);
        REQUIRE(arr[2].value == 2);
    }

    SECTION("copy constructor throws during copy, original unchanged")
    {
        ChunkedThrow source;
        for(int i{}; i < 5; ++i)
        {
            source.push_back(ThrowingCopy{ i });
        }

        ThrowingCopy::reset(3); // бросить на третьем копировании
        REQUIRE_THROWS_AS(ChunkedThrow{ source }, std::runtime_error);
        REQUIRE(source.size() == 5);
        for(int i{}; i < 5; ++i)
        {
            REQUIRE(source[i].value == i);
        }
    }

    SECTION("constructor from container throws during copy, container unchanged")
    {
        std::vector<ThrowingCopy> vec;
        for(int i{}; i < 4; ++i)
        {
            vec.emplace_back(i);
        }
        ThrowingCopy::reset(2); // бросить на втором копировании
        REQUIRE_THROWS_AS(ChunkedThrow{ vec }, std::runtime_error);
        REQUIRE(vec.size() == 4);
        for(int i{}; i < 4; ++i)
        {
                REQUIRE(vec[i].value == i);
        }
    }

    SECTION("emplace_back does not copy, noexcept")
    {
        ThrowingCopy::reset(1);
        ChunkedThrow arr;
        REQUIRE_NOTHROW(arr.emplace_back(42));
        REQUIRE(arr.size() == 1);
        REQUIRE(arr[0].value == 42);
        REQUIRE_NOTHROW(arr.emplace_back(43));
        REQUIRE(arr.size() == 2);
        REQUIRE(arr[1].value == 43);
    }
}
