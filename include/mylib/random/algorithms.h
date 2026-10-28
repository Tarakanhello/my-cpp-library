#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include "mylib/containers/vector.h"
#include "mylib/random/distributions.h"

namespace mylib::algorithms
{

template< typename T>
using Vector = mylib::Vector<T>;

inline Vector<double> randomInitVector(int n)
{
    Vector<double> result(n);
    while(true)
    {
        double sum{};
        for(size_t i{}; i < static_cast<size_t>(n); ++i)
        {
            result[i] = mylib::GlobalRNG().uniform01() * mylib::random::sign(mylib::GlobalRNG());
            sum += result[i];
        }

        if(sum != 0)
        {
            break;
        }
    }


    auto normalization{ []<typename T>(T& vec)
    {
        double normLength{};

        if constexpr(requires { vec.norm(); })
        {
            normLength = vec.norm();
        }
        else
        {
            double result{};
            for(const auto& element : vec)
            {
                result += element * element;
            }
            normLength = std::sqrt(result);
        }

        for(auto& elem : vec)
        {
            elem *= (1 / normLength);
        }
    } };

    normalization(result);


    return result;

}

} // end mylib::random namespace




#endif // ALGORITHMS_H
