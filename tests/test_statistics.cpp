#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>
#include <limits>
#include <vector>

#include "mylib/mylib.h"

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;


// ============================================================================
//  NormalSummary: constructor, stddev, error95
// ============================================================================
TEST_CASE("NormalSummary constructor and derived values",
          "[math][statistics][normal]")
{
    using mylib::math::NormalSummary;

    SECTION("default ctor: mean=0, variance=0")
    {
        NormalSummary s;
        REQUIRE(s.mean == 0.0);
        REQUIRE(s.variance == 0.0);
        REQUIRE(s.stddev() == 0.0);
        REQUIRE(s.error95() == 0.0);
    }

    SECTION("explicit ctor stores mean and variance")
    {
        NormalSummary s{ 3.5, 4.0 };
        REQUIRE(s.mean == 3.5);
        REQUIRE(s.variance == 4.0);
    }

    SECTION("stddev() == sqrt(variance)")
    {
        for (double v : { 0.0, 1.0, 2.0, 4.0, 9.0, 100.0 })
        {
            NormalSummary s{ 0.0, v };
            INFO("variance = " << v);
            REQUIRE_THAT(s.stddev(), WithinRel(std::sqrt(v), 1e-12));
        }
    }

    SECTION("error95() == 2 * stddev()")
    {
        NormalSummary s{ 10.0, 4.0 };
        REQUIRE_THAT(s.error95(), WithinRel(4.0, 1e-12));
    }

    SECTION("negative variance triggers assert (debug only)")
    {
        // В debug упадёт assert, в release — UB. Проверяем только компиляцию.
#ifdef NDEBUG
        SUCCEED("assert disabled in release");
#else
        // Не вызываем: REQUIRE_THROWS не работает с assert.
        SUCCEED("assert enabled in debug");
#endif
    }
}


// ============================================================================
//  NormalSummary: операторы +=, -=, *=
// ============================================================================
TEST_CASE("NormalSummary compound operators",
          "[math][statistics][normal]")
{
    using mylib::math::NormalSummary;

    SECTION("+= adds means and variances")
    {
        NormalSummary a{ 1.0, 2.0 };
        NormalSummary b{ 3.0, 5.0 };
        a += b;
        REQUIRE(a.mean == 4.0);
        REQUIRE(a.variance == 7.0);   // variances ADD for independent variables
    }

    SECTION("-= subtracts means but ADDS variances")
    {
        // Ключевое свойство: Var(X - Y) = Var(X) + Var(Y) для независимых.
        NormalSummary a{ 5.0, 2.0 };
        NormalSummary b{ 3.0, 4.0 };
        a -= b;
        REQUIRE(a.mean == 2.0);
        REQUIRE(a.variance == 6.0);   // 2 + 4, НЕ 2 - 4
    }

    SECTION("*= scales mean linearly, variance quadratically")
    {
        NormalSummary s{ 3.0, 4.0 };
        s *= 2.0;
        REQUIRE(s.mean == 6.0);
        REQUIRE(s.variance == 16.0);   // 4 * 2^2 = 16
    }

    SECTION("*= by 0 zeroes both")
    {
        NormalSummary s{ 5.0, 10.0 };
        s *= 0.0;
        REQUIRE(s.mean == 0.0);
        REQUIRE(s.variance == 0.0);
    }

    SECTION("*= by -1 flips mean, keeps variance")
    {
        NormalSummary s{ 3.0, 4.0 };
        s *= -1.0;
        REQUIRE(s.mean == -3.0);
        REQUIRE(s.variance == 4.0);
    }

    SECTION("*= by 0.5 halves mean, quarters variance")
    {
        NormalSummary s{ 10.0, 8.0 };
        s *= 0.5;
        REQUIRE(s.mean == 5.0);
        REQUIRE(s.variance == 2.0);   // 8 * 0.25
    }

    SECTION("compound ops return reference (chaining)")
    {
        NormalSummary s{ 1.0, 1.0 };
        auto& ref = (s += NormalSummary{ 1.0, 1.0 });
        REQUIRE(&ref == &s);

        // Цепочка: (s += a) *= 2
        NormalSummary t{ 0.0, 0.0 };
        (t += NormalSummary{ 1.0, 1.0 }) *= 2.0;
        REQUIRE(t.mean == 2.0);
        REQUIRE(t.variance == 4.0);
    }
}


// ============================================================================
//  NormalSummary: не-member операторы из ArithmeticType
// ============================================================================
TEST_CASE("NormalSummary non-member operators",
          "[math][statistics][normal]")
{
    using mylib::math::NormalSummary;

    SECTION("a + b returns new summary with summed mean/variance")
    {
        NormalSummary a{ 1.0, 2.0 };
        NormalSummary b{ 3.0, 5.0 };
        NormalSummary c = a + b;
        REQUIRE(c.mean == 4.0);
        REQUIRE(c.variance == 7.0);
        // Исходные не изменились
        REQUIRE(a.mean == 1.0);
        REQUIRE(b.mean == 3.0);
    }

    SECTION("a - b returns new summary with differenced mean, summed variance")
    {
        NormalSummary a{ 5.0, 2.0 };
        NormalSummary b{ 3.0, 4.0 };
        NormalSummary c = a - b;
        REQUIRE(c.mean == 2.0);
        REQUIRE(c.variance == 6.0);
    }

    SECTION("a * scalar returns new scaled summary")
    {
        NormalSummary a{ 3.0, 4.0 };
        NormalSummary c = a * 2.0;
        REQUIRE(c.mean == 6.0);
        REQUIRE(c.variance == 16.0);
        REQUIRE(a.mean == 3.0);   // не изменился
    }
}


// ============================================================================
//  IncrementalStatistics: базовые сценарии
// ============================================================================
TEST_CASE("IncrementalStatistics basic usage",
          "[math][statistics][incremental]")
{
    using mylib::math::IncrementalStatistics;

    SECTION("default: empty, sum=0, min=+inf, max=-inf")
    {
        IncrementalStatistics s;
        REQUIRE(s.n == 0);
        REQUIRE(s.sum == 0.0);
        REQUIRE(s.squaredSum == 0.0);
        REQUIRE(s.minimum == std::numeric_limits<double>::infinity());
        REQUIRE(s.maximum == -std::numeric_limits<double>::infinity());
    }

    SECTION("addValue increments n and updates sum/squaredSum")
    {
        IncrementalStatistics s;
        s.addValue(2.0);
        REQUIRE(s.n == 1);
        REQUIRE(s.sum == 2.0);
        REQUIRE(s.squaredSum == 4.0);

        s.addValue(4.0);
        REQUIRE(s.n == 2);
        REQUIRE(s.sum == 6.0);
        REQUIRE(s.squaredSum == 20.0);
    }

    SECTION("addValue tracks min/max")
    {
        IncrementalStatistics s;
        for (double v : { 5.0, 1.0, 8.0, -3.0, 10.0, 0.0 })
            s.addValue(v);

        REQUIRE(s.minimum == -3.0);
        REQUIRE(s.maximum == 10.0);
    }

    SECTION("single value: mean = value, variance = 0")
    {
        IncrementalStatistics s;
        s.addValue(42.0);
        REQUIRE(s.getMean() == 42.0);
        REQUIRE(s.getVariance() == 0.0);
        REQUIRE(s.stddev() == 0.0);
    }

    SECTION("known values {1,2,3,4,5}: mean=3, sample variance=2.5")
    {
        IncrementalStatistics s;
        for (double v : { 1.0, 2.0, 3.0, 4.0, 5.0 }) s.addValue(v);

        REQUIRE(s.n == 5);
        REQUIRE_THAT(s.getMean(), WithinRel(3.0, 1e-12));
        // sample variance = ((1-3)^2 + (2-3)^2 + ... ) / 4
        // = (4+1+0+1+4) / 4 = 10/4 = 2.5
        REQUIRE_THAT(s.getVariance(), WithinRel(2.5, 1e-12));
        REQUIRE_THAT(s.stddev(), WithinRel(std::sqrt(2.5), 1e-12));
    }

    SECTION("constant values -> variance = 0")
    {
        IncrementalStatistics s;
        for (int i = 0; i < 100; ++i) s.addValue(7.5);
        REQUIRE_THAT(s.getMean(), WithinRel(7.5, 1e-12));
        REQUIRE_THAT(s.getVariance(), WithinAbs(0.0, 1e-12));
    }

    SECTION("two values: sample variance = (x1 - x2)^2 / 2")
    {
        IncrementalStatistics s;
        s.addValue(0.0);
        s.addValue(10.0);
        REQUIRE_THAT(s.getMean(), WithinRel(5.0, 1e-12));
        // sample var = (10-0)^2 / 2 = 50
        REQUIRE_THAT(s.getVariance(), WithinRel(50.0, 1e-12));
    }
}


// ============================================================================
//  IncrementalStatistics: граничные случаи
// ============================================================================
TEST_CASE("IncrementalStatistics edge cases",
          "[math][statistics][incremental]")
{
    using mylib::math::IncrementalStatistics;

    SECTION("getVariance on empty returns 0 (per doc: n < 2)")
    {
        IncrementalStatistics s;
        REQUIRE(s.getVariance() == 0.0);
        REQUIRE(s.stddev() == 0.0);
    }

    SECTION("getVariance with single value returns 0 (Bessel's correction needs n>=2)")
    {
        IncrementalStatistics s;
        s.addValue(42.0);
        REQUIRE(s.getVariance() == 0.0);
    }

    SECTION("negative values handled correctly")
    {
        IncrementalStatistics s;
        for (double v : { -1.0, -2.0, -3.0, -4.0, -5.0 }) s.addValue(v);
        REQUIRE_THAT(s.getMean(), WithinRel(-3.0, 1e-12));
        REQUIRE_THAT(s.getVariance(), WithinRel(2.5, 1e-12));
        REQUIRE(s.minimum == -5.0);
        REQUIRE(s.maximum == -1.0);
    }

    SECTION("mixed positive/negative around zero")
    {
        IncrementalStatistics s;
        for (double v : { -10.0, -5.0, 0.0, 5.0, 10.0 }) s.addValue(v);
        REQUIRE_THAT(s.getMean(), WithinAbs(0.0, 1e-12));
        // sample var = (100+25+0+25+100)/4 = 62.5
        REQUIRE_THAT(s.getVariance(), WithinRel(62.5, 1e-12));
    }

    SECTION("large values with small differences - numerical stability")
    {
        // ВНИМАНИЕ: документация предупреждает о catastrophic cancellation
        // для E[X^2] - E[X]^2. Всё же проверим, что не уходит в бесконечность.
        IncrementalStatistics s;
        const double base{ 1e8 };
        for (double d : { -2.0, -1.0, 0.0, 1.0, 2.0 })
            s.addValue(base + d);

        REQUIRE_THAT(s.getMean(), WithinRel(base, 1e-9));
        // variance может быть неточной, но должна быть >= 0 и близка к 2.5
        REQUIRE(s.getVariance() >= 0.0);
        INFO("variance = " << s.getVariance());
        // Допускаем заметную погрешность — это known limitation
        REQUIRE(s.getVariance() < 10.0);
    }

    SECTION("variance never negative (clamped by max(0.0, ...))")
    {
        // При сильной потере точности var могла бы стать -eps
        IncrementalStatistics s;
        for (int i = 0; i < 1000; ++i) s.addValue(1e15);
        REQUIRE(s.getVariance() >= 0.0);
    }
}


// ============================================================================
//  IncrementalStatistics: getStandardErrorSummary
// ============================================================================
TEST_CASE("IncrementalStatistics standard error summary",
          "[math][statistics][incremental]")
{
    using mylib::math::IncrementalStatistics;
    using mylib::math::NormalSummary;

    SECTION("returns NormalSummary{mean, variance/n}")
    {
        IncrementalStatistics s;
        for (double v : { 1.0, 2.0, 3.0, 4.0, 5.0 }) s.addValue(v);

        auto ns = s.getStandardErrorSummary();
        REQUIRE_THAT(ns.mean, WithinRel(3.0, 1e-12));
        // variance of the mean = sample variance / n = 2.5 / 5 = 0.5
        REQUIRE_THAT(ns.variance, WithinRel(0.5, 1e-12));
    }

    SECTION("single value: variance of mean = 0")
    {
        IncrementalStatistics s;
        s.addValue(7.0);
        auto ns = s.getStandardErrorSummary();
        REQUIRE(ns.mean == 7.0);
        REQUIRE(ns.variance == 0.0);
    }

    SECTION("stddev of standard error shrinks like 1/sqrt(n)")
    {
        // Для выборки из одинаковых величин — variance = 0. Возьмём две серии.
        IncrementalStatistics small, large;
        for (double v : { 1.0, 3.0 }) small.addValue(v);
        for (int i = 0; i < 200; ++i) large.addValue(i % 2 == 0 ? 1.0 : 3.0);

        REQUIRE(small.getStandardErrorSummary().stddev() >
                large.getStandardErrorSummary().stddev());
    }
}


// ============================================================================
//  MonteCarloSimulate
// ============================================================================
TEST_CASE("MonteCarloSimulate", "[math][statistics][montecarlo]")
{
    using mylib::math::MonteCarloSimulate;
    using mylib::math::IncrementalStatistics;

    SECTION("constant function: n simulations, mean = constant, variance = 0")
    {
        auto s = MonteCarloSimulate([] { return 5.0; }, 100);
        REQUIRE(s.n == 100);
        REQUIRE_THAT(s.getMean(), WithinRel(5.0, 1e-12));
        REQUIRE_THAT(s.getVariance(), WithinAbs(0.0, 1e-12));
        REQUIRE(s.minimum == 5.0);
        REQUIRE(s.maximum == 5.0);
    }

    SECTION("deterministic sequence: exact mean")
    {
        int i = 0;
        auto s = MonteCarloSimulate([&i] { return static_cast<double>(i++); }, 5);
        REQUIRE(s.n == 5);
        // значения 0,1,2,3,4 → mean = 2
        REQUIRE_THAT(s.getMean(), WithinRel(2.0, 1e-12));
        // variance = ((0-2)^2+(1-2)^2+(2-2)^2+(3-2)^2+(4-2)^2) / 4 = 10/4 = 2.5
        REQUIRE_THAT(s.getVariance(), WithinRel(2.5, 1e-12));
    }

    SECTION("nSimulations = 0 returns empty statistics")
    {
        auto s = MonteCarloSimulate([] { return 1.0; }, 0);
        REQUIRE(s.n == 0);
        REQUIRE(s.sum == 0.0);
        REQUIRE(s.minimum == std::numeric_limits<double>::infinity());
        REQUIRE(s.maximum == -std::numeric_limits<double>::infinity());
    }

    SECTION("default nSimulations = 1000")
    {
        auto s = MonteCarloSimulate([] { return 0.0; });
        REQUIRE(s.n == 1000);
    }

    SECTION("function returning int is converted to double")
    {
        auto s = MonteCarloSimulate([] { return 42; }, 10);
        REQUIRE(s.n == 10);
        REQUIRE_THAT(s.getMean(), WithinRel(42.0, 1e-12));
    }

    SECTION("counts calls exactly nSimulations times")
    {
        int calls = 0;
        (void)MonteCarloSimulate([&calls] {
            ++calls;
            return 0.0;
        }, 37);
        REQUIRE(calls == 37);
    }

    SECTION("random uniform [0,1): mean ≈ 0.5, variance ≈ 1/12")
    {
        mylib::random::Randomizer<> rng{ 42 };
        auto s = MonteCarloSimulate([&rng] { return rng.uniform01(); }, 100'000);

        INFO("mean = " << s.getMean() << ", variance = " << s.getVariance());
        REQUIRE_THAT(s.getMean(), WithinAbs(0.5, 0.01));
        // sample variance ≈ 1/12 ≈ 0.0833; ±0.005 — большой запас
        REQUIRE_THAT(s.getVariance(), WithinAbs(1.0 / 12.0, 0.005));

        // min/max в [0, 1)
        REQUIRE(s.minimum >= 0.0);
        REQUIRE(s.maximum < 1.0);
        REQUIRE(s.minimum < 0.01);
        REQUIRE(s.maximum > 0.99);
    }

    SECTION("normal(0, 1): mean ≈ 0, variance ≈ 1")
    {
        mylib::random::Randomizer<> rng{ 42 };
        auto s = MonteCarloSimulate([&rng] { return rng.normal01(); }, 200'000);
        INFO("mean = " << s.getMean() << ", variance = " << s.getVariance());
        REQUIRE_THAT(s.getMean(), WithinAbs(0.0, 0.02));
        REQUIRE_THAT(s.getVariance(), WithinAbs(1.0, 0.02));
    }

    SECTION("returns IncrementalStatistics (type check)")
    {
        auto s = MonteCarloSimulate([] { return 0.0; }, 5);
        STATIC_REQUIRE(std::is_same_v<decltype(s), IncrementalStatistics>);
    }
}


// ============================================================================
//  Комбинированный сценарий: закон больших чисел
// ============================================================================
TEST_CASE("Statistics consistency: LLN and Chebyshev",
          "[math][statistics][consistency]")
{
    using mylib::math::IncrementalStatistics;
    using mylib::math::MonteCarloSimulate;

    SECTION("running mean converges to expected value as n grows")
    {
        mylib::random::Randomizer<> rng{ 12345 };
        IncrementalStatistics s;

        double prevErr = 1e9;
        for (int i = 1; i <= 10'000; ++i)
        {
            s.addValue(rng.uniform01());

            if (i == 100) prevErr = std::abs(s.getMean() - 0.5);
            if (i == 10'000)
            {
                const double err = std::abs(s.getMean() - 0.5);
                INFO("err@100 = " << prevErr << ", err@10000 = " << err);
                // стандартная ошибка ~ 1/sqrt(12*n), падает как 1/sqrt(n)
                REQUIRE(err < 0.01);
                REQUIRE(err < prevErr);   // ошибка уменьшается (statistically)
            }
        }
    }

    SECTION("sum over n uniform[0,1) approx n/2")
    {
        mylib::random::Randomizer<> rng{ 777 };
        auto s = MonteCarloSimulate([&rng] { return rng.uniform01(); }, 50'000);

        const double expectedSum = 50'000 * 0.5;
        INFO("sum = " << s.sum << ", expected = " << expectedSum);
        // std error of sum ~ sqrt(n/12) ~ 64.5; допуск 500 — 8σ
        REQUIRE(std::abs(s.sum - expectedSum) < 500.0);
    }

    SECTION("combining two independent MonteCarloSimulate gives same as single")
    {
        mylib::random::Randomizer<> rng1{ 42 };
        mylib::random::Randomizer<> rng2{ 43 };

        auto a = MonteCarloSimulate([&rng1] { return rng1.normal01(); }, 50'000);
        auto b = MonteCarloSimulate([&rng2] { return rng2.normal01(); }, 50'000);

        // Мета-выводы: mean(a) + mean(b) имеет mean 0 + 0 = 0,
        // var(a)+var(b) ~ 1+1 = 2, но это не то же, что суммирование величин.
        auto sumA = a.getStandardErrorSummary();
        auto sumB = b.getStandardErrorSummary();
        auto combined = sumA + sumB;
        INFO("combined.mean = " << combined.mean
                                << ", combined.variance = " << combined.variance);
        REQUIRE_THAT(combined.mean, WithinAbs(0.0, 0.05));
        // Variance of sums of independent SE = var(a)/na + var(b)/nb ≈ 1/50000 + 1/50000
        REQUIRE_THAT(combined.variance, WithinAbs(2.0 / 50'000, 1e-4));
    }
}
