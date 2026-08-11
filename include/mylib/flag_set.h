#ifndef FLAG_SET_H
#define FLAG_SET_H

#include "mylib/bit_operations.h"
#include <format>

namespace mylib
{

    class FlagSet final
    {
    private:
        uint64_t m_flags{}; // хранилище
        int m_numFlags{};   // реальное количество флагов

        void checkIndex(int index, std::string_view str)
        {
            if(index > m_numFlags - 1)
            {
                throw std::out_of_range(std::format("mylib::FlagSet::{}: index is out of range", str));
            }
        }

    public:
        explicit FlagSet() = default;

        explicit FlagSet(int numFlags)
            : m_flags { bit::lowerMask(numFlags) }
            , m_numFlags{ numFlags }
        {}

        void set(int index, bool value = true)
        {
            checkIndex(index, "set");

            bit::set(m_flags, index, value);
        }

        void clear(int index)
        {
            checkIndex(index, "clear");

            set(index, false);
        }

        void toggle(int index)
        {
            checkIndex(index, "toggle");

            m_flags = bit::flip(m_flags, index);
        }

    };

} // end mylib namespace

#endif // FLAG_SET_H
