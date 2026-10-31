#ifndef STATISTICS_H
#define STATISTICS_H

#include <cmath>
#include <cassert>

#include "mylib/core/structs.h"

namespace mylib::math
{

/**
 * @brief Accumulates mean and variance for a Normal distribution.
 *
 * This struct is useful for Monte Carlo simulations where you need to combine
 * or scale the statistical properties of independent random variables.
 */
struct NormalSummary : public ArithmeticType<NormalSummary>
{
    double mean{};
    double variance{};

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
        assert(theVariance);
    }

    /// @brief Returns the standard deviation (square root of variance).
    double stddev() const { return std::sqrt(variance); }

    /// @brief Returns the 95% error margin (approx. 2 * standard deviation).
    double error95() const { return 2 * stddev(); }

    /**
     * @brief Combines two independent summaries (sum of variables).
     *
     * For independent variables: mean = m1 + m2, variance = v1 + v2.
     */
    NormalSummary operator+=(NormalSummary const& b)
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
     */
    NormalSummary operator-=(NormalSummary const& b)
    {
        mean -= b.mean;
        variance += b.variance;
        return *this;
    }

    /**
     * @brief Scales the distribution by a constant.
     *
     * The mean scales linearly, but the variance scales by the square of the constant.
     */
     NormalSummary operator*=(double a)
    {
        mean *= a;
        variance *= a * a;
        return *this;
    }
};

}

#endif // STATISTICS_H
