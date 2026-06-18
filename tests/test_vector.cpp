#include <catch2/catch_all.hpp>

#include "mylib/mylib.h"

namespace
{
    // Вспомогательная функция для проверки содержимого вектора
    template<typename T>
    void checkVector(const mylib::Vector<T>& vec, const std::initializer_list<T>& expected)
    {
        REQUIRE(vec.size() == expected.size());
        auto it{ expected.begin() };

        for(size_t i{}; i < expected.size(); ++i, ++it)
        {
            REQUIRE(vec[i] == *it);
        }
    }


    struct TestStruct
    {
        int a;
        std::string s;
        TestStruct(int x = 0, const std::string& str = "")
            : a(x)
            , s(str)
        {}

        bool operator==(const TestStruct& other) const
        {
            return a == other.a && s == other.s;
        }
    };

} // end namespace

TEST_CASE("Vector construction and basis properties", "[vector][construction]")
{
    SECTION("Default constructor")
    {
        mylib::Vector<int> v;
        REQUIRE(v.size() == 0);
        REQUIRE(v.empty());
        REQUIRE(v.data() == nullptr);
    }

    SECTION("Constructor with size and default value")
    {
        mylib::Vector<int> v(5);
        REQUIRE(v.size() == 5);
        for(size_t i{}; i < 5; ++i)
        {
            REQUIRE(v[i] == 0);
        }
    }

    SECTION("Constructor with size and explicit value")
    {
        mylib::Vector<int> v(3, 42);
        REQUIRE(v.size() == 3);
        for(size_t i{}; i < 3; ++i)
        {
            REQUIRE(v[i] == 42);
        }
    }

    SECTION("Constructor with size = 0")
    {
        mylib::Vector<int> v(0, 10);
        REQUIRE(v.size() == 0);
        REQUIRE(v.empty());
        REQUIRE(v.data() == nullptr);
    }

    SECTION("Initializer list constructor")
    {
        mylib::Vector<int> v1{ 1, 2, 3, 4, 5 };
        checkVector(v1, { 1, 2, 3, 4, 5 });

        mylib::Vector<int> v2 = { 6, 7, 8, 9, 10 };
        checkVector(v2, { 6, 7, 8, 9, 10 });
    }

    SECTION("Initializer list constructor with empty list")
    {
        mylib::Vector<int> v{};
        REQUIRE(v.size() == 0);
        REQUIRE(v.empty());
        REQUIRE(v.data() == nullptr);
    }
}



TEST_CASE("Vector copy and move semantics", "[vector][copy][move]")
{
    std::initializer_list<int> listA{ 1, 2, 3 };
    std::initializer_list<int> listB{ 4, 5 };

    mylib::Vector<int> a{ listA };
    mylib::Vector<int> b{ listB };

    SECTION("Copy constructor")
    {
        mylib::Vector<int> copy{ a };

        checkVector(copy, listA);

        // Проверяем, что оригинал не изменился
        checkVector(a, listA);
    }

    SECTION("Copy assignment operator")
    {
        a = b;
        checkVector(a, listB);
        checkVector(b, listB);
    }

    SECTION("Copy assignment self-assignment")
    {
        a = a;
        checkVector(a, listA);
    }

    SECTION("Move constructor")
    {
        mylib::Vector<int> moved{std::move(a) };
        checkVector(moved, listA);

        REQUIRE(a.size() == 0);
        REQUIRE(a.empty());
        REQUIRE(a.data() == nullptr);
    }

    SECTION("Move assignment operator")
    {
        a = std::move(b);
        checkVector(a, listB);

        REQUIRE(b.size() == 0);
        REQUIRE(b.empty());
        REQUIRE(b.data() == nullptr);
    }

    SECTION("Move assignment self-assignment")
    {
        a = std::move(a);
        checkVector(a, listA);
    }
}



TEST_CASE("Vector modifiers", "[vector][modifiers]")
{
    SECTION("append")
    {
        mylib::Vector<int> v;

        REQUIRE(v.capacity() == 0);

        v.append(10);
        REQUIRE(v.size() == 1);
        REQUIRE(v.capacity() == 8);
        REQUIRE(v[0] == 10);
        v.append(20);
        v.append(30);
        checkVector(v, {10, 20, 30});
    }

    SECTION("resize increasing size")
    {
        mylib::Vector<int> v{ 1, 2, 3 };
        v.resize(5, 42);
        checkVector(v, {1, 2, 3, 42, 42});
    }

    SECTION("resize decreasing size")
    {
        mylib::Vector<int> v{ 1, 2, 3, 4, 5 };
        v.resize(3);
        checkVector(v, {1, 2, 3});
        v.append(6);
        v.append(7);
        checkVector(v, {1, 2, 3, 6, 7});
    }

    SECTION("resize to zero")
    {
        mylib::Vector<int> v{ 1, 2, 3, 4, 5, 6, 7, 8, 9 }; // больше MinCapacity
        REQUIRE(v.capacity() == 16);

        v.resize(0);
        REQUIRE(v.size() == 0);
        REQUIRE(v.empty());
        REQUIRE(v.capacity() == 8);
        // Добавим 9 элементов, чтобы проверить, что перераспределение произойдёт корректно
        for (int i = 0; i < 9; ++i)
        {
            v.append(i);
        }
        REQUIRE(v.size() == 9);
        for (int i = 0; i < 9; ++i)
        {
            REQUIRE(v[i] == i);
        }
    }

    SECTION("resize with same size (no-op)")
    {
        mylib::Vector<int> v{ 1, 2, 3 };
        v.resize(3, 100);
        checkVector(v, {1, 2, 3}); // значения не изменились
    }

    SECTION("append after shrink")
    {
        mylib::Vector<int> v{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        REQUIRE(v.capacity() == 16);
        v.resize(3); // уменьшаем размер,
        REQUIRE(v.capacity() == 8);
        v.append(11);
        // Проверяем, что размер увеличился, и старые элементы сохранились
        REQUIRE(v.size() == 4);
        REQUIRE(v[0] == 1);
        REQUIRE(v[1] == 2);
        REQUIRE(v[2] == 3);
        REQUIRE(v[3] == 11);
    }
}



TEST_CASE("Vector element access and data", "[vector][access]")
{
    SECTION("operator[]")
    {
        mylib::Vector<int> v{ 10, 20, 30 };
        REQUIRE(v[0] == 10);
        REQUIRE(v[1] == 20);
        REQUIRE(v[2] == 30);

        v[1] = 99;
        REQUIRE(v[1] == 99);
    }

    SECTION("operator[] const")
    {
        const mylib::Vector<int> v{ 1, 2, 3 };
        REQUIRE(v[0] == 1);
        REQUIRE(v[1] == 2);
        REQUIRE(v[2] == 3);

        //v[1] = 99; // ошибка компиляции
    }

    SECTION("data() non-const")
    {
        mylib::Vector<int> v{ 5, 6, 7 };
        int* ptr{ v.data() };

        REQUIRE(ptr == &v[0]);
        ptr[1] = 42;
        REQUIRE(v[1] == 42);
    }

    SECTION("data() const")
    {
        const mylib::Vector<int> v{ 8, 9, 10 };
        const int* ptr{ v.data() };

        REQUIRE(ptr == &v[0]);
        // ptr[2] = 42; // ошибка компиляции
        REQUIRE(ptr[2] == 10);
    }

    SECTION("size and empty")
    {
        mylib::Vector<int> v;
        REQUIRE(v.size() == 0);
        REQUIRE(v.empty());
        REQUIRE(v.capacity() == 0);

        v.append(1);
        REQUIRE(v.size() == 1);
        REQUIRE(!v.empty());
        REQUIRE(v.capacity() == 8);
    }
}



// Дополнительный тест на работу с большими данными и проверку capacity (неявно)
TEST_CASE("Vector capacity growth", "[vector][capacity]")
{
    SECTION("append many elements")
    {
        mylib::Vector<int> v;
        REQUIRE(v.capacity() ==0);

        const size_t N{ 1000 };
        for(size_t i{}; i < N; ++i)
        {
            v.append(static_cast<int>(i));
        }

        size_t cap{ 8 };
        while(cap < N)
        {
            cap *=2;
        }

        REQUIRE(v.size() == N);
        REQUIRE(v.capacity() == cap);

        for (size_t i{}; i < N; ++i)
        {
            REQUIRE(v[i] == static_cast<int>(i));
        }
    }

    SECTION("reserve and append")
    {
        mylib::Vector<int> v;
        for (int i{}; i < 20; ++i)
        {
            v.append(i);
        }

        v.resize(0);
        REQUIRE(v.capacity() == 8);

        for (int i{}; i < 20; ++i)
        {
            v.append(i);
        }


        REQUIRE(v.size() == 20);
        REQUIRE(v.capacity() == 32);

        for (int i{}; i < 20; ++i)
        {
            REQUIRE(v[i] == i);
        }
    }
}



TEST_CASE("Vector with non-trivial type", "[vector][custom-type]")
{
    SECTION("construct, copy, move")
    {
        mylib::Vector<TestStruct> v;
        v.append(TestStruct{ 1, "one" });
        v.append(TestStruct{ 2, "two" });

        REQUIRE(v.size() == 2);
        REQUIRE(v[0].a == 1);
        REQUIRE(v[0].s == "one");

        mylib::Vector<TestStruct> v2{ v };
        REQUIRE(v2[0].a == 1);
        REQUIRE(v2[1].s == "two");

        mylib::Vector<TestStruct> v3{ std::move(v) };
        REQUIRE(v3.size() == 2);
        REQUIRE(v.size() == 0);
    }
}

