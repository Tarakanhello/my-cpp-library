#include <catch2/catch_all.hpp>
#include <numeric>

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

        auto operator<=>(const TestStruct& other) const = default;

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



TEST_CASE("Vector iterators without nested types", "[vector][iterators]")
{
    mylib::Vector<int> v{ 5, 3, 1, 4, 2 };

    SECTION("std::distance should work")
    {
        auto dist{ std::distance(v.begin(), v.end()) };

        REQUIRE(dist == 5);
    }
}



TEST_CASE("Vector::at() bounds checking", "[vector][at]")
{
    mylib::Vector<int> v{ 1, 2, 3 };

    SECTION("Valid index returns reference")
    {
        REQUIRE(v.at(0) == 1);
        REQUIRE(v.at(1) == 2);
        REQUIRE(v.at(2) == 3);

        v.at(1) = 42;
        REQUIRE(v.at(1) == 42);
    }

    SECTION("Const version works")
    {
        const mylib::Vector<int> cv{ 10, 20, 30 };
        REQUIRE(cv.at(1) == 20);
    }

    SECTION("Out-of-range throws std::out_of_range")
    {
        REQUIRE_THROWS_AS(v.at(3), std::out_of_range);
        REQUIRE_THROWS_AS(v.at(100), std::out_of_range);
        REQUIRE_THROWS_AS(v.at(v.size()), std::out_of_range);

        const mylib::Vector<int> cv{ 5 };
        REQUIRE_THROWS_AS(cv.at(1), std::out_of_range);
    }

    SECTION("Empty vector throws")
    {
        mylib::Vector<int> empty;
        REQUIRE_THROWS_AS(empty.at(0), std::out_of_range);
    }
}



TEST_CASE("Vector::push_back and emplace_back", "[vector][push_back][emplace_back]")
{
    SECTION("push_back lvalue")
    {
        mylib::Vector<int> v;
        int x{ 5 };
        v.push_back(x);
        v.push_back(x + 1);
        checkVector(v, {5, 6});
    }

    SECTION("push_back rvalue")
    {
        mylib::Vector<int> v;
        v.push_back(10);
        v.push_back(20);
        checkVector(v, {10, 20});

        std::string s{ "hello" };
        mylib::Vector<std::string> sv;
        sv.push_back(s);                   // lvalue
        sv.push_back(std::string{ "world" }); // rvalue
        REQUIRE(sv.size() == 2);
        REQUIRE(sv[0] == "hello");
        REQUIRE(sv[1] == "world");
    }

    SECTION("emplace_back constructs in-place")
    {
        mylib::Vector<TestStruct> v;
        v.emplace_back(1, "one");   // конструктор TestStruct(int, string)
        v.emplace_back(2, "two");
        REQUIRE(v.size() == 2);
        REQUIRE(v[0].a == 1);
        REQUIRE(v[0].s == "one");
        REQUIRE(v[1].a == 2);
        REQUIRE(v[1].s == "two");

        // emplace_back с одним аргументом (конструктор по умолчанию с одним параметром)
        mylib::Vector<int> iv;
        iv.emplace_back(42);
        REQUIRE(iv[0] == 42);
    }

    SECTION("push_back/emplace_back trigger reallocation")
    {
        mylib::Vector<int> v;
        REQUIRE(v.capacity() == 0);

        for (int i = 0; i < 100; ++i)
        {
            v.push_back(i);
        }
        REQUIRE(v.size() == 100);
        REQUIRE(v.capacity() >= 100);

        // Проверим, что все элементы на месте
        for (int i{}; const auto& vec : v )
        {
            REQUIRE(vec == i++);
        }

        // Аналогично с emplace_back
        mylib::Vector<int> v2;
        for (int i = 0; i < 100; ++i)
        {
            v2.emplace_back(i);
        }

        REQUIRE(v2.size() == 100);
        for (int i{ 0 }; const auto& vec : v2)
        {
            REQUIRE(vec == i++);
        }
    }
}



TEST_CASE("Vector::reserve and shrink_to_fit", "[vector][reserve][shrink_to_fit]")
{
    SECTION("reserve increases capacity if needed")
    {
        mylib::Vector<int> v;
        REQUIRE(v.capacity() == 0);

        v.reserve(100);
        REQUIRE(v.capacity() >= 100);
        size_t cap{ v.capacity() };
        v.push_back(1);
        REQUIRE(v.capacity() == cap);

        v.reserve(5); // меньше текущей – ничего не меняет
        REQUIRE(v.capacity() == cap);

        cap = v.capacity();
        v.reserve(cap + 1); // больше – увеличивает
        REQUIRE(v.capacity() >= cap + 1);

        // Проверяем, что элементы не повреждены
        v.push_back(2);
        v.push_back(3);
        v.reserve(100);
        REQUIRE(v[0] == 1);
        REQUIRE(v[1] == 2);
        REQUIRE(v.size() == 3);
    }

    SECTION("shrink_to_fit reduces capacity to size")
    {
        mylib::Vector<int> v;
        for (int i = 0; i < 100; ++i)
        {
            v.push_back(i);
        }

        REQUIRE(v.capacity() >= 100);

        v.resize(10);
        REQUIRE(v.capacity() < 100); // уменьшили более чем в 4 раза
        REQUIRE(v.capacity() != 10); // capacity должно быть кратно 2

        v.shrink_to_fit();
        REQUIRE(v.capacity() == 10);
        checkVector(v, {0,1,2,3,4,5,6,7,8,9});

        // Повторный вызов на уже уменьшенном – ничего не меняет
        size_t cap{ v.capacity() };
        v.shrink_to_fit();
        REQUIRE(v.capacity() == cap);

        // shrink на пустом векторе
        mylib::Vector<int> empty;
        empty.shrink_to_fit();
        REQUIRE(empty.capacity() == 0);
    }
}



TEST_CASE("Vector comparison operators", "[vector][comparison]")
{
    using V = mylib::Vector<int>;

    SECTION("operator==")
    {
        V v1{1, 2, 3};
        V v2{1, 2, 3};
        V v3{1, 2, 4};
        V v4{1, 2};
        V v5{1, 2, 3, 4};

        REQUIRE(v1 == v2);
        REQUIRE(v1 != v3);
        REQUIRE(v1 != v4);
        REQUIRE(v1 != v5);
        REQUIRE(v3 != v4);

        // Пустые
        V e1, e2;
        REQUIRE(e1 == e2);
        REQUIRE(e1 != v1);
    }

    SECTION("Three-way comparison (operator<=> gives all ordering)")
    {
        V v1{1, 2, 3};
        V v2{1, 2, 4};
        V v3{1, 2, 2};
        V v4{1, 2, 3, 0};
        V v5{1, 2};   // короче

        // Проверяем все операторы, которые генерируются из <=>
        REQUIRE((v1 < v2) == true);
        REQUIRE((v1 < v3) == false);
        REQUIRE((v1 > v3) == true);
        REQUIRE((v1 <= v1) == true);
        REQUIRE((v1 >= v1) == true);
        REQUIRE((v2 > v1) == true);
        REQUIRE((v2 >= v1) == true);
        REQUIRE((v2 <= v1) == false);

        // Сравнение при разных размерах
        REQUIRE((v1 < v4) == true);
        REQUIRE((v1 > v4) == false);
        REQUIRE((v1 < v4) == (v1.size() < v4.size())); // т.к. все общие равны
        REQUIRE((v1 < v5) == false);   // v5 короче, первые два равны => v5 < v1
        REQUIRE((v5 < v1) == true);

        // Проверка с пользовательским типом
        mylib::Vector<std::string> s1{ "abc", "def" };
        mylib::Vector<std::string> s2{ "abd", "efg" };
        REQUIRE(s1 < s2);
    }

    SECTION("Lexicographic order")
    {
        V a{1, 2, 3};
        V b{1, 2, 3, 0};
        V c{1, 2, 2};

        REQUIRE(a < b);   // a короче и все общие равны
        REQUIRE(b > a);
        REQUIRE(a > c);   // на третьей позиции 3 > 2
        REQUIRE(c < a);
        REQUIRE(a == a);
        REQUIRE(a != b);
    }
}



TEST_CASE("Vector iterator operations", "[vector][iterator][random_access]")
{
    mylib::Vector<int> v{ 10, 20, 30, 40, 50 };

    SECTION("Increment/decrement")
    {
        auto it{ v.begin() };
        REQUIRE(*it == 10);
        ++it;
        REQUIRE(*it == 20);
        it++;
        REQUIRE(*it == 30);
        --it;
        REQUIRE(*it == 20);
        it--;
        REQUIRE(*it == 10);
    }

    SECTION("Arithmetic")
    {
        auto it{ v.begin() };
        auto it2{ it + 2 };
        REQUIRE(*it2 == 30);
        REQUIRE(it2 - it == 2);
        it += 3;
        REQUIRE(*it == 40);
        it -= 2;
        REQUIRE(*it == 20);
        REQUIRE(it[1] == 30);
        REQUIRE(it[3] == 50);
    }

    SECTION("Comparison")
    {
        auto it1{ v.begin() };
        auto it2{ v.begin() + 2 };
        REQUIRE(it1 < it2);
        REQUIRE(it1 <= it2);
        REQUIRE(it2 > it1);
        REQUIRE(it2 >= it1);
        REQUIRE(it1 == v.begin());
        REQUIRE(it1 != it2);
    }

    SECTION("Reverse iterators")
    {
        auto rit{ v.rbegin() };
        REQUIRE(*rit == 50);
        ++rit;
        REQUIRE(*rit == 40);
        --rit;
        REQUIRE(*rit == 50);
    }

    SECTION("Const iterators")
    {
        const mylib::Vector<int> cv{ 1, 2, 3 };
        auto cit{ cv.cbegin() };
        REQUIRE(*cit == 1);
        ++cit;
        REQUIRE(*cit == 2);
    }

    SECTION("Iterator with std algorithms")
    {
        // std::reverse
        std::reverse(v.begin(), v.end());
        checkVector(v, {50, 40, 30, 20, 10});

        // std::sort
        std::sort(v.begin(), v.end());
        checkVector(v, {10, 20, 30, 40, 50});

        // std::find
        auto found{ std::find(v.begin(), v.end(), 30) };
        REQUIRE(found != v.end());
        REQUIRE(*found == 30);
        REQUIRE(found - v.begin() == 2);

        // std::accumulate
        int sum = std::accumulate(v.begin(), v.end(), 0);
        REQUIRE(sum == 150);
    }
}



TEST_CASE("Vector comparisons with non-trivial types", "[vector][comparison][custom]")
{
    mylib::Vector<TestStruct> v1{ {1, "a"}, {2, "b"} };
    mylib::Vector<TestStruct> v2{ {1, "a"}, {2, "c"} };
    mylib::Vector<TestStruct> v3{ {1, "a"}, {2, "b"} };
    mylib::Vector<TestStruct> v4{ {1, "a"} };

    REQUIRE(v1 == v3);
    REQUIRE(v1 != v2);
    REQUIRE(v1 < v2);
    REQUIRE(v2 > v1);
    REQUIRE(v4 < v1);
    REQUIRE(v1 > v4);
}

