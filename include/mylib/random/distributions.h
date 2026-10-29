#ifndef DISTRIBUTIONS_H
#define DISTRIBUTIONS_H

#include <cassert>
#include <chrono>
#include <cstdint>
#include <numbers>
#include <utility>
#include <random>

#include "mylib/random/engines.h"

#include "mylib/core/concepts.h"
#include "mylib/math/bit_operations.h"


namespace mylib::random
{
    /**
     * @brief Generates either 1 or -1 with a 50/50 probability.
     *
     * @param RNG The Uniform Random Bit Generator to use.
     * Useful for random walks or physics simulations.
     */
    int sign(std::uniform_random_bit_generator auto& RNG)
    {
        std::uniform_int_distribution coin{ 0, 1 };

        return coin(RNG) ? -1 : 1;
    }

    /**
     * @brief Generates a uniformly distributed real number in [0, 1).
     *
     * @param RNG The Uniform Random Bit Generator to use.
     * @return A random double in the half-open interval [0, 1).
     */
    double uniform01(std::uniform_random_bit_generator auto& RNG)
    {
        std::uniform_real_distribution doubles{ 0.0, 1.0 };

        return doubles(RNG);
    }

    /**
     * @brief Generates a random number following an exponential distribution.
     *
     * @param RNG The Uniform Random Bit Generator to use.
     * @param a The rate parameter (alpha). Must be > 0.
     * @return Time until the next event.
     *
     * @note Uses inverse transform sampling: x = -ln(u) / a.
     */
    double exponential(std::uniform_random_bit_generator auto& RNG, double a = 1.0)
    {
        assert(a > 0 && "mylib::random::Randomizer::exponential(double alpha): alpha must be positive");


        double u{ uniform01(RNG) };

        if(u == 0.0)
        {
            u = std::nextafter(0.0, 1.0);
        }

        return -std::log(u) / a;
    }

    /**
     * @brief Generates a random number following a Cauchy distribution.
     *
     * @param RNG The Uniform Random Bit Generator to use.
     * @param m The location parameter (median). Default is 0.
     * @param q The scale parameter (width). Default is 1.
     * @return A random number from the Cauchy distribution.
     */
    double cauchy(std::uniform_random_bit_generator auto& RNG, double m = 0.0, double q = 1.0)
    {
        assert(q > 0.0 && "Scale parameter 'q' must be positive!");

        double u{ uniform01(RNG) };

        if(u == 0.0)
        {
            u = std::nextafter(0.0, 1.0);
        }

        return m + q * std::tan(std::numbers::pi * (u - 0.5));
    }

    /**
     * @brief Generates TWO independent standard normal random numbers (mean=0, stddev=1)
     *        using the Marsaglia polar method.
     *
     * @param RNG The Uniform Random Bit Generator to use.
     * @return A pair of two independent standard normal values.
     *
     * @note Returning a pair allows the caller to cache the second value, which is
     *       a standard optimization for the polar method.
     */
    std::pair<double, double> normal01(std::uniform_random_bit_generator auto& RNG)
    {

        double a{};
        double b{};
        double c{};

        do
        {
            a = 2.0 * uniform01(RNG) - 1.0;
            b = 2.0 * uniform01(RNG) - 1.0;

            c = a * a + b * b;
        } while( c >= 1.0 || c == 0.0);


        double temp{ std::sqrt(-2.0 * std::log(c) / c) };

        return std::pair{ a * temp, b * temp };
    }

    /**
     * @brief Generates TWO independent random numbers following a Normal (Gaussian) distribution.
     *
     * Scales and shifts standard normal values to have the specified mean and standard deviation.
     *
     * @param RNG The Uniform Random Bit Generator to use.
     * @param m The mean (mu) of the distribution. Shifts the center of the bell curve.
     * @param q The standard deviation (sigma) of the distribution. Controls the width
     *          of the bell curve. Must be strictly greater than 0.
     * @return A pair of two independent normal random values.
     */
    std::pair<double, double> normal(std::uniform_random_bit_generator auto& RNG, double m, double q)
    {
        assert(q > 0.0 && "Standard deviation must be positive!");

        std::pair<double, double> stdNRN{ normal01(RNG) };
        return std::pair{ m + q * stdNRN.first, m + q * stdNRN.second };
    }

    /**
     * @brief Generates a random number following a Gamma distribution.
     *
     * @param RNG The Uniform Random Bit Generator to use.
     * @param shape The shape parameter (often denoted as k or alpha). Must be > 0.
     * @param scale The scale parameter (often denoted as theta or beta). Must be > 0. Default is 1.0.
     * @return A random number from the Gamma distribution.
     *
     * @note Uses the Marsaglia-Tsang method for shape >= 1, and a mathematical
     *       trick (boost) for shape < 1.
     */
    double gamma(std::uniform_random_bit_generator auto& RNG, double shape = 1.0, double scale = 1.0)
    {
        assert(shape > 0.0 && scale > 0.0);

        // Если форма меньше 1, используем трюк:
        // Гамма(k) = Гамма(k+1) * U^(1/k)
        // где U - равномерное распределение от 0 до 1
        if (shape < 1.0)
        {
            double u{ uniform01(RNG) };
            return gamma(RNG, shape + 1.0, 1.0) * std::pow(u, 1.0 / shape) * scale;
        }

        // Алгоритм Марсальи-Цанга для shape >= 1
        double d = shape - 1.0 / 3.0;
        double c = 1.0 / std::sqrt(9.0 * d);

        for (;;)
        {
            auto xPair{ normal01(RNG) };
            double x{ xPair.first };
            double v{ 1.0 + c * x };

            // Если первое число не подошло, пробуем второе
            if (v <= 0.0)
            {
                x = xPair.second;
                v = 1.0 + c * x;
                if (v <= 0.0)
                {
                    continue; // Если и второе не подошло, генерируем новую пару
                }
            }

            v = v * v * v;
            double u{ uniform01(RNG) };

            // Быстрая проверка
            if (u < 1.0 - 0.0331 * (x * x) * (x * x))
            {
                return d * v * scale;
            }

            // Точная проверка
            if (std::log(u) < 0.5 * x * x + d * (1.0 - v + std::log(v)))
            {
                return d * v * scale;
            }
        }
    }

    /**
     * @brief Generates a random number following a Chi-Squared distribution.
     *
     * @param RNG The Uniform Random Bit Generator to use.
     * @param k Degrees of freedom. Must be > 0.
     * @return A random number from the Chi-Squared distribution.
     *
     * @note The Chi-Squared distribution with k degrees of freedom is a special case
     *       of the Gamma distribution: Gamma(shape = k/2, scale = 2).
     */
    double chiSquared(std::uniform_random_bit_generator auto& RNG, double k)
    {
        assert(k > 0.0);
        // Хи-квадрат — это просто Гамма с формой k/2 и масштабом 2
        return gamma(RNG, k / 2.0, 2.0);
    }

    /**
     * @brief Generates a random number following a Student's t-distribution.
     *
     * @param RNG The Uniform Random Bit Generator to use.
     * @param v Degrees of freedom. Must be > 0.
     * @return A random number from the Student's t-distribution.
     *
     * @note Formula: T = Z / sqrt(V / v), where Z is standard normal,
     *       and V is Chi-Squared with v degrees of freedom.
     */
    double studentT(std::uniform_random_bit_generator auto& RNG, double v)
    {
        assert(v > 0.0);
        auto z = normal01(RNG);
        double chisq = chiSquared(RNG, v);

        return z.first / std::sqrt(chisq / v);
    }

    /**
     * @brief Generates a random number following a Beta distribution.
     *
     * @param RNG The Uniform Random Bit Generator to use.
     * @param alpha First shape parameter. Must be > 0.
     * @param beta_param Second shape parameter. Must be > 0.
     * @return A random number from the Beta distribution (always in range [0, 1]).
     *
     * @note Formula: B = X / (X + Y), where X is Gamma(alpha, 1) and Y is Gamma(beta_param, 1).
     */
    double beta(std::uniform_random_bit_generator auto& RNG, double alpha, double beta_param)
    {
        assert(alpha > 0.0 && beta_param > 0.0);

        double x = gamma(RNG, alpha, 1.0);
        double y = gamma(RNG, beta_param, 1.0);

        return x / (x + y);
    }

    /**
     * @brief Generates a random number following a Fisher-Snedecor (F) distribution.
     *
     * @param RNG The Uniform Random Bit Generator to use.
     * @param m Numerator degrees of freedom. Must be > 0.
     * @param n Denominator degrees of freedom. Must be > 0.
     * @return A random number from the F-distribution.
     *
     * @note Formula: F = (U1 / m) / (U2 / n), where U1 and U2 are Chi-Squared variables.
     */
    double fisherF(std::uniform_random_bit_generator auto& RNG, double m, double n)
    {
        assert(m > 0.0 && n > 0.0);

        double u1 = chiSquared(RNG, m);
        double u2 = chiSquared(RNG, n);

        return (u1 / m) / (u2 / n);
    }

    /**
     * @brief Triangular distribution. Returns a value between 0 and 1 with a peak at `middle`.
     *
     * @param RNG The Uniform Random Bit Generator to use.
     * @param middle The location of the peak, in range (0, 1). Default is 0.5.
     * @return A random value in [0, 1].
     */
    double triangular(std::uniform_random_bit_generator auto& RNG, double middle = 0.5)
    {
        assert(middle > 0.0 && middle < 1.0);
        double u = uniform01(RNG);
        return u <= middle ? std::sqrt(middle * u) : 1.0 - std::sqrt((1.0 - middle) * (1.0 - u));
    }

    /**
     * @brief Scaled Triangular distribution [a, b].
     *
     * @param RNG The Uniform Random Bit Generator to use.
     * @param a The lower bound.
     * @param b The upper bound.
     * @param middle The location of the peak, in range (a, b).
     * @return A random value in [a, b].
     */
    double triangular(std::uniform_random_bit_generator auto& RNG, double a, double b, double middle)
    {
        return a + (b - a) * triangular(RNG, (middle - a) / (b - a));
    }

    /**
     * @brief Lévy distribution (heavy tails).
     *
     * @param RNG The Uniform Random Bit Generator to use.
     * @param scale The scale parameter. Default is 0.455.
     * @return A random number from the Lévy distribution.
     */
    double levy(std::uniform_random_bit_generator auto& RNG, double scale = 0.455)
    {
        // Note: normal(0, 1/sqrt(scale)) means standard deviation is 1/sqrt(scale)
        double temp = normal(RNG, 0.0, 1.0 / std::sqrt(scale)).first;
        return 1.0 / (temp * temp);
    }

    /**
     * @brief Symmetrized Lévy distribution.
     *
     * @param RNG The Uniform Random Bit Generator to use.
     * @param scale The scale parameter. Default is 0.455.
     * @return A random number from the symmetrized Lévy distribution.
     */
    double symmetrizedLevy(std::uniform_random_bit_generator auto& RNG, double scale = 0.455)
    {
        return sign(RNG) * levy(RNG, scale);
    }

    /**
     * @brief Bernoulli distribution (true with probability p, false with 1-p).
     *
     * @param RNG The Uniform Random Bit Generator to use.
     * @param p The probability of success (true). Must be in range [0, 1].
     * @return true with probability p, false with probability (1-p).
     */
    bool bernoulli(std::uniform_random_bit_generator auto& RNG, double p)
    {
        assert(p >= 0.0 && p <= 1.0);
        return uniform01(RNG) < p;
}

    /**
     * @brief Binomial distribution (number of successes in n trials with probability p).
     *
     * @param RNG The Uniform Random Bit Generator to use.
     * @param p The probability of success in each trial. Must be in range [0, 1].
     * @param n The number of trials. Must be >= 0.
     * @return The number of successful trials.
     */
    int binomial(std::uniform_random_bit_generator auto& RNG, double p, int n)
    {
        assert(p >= 0.0 && p <= 1.0 && n >= 0);
        int result = 0;
        for (int i = 0; i < n; ++i)
        {
            result += bernoulli(RNG, p);
        }
        return result;
    }

    /**
     * @brief Geometric distribution (number of trials until first success).
     *
     * @param RNG The Uniform Random Bit Generator to use.
     * @param p The probability of success in each trial. Must be in range (0, 1].
     * @return The number of trials (including the successful one).
     */
    int geometric(std::uniform_random_bit_generator auto& RNG, double p)
    {
        assert(p > 0.0 && p <= 1.0);
        int result = 1;
        while (!bernoulli(RNG, p))
        {
            ++result;
        }
        return result;
    }

    /**
     * @brief Optimized geometric distribution for p=0.5 using bitwise operations.
     * Counts trailing zeros in a random 64-bit number.
     *
     * @param RNG The Uniform Random Bit Generator to use.
     * @return The number of trials until the first success.
     */
    int geometric05(std::uniform_random_bit_generator auto& RNG)
    {

        // В C++20 можно использовать std::countr_zero(next())
        // Или функцию bit::rightmostNullCount из вашей библиотеки
        return bit::rightmostNullCount(RNG()) + 1;
    }

    /**
     * @brief Generates a random number following a Poisson distribution.
     *
     * @param RNG The Uniform Random Bit Generator to use.
     * @param l The expected number of events (lambda). It equals the rate of events
     *          multiplied by the time interval. Must be > 0.
     * @return The number of events (integer).
     */
    int poisson(std::uniform_random_bit_generator auto& RNG, double l)
    {
        assert(l > 0.0);

        int result = -1;

        // Оптимизация: вместо медленного сложения логарифмов,
        // мы умножаем равномерные числа и сравниваем с exp(-l)
        for(double p = 1.0; p > std::exp(-l); p *= uniform01(RNG))
        {
            ++result;
        }

        return result;
    }

}

namespace mylib::random
{

/**
 * @brief A high-level convenience wrapper around a Uniform Random Bit Generator (URBG).
 *
 * This class provides an easy-to-use interface for common random generation tasks,
 * such as rolling dice, flipping coins, or generating floating-point numbers.
 * It eliminates the need to manually instantiate and manage `<random>` distributions
 * for everyday use cases.
 *
 * By default, it uses the `defaultRandomizer` (which is `MersenneTwister<uint64_t>`),
 * but it can wrap any type that satisfies the `std::uniform_random_bit_generator` concept.
 *
 * @tparam GENERATOR The underlying random number engine.
 */
template<std::uniform_random_bit_generator GENERATOR = random::defaultRandomizer<>>
class Randomizer final
{
public:
    /// @brief The type of values produced by the underlying generator.
    using result_type = typename GENERATOR::result_type;

    /// @brief Returns the minimum possible value of the underlying generator.
    static constexpr result_type min() { return GENERATOR::min(); }

    /// @brief Returns the maximum possible value of the underlying generator.
    static constexpr result_type max() { return GENERATOR::max(); }

private:
    GENERATOR m_g;

    double m_cachedNormal{ 0.0 };
    bool m_hasCachedNormal{ false };

    /**
     * @brief Creates a default-initialized generator using high-entropy sources.
     *
     * Combines `std::random_device` (hardware entropy) with the current system time
     * to create a `std::seed_seq`. This ensures that two objects created at the same
     * time will not produce identical sequences.
     *
     * @return A fully initialized GENERATOR object.
     */
    static GENERATOR createDefault()
    {
        if constexpr(std::is_constructible_v<GENERATOR, std::seed_seq&>)
        {
            std::random_device rd{};
            std::seed_seq seq{ rd(),
                              rd(),
                              rd(),
                              rd(),
                              rd(),
                              rd(),
                              rd(),
                              static_cast<std::uint32_t>(std::chrono::system_clock::now().time_since_epoch().count()) };
            return GENERATOR{ seq };
        }
        else if constexpr(std::is_default_constructible_v<GENERATOR>)
        {
            return GENERATOR{};
        }
        else
        {
            throw std::invalid_argument("GENERATOR must be constructible from std::seed_seq or default-constructible");
        }
    }

public:
    /**
     * @brief Default constructor. Initializes the generator with high entropy.
     */
    Randomizer()
        : m_g{ createDefault() }
    {}

    /**
     * @brief Constructs the generator with a specific seed.
     * @param seed The starting value. Useful for reproducible tests.
     */
    explicit Randomizer(result_type seed)
        : m_g{ seed }
    {}

    /**
     * @brief Constructs the generator using a SeedSequence (e.g., std::seed_seq).
     * @param seq The sequence object to use for initialization.
     */
    template<SeedSequence SeedSeq>
    explicit Randomizer(SeedSeq& seq)
        : m_g{ seq }
    {}

    /**
     * @brief Advances the generator and returns the next raw random value.
     * Required to satisfy the UniformRandomBitGenerator concept.
     */
    result_type operator()() noexcept
    {
        return m_g();
    }

    /**
     * @brief Alias for operator(). Returns the next raw random value.
     */
    result_type next() noexcept
    {
        return (*this)();
    }

    /**
     * @brief Generates a uniformly distributed integer in the range [0, n - 1].
     *
     * @note This function does NOT use the modulo operator (`%`). Using `%` introduces
     *       a slight statistical bias (modulo bias). Instead, it uses
     *       `std::uniform_int_distribution`, which employs rejection sampling to
     *       guarantee mathematically perfect uniformity.
     *
     * @param n The exclusive upper bound (must be > 0).
     * @return A random integer between 0 and n - 1.
     */
    result_type mod(result_type n)
    {
        assert(n > 0 && "mylib::random::Randomizer::mod(result_type n): n must be positive");

        std::uniform_int_distribution<result_type> numbers{ 0, static_cast<result_type>(n - 1) };

        return numbers(m_g);
    }

    /**
     * @brief Generates either 1 or -1 with a 50/50 probability.
     * Useful for random walks or physics simulations.
     */
    int sign() { return mod(2) ? 1 : -1; }

    /**
     * @brief Generates a uniformly distributed integer in the inclusive range [a, b].
     *
     * @tparam IntType The integer type (e.g., int, long long). Defaults to int.
     * @param a The lower bound (inclusive).
     * @param b The upper bound (inclusive).
     * @return A random integer between a and b.
     *
     * @note We use `IntType` for the distribution, NOT `result_type`. If `result_type`
     *       is unsigned (uint64_t) and you pass negative values for `a` or `b`, using
     *       `result_type` would cause severe underflow bugs.
     * @note `noexcept` is omitted because `std::uniform_int_distribution::operator()`
     *       is not guaranteed to be `noexcept` by the C++ standard.
     */
    template<std::integral IntType = int>
    IntType inRange(IntType a, IntType b)
    {
        assert(b >= a && "mylib::random::Randomizer::inRange(IntType a, IntType b): b must be b >= a");

        std::uniform_int_distribution<IntType> numbers{ a, b };

        return numbers(m_g);
    }

    /**
     * @brief Generates a uniformly distributed floating-point number in the half-open interval [0, 1).
     *
     * @tparam RealType The floating-point type to generate (e.g., float, double, long double).
     * @return A random number in [0, 1).
     *
     * @note Not marked `noexcept` because `std::uniform_real_distribution::operator()`
     *       is not guaranteed to be `noexcept` by the C++ standard.
     */
    template<std::floating_point RealType = double>
    RealType uniform01()
    {
        return random::uniform01(m_g);
    }

    /**
     * @brief Generates a random number following an exponential distribution.
     *
     * @param a The rate parameter (alpha). Must be > 0.
     * @return Time until the next event.
     *
     * @note Uses inverse transform sampling: x = -ln(u) / a.
     */
    double exponential(double a = 1.0)
    {
        assert(a > 0 && "mylib::random::Randomizer::exponential(double alpha): alpha must be positive");

        return random::exponential(m_g, a);;
    }

    /**
     * @brief Generates a random number following a Cauchy distribution.
     *
     * @param m The location parameter (median). Default is 0.
     * @param q The scale parameter (width). Default is 1.
     * @return A random number from the Cauchy distribution.
     */
    double cauchy(double m = 0.0, double q = 1.0)
    {
        assert(q > 0.0 && "Scale parameter 'q' must be positive!");

        return random::cauchy(m_g, m, q);
    }

    /**
     * @brief Generates a standard normal random number (mean=0, stddev=1)
     *        using the Marsaglia polar method.
     *
     * @return A random number from the standard normal distribution.
     */
    double normal01()
    {
        if(m_hasCachedNormal)
        {
            m_hasCachedNormal = false;
            return m_cachedNormal;
        }
        auto [first, second] { random::normal01(m_g) };

        m_cachedNormal = second;
        m_hasCachedNormal = true;

        return first;
    }

    /**
     * @brief Generates a random number following a Normal (Gaussian) distribution.
     *
     * Scales and shifts a standard normal value (mean=0, stddev=1) generated by
     * normal01() to have the specified mean and standard deviation.
     *
     * @param m The mean (mu) of the distribution. Shifts the center of the bell curve.
     * @param q The standard deviation (sigma) of the distribution. Controls the width
     *          of the bell curve. Must be strictly greater than 0.
     * @return A random number from the specified normal distribution.
     *
     * @note For a standard normal distribution, use normal01() instead.
     */
    double normal(double m, double q)
    {
        assert(q > 0.0 && "Standard deviation must be positive!");
        return m + q * normal01();
    }

    /**
     * @brief Generates a random number following a Gamma distribution.
     *
     * Uses the Marsaglia-Tsang method for shape >= 1, and a mathematical
     * trick (boost) for shape < 1.
     *
     * @param shape The shape parameter (often denoted as k or alpha). Must be > 0.
     * @param scale The scale parameter (often denoted as theta or beta). Must be > 0. Default is 1.0.
     * @return A random number from the Gamma distribution.
     */
    double gamma(double shape = 1.0, double scale = 1.0)
    {
        assert(shape > 0.0 && scale > 0.0);

        return random::gamma(m_g, shape, scale);
    }

    /**
     * @brief Generates a random number following a Chi-Squared distribution.
     *
     * The Chi-Squared distribution with k degrees of freedom is a special case
     * of the Gamma distribution: Gamma(shape = k/2, scale = 2).
     *
     * @param k Degrees of freedom. Must be > 0.
     * @return A random number from the Chi-Squared distribution.
     */
    double chiSquared(double k)
    {
        assert(k > 0.0);
        // Хи-квадрат — это просто Гамма с формой k/2 и масштабом 2
        return gamma(k / 2.0, 2.0);
    }

    /**
     * @brief Generates a random number following a Student's t-distribution.
     *
     * Formula: T = Z / sqrt(V / v)
     * where Z is standard normal, and V is Chi-Squared with v degrees of freedom.
     *
     * @param v Degrees of freedom. Must be > 0.
     * @return A random number from the Student's t-distribution.
     */
    double studentT(double v)
    {
        assert(v > 0.0);
        return random::studentT(m_g, v);
    }

    /**
     * @brief Generates a random number following a Beta distribution.
     *
     * Formula: B = X / (X + Y)
     * where X is Gamma(alpha, 1) and Y is Gamma(beta_param, 1).
     *
     * @param alpha First shape parameter. Must be > 0.
     * @param beta_param Second shape parameter. Must be > 0.
     * @return A random number from the Beta distribution (always in range [0, 1]).
     */
    double beta(double alpha, double beta_param)
    {
        assert(alpha > 0.0 && beta_param > 0.0);

        return random::beta(m_g, alpha, beta_param);
    }

    /**
     * @brief Generates a random number following a Fisher-Snedecor (F) distribution.
     *
     * Formula: F = (U1 / m) / (U2 / n)
     * where U1 is Chi-Squared with m degrees of freedom, and U2 is Chi-Squared with n.
     *
     * @param m Numerator degrees of freedom. Must be > 0.
     * @param n Denominator degrees of freedom. Must be > 0.
     * @return A random number from the F-distribution.
     */
    double fisherF(double m, double n)
    {
        return random::fisherF(m_g, m, n);
    }

    /**
     * @brief Triangular distribution. Returns a value between 0 and 1 with a peak at `middle`.
     */
    double triangular(double middle = 0.5)
    {
        assert(middle > 0.0 && middle < 1.0);
        return random::triangular(m_g, middle);
    }

    /**
     * @brief Scaled Triangular distribution [a, b].
     */
    double triangular(double a, double b, double middle)
    {
        return random::triangular(m_g, a, b, middle);
    }

    /**
     * @brief Lévy distribution (heavy tails).
     */
    double levy(double scale = 0.455)
    {
        // Note: normal(0, 1/sqrt(scale)) means standard deviation is 1/sqrt(scale)
        double temp = normal(0.0, 1.0 / std::sqrt(scale));
        return 1.0 / (temp * temp);
    }

    /**
         * @brief Symmetrized Lévy distribution.
         */
    double symmetrizedLevy(double scale = 0.455)
    {
        return sign() * levy(scale);
    }

    /**
     * @brief Bernoulli distribution (true with probability p, false with 1-p).
     */
    bool bernoulli(double p)
    {
        assert(p >= 0.0 && p <= 1.0);
        return uniform01() < p;
    }

    /**
     * @brief Binomial distribution (number of successes in n trials with probability p).
     */
    int binomial(double p, int n)
    {
        assert(p >= 0.0 && p <= 1.0 && n >= 0);

        return random::binomial(m_g, p, n);
    }

    /**
     * @brief Geometric distribution (number of trials until first success).
     */
    int geometric(double p)
    {
        assert(p > 0.0 && p <= 1.0);

        return random::geometric(m_g, p);
    }

    /**
     * @brief Optimized geometric distribution for p=0.5 using bitwise operations.
     * Counts trailing zeros in a random 64-bit number.
     */
    int geometric05()
    {
        // В C++20 можно использовать std::countr_zero(next())
        // Или функцию bit::rightmostNullCount из вашей библиотеки
        return bit::rightmostNullCount(next()) + 1;
    }

    /**
     * @brief Generates a random number following a Poisson distribution.
     *
     * @param l The expected number of events (lambda).
     *          It equals the rate of events multiplied by the time interval.
     *          Must be > 0.
     * @return The number of events (integer).
     */
    int poisson(double l)
    {
        assert(l > 0.0);

        return random::poisson(m_g, l);
    }
};

inline Randomizer<>& GlobalRNG()
{
    static Randomizer<> r;
    return r;
}

} // end mylib::random namespace

#endif // DISTRIBUTIONS_H
