#include <catch2/catch_all.hpp>

#include <array>
#include <list>
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

        for (size_t i = 0; i < expected.size(); ++i, ++it)
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
        for (size_t i = 0; i < size; ++i)
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
        for (size_t i{}; i < original.size(); ++i)
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
