#ifndef BIT_OPERATIONS_H
#define BIT_OPERATIONS_H

#include <cstdint>
#include <stdexcept>

namespace mylib
{

    namespace bit
    {
        template<int x>
        constexpr uint64_t twoPower() noexcept
        {
            static_assert(x >= 0 && x < 64, "mylib::bit::twoPower(): x must be positive and less then 64");
            return 1ULL << x;
        }

        constexpr uint64_t twoPower(int x)
        {
            if(x < 0 || x >= 64)
            {
                throw std::out_of_range("mylib::bit::twoPower(int): x must be positive and less then 64");
            }

            return 1ULL << x;
        }
    } // end bit namespace

} // end mylib namespace
#endif // BIT_OPERATIONS_H
