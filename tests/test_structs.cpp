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
        explicit TestInt(int i = 0)
            : m_int{ i }
        {}

        TestInt& operator=(const TestInt& other) noexcept
        {
            if(this == &other)
            {
                return *this;
            }

            m_int = other.get();

            return *this;
        }

        TestInt(TestInt&& other) noexcept
            : m_int{ other.m_int }
        {
            other.m_int = 0;
        }

        TestInt& operator=(TestInt&& other) noexcept
        {
            if(this == &other)
            {
                return *this;
            }

            m_int = other.m_int;
            other.m_int = 0;

            return *this;
        }
        TestInt(const TestInt& other) = default;

        bool operator==(int i) const noexcept{ return m_int == i; }
        bool operator==(const TestInt& other) const noexcept { return m_int == other.get(); }



        int get() const noexcept
        {
            return m_int;
        }

        TestInt& operator+=(const TestInt& other) noexcept
        {
            m_int += other.get();

            return *this;
        }

        TestInt& operator-=(const TestInt& other) noexcept
        {
            m_int -= other.get();

            return *this;
        }

        TestInt& operator*=(const TestInt& other) noexcept
        {
            m_int *= other.get();
            return *this;
        }

        TestInt& operator<<=(int shift) noexcept
        {
            m_int <<= shift;
            return *this;
        }

        TestInt& operator>>=(int shift) noexcept
        {
            m_int >>= shift;
            return *this;
        }

        TestInt& operator%=(const TestInt& other)
        {
            if(other.m_int == 0)
            {
                throw std::invalid_argument("modulo by zero");
            }
            m_int %= other.get();
            return *this;
        }

        TestInt& operator/=(const TestInt& other)
        {
            if(other.get() == 0)
            {
                throw std::invalid_argument("division by zero");
            }

            m_int /= other.get();

            return *this;
        }

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

    mylib::comparators::PairFirstComparator<int, std::string, mylib::comparators::ReverseComparator<int>> trans;

    CHECK(trans(p1, p2) == true);
    CHECK(trans(p2, p1) == false);

}
