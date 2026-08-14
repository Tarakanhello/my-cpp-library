#ifndef BITSET_H
#define BITSET_H

#include <algorithm>
#include <cstdint>
#include <format>
#include <limits>

#include "mylib/bit_operations.h"
#include "mylib/math.h"
#include "mylib/vector.h"

namespace mylib
{

    template<typename WORD = std::uint64_t>
        requires std::unsigned_integral<WORD>
    class Bitset final
    {
    private:
        using Container = Vector<WORD>;

        enum{ numberOfDigits = std::numeric_limits<WORD>::digits };
        size_t m_bitSize{};
        Container m_storage{};

        void zeroOutReminder()
        {
            if(m_bitSize > 0)
            {
                m_storage.back() &= bit::lowerMask(lastWordBits());
            }
        }

        size_t index(size_t index) const noexcept { return index / numberOfDigits; }
        size_t offset(size_t index) const noexcept { return index % numberOfDigits; }

        bool getBit(size_t i) const
        {
            assert(i >= 0 && i < m_bitSize);

            return bit::get(m_storage[index(i)], offset(i));
        }

        std::uint64_t wordsNeeded() const { return math::ceiling(m_bitSize, numberOfDigits); }

    public:
        class BitReference final
        {
        private:
            WORD* m_blockPtr{ nullptr };
            int m_index{};

        public:
            BitReference(WORD* ptr, size_t offset)
                : m_blockPtr{ ptr }
            {
                size_t limit{ 8 * sizeof(WORD) };
                if(offset >= limit)
                {
                    throw std::out_of_range(std::format("mylib::Bitset::BitReference(WORD, size_t): offset must be in [0, {}]", limit - 1));
                }

                m_index = static_cast<int>(offset);
            }

            explicit operator bool() const { return (bit::get(*m_blockPtr, m_index)); }

            BitReference& operator=(bool value)
            {
                bit::set(*m_blockPtr, m_index, value);

                return *this;
            }

            BitReference& operator=(const BitReference& other)
            {
                *this = static_cast<bool>(other);
                return *this;
            }
        };

        explicit Bitset(std::uint64_t initialSize = 0)
            : m_bitSize{ initialSize }
            , m_storage(wordsNeeded())
        {}

        template<typename CONTAINER>
            requires requires(CONTAINER c)
            {
                c.begin();
                c.end();
                c.size();
            }
            && std::unsigned_integral<typename CONTAINER::value_type>
            && std::same_as<typename CONTAINER::value_type, WORD>
        Bitset(const CONTAINER& container)
            : m_bitSize{ numberOfDigits * container.size() }
            , m_storage(container.size())
        {
            std::ranges::copy(container, m_storage);
        }

        int lastWordBits() const noexcept
        {
            assert(m_bitSize > 0);

            std::uint64_t result{ offset(m_bitSize) };

            return result == 0 ? numberOfDigits : result;
        }

        int garbageBits() const noexcept { return m_bitSize > 0 ? numberOfDigits - lastWordBits() : 0; }

        const Container& get() const noexcept { return m_storage; }
        const WORD* getData() const noexcept { return m_storage.data(); }

        size_t size() const noexcept { return m_bitSize; }
        size_t wordSize() const noexcept { return m_storage.size(); }

        explicit operator bool() const noexcept { return !m_storage.empty(); }

        BitReference operator[](size_t i)
        {
            if(i >= m_bitSize)
            {
                throw std::out_of_range("mylib::BitSet::operator[]: index must be < m_bitSize");
            }

            return BitReference(&m_storage[index(i)], offset(i));
        }

        bool operator[](size_t i) const { return getBit(i); }

        void set(int i, bool value = true)
        {
            assert(i >= 0 && i < m_bitSize);
            bit::set(m_storage[index(i)], offset(i), value);
        }



    };

    template<typename CONTAINER>
    Bitset(const CONTAINER&) -> Bitset<typename CONTAINER::value_type>;

} // end namespace mylib

#endif // BITSET_H
