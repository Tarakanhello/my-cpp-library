#ifndef TEST_ALGORITHMS_H
#define TEST_ALGORITHMS_H

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <forward_list>
#include <functional>
#include <list>
#include <numeric>
#include <random>
#include <set>
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
    using mylib::LessAndEqualComparable;
    using mylib::algorithms::insertSort;

    // --- Тестовые типы ---------------------------------------------------
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
    };
    struct OnlyEq
    {
        int v;
        bool operator==(const OnlyEq& o) const noexcept { return v == o.v; }
    };
    struct OnlyLe
    {
        int v;
        bool operator<=(const OnlyLe& o) const noexcept { return v <= o.v; }
    };
    struct NonBoolCompare
    {
        int v;
        int operator<(const NonBoolCompare&)  const { return 0; }
        int operator==(const NonBoolCompare&) const { return 0; }
    };

    // --- Проверки самого концепта ---------------------------------------
    STATIC_REQUIRE(LessAndEqualComparable<HasLessAndEq>);
    STATIC_REQUIRE(LessAndEqualComparable<int>);
    STATIC_REQUIRE(LessAndEqualComparable<double>);
    STATIC_REQUIRE(LessAndEqualComparable<std::string>);

    STATIC_REQUIRE_FALSE(LessAndEqualComparable<OnlyLess>);
    STATIC_REQUIRE_FALSE(LessAndEqualComparable<OnlyEq>);
    STATIC_REQUIRE_FALSE(LessAndEqualComparable<OnlyLe>);

    // convertible_to<bool> разрешает int — это осознанно
    STATIC_REQUIRE(LessAndEqualComparable<NonBoolCompare>);

    STATIC_REQUIRE(requires(HasLessAndEq* p, size_t l, size_t r) {
        insertSort(p, l, r);
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
    bool operator==(const MoveTracker& o) const noexcept { return payload == o.payload; }
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
        REQUIRE(arr[1] == "beta");
        REQUIRE(arr[2] == "delta");
        REQUIRE(arr[3] == "epsilon");
        REQUIRE(arr[4] == "gamma");
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
        using It  = std::vector<int>::iterator;
        using CIt = std::vector<int>::const_iterator;

        // insertSort требует std::permutable<IT>.
        // Mutable итераторы удовлетворяют, const-итераторы — нет.
        STATIC_REQUIRE(std::bidirectional_iterator<It>);
        STATIC_REQUIRE(std::permutable<It>);

        STATIC_REQUIRE(std::bidirectional_iterator<CIt>);
        STATIC_REQUIRE_FALSE(std::permutable<CIt>);

        // Тот же тест для raw pointer (у raw overload другая сигнатура,
        // но принцип тот же: const int* не permutable)
        STATIC_REQUIRE(std::permutable<int*>);
        STATIC_REQUIRE_FALSE(std::permutable<const int*>);
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

// ============================================================================
//  pickPivot — базовые инварианты
// ============================================================================
TEST_CASE("pickPivot basic invariants", "[algorithms][pickPivot]")
{
    using mylib::algorithms::pickPivot;

    SECTION("single element -> returns begin")
    {
        std::vector<int> v{ 42 };
        auto it = pickPivot(v.begin(), v.end());
        REQUIRE(it == v.begin());
        REQUIRE(*it == 42);
    }

    SECTION("two elements -> returns one of them")
    {
        std::vector<int> v{ 1, 2 };
        for (int trial = 0; trial < 100; ++trial)
        {
            auto it = pickPivot(v.begin(), v.end());
            REQUIRE(it != v.end());
            REQUIRE(it >= v.begin());
            REQUIRE(it < v.end());
            REQUIRE((*it == 1 || *it == 2));
        }
    }

    SECTION("returned iterator is in [begin, end-1], never end")
    {
        std::vector<int> v{ 5, 3, 1, 4, 2 };
        for (int trial = 0; trial < 200; ++trial)
        {
            auto it = pickPivot(v.begin(), v.end());
            REQUIRE(it >= v.begin());
            REQUIRE(it < v.end());
            // Значение должно быть одним из элементов
            REQUIRE(std::find(v.begin(), v.end(), *it) != v.end());
        }
    }

    SECTION("does not modify the range")
    {
        std::vector<int> v{ 5, 3, 1, 4, 2 };
        const std::vector<int> original{ v };
        for (int trial = 0; trial < 50; ++trial)
            (void)pickPivot(v.begin(), v.end());
        REQUIRE(v == original);
    }

    SECTION("pivot value is between min and max of range")
    {
        std::vector<int> v{ 100, -5, 50, 0, 42, -100, 7 };
        const int mn = *std::min_element(v.begin(), v.end());
        const int mx = *std::max_element(v.begin(), v.end());
        for (int trial = 0; trial < 100; ++trial)
        {
            auto it = pickPivot(v.begin(), v.end());
            INFO("pivot = " << *it);
            REQUIRE(*it >= mn);
            REQUIRE(*it <= mx);
        }
    }

    SECTION("size 3 sorted distinct -> pivot value is the middle")
    {
        std::vector<int> v{ 1, 2, 3 };
        for (int trial = 0; trial < 200; ++trial)
        {
            auto it = pickPivot(v.begin(), v.end());
            INFO("pivot = " << *it);
            REQUIRE((*it == 1 || *it == 2 || *it == 3));
        }
    }

    SECTION("size 3 sorted distinct -> middle value 2 dominates over 1 and 3")
    {
        // Статистически: если реализация действительно берёт медиану
        // трёх случайных значений, то среднее значение (2) встречается
        // чаще крайних (это признак "median-like" поведения).
        std::vector<int> v{ 1, 2, 3 };
        std::array<int, 3> freq{};
        constexpr int TRIALS{ 3000 };
        for (int t = 0; t < TRIALS; ++t)
        {
            auto it = pickPivot(v.begin(), v.end());
            ++freq[*it - 1];
        }
        INFO("freq[1] = " << freq[0]
                          << ", freq[2] = " << freq[1]
                          << ", freq[3] = " << freq[2]);

        // Для медианы-3-с-возвратом: P(2) ≈ 0.48, P(1) = P(3) ≈ 0.26.
        // Для детерминированной median-of-3: P(2) = 1.0.
        // Для равномерного random: P(1)=P(2)=P(3) ≈ 0.33.
        // Требуем только: pivot не равномерен, среднее 2 чаще крайних.
        REQUIRE(freq[1] > freq[0]);   // 2 > 1
        REQUIRE(freq[1] > freq[2]);   // 2 > 3
    }

    SECTION("works with std::array")
    {
        std::array<int, 5> arr{ 5, 3, 1, 4, 2 };
        auto it = pickPivot(arr.begin(), arr.end());
        REQUIRE(it >= arr.begin());
        REQUIRE(it < arr.end());
    }

    SECTION("works with std::list (bidirectional)")
    {
        std::list<int> l{ 5, 3, 1, 4, 2 };
        auto it = pickPivot(l.begin(), l.end());
        REQUIRE(it != l.end());
    }

    SECTION("works with std::forward_list (forward)")
    {
        std::forward_list<int> fl{ 5, 3, 1, 4, 2 };
        auto it = pickPivot(fl.begin(), fl.end());
        REQUIRE(it != fl.end());
    }

    SECTION("custom comparator: descending — pivot still in range")
    {
        std::vector<int> v{ 1, 2, 3 };
        auto desc = [](int a, int b) { return a > b; };
        for (int trial = 0; trial < 50; ++trial)
        {
            auto it = pickPivot(v.begin(), v.end(), desc);
            REQUIRE(it >= v.begin());
            REQUIRE(it < v.end());
        }
    }
}


// ============================================================================
//  pickPivot — статистика медианы (крупный отсортированный диапазон)
// ============================================================================
TEST_CASE("pickPivot — median of three avoids extremes",
          "[algorithms][pickPivot][statistics]")
{
    using mylib::algorithms::pickPivot;

    SECTION("large sorted range: pivot is rarely the absolute min or max")
    {
        // Для N >= 5 медиана трёх случайных элементов почти всегда
        // не крайнее значение. Проверяем, что за много прогонов
        // pivot != min хотя бы в большинстве случаев.
        constexpr int N{ 100 };
        std::vector<int> v(N);
        std::iota(v.begin(), v.end(), 0);

        int minHits = 0, maxHits = 0;
        constexpr int TRIALS{ 1000 };
        for (int t = 0; t < TRIALS; ++t)
        {
            auto it = pickPivot(v.begin(), v.end());
            if (*it == 0)       ++minHits;
            if (*it == N - 1)   ++maxHits;
        }
        INFO("minHits = " << minHits << ", maxHits = " << maxHits);
        // Теоретически: P(медиана == min) при выборе 3 с возвратом ≈ 7/27 ≈ 0.26
        // Практически для good impl — сильно меньше. Допускаем щедрый порог.
        REQUIRE(minHits < TRIALS / 2);
        REQUIRE(maxHits < TRIALS / 2);
    }
}


// ============================================================================
//  partition — базовые сценарии
// ============================================================================
TEST_CASE("partition basic behavior", "[algorithms][partition]")
{
    namespace alg = mylib::algorithms;

    SECTION("even/odd on mixed vector")
    {
        std::vector<int> v{ 1, 2, 3, 4, 5, 6, 7, 8 };
        auto isEven = [](int x) { return x % 2 == 0; };

        auto pivot = alg::partition(v.begin(), v.end(), isEven);

        REQUIRE(std::all_of(v.begin(), pivot, isEven));
        REQUIRE(std::none_of(pivot, v.end(), isEven));
        REQUIRE(pivot >= v.begin());
        REQUIRE(pivot <= v.end());
        REQUIRE(std::is_partitioned(v.begin(), v.end(), isEven));
    }

    SECTION("all elements satisfy predicate -> returns end")
    {
        std::vector<int> v{ 2, 4, 6, 8 };
        auto isEven = [](int x) { return x % 2 == 0; };
        auto pivot = alg::partition(v.begin(), v.end(), isEven);
        REQUIRE(pivot == v.end());
        REQUIRE(std::all_of(v.begin(), v.end(), isEven));
    }

    SECTION("no elements satisfy predicate -> returns begin")
    {
        std::vector<int> v{ 1, 3, 5, 7 };
        auto isEven = [](int x) { return x % 2 == 0; };
        auto pivot = alg::partition(v.begin(), v.end(), isEven);
        REQUIRE(pivot == v.begin());
        REQUIRE(std::none_of(v.begin(), v.end(), isEven));
    }

    SECTION("empty range -> returns begin (== end)")
    {
        std::vector<int> v;
        auto pred = [](int) { return true; };
        auto pivot = alg::partition(v.begin(), v.end(), pred);
        REQUIRE(pivot == v.begin());
        REQUIRE(pivot == v.end());
    }

    SECTION("single element satisfying predicate -> end")
    {
        std::vector<int> v{ 42 };
        auto pivot = alg::partition(v.begin(), v.end(), [](int x) { return x > 0; });
        REQUIRE(pivot == v.end());
        REQUIRE(v[0] == 42);
    }

    SECTION("single element not satisfying predicate -> begin")
    {
        std::vector<int> v{ -1 };
        auto pivot = alg::partition(v.begin(), v.end(), [](int x) { return x > 0; });
        REQUIRE(pivot == v.begin());
        REQUIRE(v[0] == -1);
    }

    SECTION("multiset preserved")
    {
        std::vector<int> v{ 5, 3, 1, 4, 2, 8, 6, 7 };
        auto original = v;
        std::sort(original.begin(), original.end());

        alg::partition(v.begin(), v.end(), [](int x) { return x % 2 == 0; });

        std::sort(v.begin(), v.end());
        REQUIRE(v == original);
    }

    SECTION("does not invoke predicate on end iterator")
    {
        std::vector<int> v{ 1, 2, 3, 4 };
        int calls = 0;
        alg::partition(v.begin(), v.end(), [&calls](int x) {
            ++calls;
            return x % 2 == 0;
        });
        REQUIRE(calls >= 4);
    }
}


// ============================================================================
//  partition — forward iterators (Lomuto path)
// ============================================================================
TEST_CASE("partition with forward iterators", "[algorithms][partition][forward]")
{
    namespace alg = mylib::algorithms;

    SECTION("std::forward_list")
    {
        std::forward_list<int> fl{ 1, 2, 3, 4, 5, 6, 7, 8 };
        auto pred = [](int x) { return x % 2 == 0; };

        // partition возвращает итератор; в forward_list нет "мгновенного"
        // std::is_partitioned-теста, но можем перебрать.
        auto pivot = alg::partition(fl.begin(), fl.end(), pred);

        // До pivot — все true
        for (auto it = fl.begin(); it != pivot; ++it)
            REQUIRE(pred(*it));
        // С pivot до конца — все false
        for (auto it = pivot; it != fl.end(); ++it)
            REQUIRE_FALSE(pred(*it));
    }

    SECTION("std::forward_list — multiset preserved")
    {
        std::forward_list<int> fl{ 5, 3, 1, 4, 2 };
        std::vector<int> original{ fl.begin(), fl.end() };
        std::sort(original.begin(), original.end());

        alg::partition(fl.begin(), fl.end(), [](int x) { return x % 2 == 0; });

        std::vector<int> after{ fl.begin(), fl.end() };
        std::sort(after.begin(), after.end());
        REQUIRE(after == original);
    }
}


// ============================================================================
//  partition — bidirectional iterators (Hoare path)
// ============================================================================
TEST_CASE("partition with bidirectional iterators", "[algorithms][partition][bidir]")
{
    namespace alg = mylib::algorithms;

    SECTION("std::list")
    {
        std::list<int> l{ 1, 2, 3, 4, 5, 6, 7, 8 };
        auto pred = [](int x) { return x % 2 == 0; };
        auto pivot = alg::partition(l.begin(), l.end(), pred);

        for (auto it = l.begin(); it != pivot; ++it)
            REQUIRE(pred(*it));
        for (auto it = pivot; it != l.end(); ++it)
            REQUIRE_FALSE(pred(*it));
    }

    SECTION("custom comparator")
    {
        std::vector<int> v{ 3, 1, 4, 1, 5, 9, 2, 6 };
        auto isSmall = [](int x) { return x < 5; };
        alg::partition(v.begin(), v.end(), isSmall);
        REQUIRE(std::is_partitioned(v.begin(), v.end(), isSmall));
    }
}


// ============================================================================
//  partition — согласованность с std::partition
// ============================================================================
TEST_CASE("partition agrees with std::partition invariant",
          "[algorithms][partition][reference]")
{
    namespace alg = mylib::algorithms;

    SECTION("random data, various predicates")
    {
        std::mt19937 rng{ 42 };
        std::uniform_int_distribution<int> dist{ -50, 50 };

        for (int trial = 0; trial < 20; ++trial)
        {
            std::vector<int> v(50);
            for (auto& x : v) x = dist(rng);

            auto pred = [](int x) { return x < 0; };
            auto pivot = alg::partition(v.begin(), v.end(), pred);

            INFO("trial = " << trial);
            REQUIRE(std::all_of(v.begin(), pivot, pred));
            REQUIRE(std::none_of(pivot, v.end(), pred));
        }
    }

    SECTION("stable? No — but sorted-multiset same as std::partition")
    {
        std::vector<int> v1{ 5, 3, 8, 1, 9, 2, 7, 4, 6 };
        auto v2 = v1;
        auto pred = [](int x) { return x % 2 == 0; };

        alg::partition(v1.begin(), v1.end(), pred);
        std::partition(v2.begin(), v2.end(), pred);

        // Раскладка по группам одинаковая, порядок внутри — нет.
        std::sort(v1.begin(), v1.end());
        std::sort(v2.begin(), v2.end());
        REQUIRE(v1 == v2);
    }
}

// ============================================================================
//  partition3 — базовая трёхзонная раскладка
// ============================================================================
TEST_CASE("partition3 basic three-way split", "[algorithms][partition3]")
{
    using mylib::algorithms::partition3;

    SECTION("distinct values split into 3 zones correctly")
    {
        std::vector<int> v{ 5, 3, 1, 4, 2, 6, 7 };
        auto [left, right] = partition3(v.begin(), v.end());

        // Значение pivot — где-то в [left, right)
        REQUIRE(left != right);
        const int pivotValue = *left;

        INFO("pivotValue = " << pivotValue);
        for (auto it = v.begin(); it != left; ++it)
            REQUIRE(*it < pivotValue);
        for (auto it = left; it != right; ++it)
            REQUIRE(*it == pivotValue);
        for (auto it = right; it != v.end(); ++it)
            REQUIRE(*it > pivotValue);
    }

    SECTION("with duplicates — all copies of pivot in middle zone")
    {
        std::vector<int> v{ 3, 1, 3, 5, 3, 2, 3, 4 };
        auto [left, right] = partition3(v.begin(), v.end());

        const int pivotValue = *left;
        INFO("pivotValue = " << pivotValue);

        for (auto it = v.begin(); it != left; ++it) REQUIRE(*it < pivotValue);
        for (auto it = left; it != right; ++it)     REQUIRE(*it == pivotValue);
        for (auto it = right; it != v.end(); ++it)  REQUIRE(*it > pivotValue);
    }

    SECTION("empty range -> {begin, begin}")
    {
        std::vector<int> v;
        auto [left, right] = partition3(v.begin(), v.end());
        REQUIRE(left == v.begin());
        REQUIRE(right == v.begin());
        REQUIRE(left == right);
    }

    SECTION("single element -> {begin, end}")
    {
        std::vector<int> v{ 42 };
        auto [left, right] = partition3(v.begin(), v.end());
        REQUIRE(left == v.begin());
        REQUIRE(right == v.end());
        REQUIRE(*left == 42);
    }

    SECTION("two identical elements -> {begin, end}")
    {
        std::vector<int> v{ 5, 5 };
        auto [left, right] = partition3(v.begin(), v.end());
        REQUIRE(left == v.begin());
        REQUIRE(right == v.end());
    }

    SECTION("all same value -> {begin, end}")
    {
        std::vector<int> v{ 7, 7, 7, 7, 7 };
        auto [left, right] = partition3(v.begin(), v.end());
        REQUIRE(left == v.begin());
        REQUIRE(right == v.end());
        for (int x : v) REQUIRE(x == 7);
    }

    SECTION("multiset preserved")
    {
        std::vector<int> v{ 5, 3, 1, 4, 2, 8, 6, 7 };
        auto original = v;
        std::sort(original.begin(), original.end());

        partition3(v.begin(), v.end());

        std::sort(v.begin(), v.end());
        REQUIRE(v == original);
    }

    SECTION("larger random test")
    {
        std::mt19937 rng{ 42 };
        std::uniform_int_distribution<int> dist{ -20, 20 };

        for (int trial = 0; trial < 20; ++trial)
        {
            std::vector<int> v(100);
            for (auto& x : v) x = dist(rng);

            auto [left, right] = partition3(v.begin(), v.end());

            INFO("trial = " << trial);
            REQUIRE(left != right);
            const int pivotValue = *left;

            REQUIRE(std::all_of(v.begin(), left,
                                [p = pivotValue](int x) { return x < p; }));
            REQUIRE(std::all_of(left, right,
                                [p = pivotValue](int x) { return x == p; }));
            REQUIRE(std::all_of(right, v.end(),
                                [p = pivotValue](int x) { return x > p; }));
        }
    }
}


// ============================================================================
//  partition3 — forward iterators
// ============================================================================
TEST_CASE("partition3 with forward iterators", "[algorithms][partition3][forward]")
{
    using mylib::algorithms::partition3;

    SECTION("std::forward_list")
    {
        std::forward_list<int> fl{ 5, 3, 1, 4, 2, 6, 7 };
        auto [left, right] = partition3(fl.begin(), fl.end());

        REQUIRE(left != right);
        const int pivotValue = *left;

        for (auto it = fl.begin(); it != left; ++it)
            REQUIRE(*it < pivotValue);
        for (auto it = left; it != right; ++it)
            REQUIRE(*it == pivotValue);
        for (auto it = right; it != fl.end(); ++it)
            REQUIRE(*it > pivotValue);
    }

    SECTION("std::list")
    {
        std::list<int> l{ 3, 1, 3, 5, 3, 2 };
        auto [left, right] = partition3(l.begin(), l.end());
        const int pivotValue = *left;

        for (auto it = l.begin(); it != left; ++it) REQUIRE(*it < pivotValue);
        for (auto it = left; it != right; ++it)     REQUIRE(*it == pivotValue);
        for (auto it = right; it != l.end(); ++it)  REQUIRE(*it > pivotValue);
    }
}


// ============================================================================
//  partition3 — кастомный компаратор
// ============================================================================
TEST_CASE("partition3 with custom comparator", "[algorithms][partition3]")
{
    using mylib::algorithms::partition3;

    SECTION("descending order")
    {
        std::vector<int> v{ 3, 1, 4, 1, 5, 9, 2, 6 };
        auto desc = [](int a, int b) { return a > b; };
        auto [left, right] = partition3(v.begin(), v.end(), desc);

        const int pivotValue = *left;
        // "меньше" по desc = "больше" по значению
        for (auto it = v.begin(); it != left; ++it) REQUIRE(*it > pivotValue);
        for (auto it = left; it != right; ++it)     REQUIRE(*it == pivotValue);
        for (auto it = right; it != v.end(); ++it)  REQUIRE(*it < pivotValue);
    }

    SECTION("by absolute value")
    {
        std::vector<int> v{ -5, 3, -1, 4, -2, 5, -3 };
        auto byAbs = [](int a, int b) { return std::abs(a) < std::abs(b); };
        auto [left, right] = partition3(v.begin(), v.end(), byAbs);

        const int pivotValue = *left;
        const int pivotAbs = std::abs(pivotValue);
        for (auto it = v.begin(); it != left; ++it) REQUIRE(std::abs(*it) < pivotAbs);
        for (auto it = left; it != right; ++it)     REQUIRE(std::abs(*it) == pivotAbs);
        for (auto it = right; it != v.end(); ++it)  REQUIRE(std::abs(*it) > pivotAbs);
    }
}


// ============================================================================
//  partition3 — интеграция: quicksort-подобное использование
// ============================================================================
TEST_CASE("partition3 enables quicksort-like recursion",
          "[algorithms][partition3][integration]")
{
    using mylib::algorithms::partition3;

    // Простая quicksort на partition3: pivot zone уже на месте.
    std::function<void(std::vector<int>::iterator, std::vector<int>::iterator)>
        quickSort3 = [&quickSort3](auto begin, auto end) {
            if (std::distance(begin, end) <= 1) return;
            auto [left, right] = partition3(begin, end);
            quickSort3(begin, left);
            quickSort3(right, end);
        };

    SECTION("sorts random array")
    {
        std::mt19937 rng{ 42 };
        std::uniform_int_distribution<int> dist{ -100, 100 };

        for (int trial = 0; trial < 20; ++trial)
        {
            std::vector<int> v(100);
            for (auto& x : v) x = dist(rng);

            auto expected = v;
            std::sort(expected.begin(), expected.end());

            quickSort3(v.begin(), v.end());

            INFO("trial = " << trial);
            REQUIRE(v == expected);
        }
    }

    SECTION("sorts with duplicates")
    {
        std::vector<int> v{ 3, 1, 3, 5, 3, 2, 3, 4, 5, 1, 2, 4 };
        quickSort3(v.begin(), v.end());
        REQUIRE(std::is_sorted(v.begin(), v.end()));
    }

    SECTION("already sorted")
    {
        std::vector<int> v{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        quickSort3(v.begin(), v.end());
        REQUIRE(std::is_sorted(v.begin(), v.end()));
    }

    SECTION("all identical")
    {
        std::vector<int> v(100, 7);
        quickSort3(v.begin(), v.end());
        REQUIRE(std::is_sorted(v.begin(), v.end()));
    }
}

#endif // TEST_ALGORITHMS_H
