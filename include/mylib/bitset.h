#ifndef BITSET_H
#define BITSET_H

#include <algorithm>
#include <cstdint>
#include <format>
#include <limits>
#include <stdexcept>

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
    public:
        class BitReference;

    private:
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

        size_t wordsNeeded() const { return static_cast<size_t>(math::ceiling(m_bitSize, numberOfDigits)); }

    public:
        explicit Bitset(size_t initialSize = 0)
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
            zeroOutReminder();
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
        size_t storageSize() const noexcept { return m_storage.size(); }

        BitReference operator[](size_t i)
        {
            if(i >= m_bitSize)
            {
                throw std::out_of_range("mylib::BitSet::operator[]: index must be < m_bitSize");
            }

            return BitReference(&m_storage[index(i)], offset(i));
        }

        bool operator[](size_t i) const
        {
            if (i >= m_bitSize)
            {
                throw std::out_of_range("mylib::BitSet::operator[] const: index out of range");
            }
            return getBit(i);
        }

        void set(int i, bool value = true)
        {
            if (i >= m_bitSize)
            {
                throw std::out_of_range("mylib::BitSet::set: index out of range");
            }

            bit::set(m_storage[index(i)], offset(i), value);
        }

        void append(bool value)
        {
            ++m_bitSize;
            if(storageSize() < wordsNeeded())
            {
                try
                {
                    m_storage.push_back(0);
                }
                catch(...)
                {
                    --m_bitSize;
                    throw;
                }

            }
            set(m_bitSize - 1, value);
        }

        void push_back(WORD value, size_t size)
        {
            if(0 == size)
            {
                return;
            }
            if (size > std::numeric_limits<WORD>::digits)
            {
                throw std::out_of_range("mylib::Bitset::push_back: size exceeds WORD bits");
            }
            size_t start{ m_bitSize };
            m_bitSize += size;
            size_t k{ wordsNeeded() - storageSize() };
            for(size_t i{}; i < k; ++i)
            {
                m_storage.push_back(0);
            }

            setValue(value, start, size);
        }

        void push_back(const Bitset& other)
        {
            if (this == &other)
            {
                Bitset temp{ other };
                push_back(temp);
                return;
            }

            if (other.size() == 0)
                return;

            // Вычисляем новый размер и резервируем память
            size_t newBitSize{ m_bitSize + other.size() };
            size_t needed{ static_cast<size_t>(math::ceiling(newBitSize, numberOfDigits)) };
            if (needed > storageSize())
            {
                m_storage.resize(needed);
            }

            // Копируем биты из other в конец текущего
            size_t destWord{ index(m_bitSize) };
            size_t destBit{ offset(m_bitSize) };

            size_t srcPos{ 0 };  // позиция в битах в other
            size_t srcBits{ other.size() };

            while(srcPos < srcBits)
            {
                // Сколько бит осталось скопировать из other
                size_t remaining{ srcBits - srcPos };
                // Сколько бит можно записать в текущее целевое слово
                size_t spaceInWord{ numberOfDigits - destBit };
                size_t chunk{ std::min(remaining, spaceInWord) };

                // Извлекаем chunk бит из other, начиная с позиции srcPos
                WORD chunkValue{ other.getValue(srcPos, chunk) };  // используем метод getValue

                // Записываем в целевое слово
                bit::setValue(m_storage[destWord], chunkValue, static_cast<int>(destBit), static_cast<int>(chunk));

                // Продвигаем позиции
                srcPos += chunk;
                destBit += chunk;
                if(destBit == numberOfDigits)
                {
                    destBit = 0;
                    ++destWord;
                }
            }

            m_bitSize = newBitSize;
            zeroOutReminder();
        }

        void removeLast()
        {
            assert(m_bitSize > 0);
            if(lastWordBits() == 1)
            {
                m_storage.pop_back();
            }

            --m_bitSize;
            zeroOutReminder();
        }

        void pop_back()
        {
            removeLast();
        }

        bool operator==(const Bitset& other) const noexcept
        {
            return m_storage == other.m_storage;
        }

        Bitset& operator &=(const Bitset& other)
        {
            if(m_bitSize != other.m_bitSize)
            {
                throw std::length_error("Bitset::operator &=: length is not the same");
            }

            for(size_t i{}; i < storageSize(); ++i)
            {
                m_storage[i] &= other.m_storage[i];
            }

            zeroOutReminder();
            return *this;
        }

        Bitset& operator |=(const Bitset& other)
        {
            if(m_bitSize != other.m_bitSize)
            {
                throw std::length_error("Bitset::operator |=: length is not the same");
            }

            for(size_t i{}; i < storageSize(); ++i)
            {
                m_storage[i] |= other.m_storage[i];
            }

            zeroOutReminder();
            return *this;
        }

        Bitset& operator ^=(const Bitset& other)
        {
            if(m_bitSize != other.m_bitSize)
            {
                throw std::length_error("Bitset::operator ^=: length is not the same");
            }

            for(size_t i{}; i < storageSize(); ++i)
            {
                m_storage[i] ^= other.m_storage[i];
            }

            zeroOutReminder();
            return *this;
        }

        void flip()
        {
            for(size_t i{}; i < storageSize(); ++i)
            {
                m_storage[i] = ~m_storage[i];
            }

            zeroOutReminder();
        }

        Bitset& operator >>=(int shift)
        {
            if(m_bitSize == 0)
            {
                return *this;   // защита от деления на ноль
            }

            if(shift < 0)
            {
                return (operator <<=(-shift));
            }

            size_t normalShift{ static_cast<size_t>(shift) % m_bitSize };
            size_t wordShift{ index(normalShift) };
            int bitShift{ offset(normalShift) };

            if(wordShift > 0) // сдвиг слов
            {
                for(size_t i{}; i + wordShift < storageSize(); ++i)
                {
                    m_storage[i] = m_storage[i + wordShift];
                    m_storage[i + wordShift] = 0;
                }
            }
            if(bitShift > 0) // сдвиг битов
            {
                // для слова 00000101 | 00111000 >>= 4 -> 10000000 | 00000011
                WORD carry{};
                for(int i{ static_cast<int>(storageSize()) - 1 - static_cast<int>(wordShift) }; i >= 0; --i)
                {
                    WORD tempCarry{ m_storage[static_cast<size_t>(i)] << (numberOfDigits - bitShift) };
                    m_storage[static_cast<size_t>(i)] >>= bitShift;
                    m_storage[static_cast<size_t>(i)] |= carry;
                    carry = tempCarry;
                }
            }

            zeroOutReminder();
            return *this;
        }


        Bitset& operator <<=(int shift)
        {
            if(m_bitSize == 0)
            {
                return *this;   // защита от деления на ноль
            }

            if(shift < 0)
            {
                return (operator >>=(-shift));
            }

            size_t normalShift{ static_cast<size_t>(shift) % m_bitSize };
            size_t wordShift{ index(normalShift) };
            int bitShift{ offset(normalShift) };

            if(wordShift > 0) // сдвиг слов
            {
                for(int i{ static_cast<int>(storageSize()) - 1 }; i - wordShift >= 0; --i)
                {
                    m_storage[i] = m_storage[i - wordShift];
                    m_storage[i - wordShift] = 0;
                }
            }
            if(bitShift > 0) // сдвиг битов
            {
                // для слова 10000000 | 00000011 <<= 4 -> 00000000 | 00111000
                WORD carry{};
                for(size_t i{ wordShift }; i < storageSize(); ++i)
                {
                    WORD tempCarry{ m_storage[i] >> (numberOfDigits - bitShift) };
                    m_storage[i] <<= bitShift;
                    m_storage[i] |= carry;
                    carry = tempCarry;
                }
            }

            zeroOutReminder();
            return *this;
        }

        void setAll(bool value = true)
        {
            for(size_t i{}; i < storageSize(); ++i)
            {
                m_storage[i] = value ? static_cast<WORD>(bit::FULL) : static_cast<WORD>(bit::ZERO);
            }

            zeroOutReminder();
        }

        void clear() noexcept
        {
            setAll(false);
        }

        bool isZero() const noexcept
        {
            for(size_t i{}; i < storageSize(); ++i)
            {
                if(static_cast<bool>(m_storage[i]))
                {
                    return false;
                }
            }

            return true;
        }

        explicit operator bool() const noexcept { return !isZero(); }

        WORD getValue(size_t i, size_t n) const
        {
            if(n > std::numeric_limits<WORD>::digits ||
               i + n > m_bitSize)
            {
                throw std::out_of_range("mylib::Bitset::getValue: invalid range");
            }

            WORD result{};
            size_t word{ index(i) };
            size_t bit{ offset(i) };
            size_t shift{};

            int numbers{ static_cast<int>(n) };
            while(numbers > 0)
            {
                size_t m{ std::min(static_cast<size_t>(numbers), numberOfDigits - bit) };   // либо все биты, либо столько,
                                                                                            // сколько поместилось в слове
                result |= bit::getValue(m_storage[word++], static_cast<int>(bit), static_cast<int>(m)) << shift;
                shift += m;
                numbers -= static_cast<int>(m);

                bit = 0;
            }

            return result;
        }

        void setValue(WORD value, size_t i, size_t n)
        {
            if(n > std::numeric_limits<WORD>::digits ||
               i + n > m_bitSize)
            {
                throw std::out_of_range("mylib::Bitset::setValue: invalid range");
            }

            size_t word{ index(i) };
            size_t bit{ offset(i) };
            size_t shift{};

            int numbers{ static_cast<int>(n) };
            while(numbers > 0)
            {
                size_t m{ std::min(numbers, numberOfDigits - bit) };
                bit::setValue(m_storage[word++], value >> shift, static_cast<int>(bit), static_cast<int>(m));
                shift += m;
                numbers -= static_cast<int>(m);

                bit = 0;
            }
        }

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
                size_t limit{ std::numeric_limits<WORD>::digits };
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
    };

    template<typename CONTAINER>
    Bitset(const CONTAINER&) -> Bitset<typename CONTAINER::value_type>;

} // end namespace mylib

#endif // BITSET_H
