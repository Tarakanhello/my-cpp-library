#ifndef BIT_OPERATIONS_H
#define BIT_OPERATIONS_H

#include <cstdint>
#include <stdexcept>

namespace mylib
{

    namespace bit
    {
        template<int x>
        [[nodiscard]] constexpr uint64_t twoPower() noexcept
        {
            static_assert(x >= 0 && x < 64, "mylib::bit::twoPower(): x must be in [0, 63]");
            return uint64_t{ 1 } << x;
        }

        [[nodiscard]] constexpr uint64_t twoPower(int x)
        {
            if(x < 0 || x >= 64)
            {
                throw std::out_of_range("mylib::bit::twoPower(int): x must be in [0, 63]");
            }

            return uint64_t{ 1 } << x;
        }

        [[nodiscard]] constexpr bool isPowerOfTwo(uint64_t x) noexcept
        {
            return (x !=0 ) && ((x & (x - 1)) == 0);
        }

        template<uint64_t X>
        [[nodiscard]] constexpr int log2floor() noexcept
        {
            static_assert(X != 0, "mylib::bit::log2floor(): X must be positive");
            uint64_t n{ X };
            int result{ 0 };
            while(n >>= 1)
            {
                ++result;
            }

            return result;
        }

        [[nodiscard]] constexpr int log2floor(uint64_t x)
        {
            if(0 == x)
            {
                throw std::out_of_range("mylib::bit:log2floor(uint64_t): x must be positive");
            }
#if defined(__GNUC__) || defined(__clang__)
            return 63 - __builtin_clzll(x);
#elif defined(_MSC_VER)
            unsigned long index;
            _BitScanReverse64(&index, x);
            return static_cast<int>(index);
#else
            int result{ 0 };
            while(x >>= 1)
            {
                ++result;
            }

            return result;
#endif
        }

    } // end bit namespace

} // end mylib namespace
#endif // BIT_OPERATIONS_H
