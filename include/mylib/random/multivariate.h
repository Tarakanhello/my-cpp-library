#ifndef MULTIVARIATE_H
#define MULTIVARIATE_H

#include <cassert>
#include <concepts>
#include "mylib/containers/vector.h"
#include "mylib/random/distributions.h"
// #include "mylib/math/matrix.h" // TODO: Раскомментировать позже

namespace mylib::random
{

/**
 * @brief Generates random vectors following a Multivariate Normal distribution.
 *
 * @note Currently uses independent normals (Identity covariance).
 *       TODO: Implement Cholesky decomposition to support arbitrary covariance matrices.
 */
class MultivariateNormal
{
private:
template<typename T>
    using Vector = mylib::Vector<T>;

    Vector<double> m_means;
    // Matrix<double> m_L; // TODO: Матрица Холецкого (L * L^T = covariance)

    double m_cachedNormal{ 0.0 };
    bool m_hasCachedNormal{ false };

public:
    /**
     * @brief Temporary constructor assuming Identity covariance.
     */
    explicit MultivariateNormal(const Vector<double>& means)
        : m_means{ means } {}

    // TODO: Добавить конструктор с ковариационной матрицей
    /*
    MultivariateNormal(const Vector<double>& means, const Matrix<double>& covariance)
        : m_means(means)
        , m_L(makeL(covariance))
    {}
    */

    /**
     * @brief Generates a random vector from the distribution.
     */
    template<std::uniform_random_bit_generator GEN>
    Vector<double> next(GEN& rng)
    {
        Vector<double> result{};

        result.reserve(m_means.size());

        for(size_t i{ 0 }; i < m_means.size(); ++i)
        {
            if(m_hasCachedNormal)
            {
                result.push_back(m_means[i] + m_cachedNormal);
                m_hasCachedNormal = false;
            }
            else
            {
                auto [first, second] = normal01(rng);
                result.push_back(m_means[i] + first);

                m_cachedNormal = second;
                m_hasCachedNormal = true;
            }

            auto [first, second] { pointUnitCircle(rng) };
        }

        return result;
    }
};

} // end namespace

#endif // MULTIVARIATE_H
