#include <catch2/catch_test_macros.hpp>

#include <functional>
#include <string>

#include "mylib/mylib.h"

namespace
{

    // ============================================================================
    // Вспомогательный класс для тестирования ArithmeticType
    // ============================================================================
    class TestInt final : public mylib::ArithmeticType<TestInt>
    {
    private:
        int m_int{};

    public:
        explicit constexpr TestInt(int i = 0)
            : m_int{ i }
        {}

        constexpr TestInt& operator=(const TestInt& other) noexcept
        {
            if(this == &other)
            {
                return *this;
            }

            m_int = other.get();

            return *this;
        }

        constexpr TestInt(TestInt&& other) noexcept
            : m_int{ other.m_int }
        {
            other.m_int = 0;
        }

        constexpr TestInt& operator=(TestInt&& other) noexcept
        {
            if(this == &other)
            {
                return *this;
            }

            m_int = other.m_int;
            other.m_int = 0;

            return *this;
        }
        constexpr TestInt(const TestInt& other) = default;

        constexpr bool operator==(int i) const noexcept{ return m_int == i; }
        constexpr bool operator==(const TestInt& other) const noexcept { return m_int == other.get(); }



        constexpr int get() const noexcept
        {
            return m_int;
        }

        constexpr TestInt& operator+=(const TestInt& other) noexcept
        {
            m_int += other.get();

            return *this;
        }

        constexpr TestInt& operator-=(const TestInt& other) noexcept
        {
            m_int -= other.get();

            return *this;
        }

        constexpr TestInt& operator*=(const TestInt& other) noexcept
        {
            m_int *= other.get();
            return *this;
        }

        constexpr TestInt& operator<<=(int shift) noexcept
        {
            m_int <<= shift;
            return *this;
        }

        constexpr TestInt& operator>>=(int shift) noexcept
        {
            m_int >>= shift;
            return *this;
        }

        constexpr TestInt& operator%=(const TestInt& other)
        {
            if(other.m_int == 0)
            {
                throw std::invalid_argument("modulo by zero");
            }
            m_int %= other.get();
            return *this;
        }

        constexpr TestInt& operator/=(const TestInt& other)
        {
            if(other.get() == 0)
            {
                throw std::invalid_argument("division by zero");
            }

            m_int /= other.get();

            return *this;
        }

    };



    struct Person
    {
        int age;
        std::string name;
    };

} // end namespace



TEST_CASE("KVPair struct test", "[KVPair]")
{
    mylib::KVPair<TestInt, std::string> emptyKVPair;
    CHECK(emptyKVPair.key == 0);
    CHECK(emptyKVPair.value == "");

    mylib::KVPair<TestInt, std::string> testKVPair{ TestInt{ 5 }, "hello" };
    CHECK(testKVPair.key == 5);
    CHECK(testKVPair.value == "hello");

    constexpr mylib::KVPair<int, int> constKVPair{ 5, 5 };
    CHECK(constKVPair.key == 5);
    CHECK(constKVPair.value == 5);
}



TEST_CASE("DefaultComparator", "[comparators]")
{
    mylib::comparators::DefaultComparator<int> comp;

    CHECK(comp(1, 2) == true);
    CHECK(comp(2, 1) == false);
    CHECK(comp(1, 1) == false);
    CHECK(comp.isEqual(1, 1) == true);
    CHECK(comp.isEqual(1, 2) == false);

    mylib::comparators::DefaultComparator<std::string> compStr;
    CHECK(compStr("abc", "abd") == true);
    CHECK(compStr("abd", "abc") == false);
    CHECK(compStr.isEqual("abc", "abc") == true);
    CHECK(compStr.isEqual("abc", "abd") == false);
}



TEST_CASE("ReverseComparator",  "[comparators]")
{
    mylib::comparators::ReverseComparator<int> rev;

    CHECK(rev(1, 2) == false);
    CHECK(rev(2, 1) == true);
    CHECK(rev(1, 1) == false);
    CHECK(rev.isEqual(1, 1) == true);
    CHECK(rev.isEqual(1, 2) == false);
}



TEST_CASE("TransformComparator", "[comparators]")
{
    auto transform{ [](int x){ return x * 2; } };
    using Comp = mylib::comparators::TransformComparator<int,
                                                         decltype(transform),
                                                         mylib::comparators::DefaultComparator<int> >;

    Comp comp{};

    CHECK(comp(1, 2) == true);
    CHECK(comp(1, 1) == false);
    CHECK(comp(2, 1) == false);

    CHECK(comp.isEqual(1, 1) == true);
    CHECK(comp.isEqual(1, 2) == false);
}



TEST_CASE("PointerTransform", "[comparators]")
{
    int a{ 10 };
    int b{ 20 };

    int* ptrA{ &a };
    int* ptrB{ &b };

    mylib::comparators::PointerTransform<int> trans;

    CHECK(trans(ptrA) == 10);
    CHECK(trans(ptrB) == 20);
}



TEST_CASE("PointerComparator", "[comparators]")
{
    int a{ 10 };
    int b{ 20 };

    int* ptrA{ &a };
    int* ptrB{ &b };

    mylib::comparators::PointerComparator<int> pointComp;

    CHECK(pointComp(ptrA, ptrB) == true);
    CHECK(pointComp(ptrB, ptrA) == false);

    CHECK(pointComp.isEqual(ptrA, ptrB) == false);
}



TEST_CASE("IndexTransform", "[comparators]")
{
    std::vector<int> vecInt{0, 1, 2, 3, 4, 5 };

    mylib::comparators::IndexTransform<int> trans{ vecInt.data() };

    CHECK(trans(0) == 0);
    CHECK(trans(5) == 5);
}



TEST_CASE("IndexComparator", "[comparators]")
{
    std::vector<int> vecInt{0, 1, 2, 2, 3, 3 };

    mylib::comparators::IndexComparator<int> indexComp{ vecInt.data() };
    mylib::comparators::IndexComparator<int, mylib::comparators::ReverseComparator<int>> indexRevComp{ vecInt.data() };

    CHECK(indexComp(0, 1) == true);
    CHECK(indexComp(2, 1) == false);

    CHECK(indexComp.isEqual(4, 5) == true);

    CHECK(indexRevComp(0, 1) == false);
    CHECK(indexRevComp(2, 1) == true);

    CHECK(indexRevComp.isEqual(4, 5) == true);

}



TEST_CASE("PairFirstTransform", "[comparators]")
{
    using Pair = std::pair<int, std::string>;

    Pair p1{1, "one"};
    Pair p2{2, "two"};

    mylib::comparators::PairFirstTransform<int, std::string> trans;

    CHECK(trans(p1) == 1);
    CHECK(trans(p2) == 2);
    CHECK(trans(p1) != trans(p2));
}



TEST_CASE("PairFirstComparator", "[comparators]")
{
    using Pair = std::pair<int, std::string>;

    Pair p1{1, "one"};
    Pair p2{2, "two"};

    mylib::comparators::PairFirstComparator<int, std::string, mylib::comparators::ReverseComparator<int>> comp;

    CHECK(comp(p1, p2) == false);
    CHECK(comp(p2, p1) == true);

    CHECK(comp.isEqual(p1, p1) == true);
    CHECK(comp.isEqual(p1, p2) == false);
}



TEST_CASE("LexicographicComparator", "[comparators]")
{
    std::vector<int> v1{ 1, 2, 3 };
    std::vector<int> v2{ 1, 2, 4 };
    std::vector<int> v3{ 1, 2 };
    std::vector<int> v4{ 1, 2, 3 };

    mylib::comparators::LexicographicComparator<std::vector<int>> comp;

    CHECK(comp(v1, v2) == true);
    CHECK(comp(v2, v1) == false);
    CHECK(comp(v1, v3) == false);   // v1 длиннее, префикс совпадает => v1 > v3
    CHECK(comp(v3, v1) == true);    // v3 короче
    CHECK(comp(v1, v4) == false);   // равны
    CHECK(comp.isEqual(v1, v4) == true);
    CHECK(comp.isEqual(v1, v2) == false);

    // Сравнение с индексом
    CHECK(comp(v1, v2, 0) == false); // 1<1 false
    CHECK(comp(v1, v2, 2) == true);  // 3<4 true
    CHECK(comp.isEqual(v1, v4, 0) == true);
    CHECK(comp.isEqual(v1, v2, 2) == false);

    mylib::comparators::LexicographicComparator<std::string> compStr;

    std::string s1 = "abc";
    std::string s2 = "abd";
    std::string s3 = "ab";
    CHECK(compStr(s1, s2) == true);
    CHECK(compStr(s2, s1) == false);
    CHECK(compStr(s1, s3) == false);
    CHECK(compStr(s3, s1) == true);
    CHECK(compStr.isEqual(s1, s1) == true);
}



TEST_CASE("argMin / argMax", "[algorithms]")
{
    std::vector<int> vec{ 3, 1, 4, 1, 5 };

    SECTION("argMin")
    {
        CHECK(mylib::comparators::argMin(vec.data(), vec.size()) == 1);
    }

    SECTION("argMax")
    {
        CHECK(mylib::comparators::argMax(vec.data(), vec.size()) == 4);
    }
}

TEST_CASE("valMin / valMax", "[algorithms]")
{
    std::vector<int> vec{ 3, 1, 4, 1, 5 };

    SECTION("valMin non-const")
    {
        CHECK(mylib::comparators::argMin(vec.data(), vec.size()) == 1);
        int& val{ mylib::comparators::valMin<int>(vec.data(), vec.size()) };
        CHECK(val == 1);
        val = 10;
        CHECK(vec[1] == 10);
    }

    SECTION("valMin const")
    {
        const int& minRef = mylib::comparators::valMin(vec.data(), vec.size());
        CHECK(minRef == 1);
    }

    SECTION("valMax non-const")
    {
        int& maxRef = mylib::comparators::valMax(vec.data(), vec.size());
        CHECK(maxRef == 5);
        maxRef = 99;
        CHECK(vec[4] == 99);
    }

    SECTION("valMax const")
    {
        const int& maxRef = mylib::comparators::valMax(vec.data(), vec.size());
        CHECK(maxRef == 5);
    }
}



TEST_CASE("argMinFunc / valMinFunc", "[algorithms]")
{
    Person people[] = {
        {25, "Alice"},
        {30, "Bob"},
        {20, "Charlie"},
        {25, "David"}
    };

    auto ageAccessor = [](const Person& p) { return p.age; };

    SECTION("argMinFunc")
    {
        std::size_t idx = mylib::comparators::argMinFunc(people, 4, ageAccessor);
        CHECK(idx == 2); // Charlie (age 20)
    }

    SECTION("valMinFunc non-const")
    {
        Person& p = mylib::comparators::valMinFunc(people, 4, ageAccessor);
        CHECK(p.age == 20);
        CHECK(p.name == "Charlie");
        p.age = 35; // изменяем
        CHECK(people[2].age == 35);
    }
}



TEST_CASE("ArithmeticType operators", "[ArithmeticType]")
{
    TestInt a{10};
    TestInt b{3};

    SECTION("operator+")
    {
        TestInt c = a + b;
        CHECK(c.get() == 13);
        CHECK(a.get() == 10); // исходные не изменились
        CHECK(b.get() == 3);
    }

    SECTION("operator-")
    {
        TestInt c = a - b;
        CHECK(c.get() == 7);
    }

    SECTION("operator*")
    {
        TestInt c = a * b;
        CHECK(c.get() == 30);
    }

    SECTION("operator/")
    {
        TestInt c = a / b;
        CHECK(c.get() == 3);
        // деление на ноль
        TestInt zero{0};
        REQUIRE_THROWS_AS(a / zero, std::invalid_argument);
    }

    SECTION("operator%")
    {
        TestInt c = a % b;
        CHECK(c.get() == 1);
        TestInt zero{0};
        REQUIRE_THROWS_AS(a % zero, std::invalid_argument);
    }

    SECTION("operator<<")
    {
        TestInt c = a << 2;
        CHECK(c.get() == 40);
    }

    SECTION("operator>>")
    {
        TestInt c = a >> 1;
        CHECK(c.get() == 5);
    }

    SECTION("constexpr context")
    {
        constexpr TestInt ca{5};
        constexpr TestInt cb{3};
        constexpr TestInt cc = ca + cb;
        static_assert(cc.get() == 8, "constexpr addition failed");
        // В рантайме тоже проверим
        CHECK(cc.get() == 8);
    }
}



// ============================================================================
// Дополнительно: проверка constexpr для компараторов
// ============================================================================
TEST_CASE("constexpr comparators", "[constexpr]")
{
    constexpr mylib::comparators::DefaultComparator<int> comp;
    static_assert(comp(1, 2) == true);
    static_assert(comp.isEqual(1, 1) == true);
    static_assert(comp.isEqual(1, 2) == false);

    constexpr mylib::comparators::ReverseComparator<int> rev;
    static_assert(rev(2, 1) == true); // т.к. 1 < 2 в обратном порядке => true
    static_assert(rev.isEqual(1, 1) == true);
}
