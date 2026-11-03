#ifndef STATISTICS_H
#define STATISTICS_H

#include <algorithm>
#include <concepts>
#include <cassert>
#include <cmath>
#include <limits>
#include <type_traits>

#include "mylib/core/structs.h"

namespace mylib::math
{

/**
 * @brief Accumulates mean and variance for a Normal distribution.
 *
 * This struct is useful for Monte Carlo simulations where you need to combine
 * or scale the statistical properties of independent random variables.
 *
 * It inherits from ArithmeticType (CRTP) to automatically provide
 * non-member arithmetic operators (+, -, *) based on the compound assignments (+=, -=, *=).
 */
struct NormalSummary : public ArithmeticType<NormalSummary>
{
    double mean{};      ///< The expected value (center of the bell curve).
    double variance{};  ///< The variance (width of the bell curve). Must be >= 0.

    /**
     * @brief Constructs a NormalSummary.
     *
     * @param theMean The expected value (mean).
     * @param theVariance The variance (must be >= 0).
     */
    explicit NormalSummary(double theMean = 0, double theVariance = 0)
        : mean{ theMean }
        , variance{ theVariance }
    {
        assert(theVariance >= 0 && "Variance cannot be negative");
    }

    /**
     * @brief Returns the standard deviation (square root of variance).
     * @return double The standard deviation.
     */
    double stddev() const { return std::sqrt(variance); }

    /**
     * @brief Returns the 95% error margin (approx. 2 * standard deviation).
     * @return double The error margin for a 95% confidence interval.
     */
    double error95() const { return 2 * stddev(); }

    /**
     * @brief Combines two independent summaries (sum of variables).
     *
     * For independent variables: mean = m1 + m2, variance = v1 + v2.
     *
     * @param b The other NormalSummary to add.
     * @return NormalSummary A reference to this object after modification.
     */
    NormalSummary& operator+=(NormalSummary const& b)
    {
        mean += b.mean;
        variance += b.variance;
        return *this;
    }

    /**
     * @brief Subtracts one summary from another (difference of variables).
     *
     * For independent variables: mean = m1 - m2, variance = v1 + v2
     * (Variances always add, even when subtracting the means).
     *
     * @param b The other NormalSummary to subtract.
     * @return NormalSummary A reference to this object after modification.
     */
    NormalSummary& operator-=(NormalSummary const& b)
    {
        mean -= b.mean;
        variance += b.variance;
        return *this;
    }

    /**
     * @brief Scales the distribution by a constant.
     *
     * The mean scales linearly, but the variance scales by the square of the constant.
     *
     * @param a The scaling factor.
     * @return NormalSummary A reference to this object after modification.
     */
     NormalSummary& operator*=(double a)
    {
        mean *= a;
        variance *= a * a;
        return *this;
    }

    NormalSummary operator*(double a) const
    {
        NormalSummary temp{ *this };
        temp *= a;
        return temp;
    }

    friend NormalSummary operator*(double a, const NormalSummary& ns)
    {
        return ns * a;
    }

};



/**
 * @brief Accumulates statistics incrementally without storing all values.
 *
 * This struct uses Welford's-like online algorithm (via sum and squaredSum)
 * to compute mean and variance in O(1) memory, making it ideal for streaming
 * data or large Monte Carlo simulations.
 *
 * @warning The variance formula used here (E[X^2] - E[X]^2) is susceptible to
 *          catastrophic cancellation for large numbers with small differences.
 *          The `max(0.0, ...)` is a band-aid to prevent negative variances due to
 *          floating-point inaccuracies.
 */
struct IncrementalStatistics
{
    double sum{ 0.0 };                                          ///< The sum of all values.
    double squaredSum{ 0.0 };                                   ///< The sum of squares of all values.
    double minimum{ std::numeric_limits<double>::infinity() };  ///< The smallest value seen.
    double maximum{ -std::numeric_limits<double>::infinity() }; ///< The largest value seen.
    size_t n{ 0 };                                              ///< The total number of values processed.

    /**
     * @brief Adds a new value to the running statistics.
     * @param x The value to add.
     */
    void addValue(double x)
    {
        ++n;
        maximum = std::max(maximum, x);
        minimum = std::min(minimum, x);

        sum += x;
        squaredSum += x * x;
    }

    /**
     * @brief Calculates the arithmetic mean of the values.
     * @return double The average value.
     * @note Requires at least one value (n > 0).
     */
    double getMean() const
    {
        assert(n > 0 && "Cannot calculate mean of empty dataset");

        return sum / n;
    }

    /**
     * @brief Calculates the sample variance.
     *
     * Uses the formula: (squaredSum - sum * mean) / (n - 1).
     * The division by (n - 1) applies Bessel's correction for sample variance.
     *
     * @return double The variance. Returns 0.0 if fewer than 2 values exist.
     */
    double getVariance() const
    {
        if(n < 2)
        {
            return 0.0;
        }

        double var{ (squaredSum - sum * getMean()) / (n - 1.0) };

        // Защита от -0.000000001 из-за ошибок округления double
        return std::max(0.0, var);
    }

    /**
     * @brief Returns the sample standard deviation.
     * @return double The standard deviation.
     */
    double stddev() const
    {
        return std::sqrt(getVariance());
    }

    /**
     * @brief Returns a NormalSummary for the standard error of the mean.
     *
     * The variance of the mean (standard error squared) is Var(X) / n.
     *
     * @return NormalSummary A summary representing the distribution of the sample mean.
     * @note Requires at least one value (n > 0).
     */
    NormalSummary getStandardErrorSummary() const
    {
        assert(n > 0 && "Cannot calculate standard error of empty dataset");

        return NormalSummary{ getMean(), getVariance() / n };
    }
};




/**
 * @brief Runs a Monte Carlo simulation and returns the accumulated statistics.
 *
 * @tparam FUNCTION The type of the callable. It must be invocable with no arguments
 *                  and return a value convertible to double.
 * @param f The function to simulate (e.g., a lambda).
 * @param nSimulations Number of iterations.
 * @return IncrementalStatistics containing mean, variance, min, max.
 */
template<std::invocable<> FUNCTION>
    requires std::convertible_to<std::invoke_result_t<FUNCTION>, double>
IncrementalStatistics MonteCarloSimulate(const FUNCTION& f, size_t nSimulations = 1000)
{
    IncrementalStatistics s;
    for(size_t i{ 0 }; i < nSimulations; ++i )
    {
        s.addValue(f());
    }

    return s;
}




/**
 * @brief Result of comparing two benchmark runs.
 */
struct CompatisonResult
{
    bool    isSignifivant;  ///< True if the difference is statistically significant.
    double  meanDifference; ///< The difference between the two means (A - B).
    double  error95;        ///< The 95% error margin for the difference.
};



/**
 * @brief Compares two IncrementalStatistics to see if one is significantly faster than the other.
 *
 * @param a The statistics of the first algorithm.
 * @param b The statistics of the second algorithm.
 * @return ComparisonResult The result of the comparison.
 */
inline CompatisonResult compareAlgorithms(const IncrementalStatistics& a, const IncrementalStatistics& b)
{
    // 1. Получаем "Стандартные ошибки средних" (Standard Error)
    // Это колокол, описывающий точность вычисления среднего времени.
    NormalSummary summaryA{ a.getStandardErrorSummary() };
    NormalSummary summaryB{ b.getStandardErrorSummary() };

    // 2. Вычисляем разницу (A - B)
    // При вычитании средние вычитаются, а дисперсии складываются.
    NormalSummary diff{ summaryA - summaryB };

    // 3. Проверяем, попадает ли ноль в 95% доверительный интервал
    // Ноль попадает, если модуль разницы меньше нашей погрешности.
    bool isSignificant{ std::abs(diff.mean) > diff.error95() };

    return{ isSignificant, diff.mean, diff.error95() };
}
}







#endif // STATISTICS_H
