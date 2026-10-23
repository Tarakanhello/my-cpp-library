#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

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
    using namespace mylib::random;

    SECTION("default template argument is defaultRandomizer")
    {
        STATIC_REQUIRE(std::is_same_v<
                       Randomizer<>,
                       Randomizer<defaultRandomizer<>>>);
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
                       Randomizer<QualityXorshift64>>);
    }

    SECTION("satisfies URBG for MersenneTwister<uint32_t>")
    {
        STATIC_REQUIRE(std::uniform_random_bit_generator<
                       Randomizer<MersenneTwister<std::uint32_t>>>);
    }

    SECTION("satisfies URBG for TinyURBG")
    {
        STATIC_REQUIRE(std::uniform_random_bit_generator<
                       Randomizer<TinyURBG>>);
    }

    SECTION("result_type matches GENERATOR::result_type")
    {
        STATIC_REQUIRE(std::is_same_v<
                       Randomizer<QualityXorshift64>::result_type,
                       QualityXorshift64::result_type>);
        STATIC_REQUIRE(std::is_same_v<
                       Randomizer<MersenneTwister<std::uint32_t>>::result_type,
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
        STATIC_REQUIRE(Randomizer<MersenneTwister<std::uint32_t>>::max()
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
    using namespace mylib::random;
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
    using namespace mylib::random;

    SECTION("default ctor does not throw")
    {
        REQUIRE_NOTHROW(Randomizer<>{});
        REQUIRE_NOTHROW(Randomizer<QualityXorshift64>{});
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
        Randomizer<MersenneTwister<std::uint64_t>> wrapped{ 42ull };
        MersenneTwister<std::uint64_t> raw{ 42ull };

        for (int i = 0; i < 500; ++i)
        {
            INFO("i = " << i);
            REQUIRE(wrapped() == raw());
        }
    }

    SECTION("explicit seed ctor matches wrapped QualityXorshift64")
    {
        Randomizer<QualityXorshift64> wrapped{ 0xC0FFEEull };
        QualityXorshift64 raw{ 0xC0FFEEull };
        for (int i = 0; i < 500; ++i)
            REQUIRE(wrapped() == raw());
    }

    SECTION("SeedSequence ctor matches wrapped generator")
    {
        std::seed_seq seq1{ 1, 2, 3, 4, 5 };
        std::seed_seq seq2{ 1, 2, 3, 4, 5 };

        Randomizer<MersenneTwister<std::uint64_t>> wrapped{ seq1 };
        MersenneTwister<std::uint64_t> raw{ seq2 };

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
    using namespace mylib::random;

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
        defaultRandomizer<> raw{ 0xDEADBEEFCAFEULL };

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
    using namespace mylib::random;

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
    using namespace mylib::random;

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
    using namespace mylib::random;

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
    using namespace mylib::random;

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
        mylib::random::Randomizer<MaxURBG> r;
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
    using namespace mylib::random;

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
    using R = mylib::random::Randomizer<TestType>;

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
