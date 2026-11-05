#ifndef TEST_ALGORITHMS_H
#define TEST_ALGORITHMS_H

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <list>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "mylib/mylib.h"


// ============================================================================
//  insertSort (raw pointer + indices, right inclusive)
// ============================================================================
TEST_CASE("insertSort raw array — basic scenarios",
          "[algorithms][insertSort][array]")
{
    using mylib::algorithms::insertSort;

    SECTION("empty range (left > right) is no-op")
    {
        int arr[]{ 3, 1, 2 };
        // left=2, right=1 — пустой диапазон
        insertSort(arr, 2, 1);
        REQUIRE(arr[0] == 3);
        REQUIRE(arr[1] == 1);
        REQUIRE(arr[2] == 2);
    }

    SECTION("single element (left == right) is no-op")
    {
        int arr[]{ 42 };
        insertSort(arr, 0, 0);
        REQUIRE(arr[0] == 42);
    }

    SECTION("two elements")
    {
        int a[]{ 2, 1 };
        insertSort(a, 0, 1);
        REQUIRE(a[0] == 1);
        REQUIRE(a[1] == 2);

        int b[]{ 1, 2 };
        insertSort(b, 0, 1);
        REQUIRE(b[0] == 1);
        REQUIRE(b[1] == 2);
    }

    SECTION("already sorted stays sorted")
    {
        int arr[]{ 1, 2, 3, 4, 5 };
        insertSort(arr, 0, 4);
        for (int i = 0; i < 5; ++i) REQUIRE(arr[i] == i + 1);
    }

    SECTION("reverse sorted becomes sorted")
    {
        int arr[]{ 5, 4, 3, 2, 1 };
        insertSort(arr, 0, 4);
        for (int i = 0; i < 5; ++i) REQUIRE(arr[i] == i + 1);
    }

    SECTION("random permutation")
    {
        int arr[]{ 3, 7, 1, 9, 2, 8, 4, 6, 5 };
        insertSort(arr, 0, 8);
        REQUIRE(std::is_sorted(arr, arr + 9));
        REQUIRE(arr[0] == 1);
        REQUIRE(arr[8] == 9);
    }

    SECTION("with duplicates")
    {
        int arr[]{ 3, 1, 2, 3, 1, 2, 1 };
        insertSort(arr, 0, 6);
        REQUIRE(std::is_sorted(arr, arr + 7));
        REQUIRE(arr[0] == 1);
        REQUIRE(arr[1] == 1);
        REQUIRE(arr[2] == 1);
        REQUIRE(arr[6] == 3);
    }

    SECTION("all identical")
    {
        int arr[]{ 7, 7, 7, 7, 7 };
        insertSort(arr, 0, 4);
        for (int x : arr) REQUIRE(x == 7);
    }

    SECTION("subrange sorting — outside untouched")
    {
        int arr[]{ 99, 3, 1, 2, 88 };
        insertSort(arr, 1, 3);   // сортируем только [1..3] = {3,1,2}
        REQUIRE(arr[0] == 99);   // не тронут
        REQUIRE(arr[4] == 88);   // не тронут
        REQUIRE(arr[1] == 1);
        REQUIRE(arr[2] == 2);
        REQUIRE(arr[3] == 3);
    }

    SECTION("negative values")
    {
        int arr[]{ -3, 5, -1, 0, -10 };
        insertSort(arr, 0, 4);
        REQUIRE(std::is_sorted(arr, arr + 5));
        REQUIRE(arr[0] == -10);
        REQUIRE(arr[4] == 5);
    }

    SECTION("double values")
    {
        double arr[]{ 3.14, 1.41, 2.71, 1.73 };
        insertSort(arr, 0, 3);
        REQUIRE(std::is_sorted(arr, arr + 4));
    }
}


// ============================================================================
//  insertSort — стабильность и компаратор
// ============================================================================
namespace
{
struct Tagged
{
    int key;
    int id;

    bool operator<(const Tagged& o)  const noexcept { return key < o.key; }
    bool operator<=(const Tagged& o) const noexcept { return key <= o.key; }
    bool operator==(const Tagged& o) const noexcept { return key == o.key && id == o.id; }
};
}

TEST_CASE("insertSort raw array — stability and comparator",
          "[algorithms][insertSort][array]")
{
    using mylib::algorithms::insertSort;

    SECTION("is stable: same key keeps original order")
    {
        // Все с key == 1: порядок id должен сохраниться
        Tagged arr[]{
            { 1, 0 }, { 2, 1 }, { 1, 2 }, { 2, 3 }, { 1, 4 }, { 2, 5 }
        };
        insertSort(arr, 0, 5);

        // Ключи отсортированы
        for (int i = 0; i < 5; ++i) REQUIRE(arr[i].key <= arr[i + 1].key);

        // Для key == 1: id должны идти 0, 2, 4
        std::vector<int> ones, twos;
        for (const auto& t : arr)
        {
            if (t.key == 1) ones.push_back(t.id);
            else            twos.push_back(t.id);
        }
        REQUIRE(ones == std::vector<int>{ 0, 2, 4 });
        REQUIRE(twos == std::vector<int>{ 1, 3, 5 });
    }

    SECTION("custom comparator: descending order")
    {
        int arr[]{ 1, 5, 3, 2, 4 };
        auto desc = [](const int& a, const int& b) { return a > b; };
        insertSort(arr, 0, 4, desc);
        REQUIRE(std::is_sorted(arr, arr + 5, std::greater<int>{}));
        REQUIRE(arr[0] == 5);
        REQUIRE(arr[4] == 1);
    }

    SECTION("custom comparator: by absolute value")
    {
        int arr[]{ -5, 3, -1, 4, -2 };
        auto byAbs = [](const int& a, const int& b) {
            return std::abs(a) < std::abs(b);
        };
        insertSort(arr, 0, 4, byAbs);
        // Ожидаемый порядок по |x|: -1(1), -2(2), 3(3), 4(4), -5(5)
        // Стабильность: -1, -2, 3, 4, -5
        REQUIRE(arr[0] == -1);
        REQUIRE(arr[1] == -2);
        REQUIRE(arr[2] == 3);
        REQUIRE(arr[3] == 4);
        REQUIRE(arr[4] == -5);
    }
}


// ============================================================================
//  insertSort — сравнение с std::sort
// ============================================================================
TEST_CASE("insertSort raw array — agreement with std::sort",
          "[algorithms][insertSort][array][reference]")
{
    using mylib::algorithms::insertSort;

    SECTION("random ints")
    {
        std::mt19937 rng{ 42 };
        std::uniform_int_distribution<int> dist{ -1000, 1000 };

        for (int trial = 0; trial < 20; ++trial)
        {
            constexpr size_t N{ 50 };
            std::array<int, N> a{}, b{};
            for (auto& x : a) x = dist(rng);
            b = a;

            insertSort(a.data(), 0, N - 1);
            std::sort(b.begin(), b.end());

            INFO("trial = " << trial);
            REQUIRE(a == b);
        }
    }

    SECTION("already sorted — O(N) best case, still correct")
    {
        std::array<int, 100> a{};
        std::iota(a.begin(), a.end(), 0);
        auto b = a;
        insertSort(a.data(), 0, 99);
        REQUIRE(a == b);
    }
}


TEST_CASE("insertSort raw array — LessAndEqualComparable constraint",
          "[algorithms][insertSort][array][traits]")
{
    // Концепт: требует < И ==, оба convertible_to<bool>.
    struct HasLessAndEq
    {
        int v;
        bool operator<(const HasLessAndEq& o)  const noexcept { return v < o.v; }
        bool operator==(const HasLessAndEq& o) const noexcept { return v == o.v; }
    };
    struct OnlyLess
    {
        int v;
        bool operator<(const OnlyLess& o) const noexcept { return v < o.v; }
        // нет ==
    };
    struct OnlyEq
    {
        int v;
        bool operator==(const OnlyEq& o) const noexcept { return v == o.v; }
        // нет <
    };
    struct OnlyLe
    {
        int v;
        bool operator<=(const OnlyLe& o) const noexcept { return v <= o.v; }
        // есть <=, но нет ни <, ни ==
    };
    struct NonBoolCompare
    {
        int v;
        int operator<(const NonBoolCompare&)  const { return 0; }   // int, не bool
        int operator==(const NonBoolCompare&) const { return 0; }
    };

    // Проходит: есть < и ==
    STATIC_REQUIRE(requires(HasLessAndEq* p, size_t l, size_t r) {
        mylib::algorithms::insertSort(p, l, r);
    });

    // Не проходит: нет ==
    STATIC_REQUIRE_FALSE(requires(OnlyLess* p, size_t l, size_t r) {
        mylib::algorithms::insertSort(p, l, r);
    });

    // Не проходит: нет <
    STATIC_REQUIRE_FALSE(requires(OnlyEq* p, size_t l, size_t r) {
        mylib::algorithms::insertSort(p, l, r);
    });

    // Не проходит: есть только <= (не ==)
    STATIC_REQUIRE_FALSE(requires(OnlyLe* p, size_t l, size_t r) {
        mylib::algorithms::insertSort(p, l, r);
    });

    // Не проходит: операторы возвращают не-bool (int convertible to bool — на самом деле проходит!)
    // ВНИМАНИЕ: convertible_to<bool> разрешает int. Проверим, что проходит.
    STATIC_REQUIRE(requires(NonBoolCompare* p, size_t l, size_t r) {
        mylib::algorithms::insertSort(p, l, r);
    });
}


// ============================================================================
//  insertSort (iterator range)
// ============================================================================
TEST_CASE("insertSort iterator range — basic scenarios",
          "[algorithms][insertSort][iterator]")
{
    using mylib::algorithms::insertSort;

    SECTION("empty range")
    {
        std::vector<int> v;
        insertSort(v.begin(), v.end());
        REQUIRE(v.empty());
    }

    SECTION("single element")
    {
        std::vector<int> v{ 42 };
        insertSort(v.begin(), v.end());
        REQUIRE(v == std::vector<int>{ 42 });
    }

    SECTION("two elements")
    {
        std::vector<int> v{ 2, 1 };
        insertSort(v.begin(), v.end());
        REQUIRE(v == std::vector<int>{ 1, 2 });
    }

    SECTION("already sorted")
    {
        std::vector<int> v{ 1, 2, 3, 4, 5 };
        insertSort(v.begin(), v.end());
        REQUIRE(v == std::vector<int>{ 1, 2, 3, 4, 5 });
    }

    SECTION("reverse sorted")
    {
        std::vector<int> v{ 5, 4, 3, 2, 1 };
        insertSort(v.begin(), v.end());
        REQUIRE(v == std::vector<int>{ 1, 2, 3, 4, 5 });
    }

    SECTION("random with duplicates")
    {
        std::vector<int> v{ 3, 1, 4, 1, 5, 9, 2, 6, 5, 3 };
        insertSort(v.begin(), v.end());
        REQUIRE(std::is_sorted(v.begin(), v.end()));
        REQUIRE(v.front() == 1);
        REQUIRE(v.back() == 9);
    }

    SECTION("subrange sorting — outside untouched")
    {
        std::vector<int> v{ 99, 3, 1, 2, 88 };
        insertSort(v.begin() + 1, v.begin() + 4);   // [begin+1, begin+4) = {3,1,2}
        REQUIRE(v[0] == 99);
        REQUIRE(v[4] == 88);
        REQUIRE(v[1] == 1);
        REQUIRE(v[2] == 2);
        REQUIRE(v[3] == 3);
    }

    SECTION("works with std::list (bidirectional)")
    {
        std::list<int> l{ 3, 1, 4, 1, 5, 9, 2, 6 };
        insertSort(l.begin(), l.end());
        REQUIRE(std::is_sorted(l.begin(), l.end()));
        REQUIRE(l.front() == 1);
        REQUIRE(l.back() == 9);
    }

    SECTION("works with std::array")
    {
        std::array<int, 5> arr{ 3, 1, 4, 1, 5 };
        insertSort(arr.begin(), arr.end());
        REQUIRE(std::is_sorted(arr.begin(), arr.end()));
    }

    SECTION("works with mylib::Vector")
    {
        mylib::Vector<int> v{ 4, 1, 3, 2 };
        insertSort(v.begin(), v.end());
        REQUIRE(std::is_sorted(v.begin(), v.end()));
        REQUIRE(v[0] == 1);
        REQUIRE(v[3] == 4);
    }

    SECTION("works with std::string characters")
    {
        std::string s{ "dbca" };
        insertSort(s.begin(), s.end());
        REQUIRE(s == "abcd");
    }
}


// ============================================================================
//  insertSort — стабильность и компаратор
// ============================================================================
TEST_CASE("insertSort iterator range — stability and comparator",
          "[algorithms][insertSort][iterator]")
{
    using mylib::algorithms::insertSort;

    SECTION("is stable with duplicates by key")
    {
        std::vector<Tagged> v{
            { 1, 0 }, { 2, 1 }, { 1, 2 }, { 2, 3 }, { 1, 4 }, { 2, 5 }
        };
        insertSort(v.begin(), v.end());

        // Ключи отсортированы
        for (size_t i = 1; i < v.size(); ++i)
            REQUIRE(v[i - 1].key <= v[i].key);

        // Для key == 1: id идут 0, 2, 4
        std::vector<int> ones;
        for (const auto& t : v) if (t.key == 1) ones.push_back(t.id);
        REQUIRE(ones == std::vector<int>{ 0, 2, 4 });
    }

    SECTION("custom comparator: descending")
    {
        std::vector<int> v{ 1, 5, 3, 2, 4 };
        auto desc = [](const int& a, const int& b) { return a > b; };
        insertSort(v.begin(), v.end(), desc);
        REQUIRE(std::is_sorted(v.begin(), v.end(), std::greater<int>{}));
    }

    SECTION("custom comparator: by absolute value")
    {
        std::vector<int> v{ -5, 3, -1, 4, -2 };
        insertSort(v.begin(), v.end(), [](int a, int b) {
            return std::abs(a) < std::abs(b);
        });
        REQUIRE(v[0] == -1);
        REQUIRE(v[1] == -2);
        REQUIRE(v[2] == 3);
        REQUIRE(v[3] == 4);
        REQUIRE(v[4] == -5);
    }

    SECTION("custom comparator with std::list")
    {
        std::list<int> l{ 3, 1, 2 };
        insertSort(l.begin(), l.end(), std::greater<int>{});
        REQUIRE(*l.begin() == 3);
    }
}


// ============================================================================
//  insertSort — тяжёлый тип: std::string (move-семантика)
// ============================================================================
namespace
{
struct MoveTracker
{
    static inline int copies = 0;
    static inline int moves  = 0;

    std::string payload;

    explicit MoveTracker(std::string p) : payload(std::move(p)) {}

    MoveTracker(const MoveTracker& o) : payload(o.payload) { ++copies; }
    MoveTracker(MoveTracker&& o) noexcept : payload(std::move(o.payload)) { ++moves; }

    MoveTracker& operator=(const MoveTracker& o)
    {
        payload = o.payload;
        ++copies;
        return *this;
    }
    MoveTracker& operator=(MoveTracker&& o) noexcept
    {
        payload = std::move(o.payload);
        ++moves;
        return *this;
    }

    bool operator<(const MoveTracker& o)  const noexcept { return payload <  o.payload; }
    bool operator<=(const MoveTracker& o) const noexcept { return payload <= o.payload; }

    static void reset() noexcept { copies = 0; moves = 0; }
};
}

TEST_CASE("insertSort — move semantics for heavy types",
          "[algorithms][insertSort][move]")
{
    using mylib::algorithms::insertSort;

    SECTION("move is used, not just copy")
    {
        std::vector<MoveTracker> v{
                                   MoveTracker{ "dddddddddddddddd" },
                                   MoveTracker{ "aaaaaaaaaaaaaaaa" },
                                   MoveTracker{ "cccccccccccccccc" },
                                   MoveTracker{ "bbbbbbbbbbbbbbbb" },
                                   };

        MoveTracker::reset();
        insertSort(v.begin(), v.end());

        INFO("copies = " << MoveTracker::copies
                         << ", moves = " << MoveTracker::moves);
        // std::move активно используется -> moves >> copies
        REQUIRE(MoveTracker::moves > 0);
        REQUIRE(MoveTracker::moves > MoveTracker::copies);
    }

    SECTION("std::string array sorted correctly")
    {
        std::array<std::string, 5> arr{
            "epsilon", "alpha", "delta", "beta", "gamma"
        };
        insertSort(arr.begin(), arr.end());
        REQUIRE(std::is_sorted(arr.begin(), arr.end()));
        REQUIRE(arr[0] == "alpha");
        REQUIRE(arr[4] == "epsilon");
    }

    SECTION("very long strings")
    {
        std::string longA(1000, 'a');
        std::string longZ(1000, 'z');
        std::array<std::string, 2> arr{ longZ, longA };
        insertSort(arr.begin(), arr.end());
        REQUIRE(arr[0] == longA);
        REQUIRE(arr[1] == longZ);
    }
}


// ============================================================================
//  insertSort — сравнение с std::sort и std::stable_sort
// ============================================================================
TEST_CASE("insertSort iterator range — agreement with std::stable_sort",
          "[algorithms][insertSort][iterator][reference]")
{
    using mylib::algorithms::insertSort;

    SECTION("random ints match std::sort")
    {
        std::mt19937 rng{ 42 };
        std::uniform_int_distribution<int> dist{ -100, 100 };

        for (int trial = 0; trial < 20; ++trial)
        {
            constexpr size_t N{ 40 };
            std::vector<int> a(N), b(N);
            for (auto& x : a) x = dist(rng);
            b = a;

            insertSort(a.begin(), a.end());
            std::sort(b.begin(), b.end());

            INFO("trial = " << trial);
            REQUIRE(a == b);
        }
    }

    SECTION("stable result matches std::stable_sort with keys")
    {
        std::mt19937 rng{ 42 };
        std::uniform_int_distribution<int> key{ 0, 5 };

        std::vector<Tagged> src;
        for (int id = 0; id < 100; ++id)
            src.push_back({ key(rng), id });

        auto a = src, b = src;
        insertSort(a.begin(), a.end());
        std::stable_sort(b.begin(), b.end());

        REQUIRE(a.size() == b.size());
        for (size_t i = 0; i < a.size(); ++i)
        {
            INFO("i = " << i);
            REQUIRE(a[i].key == b[i].key);
            REQUIRE(a[i].id  == b[i].id);   // stability preserved
        }
    }
}


// ============================================================================
//  insertSort — типы итераторов и constraint
// ============================================================================
TEST_CASE("insertSort iterator range — iterator types and constraints",
          "[algorithms][insertSort][iterator][traits]")
{
    using mylib::algorithms::insertSort;

    SECTION("works with bidirectional, random access, contiguous")
    {
        // bidirectional: std::list
        STATIC_REQUIRE(requires(std::list<int>& l) {
            insertSort(l.begin(), l.end());
        });
        // random_access: std::vector
        STATIC_REQUIRE(requires(std::vector<int>& v) {
            insertSort(v.begin(), v.end());
        });
        // contiguous: int[]
        STATIC_REQUIRE(requires(int* p, int* q) {
            insertSort(p, q);
        });
    }

    SECTION("works with const-correct value types")
    {
        // Только для mutable контейнеров (sorting modifies)
        STATIC_REQUIRE_FALSE(requires(const std::vector<int>& v) {
            insertSort(v.begin(), v.end());
        });
    }

    SECTION("return type is void")
    {
        std::vector<int> v{ 2, 1 };
        STATIC_REQUIRE(std::is_void_v<
                       decltype(insertSort(v.begin(), v.end()))
                       >);
    }
}


// ============================================================================
//  insertSort — согласованность двух перегрузок
// ============================================================================
TEST_CASE("insertSort — raw-array and iterator overloads agree",
          "[algorithms][insertSort][consistency]")
{
    using mylib::algorithms::insertSort;

    SECTION("same data → same result")
    {
        std::mt19937 rng{ 7 };
        std::uniform_int_distribution<int> dist{ -50, 50 };

        for (int trial = 0; trial < 10; ++trial)
        {
            std::array<int, 20> raw{};
            for (auto& x : raw) x = dist(rng);

            std::vector<int> viaIter{ raw.begin(), raw.end() };

            // raw inclusive: right = N-1
            insertSort(raw.data(), 0, raw.size() - 1);
            insertSort(viaIter.begin(), viaIter.end());

            INFO("trial = " << trial);
            for (size_t i = 0; i < raw.size(); ++i)
                REQUIRE(raw[i] == viaIter[i]);
        }
    }
}
#endif // TEST_ALGORITHMS_H
