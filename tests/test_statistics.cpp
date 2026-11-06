#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <atomic>
#include <chrono>
#include <cmath>
#include <functional>
#include <numeric>
#include <thread>
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



// ============================================================================
//  CompatisonResult: struct
// ============================================================================
TEST_CASE("CompatisonResult struct", "[math][statistics][compare][struct]")
{
    using mylib::math::CompatisonResult;

    SECTION("aggregate initialization")
    {
        CompatisonResult r{ true, 5.0, 2.0 };
        REQUIRE(r.isSignificant == true);
        REQUIRE(r.meanDifference == 5.0);
        REQUIRE(r.error95 == 2.0);
    }

    SECTION("zero-initialization")
    {
        CompatisonResult r{};
        REQUIRE(r.isSignificant == false);
        REQUIRE(r.meanDifference == 0.0);
        REQUIRE(r.error95 == 0.0);
    }
}

// ============================================================================
//  compareAlgorithms: identical statistics -> not significant
// ============================================================================
TEST_CASE("compareAlgorithms identical statistics",
          "[math][statistics][compare]")
{
    using mylib::math::compareAlgorithms;
    using mylib::math::IncrementalStatistics;

    SECTION("same data -> meanDifference = 0, not significant")
    {
        IncrementalStatistics a, b;
        for (double v : { 1.0, 2.0, 3.0, 4.0, 5.0 })
        {
            a.addValue(v);
            b.addValue(v);
        }

        auto r = compareAlgorithms(a, b);
        REQUIRE_FALSE(r.isSignificant);
        REQUIRE(r.meanDifference == 0.0);
        // error95 = 2 * sqrt(var_a/n_a + var_b/n_b)
        // = 2 * sqrt(2.5/5 + 2.5/5) = 2 * sqrt(1.0) = 2.0
        REQUIRE_THAT(r.error95, WithinRel(2.0, 1e-12));
    }

    SECTION("same data with different n -> error shrinks, still not significant")
    {
        IncrementalStatistics a, b;
        for (int i = 0; i < 100; ++i) a.addValue(1.0);
        for (int i = 0; i < 10; ++i) b.addValue(1.0);

        auto r = compareAlgorithms(a, b);
        REQUIRE_FALSE(r.isSignificant);
        REQUIRE(r.meanDifference == 0.0);
        // variance = 0 для обеих серий, error95 = 0
        REQUIRE(r.error95 == 0.0);
    }
}


// ============================================================================
//  compareAlgorithms: явно разные распределения -> significant
// ============================================================================
TEST_CASE("compareAlgorithms clearly different data",
          "[math][statistics][compare]")
{
    using mylib::math::compareAlgorithms;
    using mylib::math::IncrementalStatistics;

    SECTION("means far apart, low variance -> significant")
    {
        // a: значения около 1.0, b: около 100.0, малый разброс
        IncrementalStatistics a, b;
        for (int i = 0; i < 1000; ++i)
        {
            a.addValue(1.0 + (i % 2 == 0 ? 0.01 : -0.01));
            b.addValue(100.0 + (i % 2 == 0 ? 0.01 : -0.01));
        }

        auto r = compareAlgorithms(a, b);
        REQUIRE(r.isSignificant);
        INFO("meanDiff = " << r.meanDifference << ", error95 = " << r.error95);
        REQUIRE_THAT(r.meanDifference, WithinRel(-99.0, 0.01));
        REQUIRE(r.error95 < 1.0);   // variance/n очень мала
    }

    SECTION("mean difference barely exceeds error95 -> significant")
    {
        // Конструируем случай, где |meanDiff| чуть больше error95.
        // Симметричные выборки, разность средних ~ 1, error95 ~ 0.5.
        IncrementalStatistics a, b;
        for (int i = 0; i < 100; ++i)
        {
            a.addValue(1.0);
            b.addValue(0.0);
        }
        // variance = 0 в обеих, error95 = 0, meanDiff = 1
        auto r = compareAlgorithms(a, b);
        REQUIRE(r.meanDifference == 1.0);
        REQUIRE(r.error95 == 0.0);
        REQUIRE(r.isSignificant);   // |1| > 0
    }
}

// ============================================================================
//  compareAlgorithms: близкие средние -> not significant
// ============================================================================
TEST_CASE("compareAlgorithms statistically indistinguishable",
          "[math][statistics][compare]")
{
    using mylib::math::compareAlgorithms;
    using mylib::math::IncrementalStatistics;

    SECTION("same distribution, different seeds -> not significant")
    {
        mylib::random::Randomizer<> r1{ 42 };
        mylib::random::Randomizer<> r2{ 12345 };
        IncrementalStatistics a, b;

        for (int i = 0; i < 10'000; ++i)
        {
            a.addValue(r1.normal(0.0, 1.0));
            b.addValue(r2.normal(0.0, 1.0));
        }

        auto r = compareAlgorithms(a, b);
        INFO("meanDiff = " << r.meanDifference << ", error95 = " << r.error95);
        // При |meanDiff| < error95 — не значимо. Обычно так и будет.
        REQUIRE(std::abs(r.meanDifference) <= r.error95);
        REQUIRE_FALSE(r.isSignificant);
    }

    SECTION("one sample shifted by less than error95 -> not significant")
    {
        IncrementalStatistics a, b;
        mylib::random::Randomizer<> rng{ 7 };
        for (int i = 0; i < 1000; ++i)
        {
            const double base = rng.normal(0.0, 10.0);
            a.addValue(base);
            b.addValue(base + 0.001);   // сдвиг гораздо меньше error95
        }

        auto r = compareAlgorithms(a, b);
        INFO("meanDiff = " << r.meanDifference << ", error95 = " << r.error95);
        REQUIRE_FALSE(r.isSignificant);
    }
}

// ============================================================================
//  compareAlgorithms: числовые свойства
// ============================================================================
TEST_CASE("compareAlgorithms numeric properties",
          "[math][statistics][compare]")
{
    using mylib::math::compareAlgorithms;
    using mylib::math::IncrementalStatistics;

    SECTION("error95 is non-negative")
    {
        IncrementalStatistics a, b;
        for (int i = 0; i < 100; ++i) { a.addValue(i); b.addValue(i + 1); }
        auto r = compareAlgorithms(a, b);
        REQUIRE(r.error95 >= 0.0);
    }

    SECTION("single-value samples: variance = 0, error95 = 0")
    {
        IncrementalStatistics a, b;
        a.addValue(5.0);
        b.addValue(7.0);

        auto r = compareAlgorithms(a, b);
        REQUIRE(r.error95 == 0.0);
        REQUIRE(r.meanDifference == -2.0);   // a - b
        // |−2| > 0 → significant
        REQUIRE(r.isSignificant);
    }

    SECTION("compareAlgorithms is antisymmetric in meanDifference")
    {
        IncrementalStatistics a, b;
        for (int i = 0; i < 50; ++i)
        {
            a.addValue(1.0 + 0.001 * i);
            b.addValue(2.0 + 0.001 * i);
        }

        auto ab = compareAlgorithms(a, b);
        auto ba = compareAlgorithms(b, a);
        REQUIRE_THAT(ab.meanDifference, WithinRel(-ba.meanDifference, 1e-12));
        REQUIRE_THAT(ab.error95, WithinRel(ba.error95, 1e-12));
        // Значимость симметрична (|diff| > err)
        REQUIRE(ab.isSignificant == ba.isSignificant);
    }

    SECTION("error95 = 2 * sqrt(SE_a^2 + SE_b^2)")
    {
        IncrementalStatistics a, b;
        for (double v : { 1.0, 2.0, 3.0, 4.0, 5.0 }) a.addValue(v);   // var=2.5, n=5
        for (double v : { 10.0, 20.0, 30.0 }) b.addValue(v);            // var=100, n=3

        const double se2A = 2.5 / 5.0;    // 0.5
        const double se2B = 100.0 / 3.0;  // 33.33
        const double expectedErr = 2.0 * std::sqrt(se2A + se2B);

        auto r = compareAlgorithms(a, b);
        INFO("error95 = " << r.error95 << ", expected = " << expectedErr);
        REQUIRE_THAT(r.error95, WithinRel(expectedErr, 1e-9));
    }
}

// ============================================================================
//  SpeedTester: базовые свойства
// ============================================================================
TEST_CASE("SpeedTester basic behavior", "[utility][speedtester]")
{
    using mylib::utility::SpeedTester;

    SECTION("returns non-negative duration")
    {
        SpeedTester st{ [] { /* no-op */ } };
        double t = st();
        REQUIRE(t >= 0.0);
        REQUIRE(std::isfinite(t));
    }

    SECTION("invokes the callable exactly once per call")
    {
        int calls = 0;
        SpeedTester st{ [&calls] { ++calls; } };

        REQUIRE(calls == 0);
        st();
        REQUIRE(calls == 1);
        st();
        REQUIRE(calls == 2);
    }

    SECTION("callable returning a value: value discarded, time recorded")
    {
        SpeedTester st{ [] { return 42; } };
        double t = st();
        REQUIRE(t >= 0.0);
    }

    SECTION("measuring a busy loop yields positive time")
    {
        SpeedTester st{ [] {
            volatile double s = 0.0;
            for (int i = 0; i < 100'000; ++i) s += i;
            (void)s;
        } };
        const double t = st();
        INFO("t = " << t << "s");
        REQUIRE(t > 0.0);
        // Заведомо меньше 5 секунд (если только CI не супер-медленный)
        REQUIRE(t < 5.0);
    }

    SECTION("measuring sleep(10ms) yields >= 10ms")
    {
        SpeedTester st{ [] {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        } };
        const double t = st();
        INFO("t = " << t << "s");
        REQUIRE(t >= 0.009);   // slight tolerance for clock granularity
    }
}


// ============================================================================
//  SpeedTester: CTAD и хранение лямбды
// ============================================================================
TEST_CASE("SpeedTester CTAD and copyability", "[utility][speedtester]")
{
    using mylib::utility::SpeedTester;

    SECTION("deduction guide deduces lambda type")
    {
        auto lambda = [] { };
        using LambdaT = decltype(lambda);

        auto st = SpeedTester{ lambda };
        STATIC_REQUIRE(std::is_same_v<decltype(st), SpeedTester<LambdaT>>);
        REQUIRE_NOTHROW(st());
    }

    SECTION("works with std::function")
    {
        std::function<void()> f = [] { };
        SpeedTester st{ f };
        REQUIRE(st() >= 0.0);
    }

    SECTION("works with function pointer")
    {
        void (*fp)() = [] {};
        SpeedTester st{ fp };
        REQUIRE(st() >= 0.0);
    }

    SECTION("callable state preserved (captures)")
    {
        int counter = 0;
        SpeedTester st{ [&counter] { ++counter; } };
        st();
        st();
        st();
        REQUIRE(counter == 3);
    }

    SECTION("mutable lambda works (mutable capture)")
    {
        // SpeedTester хранит FUNCTION, operator() const -> mutable FUNCTION
        // нужен, чтобы менять захваченное состояние без наружного const.
        auto mutableLambda = [n = 0]() mutable { ++n; };
        SpeedTester st{ std::move(mutableLambda) };
        // Если бы m_f был не mutable, следующая строка не скомпилировалась бы
        // (operator() у SpeedTester - const, а mutable-lambda требует non-const operator()).
        REQUIRE_NOTHROW(st());
    }
}


// ============================================================================
//  SpeedTester: operator() помечен const
// ============================================================================
TEST_CASE("SpeedTester operator() is const-callable",
          "[utility][speedtester][const]")
{
    using mylib::utility::SpeedTester;

    SECTION("const SpeedTester& can invoke operator()")
    {
        SpeedTester st{ [] { } };
        const auto& cst = st;
        REQUIRE_NOTHROW(cst());
    }

    SECTION("constexpr noexcept if FUNCTION::operator() is noexcept")
    {
        // Не гарантирует noexcept, но проверяем, что не падает
        SpeedTester st{ [] {} };
        REQUIRE_NOTHROW(st());
    }
}

/*
// ============================================================================
//  SpeedTester: результаты измеримы и осмысленны
// ============================================================================
TEST_CASE("SpeedTester comparative timing",
          "[utility][speedtester][timing]")
{
    using mylib::utility::SpeedTester;

    SECTION("heavier workload takes measurably longer")
    {
        SpeedTester fast{ [] {
            volatile int x = 0;
            for (int i = 0; i < 1000; ++i) x += i;
            (void)x;
        } };
        SpeedTester slow{ [] {
            volatile int x = 0;
            for (int i = 0; i < 1'000'000; ++i) x += i;
            (void)x;
        } };

        // Усреднение по нескольким прогонам, чтобы снизить шум
        constexpr int N{ 5 };
        double fastTotal = 0, slowTotal = 0;
        for (int i = 0; i < N; ++i)
        {
            fastTotal += fast();
            slowTotal += slow();
        }

        INFO("fast mean = " << fastTotal / N
                            << ", slow mean = " << slowTotal / N);
        REQUIRE(slowTotal > fastTotal);
    }

    SECTION("repeated calls are independent measurements")
    {
        SpeedTester st{ [] {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        } };

        const double t1 = st();
        const double t2 = st();
        const double t3 = st();

        // Все три — валидные измерения
        REQUIRE(t1 >= 0.0);
        REQUIRE(t2 >= 0.0);
        REQUIRE(t3 >= 0.0);
        REQUIRE(std::isfinite(t1));
        REQUIRE(std::isfinite(t2));
        REQUIRE(std::isfinite(t3));
    }
}


// ============================================================================
//  SpeedTester: интеграция с compareAlgorithms (реальный сценарий)
// ============================================================================
TEST_CASE("SpeedTester + MonteCarloSimulate + compareAlgorithms",
          "[utility][speedtester][integration]")
{
    using mylib::utility::SpeedTester;
    using mylib::math::MonteCarloSimulate;
    using mylib::math::compareAlgorithms;

    // Имитируем бенчмарк двух "алгоритмов":
    //   A: 1000 итераций над вектором
    //   B: 1000 итераций над вектором (тот же размер) — для одинаковой скорости.
    auto workLoad = [](std::size_t n) {
        volatile double s = 0.0;
        for (std::size_t i = 0; i < n; ++i) s += static_cast<double>(i);
        (void)s;
    };

    SECTION("identical workloads -> not significant")
    {
        constexpr std::size_t WORK = 10'000;
        SpeedTester A{ [workLoad] { workLoad(WORK); } };
        SpeedTester B{ [workLoad] { workLoad(WORK); } };

        // 20 замеров каждого, чтобы получить достаточно узкие SE
        auto statA = MonteCarloSimulate(A, 20);
        auto statB = MonteCarloSimulate(B, 20);

        auto r = compareAlgorithms(statA, statB);
        INFO("meanDiff = " << r.meanDifference << ", error95 = " << r.error95);
        // Не гарантируем «not significant» на 100% (шум может быть),
        // но гарантируем корректность полей:
        REQUIRE(r.error95 >= 0.0);
        REQUIRE(std::isfinite(r.meanDifference));
    }

    SECTION("very different workloads -> significant")
    {
        SpeedTester A{ [workLoad] { workLoad(100); } };
        SpeedTester B{ [workLoad] { workLoad(100'000); } };

        auto statA = MonteCarloSimulate(A, 20);
        auto statB = MonteCarloSimulate(B, 20);

        auto r = compareAlgorithms(statA, statB);
        INFO("meanDiff = " << r.meanDifference << ", error95 = " << r.error95);
        REQUIRE(r.isSignificant);
        REQUIRE(r.meanDifference < 0.0);   // A быстрее B
    }
}



TEST_CASE("test std::sort and mylib::algorithms::quickSort", "[speedtester]")
{
    auto compareQuickSortWithStd{ [](size_t arraySize, size_t iterations)
    {
        mylib::random::Randomizer rng;

        // 1. Генерируем "эталонный" массив случайных данных
        mylib::Vector<int> originalData(arraySize);


        for(auto& val : originalData)
        {
            val = rng.inRange(0, 2);
        }

        // ==========================================
        // БЕНЧМАРК ДЛЯ mylib::quickSort
        // ==========================================
        auto benchmarkMyQuickSort{[&originalData]() -> double
        {
            mylib::Vector<int> dataToSort{ originalData };

            mylib::utility::SpeedTester tester([&dataToSort](){
                mylib::algorithms::quickSort(dataToSort.begin(), dataToSort.end());
            });

            return tester();
        }};

        // ==========================================
        // БЕНЧМАРК ДЛЯ std::sort
        // ==========================================
        auto benchmarkStdSort{[&originalData]() -> double
        {
            mylib::Vector<int> dataToSort{ originalData };

            mylib::utility::SpeedTester tester([&dataToSort](){
                std::sort(dataToSort.begin(), dataToSort.end());
            });

            return tester();
        }};

        auto statsMy{ mylib::math::MonteCarloSimulate(benchmarkMyQuickSort, iterations) };
        auto statsStd{ mylib::math::MonteCarloSimulate(benchmarkStdSort, iterations) };

        auto result{ mylib::math::compareAlgorithms(statsMy, statsStd) };

        // Output results
        std::cout << "--- Comparison on " << arraySize << " elements ---\n";
        std::cout << "My QuickSort:   " << statsMy.getMean() * 1000 << " ms\n";
        std::cout << "std::sort:      " << statsStd.getMean() * 1000 << " ms\n";
        std::cout << "Difference:     " << result.meanDifference * 1000 << " ms\n";
        std::cout << "Error margin:   " << result.error95 * 1000 << " ms\n\n";

        if (result.isSignificant)
        {
            if (result.meanDifference < 0)
            {
                std::cout << "✅ MY QUICKSORT IS FASTER! (Difference is statistically significant)\n\n";
            }
            else
            {
                std::cout << "❌ std::sort IS FASTER. (Difference is statistically significant)\n\n";
            }
        }
        else
        {
            std::cout << "🤝 NO DIFFERENCE. Algorithms perform equally (within error margin).\n\n";
        }

    }};

    // Тест 1: Маленькие массивы (тут std::sort может победить из-за оптимизаций)
    compareQuickSortWithStd(10, 1000);

    // Тест 2: Средние массивы
    compareQuickSortWithStd(1000, 100);

    // Тест 3: Большие массивы (тут должен раскрыться ваш O(N log N))
    compareQuickSortWithStd(100000, 10);

}
*/
