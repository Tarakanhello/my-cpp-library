#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <deque>
#include <map>
#include <numeric>
#include <random>
#include <set>
#include <thread>
#include <vector>

#include "mylib/random/algorithms.h"

// ============================================================================
//  randomInitVector
// ============================================================================
TEST_CASE("randomInitVector", "[algorithms][random][vector]")
{
    using mylib::algorithms::randomInitVector;

    SECTION("returns vector of requested size")
    {
        for (size_t n : { 1u, 2u, 3u, 5u, 10u, 100u })
        {
            INFO("n = " << n);
            auto v = randomInitVector(n);
            REQUIRE(v.size() == n);
        }
    }

    SECTION("has unit Euclidean norm")
    {
        for (size_t n : { 1u, 2u, 3u, 5u, 10u, 100u, 1000u })
        {
            auto v = randomInitVector(n);
            double sumSq = 0.0;
            for (size_t i = 0; i < n; ++i)
                sumSq += v[i] * v[i];
            const double norm = std::sqrt(sumSq);
            INFO("n = " << n << ", norm = " << norm);
            REQUIRE(std::abs(norm - 1.0) < 1e-9);
        }
    }

    SECTION("all components are in [-1, 1] and finite")
    {
        auto v = randomInitVector(100);
        for (size_t i = 0; i < v.size(); ++i)
        {
            INFO("i = " << i << ", v = " << v[i]);
            REQUIRE(std::isfinite(v[i]));
            REQUIRE(v[i] >= -1.0);
            REQUIRE(v[i] <= 1.0);
        }
    }

    SECTION("n = 1 returns exactly ±1")
    {
        for (int i = 0; i < 100; ++i)
        {
            auto v = randomInitVector(1);
            REQUIRE(v.size() == 1);
            REQUIRE((v[0] == 1.0 || v[0] == -1.0));
        }
    }

    SECTION("different calls produce different vectors")
    {
        auto a = randomInitVector(10);
        auto b = randomInitVector(10);
        double diff = 0.0;
        for (size_t i = 0; i < 10; ++i)
            diff += std::abs(a[i] - b[i]);
        INFO("L1 difference = " << diff);
        REQUIRE(diff > 1e-6);
    }
}


// ============================================================================
//  shuffle (explicit RNG)
// ============================================================================
TEST_CASE("mylib::algorithms::shuffle with explicit RNG",
          "[algorithms][shuffle]")
{
    using mylib::algorithms::shuffle;
    using mylib::random::QualityXorshift64;

    SECTION("deterministic for same seed")
    {
        std::vector<int> a(100), b(100);
        std::iota(a.begin(), a.end(), 0);
        std::iota(b.begin(), b.end(), 0);

        QualityXorshift64 r1{ 42 }, r2{ 42 };
        shuffle(a, r1);
        shuffle(b, r2);
        REQUIRE(a == b);
    }

    SECTION("produces a permutation (multiset preserved)")
    {
        std::vector<int> v(100);
        std::iota(v.begin(), v.end(), 0);
        const auto original = v;

        QualityXorshift64 rng{ 42 };
        shuffle(v, rng);

        auto sorted = v;
        std::sort(sorted.begin(), sorted.end());
        REQUIRE(sorted == original);
    }

    SECTION("empty / single / two elements")
    {
        QualityXorshift64 rng{ 42 };

        std::vector<int> empty;
        REQUIRE_NOTHROW(shuffle(empty, rng));
        REQUIRE(empty.empty());

        std::vector<int> one{ 42 };
        REQUIRE_NOTHROW(shuffle(one, rng));
        REQUIRE(one == std::vector<int>{ 42 });

        std::vector<int> two{ 1, 2 };
        REQUIRE_NOTHROW(shuffle(two, rng));
        REQUIRE(two.size() == 2);
        REQUIRE((two == std::vector<int>{ 1, 2 } ||
                 two == std::vector<int>{ 2, 1 }));
    }

    SECTION("works with std::array, mylib::Vector, std::deque")
    {
        QualityXorshift64 rng{ 42 };

        std::array<int, 5> arr{ 1, 2, 3, 4, 5 };
        shuffle(arr, rng);
        auto arrSorted = arr;
        std::sort(arrSorted.begin(), arrSorted.end());
        REQUIRE(arrSorted == std::array<int, 5>{ 1, 2, 3, 4, 5 });

        mylib::Vector<int> mv{ 1, 2, 3, 4, 5 };
        shuffle(mv, rng);
        REQUIRE(mv.size() == 5);

        std::deque<int> dq{ 1, 2, 3, 4, 5 };
        shuffle(dq, rng);
        REQUIRE(dq.size() == 5);
    }

    SECTION("uniform permutations for n = 3")
    {
        QualityXorshift64 rng{ 42 };
        std::array<int, 3> base{ 0, 1, 2 };
        std::map<std::array<int, 3>, int> freq;

        constexpr int N{ 60'000 };
        for (int i = 0; i < N; ++i)
        {
            auto p = base;
            shuffle(p, rng);
            ++freq[p];
        }

        REQUIRE(freq.size() == 6);
        for (const auto& [perm, count] : freq)
        {
            INFO("perm count = " << count);
            REQUIRE(count > 8'000);
            REQUIRE(count < 12'000);
        }
    }

    SECTION("different seeds produce different shuffles")
    {
        std::vector<int> a(50), b(50);
        std::iota(a.begin(), a.end(), 0);
        std::iota(b.begin(), b.end(), 0);

        QualityXorshift64 r1{ 1 }, r2{ 2 };
        shuffle(a, r1);
        shuffle(b, r2);
        REQUIRE(a != b);
    }
}


// ============================================================================
//  shuffle (thread_local)
// ============================================================================
TEST_CASE("mylib::algorithms::shuffle without explicit RNG",
          "[algorithms][shuffle]")
{
    using mylib::algorithms::shuffle;

    SECTION("does not throw and preserves size")
    {
        std::vector<int> v{ 1, 2, 3, 4, 5 };
        REQUIRE_NOTHROW(shuffle(v));
        REQUIRE(v.size() == 5);
    }

    SECTION("produces a permutation")
    {
        std::vector<int> v(100);
        std::iota(v.begin(), v.end(), 0);
        const auto original = v;
        shuffle(v);
        auto sorted = v;
        std::sort(sorted.begin(), sorted.end());
        REQUIRE(sorted == original);
    }

    SECTION("empty is safe")
    {
        std::vector<int> empty;
        REQUIRE_NOTHROW(shuffle(empty));
        REQUIRE(empty.empty());
    }
}


// ============================================================================
//  sampleIndices
// ============================================================================
TEST_CASE("mylib::algorithms::sampleIndices",
          "[algorithms][sample][indices]")
{
    using mylib::algorithms::sampleIndices;
    using mylib::random::QualityXorshift64;

    SECTION("returns k distinct sorted indices in [0, n)")
    {
        QualityXorshift64 rng{ 42 };
        for (size_t n : { size_t{ 5 }, size_t{ 10 }, size_t{ 100 }, size_t{ 1000 } })
        {
            for (size_t k : { size_t{ 1 }, n / 2, n })
            {
                if (k == 0 || k > n) continue;
                INFO("n = " << n << ", k = " << k);

                auto v = sampleIndices(k, n, rng);
                REQUIRE(v.size() == k);
                REQUIRE(std::is_sorted(v.begin(), v.end()));

                std::set<size_t> s(v.begin(), v.end());
                REQUIRE(s.size() == k);

                for (size_t x : v) REQUIRE(x < n);
            }
        }
    }

    SECTION("k == n returns exactly 0..n-1")
    {
        QualityXorshift64 rng{ 42 };
        auto v = sampleIndices(10, 10, rng);
        REQUIRE(v.size() == 10);
        for (size_t i = 0; i < 10; ++i) REQUIRE(v[i] == i);
    }

    SECTION("k = 1 covers all indices over many runs")
    {
        QualityXorshift64 rng{ 42 };
        std::set<size_t> seen;
        for (int i = 0; i < 10'000; ++i)
        {
            auto v = sampleIndices(1, 10, rng);
            REQUIRE(v.size() == 1);
            seen.insert(v[0]);
        }
        REQUIRE(seen.size() == 10);
    }

    SECTION("deterministic for same seed")
    {
        QualityXorshift64 r1{ 42 }, r2{ 42 };
        auto a = sampleIndices(5, 100, r1);
        auto b = sampleIndices(5, 100, r2);
        REQUIRE(a == b);
    }

    SECTION("uniform combinations: k = 2 from n = 5")
    {
        QualityXorshift64 rng{ 42 };
        std::map<std::pair<size_t, size_t>, int> freq;
        constexpr int N{ 50'000 };
        for (int i = 0; i < N; ++i)
        {
            auto v = sampleIndices(2, 5, rng);
            ++freq[{ v[0], v[1] }];
        }
        REQUIRE(freq.size() == 10);
        for (const auto& [comb, count] : freq)
        {
            INFO("count = " << count);
            REQUIRE(count > 3'500);
            REQUIRE(count < 6'500);
        }
    }
}


// ============================================================================
//  randomCombination
// ============================================================================
TEST_CASE("mylib::algorithms::randomCombination",
          "[algorithms][sample][combination]")
{
    using mylib::algorithms::randomCombination;
    using mylib::random::QualityXorshift64;

    SECTION("returns k unique indices in [0, n)")
    {
        QualityXorshift64 rng{ 42 };
        for (size_t n : { size_t{ 5 }, size_t{ 10 }, size_t{ 100 } })
        {
            for (size_t k : { size_t{ 1 }, size_t{ 2 }, n })
            {
                if (k > n) continue;
                INFO("n = " << n << ", k = " << k);

                auto v = randomCombination(k, n, rng);
                REQUIRE(v.size() == k);

                std::set<size_t> s(v.begin(), v.end());
                REQUIRE(s.size() == k);
                for (size_t x : v) REQUIRE(x < n);
            }
        }
    }

    SECTION("order is randomized (not always sorted)")
    {
        QualityXorshift64 rng{ 42 };
        bool anyNonSorted = false;
        for (int i = 0; i < 100; ++i)
        {
            auto v = randomCombination(5, 100, rng);
            if (!std::is_sorted(v.begin(), v.end()))
            {
                anyNonSorted = true;
                break;
            }
        }
        REQUIRE(anyNonSorted);
    }

    SECTION("k == n returns permutation of 0..n-1")
    {
        QualityXorshift64 rng{ 42 };
        auto v = randomCombination(10, 10, rng);
        REQUIRE(v.size() == 10);
        std::vector<size_t> sorted(v.begin(), v.end());
        std::sort(sorted.begin(), sorted.end());
        for (size_t i = 0; i < 10; ++i) REQUIRE(sorted[i] == i);
    }

    SECTION("deterministic for same seed")
    {
        QualityXorshift64 r1{ 42 }, r2{ 42 };
        auto a = randomCombination(5, 100, r1);
        auto b = randomCombination(5, 100, r2);
        REQUIRE(a == b);
    }
}


// ============================================================================
//  ReservoirSampler — API and invariants
// ============================================================================
TEST_CASE("ReservoirSampler construction and invariants",
          "[algorithms][reservoir]")
{
    using mylib::algorithms::ReservoirSampler;
    using mylib::random::QualityXorshift64;

    SECTION("no default, no copy, no move")
    {
        using RS = ReservoirSampler<int, QualityXorshift64>;
        STATIC_REQUIRE_FALSE(std::is_default_constructible_v<RS>);
        STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<RS>);
        STATIC_REQUIRE_FALSE(std::is_copy_assignable_v<RS>);
        STATIC_REQUIRE_FALSE(std::is_move_constructible_v<RS>);
        STATIC_REQUIRE_FALSE(std::is_move_assignable_v<RS>);
    }

    SECTION("size() returns k, processed() starts at 0")
    {
        QualityXorshift64 rng{ 42 };
        ReservoirSampler<int, QualityXorshift64> s{ 5, rng };
        REQUIRE(s.size() == 5);
        REQUIRE(s.processed() == 0);
        REQUIRE(s.getSelectedItems().empty());
    }

    SECTION("processed() increments per item")
    {
        QualityXorshift64 rng{ 42 };
        ReservoirSampler<int, QualityXorshift64> s{ 3, rng };
        for (int i = 0; i < 100; ++i)
        {
            s.processItem(i);
            INFO("i = " << i);
            REQUIRE(s.processed() == static_cast<size_t>(i + 1));
        }
    }

    SECTION("first k items fill reservoir in order")
    {
        QualityXorshift64 rng{ 42 };
        ReservoirSampler<int, QualityXorshift64> s{ 3, rng };
        s.processItem(10);
        s.processItem(20);
        s.processItem(30);

        auto items = s.getSelectedItems();
        REQUIRE(items.size() == 3);
        REQUIRE(items[0] == 10);
        REQUIRE(items[1] == 20);
        REQUIRE(items[2] == 30);
    }

    SECTION("k = 0 never holds items")
    {
        QualityXorshift64 rng{ 42 };
        ReservoirSampler<int, QualityXorshift64> s{ 0, rng };
        for (int i = 0; i < 100; ++i) s.processItem(i);
        REQUIRE(s.processed() == 100);
        REQUIRE(s.getSelectedItems().empty());
    }

    SECTION("stream smaller than k returns all items")
    {
        QualityXorshift64 rng{ 42 };
        ReservoirSampler<int, QualityXorshift64> s{ 100, rng };
        for (int i = 0; i < 10; ++i) s.processItem(i);
        auto items = s.getSelectedItems();
        REQUIRE(items.size() == 10);
        for (size_t i = 0; i < 10; ++i)
            REQUIRE(items[i] == static_cast<int>(i));
    }

    SECTION("stream == k returns all items")
    {
        QualityXorshift64 rng{ 42 };
        ReservoirSampler<int, QualityXorshift64> s{ 10, rng };
        for (int i = 0; i < 10; ++i) s.processItem(i);
        auto items = s.getSelectedItems();
        REQUIRE(items.size() == 10);
    }

    SECTION("getSelectedItems returns independent copy")
    {
        QualityXorshift64 rng{ 42 };
        ReservoirSampler<int, QualityXorshift64> s{ 2, rng };
        s.processItem(1);
        s.processItem(2);

        auto a = s.getSelectedItems();
        auto b = s.getSelectedItems();
        REQUIRE(&a != &b);
        REQUIRE(a == b);
    }

    SECTION("works with non-trivial T")
    {
        QualityXorshift64 rng{ 42 };
        ReservoirSampler<std::string, QualityXorshift64> s{ 3, rng };
        for (const char* p : { "a", "b", "c", "d", "e" })
            s.processItem(std::string{ p });

        auto items = s.getSelectedItems();
        REQUIRE(items.size() == 3);
        for (const auto& str : items)
        {
            REQUIRE((str == "a" || str == "b" || str == "c"
                     || str == "d" || str == "e"));
        }
    }
}


// ============================================================================
//  ReservoirSampler — statistical correctness
// ============================================================================
TEST_CASE("ReservoirSampler statistical correctness",
          "[algorithms][reservoir][stats]")
{
    using mylib::algorithms::ReservoirSampler;
    using mylib::random::QualityXorshift64;

    SECTION("k = 1, n = 10: every item selected with ~1/10 probability")
    {
        QualityXorshift64 rng{ 42 };
        std::array<int, 10> freq{};
        constexpr int RUNS{ 100'000 };

        for (int run = 0; run < RUNS; ++run)
        {
            ReservoirSampler<int, QualityXorshift64> s{ 1, rng };
            for (int i = 0; i < 10; ++i) s.processItem(i);
            auto items = s.getSelectedItems();
            REQUIRE(items.size() == 1);
            ++freq[items[0]];
        }

        for (int i = 0; i < 10; ++i)
        {
            INFO("item " << i << ": " << freq[i]);
            REQUIRE(freq[i] > 8'500);
            REQUIRE(freq[i] < 11'500);
        }
    }

    SECTION("k = 2, n = 5: every 2-subset equally likely")
    {
        QualityXorshift64 rng{ 42 };
        std::map<std::pair<int, int>, int> freq;
        constexpr int RUNS{ 20'000 };

        for (int run = 0; run < RUNS; ++run)
        {
            ReservoirSampler<int, QualityXorshift64> s{ 2, rng };
            for (int i = 0; i < 5; ++i) s.processItem(i);
            auto items = s.getSelectedItems();
            REQUIRE(items.size() == 2);
            int x = items[0], y = items[1];
            if (x > y) std::swap(x, y);
            ++freq[{ x, y }];
        }

        REQUIRE(freq.size() == 10);
        // Mean = 2000. Bounds ~±15% (6σ).
        for (const auto& [subset, count] : freq)
        {
            INFO("count = " << count);
            REQUIRE(count > 1'700);
            REQUIRE(count < 2'300);
        }
    }
}


// ============================================================================
//  reservoirSample (convenience function)
// ============================================================================
TEST_CASE("mylib::algorithms::reservoirSample",
          "[algorithms][reservoir]")
{
    using mylib::algorithms::reservoirSample;
    using mylib::random::QualityXorshift64;

    SECTION("returns exactly size elements from a large range")
    {
        QualityXorshift64 rng{ 42 };
        std::vector<int> data{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        auto v = reservoirSample(data, 3, rng);
        REQUIRE(v.size() == 3);
    }

    SECTION("all selected values come from the input")
    {
        QualityXorshift64 rng{ 42 };
        std::vector<int> data{ 1, 2, 3, 4, 5 };
        auto v = reservoirSample(data, 3, rng);
        for (size_t i = 0; i < v.size(); ++i)
        {
            INFO("v[" << i << "] = " << v[i]);
            REQUIRE(std::find(data.begin(), data.end(), v[i]) != data.end());
        }
    }

    SECTION("no duplicates")
    {
        QualityXorshift64 rng{ 42 };
        std::vector<int> data{ 1, 2, 3, 4, 5 };
        auto v = reservoirSample(data, 3, rng);
        std::set<int> s(v.begin(), v.end());
        REQUIRE(s.size() == v.size());
    }

    SECTION("k > n returns all elements")
    {
        QualityXorshift64 rng{ 42 };
        std::vector<int> data{ 1, 2, 3 };
        auto v = reservoirSample(data, 10, rng);
        REQUIRE(v.size() == 3);
    }

    SECTION("k == 0 returns empty")
    {
        QualityXorshift64 rng{ 42 };
        std::vector<int> data{ 1, 2, 3 };
        auto v = reservoirSample(data, 0, rng);
        REQUIRE(v.empty());
    }

    SECTION("k == n returns permutation of the input")
    {
        QualityXorshift64 rng{ 42 };
        std::vector<int> data{ 1, 2, 3, 4, 5 };
        auto v = reservoirSample(data, 5, rng);
        REQUIRE(v.size() == 5);
        std::vector<int> sorted(v.begin(), v.end());
        std::sort(sorted.begin(), sorted.end());
        REQUIRE(sorted == std::vector<int>{ 1, 2, 3, 4, 5 });
    }

    SECTION("deterministic for same seed")
    {
        QualityXorshift64 r1{ 42 }, r2{ 42 };
        std::vector<int> data{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        auto a = reservoirSample(data, 3, r1);
        auto b = reservoirSample(data, 3, r2);
        REQUIRE(a.size() == b.size());
        for (size_t i = 0; i < a.size(); ++i)
            REQUIRE(a[i] == b[i]);
    }

    SECTION("works with mylib::Vector and std::array inputs")
    {
        QualityXorshift64 rng{ 42 };

        mylib::Vector<int> mv;
        for (int i = 0; i < 10; ++i) mv.push_back(i);
        auto v1 = reservoirSample(mv, 3, rng);
        REQUIRE(v1.size() == 3);

        std::array<int, 10> arr{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        auto v2 = reservoirSample(arr, 3, rng);
        REQUIRE(v2.size() == 3);
    }
}


// ============================================================================
//  ReservoirSampler — thread safety (smoke)
// ============================================================================
TEST_CASE("ReservoirSampler concurrent reads and writes",
          "[algorithms][reservoir][thread]")
{
    using mylib::algorithms::ReservoirSampler;
    using mylib::random::QualityXorshift64;

    SECTION("writer + reader don't corrupt state")
    {
        QualityXorshift64 rng{ 42 };
        ReservoirSampler<int, QualityXorshift64> s{ 10, rng };

        std::atomic<bool> ok{ true };

        std::thread writer([&] {
            for (int i = 0; i < 500; ++i) s.processItem(i);
        });

        std::thread reader([&] {
            for (int i = 0; i < 500; ++i)
            {
                auto v = s.getSelectedItems();
                if (v.size() > 10) ok = false;
                if (s.processed() > 500) ok = false;
            }
        });

        writer.join();
        reader.join();

        REQUIRE(ok);
        REQUIRE(s.processed() == 500);
        REQUIRE(s.getSelectedItems().size() == 10);
    }
}

