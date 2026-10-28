#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <random>
#include <set>
#include <type_traits>
#include <unordered_set>
#include <vector>

#include "mylib/mylib.h"

namespace
{
using Rng = mylib::random::QualityXorshift64;
using R   = Rng::result_type;

struct TinyURBG
{
    using result_type = std::uint8_t;

    static constexpr result_type min() noexcept { return 0; }
    static constexpr result_type max() noexcept { return 255; }

    result_type operator()() noexcept { return s++; }

    TinyURBG() = default;
    explicit TinyURBG(result_type seed) noexcept : s(seed) {}

    result_type s = 0;
};

static_assert(std::uniform_random_bit_generator<TinyURBG>);
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

// ============================================================================
//  Randomizer: type traits, concepts, aliases
// ============================================================================
TEST_CASE("Randomizer type traits and concepts", "[rng][randomizer][traits]")
{
    using namespace mylib;

    SECTION("default template argument is defaultRandomizer")
    {
        STATIC_REQUIRE(std::is_same_v<
                       Randomizer<>,
                       Randomizer<random::defaultRandomizer<>>>);
        STATIC_REQUIRE(std::is_same_v<
                       Randomizer<>::result_type,
                       std::uint64_t>);
    }

    SECTION("satisfies uniform_random_bit_generator for default")
    {
        STATIC_REQUIRE(std::uniform_random_bit_generator<Randomizer<>>);
    }

    SECTION("satisfies URBG for QualityXorshift64")
    {
        STATIC_REQUIRE(std::uniform_random_bit_generator<
                       Randomizer<random::QualityXorshift64>>);
    }

    SECTION("satisfies URBG for MersenneTwister<uint32_t>")
    {
        STATIC_REQUIRE(std::uniform_random_bit_generator<
                       Randomizer<random::MersenneTwister<std::uint32_t>>>);
    }

    SECTION("satisfies URBG for TinyURBG")
    {
        STATIC_REQUIRE(std::uniform_random_bit_generator<
                       Randomizer<TinyURBG>>);
    }

    SECTION("result_type matches GENERATOR::result_type")
    {
        STATIC_REQUIRE(std::is_same_v<
                       Randomizer<random::QualityXorshift64>::result_type,
                       random::QualityXorshift64::result_type>);
        STATIC_REQUIRE(std::is_same_v<
                       Randomizer<random::MersenneTwister<std::uint32_t>>::result_type,
                       std::uint32_t>);
        STATIC_REQUIRE(std::is_same_v<
                       Randomizer<TinyURBG>::result_type,
                       std::uint8_t>);
    }

    SECTION("min/max proxy through to GENERATOR")
    {
        STATIC_REQUIRE(Randomizer<>::min() == Randomizer<>::max() - Randomizer<>::max());
        STATIC_REQUIRE(Randomizer<TinyURBG>::min() == std::uint8_t{ 0 });
        STATIC_REQUIRE(Randomizer<TinyURBG>::max() == std::uint8_t{ 255 });
        STATIC_REQUIRE(Randomizer<random::MersenneTwister<std::uint32_t>>::max()
                       == std::numeric_limits<std::uint32_t>::max());
    }

    SECTION("constructibility")
    {
        using R = Randomizer<>;
        STATIC_REQUIRE(std::is_default_constructible_v<R>);
        STATIC_REQUIRE(std::is_constructible_v<R, R::result_type>);
        // explicit seed ctor — implicit conversion запрещён
        STATIC_REQUIRE_FALSE(std::is_convertible_v<R::result_type, R>);
        // SeedSequence ctor
        STATIC_REQUIRE(std::is_constructible_v<R, std::seed_seq&>);
    }
}


// ============================================================================
//  Randomizer: noexcept contract
// ============================================================================
TEST_CASE("Randomizer noexcept contract", "[rng][randomizer][noexcept]")
{
    using namespace mylib;
    using R = Randomizer<>;

    STATIC_REQUIRE(noexcept(std::declval<R&>()()));
    STATIC_REQUIRE(noexcept(std::declval<R&>().next()));

    // mod / sign / inRange — не noexcept (используют distributions)
    STATIC_REQUIRE_FALSE(noexcept(std::declval<R&>().mod(R::result_type{ 10 })));
    STATIC_REQUIRE_FALSE(noexcept(std::declval<R&>().sign()));
    STATIC_REQUIRE_FALSE(noexcept(std::declval<R&>().inRange(0, 6)));
}


// ============================================================================
//  Randomizer: конструкторы
// ============================================================================
TEST_CASE("Randomizer constructors", "[rng][randomizer][construction]")
{
    using namespace mylib;

    SECTION("default ctor does not throw")
    {
        REQUIRE_NOTHROW(Randomizer<>{});
        REQUIRE_NOTHROW(Randomizer<random::QualityXorshift64>{});
        REQUIRE_NOTHROW(Randomizer<TinyURBG>{});
    }

    SECTION("default ctor produces distinct streams (high entropy)")
    {
        Randomizer<> a;
        Randomizer<> b;

        bool anyDiff = false;
        for (int i = 0; i < 10; ++i)
            if (a() != b()) { anyDiff = true; break; }
        REQUIRE(anyDiff);
    }

    SECTION("explicit seed ctor is deterministic")
    {
        Randomizer<> a{ 42 };
        Randomizer<> b{ 42 };
        for (int i = 0; i < 1000; ++i)
            REQUIRE(a() == b());
    }

    SECTION("explicit seed ctor matches wrapped generator")
    {
        // Randomizer должен быть прозрачной обёрткой: последовательность
        // от seed == последовательность самого GENERATOR'а от того же seed.
        Randomizer<random::MersenneTwister<std::uint64_t>> wrapped{ 42ull };
        random::MersenneTwister<std::uint64_t> raw{ 42ull };

        for (int i = 0; i < 500; ++i)
        {
            INFO("i = " << i);
            REQUIRE(wrapped() == raw());
        }
    }

    SECTION("explicit seed ctor matches wrapped QualityXorshift64")
    {
        Randomizer<random::QualityXorshift64> wrapped{ 0xC0FFEEull };
        random::QualityXorshift64 raw{ 0xC0FFEEull };
        for (int i = 0; i < 500; ++i)
            REQUIRE(wrapped() == raw());
    }

    SECTION("SeedSequence ctor matches wrapped generator")
    {
        std::seed_seq seq1{ 1, 2, 3, 4, 5 };
        std::seed_seq seq2{ 1, 2, 3, 4, 5 };

        Randomizer<random::MersenneTwister<std::uint64_t>> wrapped{ seq1 };
        random::MersenneTwister<std::uint64_t> raw{ seq2 };

        for (int i = 0; i < 500; ++i)
        {
            INFO("i = " << i);
            REQUIRE(wrapped() == raw());
        }
    }

    SECTION("different seeds -> different streams")
    {
        Randomizer<> a{ 1 };
        Randomizer<> b{ 2 };

        bool anyDiff = false;
        for (int i = 0; i < 10; ++i)
            if (a() != b()) { anyDiff = true; break; }
        REQUIRE(anyDiff);
    }

    SECTION("TinyURBG seed ctor takes uint8_t")
    {
        Randomizer<TinyURBG> r{ std::uint8_t{ 100 } };
        // TinyURBG начинает счёт с seed, поэтому первые 5 значений — 100..104
        for (std::uint8_t expected = 100; expected < 105; ++expected)
        {
            INFO("expected = " << static_cast<int>(expected));
            REQUIRE(r() == expected);
        }
    }
}


// ============================================================================
//  Randomizer: operator() и next()
// ============================================================================
TEST_CASE("Randomizer operator() and next()", "[rng][randomizer][output]")
{
    using namespace mylib;

    SECTION("next() == operator() (same state, same value)")
    {
        Randomizer<> a{ 42 };
        Randomizer<> b{ 42 };

        for (int i = 0; i < 200; ++i)
        {
            INFO("i = " << i);
            REQUIRE(a.next() == b());
        }
    }

    SECTION("calling next() advances state, like operator()")
    {
        Randomizer<> viaNext{ 42 };
        Randomizer<> viaCall{ 42 };

        for (int i = 0; i < 10; ++i) { (void)viaNext.next(); (void)viaCall(); }

        for (int i = 0; i < 100; ++i)
            REQUIRE(viaNext() == viaCall());
    }

    SECTION("operator() matches wrapped generator stream")
    {
        Randomizer<> wrapped{ 0xDEADBEEFCAFEULL };
        random::defaultRandomizer<> raw{ 0xDEADBEEFCAFEULL };

        for (int i = 0; i < 500; ++i)
        {
            INFO("i = " << i);
            REQUIRE(wrapped() == raw());
        }
    }
}


// ============================================================================
//  Randomizer: mod(n)
// ============================================================================
TEST_CASE("Randomizer::mod", "[rng][randomizer][mod]")
{
    using namespace mylib;

    SECTION("returns values in [0, n-1]")
    {
        Randomizer<> r{ 42 };
        for (Randomizer<>::result_type n : {
                                            Randomizer<>::result_type{ 1 },
                                            Randomizer<>::result_type{ 2 },
                                            Randomizer<>::result_type{ 3 },
                                            Randomizer<>::result_type{ 6 },
                                            Randomizer<>::result_type{ 100 },
                                            Randomizer<>::result_type{ 1000 } })
        {
            for (int i = 0; i < 2000; ++i)
            {
                auto v = r.mod(n);
                INFO("n = " << n << ", v = " << v);
                REQUIRE(v < n);
            }
        }
    }

    SECTION("mod(1) is always 0")
    {
        Randomizer<> r{ 0xDEAD };
        for (int i = 0; i < 1000; ++i)
            REQUIRE(r.mod(1) == 0);
    }

    SECTION("mod(6) covers all 6 values (basic uniformity)")
    {
        Randomizer<> r{ 42 };
        std::array<int, 6> freq{};
        constexpr int N{ 60'000 };
        for (int i = 0; i < N; ++i) ++freq[r.mod(6)];

        for (int face = 0; face < 6; ++face)
        {
            INFO("face = " << face << ", count = " << freq[face]);
            REQUIRE(freq[face] > 9'000);
            REQUIRE(freq[face] < 11'000);
        }
    }

    SECTION("mod(100) covers all 100 values")
    {
        Randomizer<> r{ 123 };
        std::set<Randomizer<>::result_type> seen;
        for (int i = 0; i < 10'000; ++i) seen.insert(r.mod(100));
        REQUIRE(seen.size() == 100);
        REQUIRE(*seen.begin() == 0);
        REQUIRE(*seen.rbegin() == 99);
    }

    SECTION("works with TinyURBG (result_type = uint8_t)")
    {
        Randomizer<TinyURBG> r{};
        for (std::uint8_t n : { std::uint8_t{ 1 }, std::uint8_t{ 3 },
             std::uint8_t{ 100 }, std::uint8_t{ 200 } })
        {
            for (int i = 0; i < 1000; ++i)
            {
                auto v = r.mod(n);
                INFO("n = " << static_cast<int>(n)
                            << ", v = " << static_cast<int>(v));
                REQUIRE(v < n);
            }
        }
    }

    SECTION("no modulo bias — high values reachable for n = max/2 + 1")
    {
        // При наивной реализации через % верхняя половина значений
        // встречалась бы вдвое реже. Статистически это трудно поймать,
        // но проверим, что верхние значения достижимы.
        Randomizer<> r{ 42 };
        const auto n = (Randomizer<>::max() / 2) + 1;
        bool sawHigh = false;
        for (int i = 0; i < 10'000; ++i)
            if (r.mod(n) >= n / 2) { sawHigh = true; break; }
        REQUIRE(sawHigh);
    }
}


// ============================================================================
//  Randomizer: sign()
// ============================================================================
TEST_CASE("Randomizer::sign", "[rng][randomizer][sign]")
{
    using namespace mylib;

    SECTION("only returns +1 or -1")
    {
        Randomizer<> r{ 42 };
        for (int i = 0; i < 10'000; ++i)
        {
            int s = r.sign();
            INFO("i = " << i << ", s = " << s);
            REQUIRE((s == 1 || s == -1));
        }
    }

    SECTION("approximately 50/50 distribution")
    {
        Randomizer<> r{ 42 };
        int pos = 0, neg = 0;
        constexpr int N{ 100'000 };
        for (int i = 0; i < N; ++i)
        {
            if (r.sign() == 1) ++pos; else ++neg;
        }
        INFO("pos = " << pos << ", neg = " << neg);
        REQUIRE(pos > 49'000);
        REQUIRE(pos < 51'000);
        REQUIRE(pos + neg == N);
    }

    SECTION("works with TinyURBG")
    {
        Randomizer<TinyURBG> r{};
        for (int i = 0; i < 1000; ++i)
        {
            int s = r.sign();
            REQUIRE((s == 1 || s == -1));
        }
    }
}


// ============================================================================
//  Randomizer: inRange(a, b)
// ============================================================================
TEST_CASE("Randomizer::inRange", "[rng][randomizer][inRange]")
{
    using namespace mylib;

    SECTION("returns values in [a, b] inclusive")
    {
        Randomizer<> r{ 42 };
        for (int i = 0; i < 5000; ++i)
        {
            int v = r.inRange(1, 6);
            INFO("v = " << v);
            REQUIRE(v >= 1);
            REQUIRE(v <= 6);
        }
    }

    SECTION("a == b always returns a")
    {
        Randomizer<> r{ 42 };
        for (int i = 0; i < 1000; ++i)
            REQUIRE(r.inRange(7, 7) == 7);
    }

    SECTION("covers all values for small range")
    {
        Randomizer<> r{ 42 };
        std::array<int, 6> freq{};
        constexpr int N{ 60'000 };
        for (int i = 0; i < N; ++i) ++freq[r.inRange(1, 6) - 1];
        for (int face = 0; face < 6; ++face)
        {
            INFO("face = " << face + 1 << ", count = " << freq[face]);
            REQUIRE(freq[face] > 9'000);
            REQUIRE(freq[face] < 11'000);
        }
    }

    SECTION("handles negative bounds")
    {
        Randomizer<> r{ 42 };
        int mn = 0, mx = 0;
        for (int i = 0; i < 5000; ++i)
        {
            int v = r.inRange(-5, 5);
            REQUIRE(v >= -5);
            REQUIRE(v <= 5);
            mn = std::min(mn, v);
            mx = std::max(mx, v);
        }
        REQUIRE(mn == -5);
        REQUIRE(mx == 5);
    }

    SECTION("IntType defaults to int")
    {
        Randomizer<> r{ 42 };
        STATIC_REQUIRE(std::is_same_v<
                       decltype(r.inRange(0, 10)),
                       int>);
        STATIC_REQUIRE(std::is_same_v<
                       decltype(r.inRange<long long>(-100, 100)),
                       long long>);
        STATIC_REQUIRE(std::is_same_v<
                       decltype(r.inRange<std::uint32_t>(0u, 10u)),
                       std::uint32_t>);
    }

    SECTION("works with long long and large negative bounds")
    {
        // Документация явно предупреждает про underflow при result_type unsigned.
        // Проверяем, что inRange<long long> с отрицательными значениями работает.
        Randomizer<> r{ 42 };
        constexpr long long lo{ -1000 };
        constexpr long long hi{ 1000 };
        for (int i = 0; i < 5000; ++i)
        {
            long long v = r.inRange<long long>(lo, hi);
            INFO("v = " << v);
            REQUIRE(v >= lo);
            REQUIRE(v <= hi);
        }
    }

    SECTION("works for full int64 range without UB")
    {
        Randomizer<> r{ 42 };
        for (int i = 0; i < 1000; ++i)
        {
            std::int64_t v = r.inRange<std::int64_t>(
                std::numeric_limits<std::int64_t>::min(),
                std::numeric_limits<std::int64_t>::max());
            // Тип гарантирует попадание в диапазон; проверяем что хотя бы раз
            // встретились отрицательные и неотрицательные
            (void)v;
        }
        // Прицельно: 1000 сэмплов из 2^64 — почти наверняка есть оба знака
        bool sawNeg = false, sawPos = false;
        for (int i = 0; i < 1000; ++i)
        {
            auto v = r.inRange<std::int64_t>(
                std::numeric_limits<std::int64_t>::min(),
                std::numeric_limits<std::int64_t>::max());
            if (v < 0) sawNeg = true;
            else if (v > 0) sawPos = true;
            if (sawNeg && sawPos) break;
        }
        INFO("sawNeg = " << sawNeg << ", sawPos = " << sawPos);
        REQUIRE(sawNeg);
        REQUIRE(sawPos);
    }

    SECTION("works with TinyURBG")
    {
        Randomizer<TinyURBG> r{};
        for (int i = 0; i < 1000; ++i)
        {
            int v = r.inRange(0, 100);
            REQUIRE(v >= 0);
            REQUIRE(v <= 100);
        }
    }

    SECTION("does not throw for normal inputs")
    {
        Randomizer<> r{ 42 };
        REQUIRE_NOTHROW(r.inRange(0, 1));
        REQUIRE_NOTHROW(r.inRange(-100, 100));
        REQUIRE_NOTHROW(r.inRange(0, 1'000'000));
    }
}


// ============================================================================
//  Randomizer: uniform01()
// ============================================================================
TEST_CASE("Randomizer::uniform01", "[rng][randomizer][uniform01]")
{
    using namespace mylib;

    SECTION("returns values in [0, 1)")
    {
        Randomizer<> r{ 42 };
        for (int i = 0; i < 10'000; ++i)
        {
            double v = r.uniform01();
            INFO("i = " << i << ", v = " << v);
            REQUIRE(v >= 0.0);
            REQUIRE(v <  1.0);
        }
    }

    SECTION("mean is close to 0.5")
    {
        Randomizer<> r{ 42 };
        constexpr int N{ 100'000 };
        double sum = 0.0;
        for (int i = 0; i < N; ++i) sum += r.uniform01();
        const double mean = sum / N;
        INFO("mean = " << mean);
        REQUIRE(std::abs(mean - 0.5) < 0.01);
    }

    SECTION("covers the [0, 1) range broadly")
    {
        Randomizer<> r{ 42 };
        constexpr int N{ 100'000 };
        double mn = 1.0, mx = 0.0;
        for (int i = 0; i < N; ++i)
        {
            double v = r.uniform01();
            mn = std::min(mn, v);
            mx = std::max(mx, v);
        }
        INFO("min = " << mn << ", max = " << mx);
        REQUIRE(mn < 0.001);
        REQUIRE(mx > 0.999);
    }

    SECTION("is deterministic for same seed")
    {
        Randomizer<> a{ 42 };
        Randomizer<> b{ 42 };
        for (int i = 0; i < 100; ++i)
            REQUIRE(a.uniform01() == b.uniform01());
    }

    SECTION("works with TinyURBG (result_type = uint8_t)")
    {
        Randomizer<TinyURBG> r{};
        for (int i = 0; i < 1000; ++i)
        {
            double v = r.uniform01();
            INFO("v = " << v);
            REQUIRE(v >= 0.0);
            REQUIRE(v <  1.0);
        }
    }

    SECTION("uniform01 advances state (deterministic sequence preserved)")
    {
        Randomizer<> a{ 42 };
        Randomizer<> b{ 42 };

        // Продвигаем a через uniform01, b через operator() — затем
        // следующие значения должны совпасть
        (void)a.uniform01();
        (void)b();
        for (int i = 0; i < 100; ++i)
            REQUIRE(a() == b());
    }

    SECTION("uniform01 handles UINT64_MAX without returning 1.0")
    {
        // custom URBG, всегда возвращающий max()
        struct MaxURBG {
            using result_type = std::uint64_t;
            static constexpr result_type min() noexcept { return 0; }
            static constexpr result_type max() noexcept { return std::numeric_limits<result_type>::max(); }
            result_type operator()() noexcept { return max(); }
        };
        mylib::Randomizer<MaxURBG> r;
        double v = r.uniform01();
        REQUIRE(v >= 0.0);
        REQUIRE(v < 1.0);   // падает, если не исправлено
    }
}


// ============================================================================
//  Randomizer: интеграция с <random> и STL
// ============================================================================
TEST_CASE("Randomizer integration with STL", "[rng][randomizer][stl]")
{
    using namespace mylib;

    SECTION("works as URBG for std::uniform_int_distribution")
    {
        Randomizer<> r{ 42 };
        std::uniform_int_distribution<int> dist{ 1, 6 };
        std::array<int, 7> freq{};
        for (int i = 0; i < 60'000; ++i) ++freq[dist(r)];
        REQUIRE(freq[0] == 0);
        for (int face = 1; face <= 6; ++face)
        {
            INFO("face = " << face);
            REQUIRE(freq[face] > 9'000);
            REQUIRE(freq[face] < 11'000);
        }
    }

    SECTION("works as URBG for std::shuffle")
    {
        Randomizer<> r{ 42 };
        std::vector<int> v(100);
        std::iota(v.begin(), v.end(), 0);
        std::vector<int> original = v;
        std::shuffle(v.begin(), v.end(), r);
        REQUIRE(v != original);

        std::vector<int> check = v;
        std::sort(check.begin(), check.end());
        REQUIRE(check == original);
    }

    SECTION("works with std::generate_n")
    {
        Randomizer<> r{ 42 };
        std::vector<Randomizer<>::result_type> v(100);
        std::generate_n(v.begin(), 100, r);
        // Не все нули
        REQUIRE(std::any_of(v.begin(), v.end(),
                            [](auto x){ return x != 0; }));
    }
}


// ============================================================================
//  Randomizer: template-параметризация по GENERATOR
// ============================================================================
TEMPLATE_TEST_CASE("Randomizer works with different GENERATORs",
                   "[rng][randomizer][template]",
                   mylib::random::QualityXorshift64,
                   mylib::random::MersenneTwister<std::uint32_t>,
                   mylib::random::MersenneTwister<std::uint64_t>,
                   TinyURBG)
{
    using R = mylib::Randomizer<TestType>;

    SECTION("default ctor does not throw")
    {
        REQUIRE_NOTHROW(R{});
    }

    SECTION("explicit seed ctor + determinism")
    {
        // Для TinyURBG result_type = uint8_t — seed приводим
        using S = typename R::result_type;
        R a{ S{ 42 } };
        R b{ S{ 42 } };
        for (int i = 0; i < 100; ++i)
            REQUIRE(a() == b());
    }

    SECTION("mod, sign, inRange, uniform01 работают")
    {
        R r{ typename R::result_type{ 42 } };

        for (int i = 0; i < 200; ++i)
        {
            REQUIRE(r.mod(typename R::result_type{ 5 }) < 5);
            int s{ r.sign() };

            INFO("i = " << i << ", s = " << s);
            REQUIRE((s == 1 || s == -1));

            int v = r.inRange(1, 6);
            REQUIRE(v >= 1);
            REQUIRE(v <= 6);

            double u = r.uniform01();
            REQUIRE(u >= 0.0);
            REQUIRE(u <  1.0);
        }
    }
}



// ============================================================================
//  Randomizer: новые распределения — noexcept, детерминизм
// ============================================================================
TEST_CASE("Randomizer new distributions — noexcept / determinism",
          "[rng][randomizer][distributions]")
{
    using namespace mylib;
    using R = Randomizer<>;

    // Ни один новый метод не noexcept (кроме операторов/next)
    SECTION("new distribution methods are not noexcept")
    {
        STATIC_REQUIRE_FALSE(noexcept(std::declval<R&>().exponential()));
        STATIC_REQUIRE_FALSE(noexcept(std::declval<R&>().cauchy()));
        STATIC_REQUIRE_FALSE(noexcept(std::declval<R&>().normal01()));
        STATIC_REQUIRE_FALSE(noexcept(std::declval<R&>().normal(0.0, 1.0)));
        STATIC_REQUIRE_FALSE(noexcept(std::declval<R&>().gamma()));
        STATIC_REQUIRE_FALSE(noexcept(std::declval<R&>().chiSquared(2.0)));
        STATIC_REQUIRE_FALSE(noexcept(std::declval<R&>().studentT(5.0)));
        STATIC_REQUIRE_FALSE(noexcept(std::declval<R&>().beta(2.0, 3.0)));
        STATIC_REQUIRE_FALSE(noexcept(std::declval<R&>().fisherF(5.0, 10.0)));
        STATIC_REQUIRE_FALSE(noexcept(std::declval<R&>().triangular()));
        STATIC_REQUIRE_FALSE(noexcept(std::declval<R&>().levy()));
        STATIC_REQUIRE_FALSE(noexcept(std::declval<R&>().symmetrizedLevy()));
        STATIC_REQUIRE_FALSE(noexcept(std::declval<R&>().bernoulli(0.5)));
        STATIC_REQUIRE_FALSE(noexcept(std::declval<R&>().binomial(0.5, 10)));
        STATIC_REQUIRE_FALSE(noexcept(std::declval<R&>().geometric(0.5)));
        STATIC_REQUIRE_FALSE(noexcept(std::declval<R&>().geometric05()));
        STATIC_REQUIRE_FALSE(noexcept(std::declval<R&>().poisson(5.0)));
    }

    SECTION("each distribution is deterministic for the same seed")
    {
        // По одному представителю — все методы идут через m_g, поэтому
        // детерминизм должен соблюдаться автоматически.
        for (int i = 0; i < 100; ++i)
        {
            R a{ 42 };
            R b{ 42 };
            INFO("i = " << i);

            REQUIRE(a.exponential(2.0) == b.exponential(2.0));
            REQUIRE(a.cauchy(1.0, 2.0) == b.cauchy(1.0, 2.0));
            REQUIRE(a.normal01() == b.normal01());
            REQUIRE(a.normal01() == b.normal01());   // cache hit
            REQUIRE(a.normal(3.0, 2.0) == b.normal(3.0, 2.0));
            REQUIRE(a.gamma(2.0, 3.0) == b.gamma(2.0, 3.0));
            REQUIRE(a.chiSquared(5.0) == b.chiSquared(5.0));
            REQUIRE(a.studentT(5.0) == b.studentT(5.0));
            REQUIRE(a.beta(2.0, 3.0) == b.beta(2.0, 3.0));
            REQUIRE(a.fisherF(5.0, 10.0) == b.fisherF(5.0, 10.0));
            REQUIRE(a.triangular(0.5) == b.triangular(0.5));
            REQUIRE(a.levy() == b.levy());
            REQUIRE(a.symmetrizedLevy() == b.symmetrizedLevy());
            REQUIRE(a.bernoulli(0.3) == b.bernoulli(0.3));
            REQUIRE(a.binomial(0.3, 100) == b.binomial(0.3, 100));
            REQUIRE(a.geometric(0.5) == b.geometric(0.5));
            REQUIRE(a.geometric05() == b.geometric05());
            REQUIRE(a.poisson(5.0) == b.poisson(5.0));
        }
    }
}


// ============================================================================
//  exponential, cauchy
// ============================================================================
TEST_CASE("Randomizer::exponential", "[rng][randomizer][exponential]")
{
    using namespace mylib;

    SECTION("values are > 0 and finite")
    {
        Randomizer<> r{ 42 };
        for (int i = 0; i < 10'000; ++i)
        {
            double v = r.exponential(1.0);
            INFO("v = " << v);
            REQUIRE(v > 0.0);
            REQUIRE(std::isfinite(v));
        }
    }

    SECTION("mean ≈ 1/lambda")
    {
        Randomizer<> r{ 42 };
        constexpr int N{ 100'000 };
        double sum = 0.0;
        for (int i = 0; i < N; ++i) sum += r.exponential(2.0);
        const double mean = sum / N;
        INFO("mean = " << mean);
        REQUIRE(std::abs(mean - 0.5) < 0.02);
    }

    SECTION("small lambda gives large mean")
    {
        Randomizer<> r{ 42 };
        constexpr int N{ 50'000 };
        double sum = 0.0;
        for (int i = 0; i < N; ++i) sum += r.exponential(0.1);
        const double mean = sum / N;
        INFO("mean = " << mean);
        REQUIRE(std::abs(mean - 10.0) < 0.3);
    }
}

TEST_CASE("Randomizer::cauchy", "[rng][randomizer][cauchy]")
{
    using namespace mylib;

    SECTION("values are finite")
    {
        Randomizer<> r{ 42 };
        for (int i = 0; i < 10'000; ++i)
        {
            double v = r.cauchy(0.0, 1.0);
            INFO("v = " << v);
            REQUIRE(std::isfinite(v));
        }
    }

    SECTION("median ≈ m")
    {
        Randomizer<> r{ 42 };
        constexpr int N{ 100'000 };
        std::vector<double> v(N);
        for (int i = 0; i < N; ++i) v[i] = r.cauchy(10.0, 1.0);
        std::sort(v.begin(), v.end());
        const double median = v[N / 2];
        INFO("median = " << median);
        REQUIRE(std::abs(median - 10.0) < 0.2);
    }

    SECTION("symmetric around m")
    {
        Randomizer<> r{ 42 };
        constexpr int N{ 100'000 };
        int left = 0, right = 0;
        for (int i = 0; i < N; ++i)
        {
            double v = r.cauchy(0.0, 1.0);
            if (v < 0.0) ++left;
            else if (v > 0.0) ++right;
        }
        INFO("left = " << left << ", right = " << right);
        REQUIRE(std::abs(left - right) < 3'000);
    }
}


// ============================================================================
//  normal01 / normal + cache
// ============================================================================
TEST_CASE("Randomizer::normal01 — statistics and cache",
          "[rng][randomizer][normal]")
{
    using namespace mylib;

    SECTION("mean ≈ 0, stddev ≈ 1")
    {
        Randomizer<> r{ 42 };
        constexpr int N{ 200'000 };
        double sum = 0.0, sq = 0.0;
        for (int i = 0; i < N; ++i)
        {
            double v = r.normal01();
            sum += v;
            sq += v * v;
        }
        const double mean = sum / N;
        const double var = sq / N - mean * mean;
        const double sd = std::sqrt(var);
        INFO("mean = " << mean << ", stddev = " << sd);
        REQUIRE(std::abs(mean) < 0.02);
        REQUIRE(std::abs(sd - 1.0) < 0.02);
    }

    SECTION("consecutive values differ (cache does not repeat)")
    {
        Randomizer<> r{ 42 };
        for (int i = 0; i < 1000; ++i)
        {
            double a = r.normal01();
            double b = r.normal01();
            INFO("i = " << i << ", a = " << a << ", b = " << b);
            REQUIRE(a != b);
        }
    }

    SECTION("cache consumes exactly 2 uniform draws per pair")
    {
        Randomizer<> r1{ 42 };
        Randomizer<> r2{ 42 };

        // r1: 2 x normal01 → cache hit second time, 2 uniform01 draws total
        (void)r1.normal01();
        (void)r1.normal01();

        // r2: 2 x uniform01 → 2 draws
        (void)r2.uniform01();
        (void)r2.uniform01();

        // Теперь m_g в одинаковом состоянии
        for (int i = 0; i < 100; ++i)
        {
            INFO("i = " << i);
            REQUIRE(r1() == r2());
        }
    }
}

TEST_CASE("Randomizer::normal — mean/stddev",
          "[rng][randomizer][normal]")
{
    using namespace mylib;

    SECTION("normal(10, 3) has mean ≈ 10, stddev ≈ 3")
    {
        Randomizer<> r{ 42 };
        constexpr int N{ 200'000 };
        double sum = 0.0, sq = 0.0;
        for (int i = 0; i < N; ++i)
        {
            double v = r.normal(10.0, 3.0);
            sum += v;
            sq += v * v;
        }
        const double mean = sum / N;
        const double var = sq / N - mean * mean;
        const double sd = std::sqrt(var);
        INFO("mean = " << mean << ", stddev = " << sd);
        REQUIRE(std::abs(mean - 10.0) < 0.05);
        REQUIRE(std::abs(sd - 3.0) < 0.05);
    }

    SECTION("normal(0, 1) ≈ normal01 statistically")
    {
        Randomizer<> r1{ 42 };
        Randomizer<> r2{ 42 };
        constexpr int N{ 200'000 };
        double s1 = 0.0, s2 = 0.0;
        for (int i = 0; i < N; ++i)
        {
            s1 += r1.normal(0.0, 1.0);
            s2 += r2.normal01();
        }
        INFO("mean1 = " << s1 / N << ", mean2 = " << s2 / N);
        REQUIRE(std::abs(s1 / N - s2 / N) < 0.01);
    }

    SECTION("negative mean handled correctly")
    {
        Randomizer<> r{ 42 };
        constexpr int N{ 100'000 };
        double sum = 0.0;
        for (int i = 0; i < N; ++i) sum += r.normal(-5.0, 2.0);
        const double mean = sum / N;
        INFO("mean = " << mean);
        REQUIRE(std::abs(mean - (-5.0)) < 0.05);
    }
}


// ============================================================================
//  gamma, chiSquared, studentT, beta, fisherF
// ============================================================================
TEST_CASE("Randomizer::gamma", "[rng][randomizer][gamma]")
{
    using namespace mylib;

    SECTION("values are positive and finite")
    {
        Randomizer<> r{ 42 };
        for (int i = 0; i < 10'000; ++i)
        {
            double v = r.gamma(2.0, 3.0);
            INFO("v = " << v);
            REQUIRE(v > 0.0);
            REQUIRE(std::isfinite(v));
        }
    }

    SECTION("mean ≈ shape * scale")
    {
        Randomizer<> r{ 42 };
        constexpr int N{ 100'000 };
        double sum = 0.0;
        for (int i = 0; i < N; ++i) sum += r.gamma(2.0, 3.0);
        const double mean = sum / N;
        INFO("mean = " << mean);
        REQUIRE(std::abs(mean - 6.0) < 0.05);
    }

    SECTION("shape < 1 branch: mean ≈ shape * scale")
    {
        Randomizer<> r{ 42 };
        constexpr int N{ 100'000 };
        double sum = 0.0;
        for (int i = 0; i < N; ++i) sum += r.gamma(0.5, 2.0);
        const double mean = sum / N;
        INFO("mean = " << mean);
        REQUIRE(std::abs(mean - 1.0) < 0.03);
    }

    SECTION("gamma(1, 1) matches exponential(1) mean")
    {
        Randomizer<> r{ 42 };
        constexpr int N{ 50'000 };
        double sum = 0.0;
        for (int i = 0; i < N; ++i) sum += r.gamma(1.0, 1.0);
        INFO("mean = " << sum / N);
        REQUIRE(std::abs(sum / N - 1.0) < 0.03);
    }
}

TEST_CASE("Randomizer::chiSquared", "[rng][randomizer][chiSquared]")
{
    using namespace mylib;

    SECTION("values are positive")
    {
        Randomizer<> r{ 42 };
        for (int i = 0; i < 5'000; ++i)
        {
            double v = r.chiSquared(5.0);
            INFO("v = " << v);
            REQUIRE(v > 0.0);
        }
    }

    SECTION("mean ≈ k")
    {
        Randomizer<> r{ 42 };
        constexpr int N{ 100'000 };
        double sum = 0.0;
        for (int i = 0; i < N; ++i) sum += r.chiSquared(5.0);
        const double mean = sum / N;
        INFO("mean = " << mean);
        REQUIRE(std::abs(mean - 5.0) < 0.05);
    }

    SECTION("chiSquared(2) ≡ exponential(0.5) in mean")
    {
        Randomizer<> r{ 42 };
        constexpr int N{ 50'000 };
        double s1 = 0.0, s2 = 0.0;
        for (int i = 0; i < N; ++i)
        {
            s1 += r.chiSquared(2.0);
            s2 += r.exponential(0.5);
        }
        INFO("chi2_mean = " << s1 / N << ", exp_mean = " << s2 / N);
        REQUIRE(std::abs(s1 / N - 2.0) < 0.05);
        REQUIRE(std::abs(s2 / N - 2.0) < 0.05);
    }
}

TEST_CASE("Randomizer::studentT", "[rng][randomizer][studentT]")
{
    using namespace mylib;

    SECTION("values are finite")
    {
        Randomizer<> r{ 42 };
        for (int i = 0; i < 5'000; ++i)
        {
            double v = r.studentT(5.0);
            INFO("v = " << v);
            REQUIRE(std::isfinite(v));
        }
    }

    SECTION("symmetric around 0 (v = 5)")
    {
        Randomizer<> r{ 42 };
        constexpr int N{ 100'000 };
        int left = 0, right = 0;
        for (int i = 0; i < N; ++i)
        {
            double v = r.studentT(5.0);
            if (v < 0.0) ++left;
            else if (v > 0.0) ++right;
        }
        INFO("left = " << left << ", right = " << right);
        REQUIRE(std::abs(left - right) < 3'000);
    }
}

TEST_CASE("Randomizer::beta", "[rng][randomizer][beta]")
{
    using namespace mylib;

    SECTION("values in (0, 1)")
    {
        Randomizer<> r{ 42 };
        for (int i = 0; i < 10'000; ++i)
        {
            double v = r.beta(2.0, 3.0);
            INFO("v = " << v);
            REQUIRE(v > 0.0);
            REQUIRE(v < 1.0);
        }
    }

    SECTION("mean ≈ alpha / (alpha + beta)")
    {
        Randomizer<> r{ 42 };
        constexpr int N{ 100'000 };
        double sum = 0.0;
        for (int i = 0; i < N; ++i) sum += r.beta(2.0, 3.0);
        const double mean = sum / N;
        INFO("mean = " << mean);
        REQUIRE(std::abs(mean - 0.4) < 0.01);
    }

    SECTION("beta(1, 1) is uniform-ish")
    {
        Randomizer<> r{ 42 };
        constexpr int N{ 100'000 };
        double sum = 0.0;
        for (int i = 0; i < N; ++i) sum += r.beta(1.0, 1.0);
        INFO("mean = " << sum / N);
        REQUIRE(std::abs(sum / N - 0.5) < 0.01);
    }
}

TEST_CASE("Randomizer::fisherF", "[rng][randomizer][fisherF]")
{
    using namespace mylib;

    SECTION("values are positive")
    {
        Randomizer<> r{ 42 };
        for (int i = 0; i < 5'000; ++i)
        {
            double v = r.fisherF(5.0, 10.0);
            INFO("v = " << v);
            REQUIRE(v > 0.0);
        }
    }

    SECTION("mean ≈ n / (n - 2) for n > 2")
    {
        Randomizer<> r{ 42 };
        constexpr int N{ 100'000 };
        double sum = 0.0;
        for (int i = 0; i < N; ++i) sum += r.fisherF(5.0, 10.0);
        const double mean = sum / N;
        INFO("mean = " << mean);
        REQUIRE(std::abs(mean - 1.25) < 0.05);
    }
}


// ============================================================================
//  triangular
// ============================================================================
TEST_CASE("Randomizer::triangular", "[rng][randomizer][triangular]")
{
    using namespace mylib;

    SECTION("triangular(0.5) — values in [0, 1] per doc, finite")
    {
        Randomizer<> r{ 42 };
        double mn = 1.0, mx = 0.0;
        for (int i = 0; i < 10'000; ++i)
        {
            double v = r.triangular(0.5);
            INFO("v = " << v);
            REQUIRE(std::isfinite(v));
            REQUIRE(v >= 0.0);
            REQUIRE(v <= 1.0);
            mn = std::min(mn, v);
            mx = std::max(mx, v);
        }
        INFO("min = " << mn << ", max = " << mx);
    }

    SECTION("triangular(0.5) — BUG EXPOSURE: should reach above 0.5")
    {
        // По документации "peak at middle": при middle=0.5 мода должна быть 0.5,
        // но реализация с sqrt(middle*u) / sqrt((1-middle)*(1-u)) даёт диапазон
        // [0, 0.5], т.е. upper bound == middle, а не 1.0.
        // Если тест падает — это реальный баг в triangular, а не в тесте.
        Randomizer<> r{ 42 };
        bool sawAbove05 = false;
        for (int i = 0; i < 10'000; ++i)
        {
            if (r.triangular(0.5) > 0.5) { sawAbove05 = true; break; }
        }
        INFO("sawAbove05 = " << sawAbove05);
        REQUIRE(sawAbove05);
    }

    SECTION("triangular(0, 10, 5) — BUG EXPOSURE: should reach [0, 10]")
    {
        // Если upper bound == middle по ошибке — верхняя половина недостижима.
        // Если тест падает — это реальный баг.
        Randomizer<> r{ 42 };
        bool sawAboveMid = false;
        for (int i = 0; i < 10'000; ++i)
        {
            if (r.triangular(0.0, 10.0, 5.0) > 5.0) { sawAboveMid = true; break; }
        }
        INFO("sawAboveMid = " << sawAboveMid);
        REQUIRE(sawAboveMid);
    }
}


// ============================================================================
//  levy, symmetrizedLevy
// ============================================================================
TEST_CASE("Randomizer::levy", "[rng][randomizer][levy]")
{
    using namespace mylib;

    SECTION("levy() values are positive")
    {
        Randomizer<> r{ 42 };
        int finite_count = 0;
        for (int i = 0; i < 10'000; ++i)
        {
            double v = r.levy();
            INFO("v = " << v);
            REQUIRE(v >= 0.0);
            if (std::isfinite(v)) ++finite_count;
        }
        // Подавляющее большинство значений — конечные (редкие inf допустимы)
        REQUIRE(finite_count > 9'900);
    }

    SECTION("levy is heavy-tailed (some large values)")
    {
        Randomizer<> r{ 42 };
        bool sawLarge = false;
        for (int i = 0; i < 10'000; ++i)
        {
            if (r.levy() > 100.0) { sawLarge = true; break; }
        }
        REQUIRE(sawLarge);
    }
}

TEST_CASE("Randomizer::symmetrizedLevy", "[rng][randomizer][symmetrizedLevy]")
{
    using namespace mylib;

    SECTION("outputs both positive and negative values")
    {
        Randomizer<> r{ 42 };
        int pos = 0, neg = 0;
        for (int i = 0; i < 20'000; ++i)
        {
            double v = r.symmetrizedLevy();
            if (v > 0.0) ++pos;
            else if (v < 0.0) ++neg;
        }
        INFO("pos = " << pos << ", neg = " << neg);
        REQUIRE(pos > 0);
        REQUIRE(neg > 0);
        // Symmetric sign: ~50/50
        REQUIRE(std::abs(pos - neg) < 2'000);
    }
}


// ============================================================================
//  discrete: bernoulli, binomial, geometric, geometric05, poisson
// ============================================================================
TEST_CASE("Randomizer::bernoulli", "[rng][randomizer][bernoulli]")
{
    using namespace mylib;

    SECTION("returns only 0 or 1")
    {
        Randomizer<> r{ 42 };
        for (int i = 0; i < 10'000; ++i)
        {
            bool b = r.bernoulli(0.5);
            (void)b;   // bool — тривиально, но проверим через mean
        }
    }

    SECTION("mean ≈ p")
    {
        Randomizer<> r{ 42 };
        constexpr int N{ 100'000 };
        int ones = 0;
        for (int i = 0; i < N; ++i) ones += r.bernoulli(0.3);
        const double mean = static_cast<double>(ones) / N;
        INFO("mean = " << mean);
        REQUIRE(std::abs(mean - 0.3) < 0.01);
    }

    SECTION("p = 0 always false")
    {
        Randomizer<> r{ 42 };
        for (int i = 0; i < 1'000; ++i)
            REQUIRE(r.bernoulli(0.0) == false);
    }

    SECTION("p = 1 always true")
    {
        Randomizer<> r{ 42 };
        for (int i = 0; i < 1'000; ++i)
            REQUIRE(r.bernoulli(1.0) == true);
    }
}

TEST_CASE("Randomizer::binomial", "[rng][randomizer][binomial]")
{
    using namespace mylib;

    SECTION("values in [0, n]")
    {
        Randomizer<> r{ 42 };
        for (int i = 0; i < 5'000; ++i)
        {
            int v = r.binomial(0.3, 100);
            INFO("v = " << v);
            REQUIRE(v >= 0);
            REQUIRE(v <= 100);
        }
    }

    SECTION("mean ≈ n*p")
    {
        Randomizer<> r{ 42 };
        constexpr int N{ 50'000 };
        long long sum = 0;
        for (int i = 0; i < N; ++i) sum += r.binomial(0.3, 100);
        const double mean = static_cast<double>(sum) / N;
        INFO("mean = " << mean);
        REQUIRE(std::abs(mean - 30.0) < 0.5);
    }

    SECTION("n = 0 always returns 0")
    {
        Randomizer<> r{ 42 };
        for (int i = 0; i < 1'000; ++i)
            REQUIRE(r.binomial(0.5, 0) == 0);
    }
}

TEST_CASE("Randomizer::geometric and geometric05",
          "[rng][randomizer][geometric]")
{
    using namespace mylib;

    SECTION("geometric(p) always >= 1")
    {
        Randomizer<> r{ 42 };
        for (int i = 0; i < 10'000; ++i)
        {
            int v = r.geometric(0.5);
            INFO("v = " << v);
            REQUIRE(v >= 1);
        }
    }

    SECTION("geometric(p) mean ≈ 1/p")
    {
        Randomizer<> r{ 42 };
        constexpr int N{ 200'000 };
        long long sum = 0;
        for (int i = 0; i < N; ++i) sum += r.geometric(0.5);
        const double mean = static_cast<double>(sum) / N;
        INFO("mean = " << mean);
        REQUIRE(std::abs(mean - 2.0) < 0.05);
    }

    SECTION("geometric(p=0.1) mean ≈ 10")
    {
        Randomizer<> r{ 42 };
        constexpr int N{ 100'000 };
        long long sum = 0;
        for (int i = 0; i < N; ++i) sum += r.geometric(0.1);
        const double mean = static_cast<double>(sum) / N;
        INFO("mean = " << mean);
        REQUIRE(std::abs(mean - 10.0) < 0.3);
    }

    SECTION("geometric05 always >= 1")
    {
        Randomizer<> r{ 42 };
        for (int i = 0; i < 10'000; ++i)
        {
            int v = r.geometric05();
            INFO("v = " << v);
            REQUIRE(v >= 1);
        }
    }

    SECTION("geometric05 mean ≈ 2")
    {
        Randomizer<> r{ 42 };
        constexpr int N{ 200'000 };
        long long sum = 0;
        for (int i = 0; i < N; ++i) sum += r.geometric05();
        const double mean = static_cast<double>(sum) / N;
        INFO("mean = " << mean);
        REQUIRE(std::abs(mean - 2.0) < 0.05);
    }
}

TEST_CASE("Randomizer::poisson", "[rng][randomizer][poisson]")
{
    using namespace mylib;

    SECTION("values are >= 0")
    {
        Randomizer<> r{ 42 };
        for (int i = 0; i < 10'000; ++i)
        {
            int v = r.poisson(5.0);
            INFO("v = " << v);
            REQUIRE(v >= 0);
        }
    }

    SECTION("mean ≈ lambda")
    {
        Randomizer<> r{ 42 };
        constexpr int N{ 200'000 };
        long long sum = 0;
        for (int i = 0; i < N; ++i) sum += r.poisson(5.0);
        const double mean = static_cast<double>(sum) / N;
        INFO("mean = " << mean);
        REQUIRE(std::abs(mean - 5.0) < 0.05);
    }

    SECTION("lambda = 1 mean ≈ 1")
    {
        Randomizer<> r{ 42 };
        constexpr int N{ 100'000 };
        long long sum = 0;
        for (int i = 0; i < N; ++i) sum += r.poisson(1.0);
        INFO("mean = " << sum / static_cast<double>(N));
        REQUIRE(std::abs(sum / static_cast<double>(N) - 1.0) < 0.03);
    }
}


// ============================================================================
//  uniform01<RealType> — шаблон
// ============================================================================
TEST_CASE("Randomizer::uniform01<RealType>", "[rng][randomizer][uniform01]")
{
    using namespace mylib;

    SECTION("return type matches template argument")
    {
        Randomizer<> r{ 42 };
        STATIC_REQUIRE(std::is_same_v<decltype(r.uniform01<float>()), float>);
        STATIC_REQUIRE(std::is_same_v<decltype(r.uniform01<double>()), double>);
        STATIC_REQUIRE(std::is_same_v<decltype(r.uniform01<long double>()), long double>);
        // По умолчанию — double
        STATIC_REQUIRE(std::is_same_v<decltype(r.uniform01()), double>);
    }

    SECTION("values in [0, 1) for each type")
    {
        Randomizer<> r{ 42 };
        for (int i = 0; i < 5'000; ++i)
        {
            float   f = r.uniform01<float>();
            double  d = r.uniform01<double>();
            long double l = r.uniform01<long double>();
            INFO("f = " << f << ", d = " << d << ", l = " << static_cast<double>(l));
            REQUIRE(f >= 0.0f); REQUIRE(f < 1.0f);
            REQUIRE(d >= 0.0);  REQUIRE(d < 1.0);
            REQUIRE(l >= 0.0L); REQUIRE(l < 1.0L);
        }
    }
}


// ============================================================================
//  GlobalRNG
// ============================================================================
TEST_CASE("mylib::GlobalRNG", "[rng][randomizer][global]")
{
    SECTION("returns a stable reference")
    {
        auto& a = mylib::GlobalRNG();
        auto& b = mylib::GlobalRNG();
        REQUIRE(&a == &b);
    }

    SECTION("advances on each call")
    {
        auto& r = mylib::GlobalRNG();
        auto v1 = r();
        auto v2 = r();
        INFO("v1 = " << v1 << ", v2 = " << v2);
        REQUIRE(v1 != v2);
    }

    SECTION("usable with distribution helpers")
    {
        auto& r = mylib::GlobalRNG();
        for (int i = 0; i < 1000; ++i)
        {
            auto v = r.inRange(1, 6);
            REQUIRE(v >= 1);
            REQUIRE(v <= 6);
        }
    }
}


// ============================================================================
//  AliasMethod: type traits, concepts, deduction guides
// ============================================================================
TEST_CASE("AliasMethod type traits and deduction guides",
          "[rng][alias][traits]")
{
    using mylib::random::AliasMethod;

    SECTION("default ctor is deleted")
    {
        STATIC_REQUIRE_FALSE(std::is_default_constructible_v<AliasMethod<double>>);
        STATIC_REQUIRE_FALSE(std::is_default_constructible_v<AliasMethod<float>>);
    }

    SECTION("initializer_list ctor constrained to exact RealType")
    {
        STATIC_REQUIRE(std::is_constructible_v<AliasMethod<double>,
                                               std::initializer_list<double>>);
        STATIC_REQUIRE(std::is_constructible_v<AliasMethod<float>,
                                               std::initializer_list<float>>);
        // initializer_list<int> не конвертируется в initializer_list<double>
        STATIC_REQUIRE_FALSE(std::is_constructible_v<AliasMethod<double>,
                                                     std::initializer_list<int>>);
        STATIC_REQUIRE_FALSE(std::is_constructible_v<AliasMethod<double>,
                                                     std::initializer_list<float>>);
    }

    SECTION("iterator ctor requires same_as value_type")
    {
        using ItD = std::vector<double>::iterator;
        using ItI = std::vector<int>::iterator;
        STATIC_REQUIRE(std::is_constructible_v<AliasMethod<double>, ItD, ItD>);
        STATIC_REQUIRE_FALSE(std::is_constructible_v<AliasMethod<double>, ItI, ItI>);
        STATIC_REQUIRE_FALSE(std::is_constructible_v<AliasMethod<double>, ItD, ItI>);
    }

    SECTION("container ctor requires sized_range + exact value_type")
    {
        STATIC_REQUIRE(std::is_constructible_v<AliasMethod<double>,
                                               const std::vector<double>&>);
        STATIC_REQUIRE(std::is_constructible_v<AliasMethod<double>,
                                               const std::array<double, 3>&>);
        STATIC_REQUIRE_FALSE(std::is_constructible_v<AliasMethod<double>,
                                                     const std::vector<int>&>);
        STATIC_REQUIRE_FALSE(std::is_constructible_v<AliasMethod<double>,
                                                     const std::vector<float>&>);
    }

    SECTION("deduction guide: from container")
    {
        std::vector<double> v{ 1.0, 2.0, 3.0 };
        auto am = AliasMethod{ v };
        STATIC_REQUIRE(std::is_same_v<decltype(am), AliasMethod<double>>);
        REQUIRE(am.size() == 3);

        std::vector<float> vf{ 1.0f, 2.0f };
        auto amf = AliasMethod{ vf };
        STATIC_REQUIRE(std::is_same_v<decltype(amf), AliasMethod<float>>);
    }

    SECTION("deduction guide: from iterator pair")
    {
        std::vector<double> v{ 1.0, 2.0, 3.0 };
        auto am = AliasMethod{ v.begin(), v.end() };
        STATIC_REQUIRE(std::is_same_v<decltype(am), AliasMethod<double>>);
        REQUIRE(am.size() == 3);
    }

    SECTION("explicit container ctor blocks implicit conversion")
    {
        // static_cast — ok, но не implicit
        std::vector<double> v{ 1.0, 2.0 };
        STATIC_REQUIRE_FALSE(
            (std::is_convertible_v<const std::vector<double>&, AliasMethod<double>>));
        REQUIRE_NOTHROW(static_cast<AliasMethod<double>>(v));
    }

    SECTION("size() is noexcept, sample() is not")
    {
        STATIC_REQUIRE(noexcept(std::declval<const AliasMethod<double>&>().size()));
        STATIC_REQUIRE_FALSE(noexcept(std::declval<const AliasMethod<double>&>().sample()));
        STATIC_REQUIRE_FALSE(
            noexcept(std::declval<const AliasMethod<double>&>()
                         .sample(std::declval<mylib::random::QualityXorshift64&>())));
    }
}


// ============================================================================
//  AliasMethod: конструкторы
// ============================================================================
TEST_CASE("AliasMethod construction", "[rng][alias][construction]")
{
    using mylib::random::AliasMethod;

    SECTION("from initializer_list")
    {
        AliasMethod<double> am{ 1.0, 2.0, 3.0 };
        REQUIRE(am.size() == 3);
    }

    SECTION("from iterator pair")
    {
        std::vector<double> v{ 0.5, 0.5 };
        AliasMethod<double> am{ v.begin(), v.end() };
        REQUIRE(am.size() == 2);
    }

    SECTION("from container")
    {
        std::vector<double> v{ 1.0, 2.0, 3.0, 4.0 };
        AliasMethod<double> am{ v };
        REQUIRE(am.size() == 4);
    }

    SECTION("from std::array")
    {
        std::array<double, 5> arr{ 1.0, 1.0, 1.0, 1.0, 1.0 };
        AliasMethod<double> am{ arr };
        REQUIRE(am.size() == 5);
    }

    SECTION("works with float and long double")
    {
        AliasMethod<float> amf{ 1.0f, 2.0f, 3.0f };
        REQUIRE(amf.size() == 3);
        AliasMethod<long double> aml{ 1.0L, 2.0L, 3.0L };
        REQUIRE(aml.size() == 3);
    }

    SECTION("single-element table")
    {
        AliasMethod<double> am{ 5.0 };
        REQUIRE(am.size() == 1);
    }
}


// ============================================================================
//  AliasMethod: валидация входных данных
// ============================================================================
TEST_CASE("AliasMethod input validation", "[rng][alias][validation]")
{
    using mylib::random::AliasMethod;

    SECTION("empty range -> invalid_argument")
    {
        STATIC_REQUIRE_FALSE(std::is_default_constructible_v<AliasMethod<double>>);

        std::vector<double> empty;
        REQUIRE_THROWS_AS(AliasMethod<double>(empty.begin(), empty.end()),
                          std::invalid_argument);
        REQUIRE_THROWS_AS((AliasMethod<double>(empty)), std::invalid_argument);

        std::array<double, 0> emptyArr{};
        REQUIRE_THROWS_AS(AliasMethod<double>(emptyArr.begin(), emptyArr.end()),
                          std::invalid_argument);
        REQUIRE_THROWS_AS((AliasMethod<double>(emptyArr)), std::invalid_argument);
    }

    SECTION("negative weight -> invalid_argument")
    {
        REQUIRE_THROWS_AS((AliasMethod<double>{ -1.0, 2.0 }),
                          std::invalid_argument);
        REQUIRE_THROWS_AS((AliasMethod<double>{ 1.0, 2.0, -0.001 }),
                          std::invalid_argument);
    }

    SECTION("NaN weight -> invalid_argument")
    {
        const double nan = std::nan("");
        REQUIRE_THROWS_AS((AliasMethod<double>{ nan, 1.0 }),
                          std::invalid_argument);
    }

    SECTION("infinite weight -> invalid_argument")
    {
        const double inf = std::numeric_limits<double>::infinity();
        REQUIRE_THROWS_AS((AliasMethod<double>{ inf, 1.0 }),
                          std::invalid_argument);
        REQUIRE_THROWS_AS((AliasMethod<double>{ -inf, 1.0 }),
                          std::invalid_argument);
    }

    SECTION("all-zero weights -> invalid_argument")
    {
        REQUIRE_THROWS_AS((AliasMethod<double>{ 0.0 }), std::invalid_argument);
        REQUIRE_THROWS_AS((AliasMethod<double>{ 0.0, 0.0, 0.0 }),
                          std::invalid_argument);
    }

    SECTION("overflowing sum -> invalid_argument")
    {
        const double big = std::numeric_limits<double>::max();
        // big + big == inf, sum не finite
        REQUIRE_THROWS_AS((AliasMethod<double>{ big, big }),
                          std::invalid_argument);
        REQUIRE_THROWS_AS((AliasMethod<double>{ big, big, big }),
                          std::invalid_argument);
    }

    SECTION("zero alongside positive is OK")
    {
        REQUIRE_NOTHROW((AliasMethod<double>{ 0.0, 1.0 }));
        REQUIRE_NOTHROW((AliasMethod<double>{ 1.0, 0.0, 0.0 }));
    }

    SECTION("very small positive weights are OK (no underflow to zero sum)")
    {
        REQUIRE_NOTHROW((AliasMethod<double>{ 1e-300, 2e-300 }));
        REQUIRE_NOTHROW((AliasMethod<double>{ 1e-30, 1e-30 }));
    }
}


// ============================================================================
//  AliasMethod: sample(gen) — детерминизм и корректность
// ============================================================================
TEST_CASE("AliasMethod::sample(gen) determinism and range",
          "[rng][alias][sample]")
{
    using mylib::random::AliasMethod;
    using mylib::random::QualityXorshift64;

    SECTION("same seed -> identical stream")
    {
        AliasMethod<double> am{ 1.0, 2.0, 3.0 };
        QualityXorshift64 a{ 42 };
        QualityXorshift64 b{ 42 };
        for (int i = 0; i < 1000; ++i)
            REQUIRE(am.sample(a) == am.sample(b));
    }

    SECTION("all outputs in [0, size())")
    {
        AliasMethod<double> am{ 1.0, 2.0, 3.0, 4.0, 5.0 };
        QualityXorshift64 gen{ 42 };
        for (int i = 0; i < 10'000; ++i)
        {
            const size_t idx = am.sample(gen);
            INFO("idx = " << idx);
            REQUIRE(idx < am.size());
        }
    }

    SECTION("sample is callable on const AliasMethod&")
    {
        const AliasMethod<double> am{ 1.0, 1.0 };
        QualityXorshift64 gen{ 42 };
        for (int i = 0; i < 100; ++i)
            REQUIRE(am.sample(gen) < 2);
    }

    SECTION("works with different GENERATOR types")
    {
        AliasMethod<double> am{ 1.0, 2.0, 3.0 };

        QualityXorshift64 xs{ 42 };
        mylib::random::MersenneTwister<std::uint64_t> mt{ 42 };
        mylib::Randomizer<> rnd{ 42 };

        for (int i = 0; i < 200; ++i)
        {
            REQUIRE(am.sample(xs) < 3);
            REQUIRE(am.sample(mt) < 3);
            REQUIRE(am.sample(rnd) < 3);
        }
    }
}


// ============================================================================
//  AliasMethod: статистические проверки
// ============================================================================
TEST_CASE("AliasMethod statistical distribution", "[rng][alias][statistics]")
{
    using mylib::random::AliasMethod;
    using mylib::random::QualityXorshift64;

    auto sampleCounts = [](const auto& am, size_t N, std::uint64_t seed = 42)
    {
        QualityXorshift64 gen{ seed };
        std::vector<size_t> counts(am.size(), 0);
        for (size_t i = 0; i < N; ++i)
            ++counts[am.sample(gen)];
        return counts;
    };

    SECTION("weights {1, 2, 3} -> {1/6, 2/6, 3/6}")
    {
        AliasMethod<double> am{ 1.0, 2.0, 3.0 };
        constexpr size_t N{ 100'000 };
        auto counts = sampleCounts(am, N);

        const std::array<double, 3> expected{ 1.0 / 6, 2.0 / 6, 3.0 / 6 };
        for (size_t k = 0; k < 3; ++k)
        {
            const double freq = static_cast<double>(counts[k]) / N;
            INFO("k = " << k << ", freq = " << freq
                        << ", expected = " << expected[k]);
            REQUIRE(freq == Catch::Approx(expected[k]).epsilon(0.05));
        }
    }

    SECTION("uniform weights {1, 1, 1, 1, 1}")
    {
        AliasMethod<double> am{ 1.0, 1.0, 1.0, 1.0, 1.0 };
        constexpr size_t N{ 100'000 };
        auto counts = sampleCounts(am, N);
        for (size_t k = 0; k < 5; ++k)
        {
            INFO("k = " << k << ", count = " << counts[k]);
            REQUIRE(counts[k] > 18'000);
            REQUIRE(counts[k] < 22'000);
        }
    }

    SECTION("weights {10, 1, 1} -> ~{10/12, 1/12, 1/12}")
    {
        AliasMethod<double> am{ 10.0, 1.0, 1.0 };
        constexpr size_t N{ 100'000 };
        auto counts = sampleCounts(am, N);

        const std::array<double, 3> expected{ 10.0 / 12, 1.0 / 12, 1.0 / 12 };
        for (size_t k = 0; k < 3; ++k)
        {
            const double freq = static_cast<double>(counts[k]) / N;
            INFO("k = " << k << ", freq = " << freq);
            REQUIRE(freq == Catch::Approx(expected[k]).epsilon(0.05));
        }
    }

    SECTION("single-element table always returns 0")
    {
        AliasMethod<double> am{ 7.5 };
        constexpr size_t N{ 1000 };
        auto counts = sampleCounts(am, N);
        REQUIRE(counts[0] == N);
    }

    SECTION("zero weights are never sampled")
    {
        AliasMethod<double> am{ 0.0, 1.0, 0.0, 1.0, 0.0 };
        constexpr size_t N{ 20'000 };
        auto counts = sampleCounts(am, N);

        REQUIRE(counts[0] == 0);
        REQUIRE(counts[2] == 0);
        REQUIRE(counts[4] == 0);
        // Только 1 и 3, примерно 50/50
        REQUIRE(counts[1] > 9'000);
        REQUIRE(counts[1] < 11'000);
        REQUIRE(counts[3] > 9'000);
        REQUIRE(counts[3] < 11'000);
    }

    SECTION("extreme weights: one dominant")
    {
        AliasMethod<double> am{ 1.0, 1e-6, 1e-6 };
        constexpr size_t N{ 20'000 };
        auto counts = sampleCounts(am, N);
        // Почти всё на индекс 0
        REQUIRE(counts[0] > static_cast<size_t>(N * 0.99));
        REQUIRE(counts[0] + counts[1] + counts[2] == N);
    }

    SECTION("float RealType also distributes correctly")
    {
        AliasMethod<float> am{ 1.0f, 2.0f, 3.0f };
        constexpr size_t N{ 50'000 };
        auto counts = sampleCounts(am, N);

        const std::array<double, 3> expected{ 1.0 / 6, 2.0 / 6, 3.0 / 6 };
        for (size_t k = 0; k < 3; ++k)
        {
            const double freq = static_cast<double>(counts[k]) / N;
            INFO("k = " << k << ", freq = " << freq);
            REQUIRE(freq == Catch::Approx(expected[k]).epsilon(0.05));
        }
    }
}


// ============================================================================
//  AliasMethod: sample() без аргументов (thread_local RNG)
// ============================================================================
TEST_CASE("AliasMethod::sample() no-arg overload",
          "[rng][alias][sample][thread_local]")
{
    using mylib::random::AliasMethod;

    SECTION("returns values in [0, size())")
    {
        AliasMethod<double> am{ 1.0, 2.0, 3.0, 4.0 };
        for (int i = 0; i < 5000; ++i)
        {
            const size_t idx = am.sample();
            INFO("idx = " << idx);
            REQUIRE(idx < am.size());
        }
    }

    SECTION("callable on const AliasMethod&")
    {
        const AliasMethod<double> am{ 1.0, 2.0, 3.0 };
        for (int i = 0; i < 100; ++i)
            REQUIRE(am.sample() < 3);
    }

    SECTION("statistical check on weights {1, 2, 3}")
    {
        AliasMethod<double> am{ 1.0, 2.0, 3.0 };
        constexpr size_t N{ 100'000 };
        std::vector<size_t> counts(3, 0);
        for (size_t i = 0; i < N; ++i)
            ++counts[am.sample()];

        const std::array<double, 3> expected{ 1.0 / 6, 2.0 / 6, 3.0 / 6 };
        for (size_t k = 0; k < 3; ++k)
        {
            const double freq = static_cast<double>(counts[k]) / N;
            INFO("k = " << k << ", freq = " << freq);
            REQUIRE(freq == Catch::Approx(expected[k]).epsilon(0.05));
        }
    }

    SECTION("single element always returns 0")
    {
        AliasMethod<double> am{ 5.0 };
        for (int i = 0; i < 100; ++i)
            REQUIRE(am.sample() == 0);
    }
}


// ============================================================================
//  AliasMethod: инварианты / дополнительные граничные случаи
// ============================================================================
TEST_CASE("AliasMethod edge cases", "[rng][alias][edge]")
{
    using mylib::random::AliasMethod;
    using mylib::random::QualityXorshift64;

    SECTION("very large table (size 1000) works")
    {
        std::vector<double> weights(1000, 1.0);
        AliasMethod<double> am{ weights };
        REQUIRE(am.size() == 1000);

        QualityXorshift64 gen{ 42 };
        for (int i = 0; i < 5000; ++i)
            REQUIRE(am.sample(gen) < 1000);
    }

    SECTION("weights with tiny differences")
    {
        // Веса, различающиеся на 1 ULP — должны всё равно работать
        AliasMethod<double> am{
            std::nextafter(1.0, 2.0),
            std::nextafter(1.0, 0.0),
            1.0
        };
        REQUIRE(am.size() == 3);

        QualityXorshift64 gen{ 42 };
        for (int i = 0; i < 1000; ++i)
            REQUIRE(am.sample(gen) < 3);
    }

    SECTION("integer-typed vector does not compile")
    {
        STATIC_REQUIRE_FALSE(
            std::is_constructible_v<AliasMethod<double>,
                                    const std::vector<int>&>);
        STATIC_REQUIRE_FALSE(
            std::is_constructible_v<AliasMethod<float>,
                                    const std::vector<double>&>);
    }

    SECTION("copy / move semantics")
    {
        AliasMethod<double> a{ 1.0, 2.0, 3.0 };
        AliasMethod<double> b{ a };        // copy
        REQUIRE(b.size() == 3);

        AliasMethod<double> c{ std::move(a) };
        REQUIRE(c.size() == 3);

        QualityXorshift64 gen1{ 42 };
        QualityXorshift64 gen2{ 42 };
        for (int i = 0; i < 100; ++i)
            REQUIRE(b.sample(gen1) == c.sample(gen2));
    }

    SECTION("all-equal weights -> uniform")
    {
        AliasMethod<double> am{ 7.0, 7.0, 7.0, 7.0, 7.0 };
        QualityXorshift64 gen{ 42 };
        std::array<int, 5> counts{};
        constexpr int N{ 50'000 };
        for (int i = 0; i < N; ++i) ++counts[am.sample(gen)];
        for (int k = 0; k < 5; ++k)
        {
            INFO("k = " << k << ", count = " << counts[k]);
            REQUIRE(counts[k] > 9'000);
            REQUIRE(counts[k] < 11'000);
        }
    }

    SECTION("one weight = 1, rest = 0")
    {
        AliasMethod<double> am{ 0.0, 0.0, 1.0, 0.0 };
        QualityXorshift64 gen{ 42 };
        for (int i = 0; i < 1000; ++i)
            REQUIRE(am.sample(gen) == 2);
    }
}


// ============================================================================
//  AliasMethod: TEMPLATE_TEST_CASE по RealType
// ============================================================================
TEMPLATE_TEST_CASE("AliasMethod works for all floating-point types",
                   "[rng][alias][template]",
                   float, double, long double)
{
    using T = TestType;
    using AM = mylib::random::AliasMethod<T>;
    using Gen = mylib::random::QualityXorshift64;

    SECTION("construction and size")
    {
        AM am{ T{ 1 }, T{ 2 }, T{ 3 } };
        REQUIRE(am.size() == 3);
    }

    SECTION("sampling stays in range")
    {
        AM am{ T{ 1 }, T{ 2 }, T{ 3 }, T{ 4 } };
        Gen gen{ 42 };
        for (int i = 0; i < 5000; ++i)
            REQUIRE(am.sample(gen) < 4);
    }

    SECTION("statistical correctness on weights {1, 2, 3}")
    {
        AM am{ T{ 1 }, T{ 2 }, T{ 3 } };
        Gen gen{ 42 };
        constexpr size_t N{ 50'000 };
        std::vector<size_t> counts(3, 0);
        for (size_t i = 0; i < N; ++i)
            ++counts[am.sample(gen)];

        const std::array<double, 3> expected{ 1.0 / 6, 2.0 / 6, 3.0 / 6 };
        for (size_t k = 0; k < 3; ++k)
        {
            const double freq = static_cast<double>(counts[k]) / N;
            INFO("k = " << k << ", freq = " << freq);
            REQUIRE(freq == Catch::Approx(expected[k]).epsilon(0.05));
        }
    }
}



// ============================================================================
//  SumHeap: type traits, concepts, deduction
// ============================================================================
TEST_CASE("SumHeap type traits", "[rng][sumheap][traits]")
{
    using mylib::random::SumHeap;

    SECTION("requires unsigned integral T")
    {
        STATIC_REQUIRE(std::is_constructible_v<SumHeap<std::uint8_t>>);
        STATIC_REQUIRE(std::is_constructible_v<SumHeap<std::uint16_t>>);
        STATIC_REQUIRE(std::is_constructible_v<SumHeap<std::uint32_t>>);
        STATIC_REQUIRE(std::is_constructible_v<SumHeap<std::uint64_t>>);

        // signed / non-integral должны отвергаться concept'ом
    }

    SECTION("default ctor is available")
    {
        STATIC_REQUIRE(std::is_default_constructible_v<SumHeap<std::uint32_t>>);
    }

    SECTION("initializer_list ctor constrained to exact T")
    {
        STATIC_REQUIRE(std::is_constructible_v<SumHeap<std::uint32_t>,
                                               std::initializer_list<std::uint32_t>>);
        STATIC_REQUIRE_FALSE(std::is_constructible_v<SumHeap<std::uint32_t>,
                                                     std::initializer_list<int>>);
        STATIC_REQUIRE_FALSE(std::is_constructible_v<SumHeap<std::uint32_t>,
                                                     std::initializer_list<std::uint8_t>>);
    }

    SECTION("iterator ctor requires same_as<T>")
    {
        using It = std::vector<std::uint32_t>::iterator;
        STATIC_REQUIRE(std::is_constructible_v<SumHeap<std::uint32_t>, It, It>);
        STATIC_REQUIRE_FALSE(std::is_constructible_v<
                             SumHeap<std::uint32_t>,
                             std::vector<int>::iterator, std::vector<int>::iterator>);
    }

    SECTION("container ctor requires sized_range + same_as<T>")
    {
        STATIC_REQUIRE(std::is_constructible_v<SumHeap<std::uint32_t>,
                                               const std::vector<std::uint32_t>&>);
        STATIC_REQUIRE(std::is_constructible_v<SumHeap<std::uint32_t>,
                                               const std::array<std::uint32_t, 3>&>);
        STATIC_REQUIRE_FALSE(std::is_constructible_v<SumHeap<std::uint32_t>,
                                                     const std::vector<int>&>);
    }

    SECTION("noexcept queries")
    {
        using S = SumHeap<std::uint32_t>;
        STATIC_REQUIRE(noexcept(std::declval<const S&>().total()));
        STATIC_REQUIRE(noexcept(std::declval<const S&>().get(0)));
        STATIC_REQUIRE(noexcept(std::declval<const S&>().cumulative(0)));
        STATIC_REQUIRE(noexcept(std::declval<S&>().add(std::uint32_t{}, 0)));
        STATIC_REQUIRE(noexcept(std::declval<S&>().subtract(std::uint32_t{}, 0)));
        STATIC_REQUIRE(noexcept(std::declval<S&>().remove(0)));
    }

    SECTION("mutators/append/next are not noexcept")
    {
        using S = SumHeap<std::uint32_t>;
        STATIC_REQUIRE_FALSE(noexcept(std::declval<S&>().add(std::uint32_t{})));
        STATIC_REQUIRE_FALSE(noexcept(std::declval<const S&>().next()));
        STATIC_REQUIRE_FALSE(noexcept(std::declval<const S&>().next(
            std::declval<mylib::random::QualityXorshift64&>())));
    }
}


// ============================================================================
//  SumHeap: конструкторы
// ============================================================================
TEST_CASE("SumHeap construction", "[rng][sumheap][construction]")
{
    using mylib::random::SumHeap;
    using S = SumHeap<std::uint32_t>;

    SECTION("default ctor produces empty collection")
    {
        S sh;
        REQUIRE(sh.total() == 0);
        REQUIRE(sh.get(0) == 0);        // out-of-range may return 0 by doc
        REQUIRE(sh.next() == 0);        // total == 0 -> 0
    }

    SECTION("initializer_list")
    {
        S sh{ 1u, 2u, 3u, 4u };
        REQUIRE(sh.total() == 10);
        for (size_t i = 0; i < 4; ++i)
            REQUIRE(sh.get(i) == i + 1);
    }

    SECTION("iterator pair")
    {
        std::vector<std::uint32_t> v{ 5u, 10u, 15u };
        S sh{ v.begin(), v.end() };
        REQUIRE(sh.total() == 30);
        REQUIRE(sh.get(0) == 5);
        REQUIRE(sh.get(1) == 10);
        REQUIRE(sh.get(2) == 15);
    }

    SECTION("container")
    {
        std::vector<std::uint32_t> v{ 7u, 8u, 9u };
        S sh{ v };
        REQUIRE(sh.total() == 24);
    }

    SECTION("std::array")
    {
        std::array<std::uint32_t, 4> arr{ 1u, 1u, 1u, 1u };
        S sh{ arr };
        REQUIRE(sh.total() == 4);
    }

    SECTION("zeros-only is allowed by ctor")
    {
        S sh{ 0u, 0u, 0u };
        REQUIRE(sh.total() == 0);
        REQUIRE(sh.next() == 0);
    }

    SECTION("works with uint8_t, uint16_t, uint64_t")
    {
        REQUIRE(SumHeap<std::uint8_t>{ std::uint8_t{ 1 }, std::uint8_t{ 2 } }.total() == 3);
        REQUIRE(SumHeap<std::uint16_t>{ std::uint16_t{ 100 }, std::uint16_t{ 200 } }.total() == 300);
        REQUIRE(SumHeap<std::uint64_t>{ std::uint64_t{ 1'000'000'000ULL },
                                       std::uint64_t{ 2'000'000'000ULL } }.total()
                == 3'000'000'000ULL);
    }
}


// ============================================================================
//  SumHeap: валидация входных данных
// ============================================================================
TEST_CASE("SumHeap input validation", "[rng][sumheap][validation]")
{
    using mylib::random::SumHeap;

    SECTION("empty range -> invalid_argument")
    {
        std::vector<std::uint32_t> empty;
        REQUIRE_THROWS_AS(
            SumHeap<std::uint32_t>(empty.begin(), empty.end()),
            std::invalid_argument);
        REQUIRE_THROWS_AS((SumHeap<std::uint32_t>(empty)),
                          std::invalid_argument);
    }

    SECTION("sum overflow -> overflow_error")
    {
        // uint8_t: 200 + 100 = 300 > 255
        REQUIRE_THROWS_AS(
            (SumHeap<std::uint8_t>{ std::uint8_t{ 200 }, std::uint8_t{ 100 } }),
            std::length_error);
        REQUIRE_THROWS_AS(
            (SumHeap<std::uint8_t>{ std::uint8_t{ 255 }, std::uint8_t{ 1 } }),
            std::length_error);

        // uint32_t: max + 1
        const auto mx = std::numeric_limits<std::uint32_t>::max();
        REQUIRE_THROWS_AS(
            (SumHeap<std::uint32_t>{ mx, std::uint32_t{ 1 } }),
            std::length_error);
        REQUIRE_THROWS_AS(
            (SumHeap<std::uint32_t>{ mx, mx }),
            std::length_error);

        // uint64_t: max + 1
        const auto mx64 = std::numeric_limits<std::uint64_t>::max();
        REQUIRE_THROWS_AS(
            (SumHeap<std::uint64_t>{ mx64, std::uint64_t{ 1 } }),
            std::length_error);
    }

    SECTION("exactly max is OK")
    {
        const auto mx = std::numeric_limits<std::uint8_t>::max();
        REQUIRE_NOTHROW(
            (SumHeap<std::uint8_t>{ std::uint8_t{ 100 }, std::uint8_t{ 155 } }));
        REQUIRE(SumHeap<std::uint8_t>{
                    std::uint8_t{ 100 }, std::uint8_t{ 155 }
                }.total() == static_cast<std::uint8_t>(mx));
    }
}


// ============================================================================
//  SumHeap: total / get / cumulative
// ============================================================================
TEST_CASE("SumHeap queries", "[rng][sumheap][queries]")
{
    using mylib::random::SumHeap;
    using S = SumHeap<std::uint32_t>;

    SECTION("total() == sum of get(i)")
    {
        S sh{ 1u, 2u, 3u, 4u, 5u };
        std::uint32_t sum = 0;
        for (size_t i = 0; i < 5; ++i) sum += sh.get(i);
        REQUIRE(sh.total() == sum);
        REQUIRE(sh.total() == 15);
    }

    SECTION("get(i) returns exact weight")
    {
        S sh{ 10u, 20u, 30u };
        REQUIRE(sh.get(0) == 10);
        REQUIRE(sh.get(1) == 20);
        REQUIRE(sh.get(2) == 30);
    }
}

TEST_CASE("SumHeap::cumulative — right-boundary invariants",
          "[rng][sumheap][cumulative]")
{
    using mylib::random::SumHeap;
    using S = SumHeap<std::uint32_t>;

    SECTION("cumulative(0) == total() (root covers [0, total()))")
    {
        for (auto w : { std::vector<std::uint32_t>{1},
                       std::vector<std::uint32_t>{1, 2},
                       std::vector<std::uint32_t>{1, 2, 3},
                       std::vector<std::uint32_t>{3, 1, 4, 1, 5},
                       std::vector<std::uint32_t>{10, 20, 30, 40} })
        {
            S sh{ w };
            INFO("size = " << w.size());
            REQUIRE(sh.cumulative(0) == sh.total());
        }
    }

    SECTION("cumulative(i) <= total() for all i")
    {
        S sh{ 3u, 1u, 4u, 1u, 5u };
        for (size_t i = 0; i < 5; ++i)
        {
            INFO("i = " << i);
            REQUIRE(sh.cumulative(i) <= sh.total());
        }
    }

    SECTION("cumulative(i) >= get(i) for all i")
    {
        S sh{ 3u, 1u, 4u, 1u, 5u };
        for (size_t i = 0; i < 5; ++i)
        {
            INFO("i = " << i << ", cum = " << sh.cumulative(i)
                 << ", own = " << sh.get(i));
            REQUIRE(sh.cumulative(i) >= sh.get(i));
        }
    }

    SECTION("cumulative(child) <= cumulative(parent) for all non-root i")
    {
        // Поддерево ребёнка — подмножество поддерева родителя,
        // значит правый конец не может быть больше.
        S sh{ 3u, 1u, 4u, 1u, 5u };
        for (size_t i = 1; i < 5; ++i)
        {
            const size_t p = (i - 1) / 2;
            INFO("i = " << i << ", parent = " << p
                        << ", cum(i) = " << sh.cumulative(i)
                        << ", cum(p) = " << sh.cumulative(p));
            REQUIRE(sh.cumulative(i) <= sh.cumulative(p));
        }
    }

    SECTION("single-element: cum(0) == total == get(0)")
    {
        S sh{ 42u };
        REQUIRE(sh.cumulative(0) == 42u);
        REQUIRE(sh.cumulative(0) == sh.total());
        REQUIRE(sh.cumulative(0) == sh.get(0));
    }

    SECTION("two-element tree {2, 3}: root own=2, left child own=3")
    {
        // m_heap = [5, 3]: total=5, own(0)=2, own(1)=3
        S sh{ 2u, 3u };
        REQUIRE(sh.total() == 5u);
        REQUIRE(sh.get(0) == 2u);
        REQUIRE(sh.get(1) == 3u);

        REQUIRE(sh.cumulative(0) == 5u);   // whole range
        REQUIRE(sh.cumulative(1) == 3u);   // left child interval [0, 3)
    }

    SECTION("cumulative(0) tracks total after mutations")
    {
        S sh{ 1u, 2u, 3u };
        REQUIRE(sh.cumulative(0) == 6u);
        REQUIRE(sh.total() == 6u);

        sh.add(10u);                        // total 16
        REQUIRE(sh.cumulative(0) == 16u);
        REQUIRE(sh.total() == 16u);

        sh.remove(0);                       // remove root's own weight (1)
        REQUIRE(sh.cumulative(0) == 15u);
        REQUIRE(sh.total() == 15u);

        sh.subtract(1u, 1);   // get(1) == 2, subtract 1 → own(1) == 1
        REQUIRE(sh.get(1) == 1u);
        REQUIRE(sh.total() == 14u);
    }
}

TEST_CASE("SumHeap::cumulative is noexcept", "[rng][sumheap][cumulative]")
{
    using mylib::random::SumHeap;
    using S = SumHeap<std::uint32_t>;
    S sh{ 1u, 2u, 3u };
    STATIC_REQUIRE(noexcept(sh.cumulative(0)));
}


// ============================================================================
//  SumHeap: модификаторы
// ============================================================================
TEST_CASE("SumHeap modifiers", "[rng][sumheap][modifiers]")
{
    using mylib::random::SumHeap;
    using S = SumHeap<std::uint32_t>;

    SECTION("add(value) appends a new element")
    {
        S sh{ 1u, 2u };
        REQUIRE(sh.total() == 3);

        sh.add(10u);
        REQUIRE(sh.total() == 13);
        REQUIRE(sh.get(2) == 10);

        sh.add(0u);                     // нулевой вес тоже допустим
        REQUIRE(sh.total() == 13);
        REQUIRE(sh.get(3) == 0);
    }

    SECTION("add(value, i) increases weight")
    {
        S sh{ 1u, 2u, 3u };
        REQUIRE(sh.total() == 6);

        sh.add(10u, 0);
        REQUIRE(sh.get(0) == 11);
        REQUIRE(sh.total() == 16);

        sh.add(5u, 2);
        REQUIRE(sh.get(2) == 8);
        REQUIRE(sh.total() == 21);
    }

    SECTION("subtract(value, i) decreases weight")
    {
        S sh{ 10u, 20u, 30u };
        sh.subtract(5u, 0);
        REQUIRE(sh.get(0) == 5);
        REQUIRE(sh.total() == 55);

        sh.subtract(20u, 1);
        REQUIRE(sh.get(1) == 0);
        REQUIRE(sh.total() == 35);
    }

    SECTION("subtract to zero")
    {
        S sh{ 5u, 5u };
        sh.subtract(5u, 0);
        REQUIRE(sh.get(0) == 0);
        REQUIRE(sh.total() == 5);
    }

    SECTION("remove(i) sets weight to zero")
    {
        S sh{ 1u, 2u, 3u };
        REQUIRE(sh.total() == 6);

        sh.remove(1);
        REQUIRE(sh.get(1) == 0);
        REQUIRE(sh.total() == 4);

        sh.remove(0);
        REQUIRE(sh.get(0) == 0);
        REQUIRE(sh.total() == 3);
    }

    SECTION("remove is idempotent")
    {
        S sh{ 1u, 2u };
        sh.remove(0);
        sh.remove(0);
        REQUIRE(sh.get(0) == 0);
        REQUIRE(sh.total() == 2);
    }

    SECTION("removed then re-added via add(value, i)")
    {
        S sh{ 1u, 2u, 3u };
        sh.remove(1);
        REQUIRE(sh.get(1) == 0);

        sh.add(7u, 1);
        REQUIRE(sh.get(1) == 7);
        REQUIRE(sh.total() == 11);
    }

    SECTION("total() tracks all mutations")
    {
        S sh{ 5u, 5u, 5u };
        REQUIRE(sh.total() == 15);

        sh.add(10u);
        REQUIRE(sh.total() == 25);

        sh.add(100u, 0);
        REQUIRE(sh.total() == 125);

        sh.subtract(50u, 0);
        REQUIRE(sh.total() == 75);

        sh.remove(1);
        REQUIRE(sh.total() == 70);

        sh.remove(2);
        REQUIRE(sh.total() == 65);
    }
}


// ============================================================================
//  SumHeap: sample(gen) — детерминизм и диапазон
// ============================================================================
TEST_CASE("SumHeap sampling with explicit generator",
          "[rng][sumheap][sample]")
{
    using mylib::random::SumHeap;
    using mylib::random::QualityXorshift64;
    using S = SumHeap<std::uint32_t>;

    SECTION("same seed -> identical stream")
    {
        S sh{ 1u, 2u, 3u, 4u };
        QualityXorshift64 a{ 42 };
        QualityXorshift64 b{ 42 };
        for (int i = 0; i < 1000; ++i)
            REQUIRE(sh.next(a) == sh.next(b));
    }

    SECTION("all outputs in [0, size)")
    {
        S sh{ 1u, 2u, 3u, 4u, 5u };
        QualityXorshift64 gen{ 42 };
        for (int i = 0; i < 10'000; ++i)
        {
            const auto idx = sh.next(gen);
            INFO("idx = " << idx);
            REQUIRE(idx < 5);
        }
    }

    SECTION("callable on const SumHeap&")
    {
        const S sh{ 1u, 2u, 3u };
        QualityXorshift64 gen{ 42 };
        for (int i = 0; i < 100; ++i)
            REQUIRE(sh.next(gen) < 3);
    }

    SECTION("empty -> returns 0")
    {
        S sh;
        QualityXorshift64 gen{ 42 };
        for (int i = 0; i < 100; ++i)
            REQUIRE(sh.next(gen) == 0);
    }

    SECTION("all-zero weights -> returns 0")
    {
        S sh{ 0u, 0u, 0u };
        QualityXorshift64 gen{ 42 };
        for (int i = 0; i < 100; ++i)
            REQUIRE(sh.next(gen) == 0);
    }

    SECTION("single element with positive weight -> returns its index")
    {
        S sh{ 0u, 0u, 5u, 0u };
        QualityXorshift64 gen{ 42 };
        for (int i = 0; i < 1000; ++i)
            REQUIRE(sh.next(gen) == 2);
    }

    SECTION("works with different URBG types")
    {
        S sh{ 1u, 2u, 3u };
        QualityXorshift64 xs{ 42 };
        mylib::random::MersenneTwister<std::uint64_t> mt{ 42 };
        mylib::Randomizer<> rnd{ 42 };
        for (int i = 0; i < 200; ++i)
        {
            REQUIRE(sh.next(xs)  < 3);
            REQUIRE(sh.next(mt)  < 3);
            REQUIRE(sh.next(rnd) < 3);
        }
    }
}


// ============================================================================
//  SumHeap: next() без аргументов
// ============================================================================
TEST_CASE("SumHeap sampling without explicit generator",
          "[rng][sumheap][sample][thread_local]")
{
    using mylib::random::SumHeap;
    using S = SumHeap<std::uint32_t>;

    SECTION("returns values in [0, size)")
    {
        S sh{ 1u, 2u, 3u, 4u };
        for (int i = 0; i < 5000; ++i)
        {
            const auto idx = sh.next();
            INFO("idx = " << idx);
            REQUIRE(idx < 4);
        }
    }

    SECTION("callable on const SumHeap&")
    {
        const S sh{ 1u, 2u };
        for (int i = 0; i < 100; ++i)
            REQUIRE(sh.next() < 2);
    }

    SECTION("empty -> returns 0")
    {
        S sh;
        for (int i = 0; i < 100; ++i)
            REQUIRE(sh.next() == 0);
    }

    SECTION("single element -> always that index")
    {
        S sh{ 0u, 0u, 7u };
        for (int i = 0; i < 100; ++i)
            REQUIRE(sh.next() == 2);
    }
}


// ============================================================================
//  SumHeap: статистика
// ============================================================================
TEST_CASE("SumHeap statistical distribution", "[rng][sumheap][statistics]")
{
    using mylib::random::SumHeap;
    using mylib::random::QualityXorshift64;
    using S = SumHeap<std::uint32_t>;

    auto sampleCounts = [](const auto& sh, size_t n, size_t N, std::uint64_t seed = 42)
    {
        QualityXorshift64 gen{ seed };
        std::vector<size_t> counts(n, 0);
        for (size_t k = 0; k < N; ++k)
            ++counts[sh.next(gen)];
        return counts;
    };

    SECTION("weights {1,2,3,4} -> {1/10, 2/10, 3/10, 4/10}")
    {
        S sh{ 1u, 2u, 3u, 4u };
        constexpr size_t N{ 100'000 };
        auto counts = sampleCounts(sh, 4, N);

        const std::array<double, 4> expected{ 0.1, 0.2, 0.3, 0.4 };
        for (size_t k = 0; k < 4; ++k)
        {
            const double freq = static_cast<double>(counts[k]) / N;
            INFO("k = " << k << ", freq = " << freq
                        << ", expected = " << expected[k]);
            REQUIRE(freq == Catch::Approx(expected[k]).epsilon(0.05));
        }
    }

    SECTION("uniform weights distribute uniformly")
    {
        S sh{ 1u, 1u, 1u, 1u, 1u };
        constexpr size_t N{ 100'000 };
        auto counts = sampleCounts(sh, 5, N);
        for (size_t k = 0; k < 5; ++k)
        {
            INFO("k = " << k << ", count = " << counts[k]);
            REQUIRE(counts[k] > 18'000);
            REQUIRE(counts[k] < 22'000);
        }
    }

    SECTION("zero weights never sampled")
    {
        S sh{ 0u, 1u, 0u, 1u, 0u };
        constexpr size_t N{ 20'000 };
        auto counts = sampleCounts(sh, 5, N);
        REQUIRE(counts[0] == 0);
        REQUIRE(counts[2] == 0);
        REQUIRE(counts[4] == 0);
        REQUIRE(counts[1] > 9'000);
        REQUIRE(counts[3] > 9'000);
    }

    SECTION("removed elements are never sampled")
    {
        S sh{ 1u, 2u, 3u, 4u };
        sh.remove(1);
        constexpr size_t N{ 20'000 };
        auto counts = sampleCounts(sh, 4, N);
        REQUIRE(counts[1] == 0);
        // Теперь осталось {1, 0, 3, 4}, сумма 8
        const std::array<double, 4> expected{ 1.0 / 8, 0.0, 3.0 / 8, 4.0 / 8 };
        for (size_t k : { size_t{ 0 }, size_t{ 2 }, size_t{ 3 } })
        {
            const double freq = static_cast<double>(counts[k]) / N;
            INFO("k = " << k << ", freq = " << freq);
            REQUIRE(freq == Catch::Approx(expected[k]).epsilon(0.05));
        }
    }

    SECTION("after add(value, i) the weight is respected")
    {
        S sh{ 1u, 2u, 3u, 4u };
        sh.add(1000u, 0);
        constexpr size_t N{ 20'000 };
        auto counts = sampleCounts(sh, 4, N);
        // total = 1010, index 0 dominates
        REQUIRE(counts[0] > 19'000);
        REQUIRE(counts[0] <= N);
    }

    SECTION("after add(value) appended element participates")
    {
        S sh{ 1u, 1u, 1u, 1u, 1u };
        sh.add(5u);   // {1,1,1,1,1,5}, total 10 -> index 5 gets 50%
        constexpr size_t N{ 20'000 };
        auto counts = sampleCounts(sh, 6, N);
        INFO("count[5] = " << counts[5]);
        REQUIRE(counts[5] > 9'000);
        REQUIRE(counts[5] < 11'000);
    }

    SECTION("thread_local next() also distributes correctly")
    {
        S sh{ 1u, 2u, 3u };
        constexpr size_t N{ 100'000 };
        std::vector<size_t> counts(3, 0);
        for (size_t i = 0; i < N; ++i)
            ++counts[sh.next()];

        const std::array<double, 3> expected{ 1.0 / 6, 2.0 / 6, 3.0 / 6 };
        for (size_t k = 0; k < 3; ++k)
        {
            const double freq = static_cast<double>(counts[k]) / N;
            INFO("k = " << k << ", freq = " << freq);
            REQUIRE(freq == Catch::Approx(expected[k]).epsilon(0.05));
        }
    }
}


// ============================================================================
//  SumHeap: edge cases
// ============================================================================
TEST_CASE("SumHeap edge cases", "[rng][sumheap][edge]")
{
    using mylib::random::SumHeap;
    using mylib::random::QualityXorshift64;
    using S = SumHeap<std::uint32_t>;

    SECTION("large table")
    {
        std::vector<std::uint32_t> v(1000, 1u);
        S sh{ v };
        REQUIRE(sh.total() == 1000);

        QualityXorshift64 gen{ 42 };
        for (int i = 0; i < 5000; ++i)
            REQUIRE(sh.next(gen) < 1000);
    }

    SECTION("single-element with max weight")
    {
        const auto mx = std::numeric_limits<std::uint32_t>::max();
        S sh{ mx };
        REQUIRE(sh.total() == mx);
        QualityXorshift64 gen{ 42 };
        for (int i = 0; i < 100; ++i)
            REQUIRE(sh.next(gen) == 0);
    }

    SECTION("adding zero preserves total")
    {
        S sh{ 5u, 5u };
        sh.add(0u);
        REQUIRE(sh.total() == 10);
        REQUIRE(sh.get(2) == 0);
    }

    SECTION("subtract all weight from one element")
    {
        S sh{ 100u, 200u, 300u };
        sh.subtract(100u, 0);
        REQUIRE(sh.get(0) == 0);
        REQUIRE(sh.total() == 500);
    }

    SECTION("many appends preserve total")
    {
        S sh{ 1u };
        std::uint32_t expected = 1;
        for (int i = 0; i < 100; ++i)
        {
            sh.add(static_cast<std::uint32_t>(i + 1));
            expected += static_cast<std::uint32_t>(i + 1);
        }
        REQUIRE(sh.total() == expected);
    }

    SECTION("copy / move preserve weights")
    {
        S sh{ 1u, 2u, 3u, 4u };
        S copy{ sh };
        REQUIRE(copy.total() == sh.total());
        for (size_t i = 0; i < 4; ++i)
            REQUIRE(copy.get(i) == sh.get(i));

        S moved{ std::move(sh) };
        REQUIRE(moved.total() == 10);
    }

    SECTION("uint8_t with weights near max")
    {
        using S8 = SumHeap<std::uint8_t>;
        S8 sh{ std::uint8_t{ 100 }, std::uint8_t{ 100 }, std::uint8_t{ 55 } };
        REQUIRE(sh.total() == std::uint8_t{ 255 });

        QualityXorshift64 gen{ 42 };
        for (int i = 0; i < 1000; ++i)
            REQUIRE(sh.next(gen) < 3);
    }
}


// ============================================================================
//  SumHeap: TEMPLATE_TEST_CASE по T
// ============================================================================
TEMPLATE_TEST_CASE("SumHeap works for all unsigned integral types",
                   "[rng][sumheap][template]",
                   std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t)
{
    using T = TestType;
    using SH = mylib::random::SumHeap<T>;
    using Gen = mylib::random::QualityXorshift64;

    SECTION("construction and total")
    {
        SH sh{ T{ 1 }, T{ 2 }, T{ 3 } };
        REQUIRE(sh.total() == T{ 6 });
    }

    SECTION("sampling stays in range")
    {
        SH sh{ T{ 1 }, T{ 2 }, T{ 3 }, T{ 4 } };
        Gen gen{ 42 };
        for (int i = 0; i < 2000; ++i)
            REQUIRE(sh.next(gen) < 4);
    }

    SECTION("statistical correctness on {1,2,3}")
    {
        SH sh{ T{ 1 }, T{ 2 }, T{ 3 } };
        Gen gen{ 42 };
        constexpr size_t N{ 50'000 };
        std::vector<size_t> counts(3, 0);
        for (size_t k = 0; k < N; ++k)
            ++counts[sh.next(gen)];

        const std::array<double, 3> expected{ 1.0 / 6, 2.0 / 6, 3.0 / 6 };
        for (size_t k = 0; k < 3; ++k)
        {
            const double freq = static_cast<double>(counts[k]) / N;
            INFO("k = " << k << ", freq = " << freq);
            REQUIRE(freq == Catch::Approx(expected[k]).epsilon(0.05));
        }
    }

    SECTION("add / subtract / remove keep total consistent")
    {
        SH sh{ T{ 5 }, T{ 5 }, T{ 5 } };
        REQUIRE(sh.total() == T{ 15 });

        sh.add(T{ 10 });
        REQUIRE(sh.total() == T{ 25 });

        sh.add(T{ 3 }, 0);
        REQUIRE(sh.get(0) == T{ 8 });
        REQUIRE(sh.total() == T{ 28 });

        sh.subtract(T{ 4 }, 1);
        REQUIRE(sh.get(1) == T{ 1 });
        REQUIRE(sh.total() == T{ 24 });

        sh.remove(2);
        REQUIRE(sh.get(2) == T{ 0 });
        REQUIRE(sh.total() == T{ 19 });
    }

    SECTION("empty -> next() returns 0")
    {
        SH sh;
        Gen gen{ 42 };
        REQUIRE(sh.next(gen) == 0);
        REQUIRE(sh.next() == 0);
    }
}

