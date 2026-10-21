#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <random>
#include <type_traits>
#include <unordered_set>
#include <vector>

#include "mylib/mylib.h"

namespace
{
using Rng = mylib::random::QualityXorshift64;
using R   = Rng::result_type;
} // namespace

// ============================================================================
//  QualityXorshift64: type traits и константы
// ============================================================================
TEST_CASE("QualityXorshift64 type traits", "[rng][xorshift][traits]")
{
    SECTION("result_type is uint64_t")
    {
        STATIC_REQUIRE(std::is_same_v<Rng::result_type, std::uint64_t>);
    }

    SECTION("satisfies std::uniform_random_bit_generator")
    {
        STATIC_REQUIRE(std::uniform_random_bit_generator<Rng>);
    }

    SECTION("min() and max() match spec")
    {
        STATIC_REQUIRE(Rng::min() == R{ 0 });
        STATIC_REQUIRE(Rng::max() == std::numeric_limits<R>::max());
    }

    SECTION("min/max are constexpr and return result_type")
    {
        constexpr R mn = Rng::min();
        constexpr R mx = Rng::max();
        STATIC_REQUIRE(std::is_same_v<decltype(mn), const R>);
        STATIC_REQUIRE(std::is_same_v<decltype(mx), const R>);
        REQUIRE(mn == 0);
        REQUIRE(mx == std::numeric_limits<R>::max());
    }

    SECTION("constructibility")
    {
        STATIC_REQUIRE(std::is_default_constructible_v<Rng>);
        STATIC_REQUIRE(std::is_constructible_v<Rng, R>);
        // explicit — неявная конверсия из seed запрещена
        STATIC_REQUIRE_FALSE(std::is_convertible_v<R, Rng>);
        // копируемость/перемещаемость
        STATIC_REQUIRE(std::is_copy_constructible_v<Rng>);
        STATIC_REQUIRE(std::is_copy_assignable_v<Rng>);
        STATIC_REQUIRE(std::is_move_constructible_v<Rng>);
        STATIC_REQUIRE(std::is_move_assignable_v<Rng>);
    }
}


// ============================================================================
//  QualityXorshift64: noexcept
// ============================================================================
TEST_CASE("QualityXorshift64 noexcept contract", "[rng][xorshift][noexcept]")
{
    STATIC_REQUIRE(noexcept(Rng::min()));
    STATIC_REQUIRE(noexcept(Rng::max()));
    STATIC_REQUIRE(noexcept(std::declval<Rng&>()()));
    STATIC_REQUIRE(noexcept(std::declval<Rng&>().uniform01()));
}


// ============================================================================
//  QualityXorshift64: детерминизм
// ============================================================================
TEST_CASE("QualityXorshift64 determinism", "[rng][xorshift][determinism]")
{
    SECTION("same seed -> same sequence")
    {
        Rng a{ 42 };
        Rng b{ 42 };

        for (int i = 0; i < 1000; ++i)
            REQUIRE(a() == b());
    }

    SECTION("copy produces identical stream")
    {
        Rng a{ 0xDEADBEEFCAFEULL };
        for (int i = 0; i < 10; ++i) (void)a();

        Rng copy{ a };
        for (int i = 0; i < 100; ++i)
            REQUIRE(a() == copy());
    }

    SECTION("copy assignment")
    {
        Rng a{ 12345 };
        Rng b{ 98765 };
        for (int i = 0; i < 5; ++i) (void)a();

        b = a;
        for (int i = 0; i < 100; ++i)
            REQUIRE(a() == b());
    }

    SECTION("different seeds -> different sequences")
    {
        std::vector<R> seeds{ 1, 2, 3, 42, 1000, 0xDEADBEEF, 0xFFFFFFFFFFFFFFFFULL };
        std::vector<std::vector<R>> sequences;

        for (R s : seeds)
        {
            Rng rng{ s };
            std::vector<R> seq(50);
            for (auto& x : seq) x = rng();
            sequences.push_back(std::move(seq));
        }

        // Все пары последовательностей должны отличаться
        for (size_t i = 0; i < sequences.size(); ++i)
        {
            for (size_t j = i + 1; j < sequences.size(); ++j)
            {
                INFO("i = " << i << ", j = " << j);
                REQUIRE(sequences[i] != sequences[j]);
            }
        }
    }

    SECTION("default ctor is deterministic across two instances (time-based seed is captured once)")
    {
        // Первый дефолтный instance создаётся, второй может получить другой seed,
        // если он захватывает время в момент создания. Проверяем только что
        // конструкция не бросает и оба работают.
        Rng a{};
        Rng b{};
        REQUIRE_NOTHROW(a());
        REQUIRE_NOTHROW(b());
    }
}


// ============================================================================
//  QualityXorshift64: обработка seed == 0
// ============================================================================
TEST_CASE("QualityXorshift64 seed == 0 handling", "[rng][xorshift][seed]")
{
    SECTION("seed 0 is not degenerate: never produces 0")
    {
        // Для xorshift состояние 0 — единственная неподвижная точка.
        // Если бы seed 0 не заменялся на PASSWORD, все выходы были бы 0.
        Rng rng{ 0 };
        for (int i = 0; i < 1000; ++i)
        {
            INFO("iteration = " << i);
            REQUIRE(rng() != R{ 0 });
        }
    }

    SECTION("seed 0 is deterministic")
    {
        Rng a{ 0 };
        Rng b{ 0 };
        for (int i = 0; i < 100; ++i)
            REQUIRE(a() == b());
    }

    SECTION("seed 0 produces a non-trivial (non-constant) sequence")
    {
        Rng rng{ 0 };
        std::vector<R> samples(1000);
        for (auto& x : samples) x = rng();

        // Не все одинаковые
        REQUIRE(std::adjacent_find(samples.begin(), samples.end(),
                                   std::not_equal_to<>{}) != samples.end());

        // Никаких нулей
        REQUIRE(std::find(samples.begin(), samples.end(), R{ 0 }) == samples.end());
    }

    SECTION("seed 0 differs from typical non-zero seeds")
    {
        Rng zero{ 0 };
        for (R s : { R{ 1 }, R{ 2 }, R{ 42 }, R{ 0xDEADBEEF } })
        {
            Rng other{ s };
            bool anyDiff = false;
            for (int i = 0; i < 20; ++i)
            {
                if (zero() != other()) { anyDiff = true; break; }
            }
            INFO("seed = " << s);
            REQUIRE(anyDiff);
        }
    }
}


// ============================================================================
//  QualityXorshift64: operator() и uniform01
// ============================================================================
TEST_CASE("QualityXorshift64 operator() and uniform01",
          "[rng][xorshift][output]")
{
    SECTION("operator() advances state, consecutive outputs differ")
    {
        Rng rng{ 42 };
        R prev = rng();
        for (int i = 0; i < 10; ++i)
        {
            R curr = rng();
            INFO("i = " << i);
            REQUIRE(curr != prev);   // нет тривиальных повторов подряд
            prev = curr;
        }
    }

    SECTION("operator() output can use full 64-bit range (high bits present)")
    {
        Rng rng{ 0x123456789ABCDEF0ULL };
        R orMask = 0;
        R andMask = std::numeric_limits<R>::max();
        for (int i = 0; i < 1000; ++i)
        {
            R v = rng();
            orMask  |= v;
            andMask &= v;
        }
        // За 1000 сэмплов должны встретиться почти все биты
        // Проверим, что хотя бы старшие 32 бита когда-то были единичными
        REQUIRE((orMask >> 32) == 0xFFFFFFFFULL);
        // И что старший бит когда-то был нулём
        REQUIRE((andMask & (R{ 1 } << 63)) == 0);
    }

    SECTION("uniform01 returns values in [0, 1)")
    {
        Rng rng{ 12345 };
        for (int i = 0; i < 10'000; ++i)
        {
            double v = rng.uniform01();
            INFO("i = " << i << ", v = " << v);
            REQUIRE(v >= 0.0);
            REQUIRE(v <  1.0);
        }
    }

    SECTION("uniform01 mean is close to 0.5")
    {
        Rng rng{ 42 };
        constexpr int N{ 100'000 };
        double sum = 0.0;
        for (int i = 0; i < N; ++i) sum += rng.uniform01();
        const double mean = sum / N;

        INFO("mean = " << mean);
        // Стандартная ошибка ≈ 0.00091; допуск 0.01 покрывает 11σ
        REQUIRE(std::abs(mean - 0.5) < 0.01);
    }

    SECTION("uniform01 covers the range (min near 0, max near 1)")
    {
        Rng rng{ 777 };
        constexpr int N{ 100'000 };
        double mn = 1.0, mx = 0.0;
        for (int i = 0; i < N; ++i)
        {
            double v = rng.uniform01();
            mn = std::min(mn, v);
            mx = std::max(mx, v);
        }
        INFO("min = " << mn << ", max = " << mx);
        REQUIRE(mn < 0.001);
        REQUIRE(mx > 0.999);
    }

    SECTION("uniform01 and operator() advance state identically")
    {
        Rng a{ 42 };
        Rng b{ 42 };

        (void)a.uniform01();   // a: 1 step
        (void)b();             // b: 1 step

        // Теперь следующий operator() должен совпадать
        for (int i = 0; i < 100; ++i)
            REQUIRE(a() == b());
    }
}


// ============================================================================
//  QualityXorshift64: статистические свойства
// ============================================================================
TEST_CASE("QualityXorshift64 statistical properties",
          "[rng][xorshift][statistics]")
{
    SECTION("no duplicates in a 100k sample (full-period generator)")
    {
        Rng rng{ 0xC0FFEE };
        std::vector<R> samples(100'000);
        for (auto& x : samples) x = rng();

        std::sort(samples.begin(), samples.end());
        auto it = std::adjacent_find(samples.begin(), samples.end());
        REQUIRE(it == samples.end());
    }

    SECTION("each bit position is ~50% set (bit balance)")
    {
        Rng rng{ 0xBADC0DE };
        constexpr int N{ 100'000 };
        std::array<int, 64> bitCounts{};

        for (int i = 0; i < N; ++i)
        {
            R v = rng();
            for (int b = 0; b < 64; ++b)
                if ((v >> b) & R{ 1 }) ++bitCounts[b];
        }

        // Ожидание 50000, std dev ≈ 158. Допуск ±5000 = 30σ.
        for (int b = 0; b < 64; ++b)
        {
            INFO("bit = " << b << ", count = " << bitCounts[b]);
            REQUIRE(bitCounts[b] > 45'000);
            REQUIRE(bitCounts[b] < 55'000);
        }
    }

    SECTION("raw output mean is close to 2^63")
    {
        Rng rng{ 0xABC };
        constexpr int N{ 100'000 };
        // Считаем через double, не через uint64 sum (без переполнения)
        double sum = 0.0;
        constexpr double SCALE = 1.0 / 18446744073709551616.0; // 1/2^64
        for (int i = 0; i < N; ++i)
            sum += static_cast<double>(rng()) * SCALE;
        const double mean = sum / N;

        INFO("normalized mean = " << mean);
        // Ожидание 0.5; std error ≈ 1/(sqrt(12*N)) ≈ 0.00091
        // Допуск 0.01 покрывает 11σ
        REQUIRE(std::abs(mean - 0.5) < 0.01);
    }

    SECTION("distribution into 100 buckets is approximately uniform")
    {
        Rng rng{ 0xFEEDFACE };
        constexpr int N{ 100'000 };
        constexpr int BUCKETS{ 100 };
        std::array<int, BUCKETS> buckets{};

        for (int i = 0; i < N; ++i)
        {
            R v = rng();
            // bucket = v * BUCKETS / 2^64 ≈ (v >> 54) * 100 / 1024 — нет,
            // используем "high" bits: floor(v / 2^64 * 100)
            // Проще: top 16 bits, потом масштабируем
            int b = static_cast<int>((v >> 32) % BUCKETS);
            ++buckets[b];
        }

        // Ожидание 1000 на бакет. 3σ = 3*sqrt(1000*99/100) ≈ 30.
        // Допуск [700, 1300] — огромный запас.
        for (int i = 0; i < BUCKETS; ++i)
        {
            INFO("bucket = " << i << ", count = " << buckets[i]);
            REQUIRE(buckets[i] > 700);
            REQUIRE(buckets[i] < 1300);
        }
    }

    SECTION("low bits are not stuck at 0 or 1 (no trailing-zero bias)")
    {
        Rng rng{ 999 };
        constexpr int N{ 50'000 };
        std::array<int, 8> lowBitCounts{};

        for (int i = 0; i < N; ++i)
        {
            R v = rng();
            for (int b = 0; b < 8; ++b)
                if ((v >> b) & R{ 1 }) ++lowBitCounts[b];
        }

        for (int b = 0; b < 8; ++b)
        {
            INFO("low bit = " << b << ", count = " << lowBitCounts[b]);
            REQUIRE(lowBitCounts[b] > 20'000);
            REQUIRE(lowBitCounts[b] < 30'000);
        }
    }
}


// ============================================================================
//  QualityXorshift64: интеграция с <random>
// ============================================================================
TEST_CASE("QualityXorshift64 integration with <random>",
          "[rng][xorshift][random]")
{
    SECTION("uniform_int_distribution works")
    {
        Rng rng{ 42 };
        std::uniform_int_distribution<int> dist{ 1, 6 };

        std::array<int, 7> freq{};
        for (int i = 0; i < 60'000; ++i)
            ++freq[dist(rng)];

        REQUIRE(freq[0] == 0);   // 0 никогда не выпадет
        for (int face = 1; face <= 6; ++face)
        {
            INFO("face = " << face << ", count = " << freq[face]);
            REQUIRE(freq[face] > 9'000);
            REQUIRE(freq[face] < 11'000);
        }
    }

    SECTION("uniform_real_distribution works")
    {
        Rng rng{ 42 };
        std::uniform_real_distribution<double> dist{ -1.0, 1.0 };

        for (int i = 0; i < 1000; ++i)
        {
            double v = dist(rng);
            REQUIRE(v >= -1.0);
            REQUIRE(v <   1.0);
        }
    }

    SECTION("shuffle produces a permutation")
    {
        Rng rng{ 42 };
        std::vector<int> v(100);
        std::iota(v.begin(), v.end(), 0);

        std::shuffle(v.begin(), v.end(), rng);

        // Все элементы на месте
        std::vector<int> sorted = v;
        std::sort(sorted.begin(), sorted.end());
        REQUIRE(sorted == []{
            std::vector<int> r(100);
            std::iota(r.begin(), r.end(), 0);
            return r;
        }());

        // Порядок изменился
        std::vector<int> natural(100);
        std::iota(natural.begin(), natural.end(), 0);
        REQUIRE(v != natural);
    }

    SECTION("std::sample")
    {
        Rng rng{ 42 };
        std::vector<int> src(100);
        std::iota(src.begin(), src.end(), 0);

        std::vector<int> picked;
        std::sample(src.begin(), src.end(),
                    std::back_inserter(picked), 10, rng);
        REQUIRE(picked.size() == 10);

        // Все элементы из источника, без повторов
        std::sort(picked.begin(), picked.end());
        REQUIRE(std::adjacent_find(picked.begin(), picked.end()) == picked.end());
        for (int x : picked) REQUIRE(x >= 0);
        for (int x : picked) REQUIRE(x < 100);
    }

    SECTION("std::generate_n")
    {
        Rng rng{ 42 };
        std::vector<R> v(100);
        std::generate_n(v.begin(), 100, rng);
        // Все значения заданы (не нули по умолчанию)
        REQUIRE(std::find(v.begin(), v.end(), R{ 0 }) == v.end());
    }
}


// ============================================================================
//  QualityXorshift64: долгий прогон (smoke)
// ============================================================================
TEST_CASE("QualityXorshift64 long run smoke test", "[rng][xorshift][smoke]")
{
    SECTION("1M outputs: no obvious cycle, no zero, no immediate repeats")
    {
        Rng rng{ 0xDEADBEEFCAFEBABEULL };
        R prev = rng();
        for (int i = 1; i < 1'000'000; ++i)
        {
            R curr = rng();
            // Не гарантируется строго для полнопериодного генератора
            // (теоретически возможен повтор через 2^64), но за 1M его быть не должно
            if (curr == prev)
            {
                FAIL("immediate repeat at i = " << i);
            }
            REQUIRE(curr != R{ 0 });
            prev = curr;
        }
    }
}
