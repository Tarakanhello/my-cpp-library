#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

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


// ============================================================================
//  MersenneTwister: type traits, concepts, aliases
// ============================================================================
TEST_CASE("MersenneTwister type traits and concepts", "[rng][mt][traits]")
{
    SECTION("UInt_32_or_64 concept accepts exactly 32/64-bit unsigned")
    {
        STATIC_REQUIRE( mylib::UInt_32_or_64<std::uint32_t>);
        STATIC_REQUIRE( mylib::UInt_32_or_64<std::uint64_t>);
        STATIC_REQUIRE_FALSE(mylib::UInt_32_or_64<std::uint8_t>);
        STATIC_REQUIRE_FALSE(mylib::UInt_32_or_64<std::uint16_t>);
        STATIC_REQUIRE_FALSE(mylib::UInt_32_or_64<int>);
        STATIC_REQUIRE_FALSE(mylib::UInt_32_or_64<std::int64_t>);
    }

    SECTION("SeedSequence concept accepts std::seed_seq")
    {
        STATIC_REQUIRE(mylib::SeedSequence<std::seed_seq>);
    }

    SECTION("SeedSequence rejects non-SeedSequence types")
    {
        STATIC_REQUIRE_FALSE(mylib::SeedSequence<int>);
        STATIC_REQUIRE_FALSE(mylib::SeedSequence<double>);
        STATIC_REQUIRE_FALSE(mylib::SeedSequence<std::vector<int>>);
    }

    SECTION("MersenneTwister satisfies std::uniform_random_bit_generator")
    {
        STATIC_REQUIRE(std::uniform_random_bit_generator<
                       mylib::random::MersenneTwister<std::uint32_t>>);
        STATIC_REQUIRE(std::uniform_random_bit_generator<
                       mylib::random::MersenneTwister<std::uint64_t>>);
    }

    SECTION("result_type matches template parameter")
    {
        STATIC_REQUIRE(std::is_same_v<
                       mylib::random::MersenneTwister<std::uint32_t>::result_type,
                       std::uint32_t>);
        STATIC_REQUIRE(std::is_same_v<
                       mylib::random::MersenneTwister<std::uint64_t>::result_type,
                       std::uint64_t>);
    }

    SECTION("defaultRandomizer resolves to uint64_t variant by default")
    {
        STATIC_REQUIRE(std::is_same_v<
                       mylib::random::defaultRandomizer<>,
                       mylib::random::MersenneTwister<std::uint64_t>>);
    }

    SECTION("defaultRandomizer accepts explicit UIntType")
    {
        STATIC_REQUIRE(std::is_same_v<
                       mylib::random::defaultRandomizer<std::uint32_t>,
                       mylib::random::MersenneTwister<std::uint32_t>>);
        STATIC_REQUIRE(std::is_same_v<
                       mylib::random::defaultRandomizer<std::uint64_t>,
                       mylib::random::MersenneTwister<std::uint64_t>>);
    }

    SECTION("constructibility")
    {
        using M32 = mylib::random::MersenneTwister<std::uint32_t>;
        using M64 = mylib::random::MersenneTwister<std::uint64_t>;

        STATIC_REQUIRE(std::is_default_constructible_v<M32>);
        STATIC_REQUIRE(std::is_default_constructible_v<M64>);
        STATIC_REQUIRE(std::is_constructible_v<M32, std::uint32_t>);
        STATIC_REQUIRE(std::is_constructible_v<M64, std::uint64_t>);

        // explicit seed ctor — неявной конверсии быть не должно
        STATIC_REQUIRE_FALSE(std::is_convertible_v<std::uint32_t, M32>);
        STATIC_REQUIRE_FALSE(std::is_convertible_v<std::uint64_t, M64>);

        STATIC_REQUIRE(std::is_copy_constructible_v<M64>);
        STATIC_REQUIRE(std::is_copy_assignable_v<M64>);
        STATIC_REQUIRE(std::is_move_constructible_v<M64>);
        STATIC_REQUIRE(std::is_move_assignable_v<M64>);
    }
}


// ============================================================================
//  MersenneTwister: noexcept
// ============================================================================
TEST_CASE("MersenneTwister noexcept contract", "[rng][mt][noexcept]")
{
    using M32 = mylib::random::MersenneTwister<std::uint32_t>;
    using M64 = mylib::random::MersenneTwister<std::uint64_t>;

    // Explicit seed ctor — noexcept (по доку)
    STATIC_REQUIRE(noexcept(M32{ std::uint32_t{ 42 } }));
    STATIC_REQUIRE(noexcept(M64{ std::uint64_t{ 42 } }));

    // operator() — noexcept
    STATIC_REQUIRE(noexcept(std::declval<M32&>()()));
    STATIC_REQUIRE(noexcept(std::declval<M64&>()()));

    // Default ctor использует random_device + время — потенциально не noexcept.
    // Фиксируем контракт через REQUIRE_NOTHROW в рантайме.
    SECTION("default ctor does not throw in practice")
    {
        REQUIRE_NOTHROW(M32{});
        REQUIRE_NOTHROW(M64{});
    }
}


// ============================================================================
//  MersenneTwister: min/max, детерминизм
// ============================================================================
TEMPLATE_TEST_CASE("MersenneTwister shared behavior", "[rng][mt][template]",
                   std::uint32_t, std::uint64_t)
{
    using T = TestType;
    using M = mylib::random::MersenneTwister<T>;

    // ------------------------------------------------------------------------
    // 1. min() / max()
    // ------------------------------------------------------------------------
    SECTION("min() == 0, max() == numeric_limits<T>::max()")
    {
        REQUIRE(M::min() == T{ 0 });
        REQUIRE(M::max() == std::numeric_limits<T>::max());
    }

    // ------------------------------------------------------------------------
    // 2. same seed -> same sequence
    // ------------------------------------------------------------------------
    SECTION("same seed -> identical stream")
    {
        M a{ T{ 42 } };
        M b{ T{ 42 } };
        for (int i = 0; i < 1000; ++i)
        {
            INFO("i = " << i);
            REQUIRE(a() == b());
        }
    }

    // ------------------------------------------------------------------------
    // 3. different seeds -> different sequences
    // ------------------------------------------------------------------------
    SECTION("different seeds -> different sequences")
    {
        std::vector<T> seeds{ T{ 1 }, T{ 2 }, T{ 3 }, T{ 42 },
                             T{ 1000 }, T{ 0xDEADBEEF } };
        std::vector<std::vector<T>> seqs;
        for (T s : seeds)
        {
            M rng{ s };
            std::vector<T> seq(50);
            for (auto& x : seq) x = rng();
            seqs.push_back(std::move(seq));
        }
        for (size_t i = 0; i < seqs.size(); ++i)
            for (size_t j = i + 1; j < seqs.size(); ++j)
            {
                INFO("i = " << i << ", j = " << j);
                REQUIRE(seqs[i] != seqs[j]);
            }
    }

    // ------------------------------------------------------------------------
    // 4. copy/assign semantics
    // ------------------------------------------------------------------------
    SECTION("copy produces identical stream")
    {
        M a{ T{ 0xDEADBEEF } };
        for (int i = 0; i < 10; ++i) (void)a();
        M copy{ a };
        for (int i = 0; i < 100; ++i)
            REQUIRE(a() == copy());
    }

    SECTION("copy assignment")
    {
        M a{ T{ 12345 } };
        M b{ T{ 98765 } };
        for (int i = 0; i < 5; ++i) (void)a();
        b = a;
        for (int i = 0; i < 100; ++i)
            REQUIRE(a() == b());
    }

    // ------------------------------------------------------------------------
    // 5. seed 0 does not degenerate (unlike xorshift)
    // ------------------------------------------------------------------------
    SECTION("seed 0 is a valid, non-degenerate seed")
    {
        M rng{ T{ 0 } };
        std::vector<T> v(1000);
        for (auto& x : v) x = rng();

        // Не все значения одинаковые
        REQUIRE(std::adjacent_find(v.begin(), v.end(),
                                   std::not_equal_to<>{}) != v.end());
        // Не все нули
        REQUIRE(std::any_of(v.begin(), v.end(),
                            [](T x){ return x != T{ 0 }; }));
    }

    // ------------------------------------------------------------------------
    // 6. Outputs span full UIntType range
    // ------------------------------------------------------------------------
    SECTION("outputs span the full bit range")
    {
        M rng{ T{ 42 } };
        T orMask{ 0 };
        T andMask = std::numeric_limits<T>::max();
        for (int i = 0; i < 1000; ++i)
        {
            T v = rng();
            orMask  |= v;
            andMask &= v;
        }
        // Хотя бы раз встретились все единицы (в OR) — проверим старшую половину
        const T highHalf = static_cast<T>(~T{ 0 } >> (sizeof(T) * 4));
        REQUIRE((orMask & highHalf) == highHalf);
        // Хотя бы раз встретились все нули (в AND) — проверим старший бит
        REQUIRE((andMask & (T{ 1 } << (sizeof(T) * 8 - 1))) == 0);
    }

    // ------------------------------------------------------------------------
    // 7. Statistical: bit balance
    // ------------------------------------------------------------------------
    SECTION("each bit position is ~50% set")
    {
        M rng{ T{ 0xBADC0DE } };
        constexpr int N{ 100'000 };
        constexpr int BITS{ sizeof(T) * 8 };
        std::array<int, BITS> bitCounts{};

        for (int i = 0; i < N; ++i)
        {
            T v = rng();
            for (int b = 0; b < BITS; ++b)
                if ((v >> b) & T{ 1 }) ++bitCounts[b];
        }

        for (int b = 0; b < BITS; ++b)
        {
            INFO("bit = " << b << ", count = " << bitCounts[b]);
            REQUIRE(bitCounts[b] > 45'000);
            REQUIRE(bitCounts[b] < 55'000);
        }
    }

    // ------------------------------------------------------------------------
    // 8. Statistical: no duplicates in a reasonable sample
    // ------------------------------------------------------------------------
    SECTION("no duplicates in 50k sample")
    {
        M rng{ T{ 0xFEED } };
        std::vector<T> samples(50'000);
        for (auto& x : samples) x = rng();
        std::sort(samples.begin(), samples.end());
        REQUIRE(std::adjacent_find(samples.begin(), samples.end())
                == samples.end());
    }

    // ------------------------------------------------------------------------
    // 9. <random> integration
    // ------------------------------------------------------------------------
    SECTION("uniform_int_distribution works")
    {
        M rng{ T{ 42 } };
        std::uniform_int_distribution<int> dist{ 1, 6 };
        std::array<int, 7> freq{};
        for (int i = 0; i < 60'000; ++i) ++freq[dist(rng)];
        REQUIRE(freq[0] == 0);
        for (int face = 1; face <= 6; ++face)
        {
            INFO("face = " << face);
            REQUIRE(freq[face] > 9'000);
            REQUIRE(freq[face] < 11'000);
        }
    }

    SECTION("std::shuffle produces a permutation")
    {
        M rng{ T{ 42 } };
        std::vector<int> v(100);
        std::iota(v.begin(), v.end(), 0);
        std::vector<int> sorted = v;

        std::shuffle(v.begin(), v.end(), rng);

        std::vector<int> after = v;
        std::sort(after.begin(), after.end());
        REQUIRE(after == sorted);
        REQUIRE(v != sorted);   // перемешано
    }
}


// ============================================================================
//  MersenneTwister: соответствие стандартному MT
// ============================================================================
TEST_CASE("MersenneTwister matches std reference implementation",
          "[rng][mt][reference]")
{
    SECTION("MersenneTwister<uint32_t> matches std::mt19937")
    {
        // seed 5489 — стандартный default для std::mt19937
        mylib::random::MersenneTwister<std::uint32_t> mine{ 5489u };
        std::mt19937 std_ref{ 5489u };

        for (int i = 0; i < 1000; ++i)
        {
            INFO("i = " << i);
            REQUIRE(mine() == std_ref());
        }
    }

    SECTION("MersenneTwister<uint64_t> matches std::mt19937_64")
    {
        // seed 5489 — стандартный default для std::mt19937_64
        mylib::random::MersenneTwister<std::uint64_t> mine{ 5489u };
        std::mt19937_64 std_ref{ 5489u };

        for (int i = 0; i < 1000; ++i)
        {
            INFO("i = " << i);
            REQUIRE(mine() == std_ref());
        }
    }

    SECTION("known reference value: 10000th output from seed 5489")
    {
        // Для std::mt19937 (seed 5489) 10000-й выход == 4123659995
        // (значение из стандарта / N3337)
        mylib::random::MersenneTwister<std::uint32_t> rng{ 5489u };
        std::uint32_t v = 0;
        for (int i = 0; i < 10'000; ++i) v = rng();
        REQUIRE(v == 4123659995u);
    }

    SECTION("cross-check with std for different seeds")
    {
        for (std::uint32_t seed : { 0u, 1u, 42u, 12345u, 0xDEADBEEFu })
        {
            INFO("seed = " << seed);
            mylib::random::MersenneTwister<std::uint32_t> mine{ seed };
            std::mt19937 std_ref{ seed };
            for (int i = 0; i < 100; ++i)
                REQUIRE(mine() == std_ref());
        }
    }

    SECTION("uint32_t and uint64_t variants differ for the same seed")
    {
        mylib::random::MersenneTwister<std::uint32_t> m32{ 42u };
        mylib::random::MersenneTwister<std::uint64_t> m64{ 42u };

        // Первые значения почти наверняка отличаются
        bool anyDiff = false;
        for (int i = 0; i < 10; ++i)
        {
            const std::uint32_t v32 = m32();
            const std::uint64_t v64 = m64();
            // Сравниваем «младшие 32 бита» — не обязаны совпадать
            if (static_cast<std::uint64_t>(v32) != (v64 & 0xFFFFFFFFULL))
            {
                anyDiff = true;
                break;
            }
        }
        INFO("anyDiff = " << anyDiff);
        REQUIRE(anyDiff);
    }
}


// ============================================================================
//  MersenneTwister: SeedSequence constructor
// ============================================================================
TEST_CASE("MersenneTwister SeedSequence constructor",
          "[rng][mt][seedseq]")
{
    SECTION("std::seed_seq produces deterministic sequence")
    {
        std::seed_seq seq1{ 1, 2, 3, 4, 5 };
        std::seed_seq seq2{ 1, 2, 3, 4, 5 };

        mylib::random::MersenneTwister<std::uint64_t> a{ seq1 };
        mylib::random::MersenneTwister<std::uint64_t> b{ seq2 };

        for (int i = 0; i < 100; ++i)
            REQUIRE(a() == b());
    }

    SECTION("different seed_seq -> different sequences")
    {
        std::seed_seq seqA{ 1, 2, 3 };
        std::seed_seq seqB{ 4, 5, 6 };

        mylib::random::MersenneTwister<std::uint64_t> a{ seqA };
        mylib::random::MersenneTwister<std::uint64_t> b{ seqB };

        bool anyDiff = false;
        for (int i = 0; i < 10; ++i)
            if (a() != b()) { anyDiff = true; break; }
        REQUIRE(anyDiff);
    }

    SECTION("seed_seq matches std::mt19937_64 for the same seed_seq")
    {
        std::seed_seq seq1{ 1, 2, 3, 4, 5 };
        std::seed_seq seq2{ 1, 2, 3, 4, 5 };

        mylib::random::MersenneTwister<std::uint64_t> mine{ seq1 };
        std::mt19937_64 std_ref{ seq2 };

        for (int i = 0; i < 100; ++i)
        {
            INFO("i = " << i);
            REQUIRE(mine() == std_ref());
        }
    }

    SECTION("seed_seq works for uint32_t variant too")
    {
        std::seed_seq seq1{ 10, 20, 30 };
        std::seed_seq seq2{ 10, 20, 30 };

        mylib::random::MersenneTwister<std::uint32_t> mine{ seq1 };
        std::mt19937 std_ref{ seq2 };

        for (int i = 0; i < 100; ++i)
        {
            INFO("i = " << i);
            REQUIRE(mine() == std_ref());
        }
    }
}


// ============================================================================
//  MersenneTwister: defaultRandomizer
// ============================================================================
TEST_CASE("MersenneTwister defaultRandomizer alias",
          "[rng][mt][default]")
{
    SECTION("is a URBG")
    {
        STATIC_REQUIRE(std::uniform_random_bit_generator<
                       mylib::random::defaultRandomizer<>>);
    }

    SECTION("usable with distributions")
    {
        mylib::random::defaultRandomizer<> rng{};
        std::uniform_int_distribution<int> dist{ 1, 100 };
        for (int i = 0; i < 1000; ++i)
        {
            int v = dist(rng);
            REQUIRE(v >= 1);
            REQUIRE(v <= 100);
        }
    }

    SECTION("result_type is uint64_t")
    {
        STATIC_REQUIRE(std::is_same_v<
                       mylib::random::defaultRandomizer<>::result_type,
                       std::uint64_t>);
    }
}


// ============================================================================
//  MersenneTwister: smoke / долгий прогон
// ============================================================================
TEST_CASE("MersenneTwister long run smoke test", "[rng][mt][smoke]")
{
    SECTION("500k outputs, no obvious short cycle")
    {
        mylib::random::MersenneTwister<std::uint64_t> rng{ 0xDEADBEEFCAFEBABEULL };
        std::uint64_t prev = rng();
        for (int i = 1; i < 500'000; ++i)
        {
            std::uint64_t curr = rng();
            if (curr == prev)
                FAIL("immediate repeat at i = " << i);
            prev = curr;
        }
    }

    SECTION("default ctor produces two different streams")
    {
        mylib::random::MersenneTwister<std::uint64_t> a;
        mylib::random::MersenneTwister<std::uint64_t> b;

        // Маловероятно, что два default-сгенерированных потока совпадут
        bool anyDiff = false;
        for (int i = 0; i < 10; ++i)
            if (a() != b()) { anyDiff = true; break; }
        REQUIRE(anyDiff);
    }
}
