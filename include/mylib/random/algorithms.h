#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include "mylib/containers/vector.h"
#include "mylib/random/distributions.h"

namespace mylib::algorithms
{

template< typename T>
using Vector = mylib::Vector<T>;

/**
 * @brief Generates a random unit vector in n-dimensional space.
 *
 * @tparam RNG The Uniform Random Bit Generator to use.
 * @param n The dimension of the vector.
 * @param rng The random number generator.
 * @return mylib::Vector<double> A vector of size n with Euclidean length 1.0.
 */
inline Vector<double> randomInitVector(size_t n)
{
    Vector<double> result(n);
    std::uniform_real_distribution<double> dist{ -1.0, 1.0 };

    double sumSq{};

    while(true)
    {
        sumSq = 0.0;

        for(size_t i{}; i < n; ++i)
        {
            result[i] = dist(mylib::random::GlobalRNG());
            sumSq += result[i] * result[i];
        }

        if(sumSq != 0.0)
        {
            break;
        }
    }

    double norm = std::sqrt(sumSq);

    for (std::size_t i = 0; i < n; ++i)
    {
        result[i] /= norm;
    }



    return result;

}

} // end mylib::random namespace




#endif // ALGORITHMS_H
