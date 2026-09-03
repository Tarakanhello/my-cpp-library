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
    /**
     * @brief Concept to check if a container has a reverse() method.
     */
    template<typename CONTAINER>
    concept HasReverse = requires(CONTAINER& c)
    {
        c.reverse();
    };

    /**
     * @brief A dynamic bitset (bit array) with arbitrary size.
     *
     * @tparam WORD Underlying unsigned integer type used as a storage word.
     *         Must be an unsigned integral type (e.g., uint64_t, uint32_t).
     *
     * The bitset stores bits in a vector of WORDs, with automatic resizing.
     * Provides standard bitwise operations, shifts, reversal, and popcount.
     * Supports direct bit access via proxy reference.
     */
    template<typename WORD = std::uint64_t>
        requires std::unsigned_integral<WORD>
    class Bitset final
    {
    private:
        using Container = Vector<WORD>;
    public:
        class BitReference;

    private:
        static constexpr size_t numberOfDigits{ std::numeric_limits<WORD>::digits };
        size_t m_bitSize{};     ///< Number of bits in the bitset.
        Container m_words{};    ///< Storage for words.

        /**
         * @brief Clears the unused bits in the last word.
         */
        void zeroOutReminder()
        {
            if(m_bitSize > 0)
            {
                m_words.back() &= bit::lowerMask(lastWordBits());
            }
        }

        /**
         * @brief Returns the word index for a given bit index.
         */
        size_t index(size_t index) const noexcept { return index / numberOfDigits; }

        /**
         * @brief Returns the bit offset within a word for a given bit index.
         */
        size_t offset(size_t index) const noexcept { return index % numberOfDigits; }

        /**
         * @brief Returns the value of a specific bit (no bounds check).
         */
        bool getBit(size_t i) const
        {
            assert(i < m_bitSize);

            return bit::get(m_words[index(i)], offset(i));
        }

        /**
         * @brief Computes the number of words needed to store m_bitSize bits.
         */
        size_t wordsNeeded() const noexcept { return static_cast<size_t>(math::ceiling(m_bitSize, numberOfDigits)); }

    public:
        /**
         * @brief Default constructor creating an empty bitset.
         * @param initialSize Initial number of bits (all zero). Default 0.
         */
        explicit Bitset(size_t initialSize = 0)
            : m_bitSize{ initialSize }
            , m_words(wordsNeeded())
        {}

        /**
         * @brief Constructs a bitset from a string of '0' and '1' characters.
         *
         * The string is interpreted as a binary representation with the first character
         * being the most significant bit (MSB). The resulting bitset will have a size
         * equal to the length of the string. The last character of the string becomes
         * bit 0 (LSB).
         *
         * @param str String of '0' and '1' characters (MSB first).
         *
         * @throw std::invalid_argument if the string contains any character other
         *        than '0' or '1'.
         * @throw std::length_error if the string length exceeds the maximum allowed
         *        size.
         *
         * @note The constructor is `explicit` to prevent accidental implicit
         *       conversions from strings.
         *
         * @see appendFromString
         * @see setFromString
         * @see toString
         */
        explicit Bitset(const std::string& str)
        {
            appendFromString(str);
        }

        /**
         * @brief Constructs a bitset from a container of WORDs.
         * @tparam CONTAINER Type of the container. Must provide begin(), end(), size(), data().
         *                   value_type must be WORD.
         * @param container Container with initial data.
         * @note The size is set to numberOfDigits * container.size().
         *       Unused bits in the last word are zeroed out.
         */
        template<typename CONTAINER>
            requires requires(const CONTAINER& c)
            {
                c.begin();
                c.end();
                c.size();
                c.data();
            }
            && std::unsigned_integral<typename CONTAINER::value_type>
            && std::same_as<typename CONTAINER::value_type, WORD>
        Bitset(const CONTAINER& container)
            : m_bitSize{ numberOfDigits * container.size() }
            , m_words(container.size())
        {
            std::ranges::copy(container, m_words.begin());
            zeroOutReminder();
        }

        /**
         * @brief Returns the number of valid bits in the last word.
         * @return Number of bits in the last word (1..numberOfDigits).
         * @pre m_bitSize > 0.
         */
        size_t lastWordBits() const noexcept
        {
            assert(m_bitSize > 0);

            size_t result{ offset(m_bitSize) };

            return result == 0 ? numberOfDigits : result;
        }

        /**
         * @brief Returns the number of garbage (unused) bits in the last word.
         * @return numberOfDigits - lastWordBits(), or 0 if empty.
         */
        size_t garbageBits() const noexcept { return m_bitSize > 0 ? numberOfDigits - lastWordBits() : 0; }

        /**
         * @brief Returns a const reference to the underlying storage container.
         */
        const Container& get() const noexcept { return m_words; }

        /**
         * @brief Returns a pointer to the raw word array.
         */
        const WORD* getData() const noexcept { return m_words.data(); }

        /**
         * @brief Returns the number of bits in the bitset.
         */
        size_t size() const noexcept { return m_bitSize; }

        /**
         * @brief Returns the number of words currently used.
         */
        size_t wordsSize() const noexcept { return m_words.size(); }

        /**
         * @brief Mutable access to a bit via proxy reference.
         * @param i Bit index (0-based).
         * @return BitReference allowing assignment and conversion to bool.
         * @throw std::out_of_range if i >= size().
         */
        BitReference operator[](size_t i)
        {
            if(i >= m_bitSize)
            {
                throw std::out_of_range("mylib::BitSet::operator[]: index must be < m_bitSize");
            }

            return BitReference(&m_words[index(i)], offset(i));
        }

        /**
         * @brief Const access to a bit.
         * @param i Bit index.
         * @return true if the bit is set, false otherwise.
         * @throw std::out_of_range if i >= size().
         */
        bool operator[](size_t i) const
        {
            if (i >= m_bitSize)
            {
                throw std::out_of_range("mylib::BitSet::operator[] const: index out of range");
            }
            return getBit(i);
        }

        /**
         * @brief Sets a specific bit to a given value.
         * @param i Bit index.
         * @param value Value to set (true=1, false=0). Default true.
         * @throw std::out_of_range if i >= size().
         */
        void set(size_t i, bool value = true)
        {
            if (i >= m_bitSize)
            {
                throw std::out_of_range("mylib::BitSet::set: index out of range");
            }

            bit::set(m_words[index(i)], offset(i), value);
        }

        /**
         * @brief Appends a single bit to the end.
         * @param value Value of the new bit.
         * @exception Strong exception guarantee – on failure the bitset remains unchanged.
         */
        void append(bool value)
        {
            ++m_bitSize;
            if(wordsSize() < wordsNeeded())
            {
                try
                {
                    m_words.push_back(0);
                }
                catch(...)
                {
                    --m_bitSize;
                    throw;
                }

            }
            set(m_bitSize - 1, value);
        }

        /**
         * @brief Appends a block of bits from a WORD value.
         * @param value The word containing the bits to append.
         * @param size Number of low-order bits to take from value (1..numberOfDigits).
         * @throw std::out_of_range if size == 0 or size > numberOfDigits.
         * @exception Strong guarantee – no change on failure.
         */
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
            size_t newBitSize{ m_bitSize + size };
            size_t neededWords{ static_cast<size_t>(math::ceiling(newBitSize, numberOfDigits)) };
            if(neededWords > m_words.size())
            {
                m_words.resize(neededWords);
            }
            m_bitSize = newBitSize;
            setValue(value, start, size);
        }

        /**
         * @brief Appends another bitset to the end.
         * @param other Bitset to append.
         * @note Handles self-append by making a temporary copy.
         * @exception Strong guarantee – no change on failure.
         */
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

            // Вычисляем новый размер и выделяем память
            size_t newBitSize{ m_bitSize + other.size() };
            size_t needed{ static_cast<size_t>(math::ceiling(newBitSize, numberOfDigits)) };
            if (needed > wordsSize())
            {
                m_words.resize(needed);
            }

            // Копируем биты из other в текущий набор
            size_t destWord{ index(m_bitSize) };
            size_t destBit{ offset(m_bitSize) };

            size_t srcPos{ 0 };  // текущая позиция в other
            size_t srcBits{ other.size() };

            while(srcPos < srcBits)
            {
                // Сколько бит осталось скопировать из other
                size_t remaining{ srcBits - srcPos };
                // Сколько места осталось в текущем слове
                size_t spaceInWord{ numberOfDigits - destBit };
                size_t chunk{ std::min(remaining, spaceInWord) };

                // Читаем chunk бит из other, начиная с позиции srcPos
                WORD chunkValue{ other.getValue(srcPos, chunk) };  // используем метод getValue

                // Записываем в текущее слово
                bit::setValue(m_words[destWord], chunkValue, static_cast<int>(destBit), static_cast<int>(chunk));

                // Продвигаем указатели
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

        /**
         * @brief Removes the last bit.
         * @pre m_bitSize > 0 (asserted in debug build).
         */
        void removeLast()
        {
            assert(m_bitSize > 0);
            if(lastWordBits() == 1)
            {
                m_words.pop_back();
            }

            --m_bitSize;
            zeroOutReminder();
        }

        /**
         * @brief Removes the last bit (same as removeLast).
         */
        void pop_back()
        {
            removeLast();
        }

        /**
         * @brief Equality comparison.
         * @return true if both bitsets have the same size and identical bits.
         */
        bool operator==(const Bitset& other) const noexcept
        {
            return m_words == other.m_words;
        }

        /**
         * @brief Three-way comparison (lexicographic order).
         * @return std::strong_ordering::less/equal/greater.
         * @note Compares by size first, then by words.
         */
        auto operator<=>(const Bitset& other) const noexcept
        {
            if(m_bitSize != other.m_bitSize)
            {
                return m_bitSize <=> other.m_bitSize;
            }

            for(size_t i{ wordsSize() }; i-- > 0;)
            {
                if(m_words[i] != other.m_words[i])
                {
                    return m_words[i] <=> other.m_words[i];
                }
            }

            return std::strong_ordering::equal;
        }

        /**
         * @brief Bitwise AND assignment.
         * @param other Bitset of the same size.
         * @return *this.
         * @throw std::length_error if sizes differ.
         */
        Bitset& operator &=(const Bitset& other)
        {
            if(m_bitSize != other.m_bitSize)
            {
                throw std::length_error("Bitset::operator &=: length is not the same");
            }

            for(size_t i{}; i < wordsSize(); ++i)
            {
                m_words[i] &= other.m_words[i];
            }

            zeroOutReminder();
            return *this;
        }

        /**
         * @brief Bitwise OR assignment.
         * @param other Bitset of the same size.
         * @return *this.
         * @throw std::length_error if sizes differ.
         */
        Bitset& operator |=(const Bitset& other)
        {
            if(m_bitSize != other.m_bitSize)
            {
                throw std::length_error("Bitset::operator |=: length is not the same");
            }

            for(size_t i{}; i < wordsSize(); ++i)
            {
                m_words[i] |= other.m_words[i];
            }

            zeroOutReminder();
            return *this;
        }

        /**
         * @brief Bitwise XOR assignment.
         * @param other Bitset of the same size.
         * @return *this.
         * @throw std::length_error if sizes differ.
         */
        Bitset& operator ^=(const Bitset& other)
        {
            if(m_bitSize != other.m_bitSize)
            {
                throw std::length_error("Bitset::operator ^=: length is not the same");
            }

            for(size_t i{}; i < wordsSize(); ++i)
            {
                m_words[i] ^= other.m_words[i];
            }

            zeroOutReminder();
            return *this;
        }

        /**
         * @brief Flips (inverts) all bits in the bitset.
         */
        void flip() noexcept
        {
            for(size_t i{}; i < wordsSize(); ++i)
            {
                m_words[i] = ~m_words[i];
            }

            zeroOutReminder();
        }

        /**
         * @brief Right shift assignment.
         * @param shift Number of bits to shift (positive moves right, negative left).
         * @return *this.
         * @note Shift amount is reduced modulo m_bitSize.
         */
        Bitset& operator>>=(int shift) noexcept
        {
            if(m_bitSize == 0)
            {
                return *this;   // ничего не делаем, если набор пуст
            }

            if(shift < 0)
            {
                return (operator <<=(-shift));
            }

            size_t normalShift{ static_cast<size_t>(shift) % m_bitSize };
            size_t wordShift{ index(normalShift) };
            int bitShift{ offset(normalShift) };

            if(wordShift > 0) // сдвиг по словам
            {
                for(size_t i{}; i + wordShift < wordsSize(); ++i)
                {
                    m_words[i] = m_words[i + wordShift];
                    m_words[i + wordShift] = 0;
                }
            }
            if(bitShift > 0)
            {
                //  00000101 | 00111000 >>= 4 -> 10000000 | 00000011
                WORD carry{};
                for(int i{ static_cast<int>(wordsSize()) - 1 - static_cast<int>(wordShift) }; i >= 0; --i)
                {
                    WORD tempCarry{ m_words[static_cast<size_t>(i)] << (numberOfDigits - bitShift) };
                    m_words[static_cast<size_t>(i)] >>= bitShift;
                    m_words[static_cast<size_t>(i)] |= carry;
                    carry = tempCarry;
                }
            }

            zeroOutReminder();
            return *this;
        }

        /**
         * @brief Left shift assignment.
         * @param shift Number of bits to shift (positive moves left, negative right).
         * @return *this.
         * @note Shift amount is reduced modulo m_bitSize.
         */
        Bitset& operator<<=(int shift) noexcept
        {
            if(m_bitSize == 0)
            {
                return *this;   // ничего не делаем, если набор пуст
            }

            if(shift < 0)
            {
                return (operator >>=(-shift));
            }

            size_t normalShift{ static_cast<size_t>(shift) % m_bitSize };
            size_t wordShift{ index(normalShift) };
            int bitShift{ offset(normalShift) };

            if(wordShift > 0) // сдвиг по словам
            {
                for(int i{ static_cast<int>(wordsSize()) - 1 }; i - wordShift >= 0; --i)
                {
                    m_words[i] = m_words[i - wordShift];
                    m_words[i - wordShift] = 0;
                }
            }
            if(bitShift > 0) // сдвиг по битам
            {
                // Пример: 10000000 | 00000011 <<= 4 -> 00000000 | 00111000
                WORD carry{};
                for(size_t i{ wordShift }; i < wordsSize(); ++i)
                {
                    WORD tempCarry{ m_words[i] >> (numberOfDigits - bitShift) };
                    m_words[i] <<= bitShift;
                    m_words[i] |= carry;
                    carry = tempCarry;
                }
            }

            zeroOutReminder();
            return *this;
        }

        /**
         * @brief Sets all bits to a given value.
         * @param value Value to set (true=1, false=0). Default true.
         */
        void setAll(bool value = true) noexcept
        {
            for(size_t i{}; i < wordsSize(); ++i)
            {
                m_words[i] = value ? static_cast<WORD>(bit::FULL) : static_cast<WORD>(bit::ZERO);
            }

            zeroOutReminder();
        }

        /**
         * @brief Clears all bits to zero.
         */
        void clear() noexcept
        {
            setAll(false);
        }

        /**
         * @brief Checks if all bits are zero.
         * @return true if no bit is set.
         */
        bool isZero() const noexcept
        {
            for(size_t i{}; i < wordsSize(); ++i)
            {
                if(static_cast<bool>(m_words[i]))
                {
                    return false;
                }
            }

            return true;
        }

        /**
         * @brief Checks if the bitset is non‑zero.
         * @return true if at least one bit is set.
         */
        explicit operator bool() const noexcept { return !isZero(); }

        /**
         * @brief Extracts a field of bits starting at position i, length n.
         * @param i Starting bit index.
         * @param n Number of bits to extract (must be <= numberOfDigits).
         * @return The extracted value in the low-order bits of the result.
         * @throw std::out_of_range if n > numberOfDigits or i+n > size().
         */
        WORD getValue(size_t i, size_t n) const
        {
            if(n > std::numeric_limits<WORD>::digits ||
               i + n > m_bitSize)
            {
                throw std::out_of_range("mylib::Bitset::getValue: invalid range");
            }

            WORD result{};
            size_t word{ index(i) };    // индекс слова, где находится начало
            size_t bit{ offset(i) };    // позиция бита внутри этого слова
            size_t shift{};             // сдвиг для укладки извлечённых бит в результат

            int numbers{ static_cast<int>(n) };
            while(numbers > 0)
            {
                // сколько бит можно взять из текущего слова (либо до конца слова,
                // либо сколько осталось до конца поля)
                size_t m{ std::min(static_cast<size_t>(numbers), numberOfDigits - bit) };

                // извлекаем m бит из текущего слова, начиная с позиции bit,
                // и помещаем их в результат со сдвигом shift
                result |= bit::getValue(m_words[word++], static_cast<int>(bit), static_cast<int>(m)) << shift;
                shift += m;
                numbers -= static_cast<int>(m);

                bit = 0; // после первого слова все последующие читаем с нулевого бита
            }

            return result;
        }

        /**
         * @brief Writes a value into a field of bits at position i, length n.
         * @param value The value to write (only low-order n bits are used).
         * @param i Starting bit index.
         * @param n Number of bits to overwrite (must be <= numberOfDigits).
         * @throw std::out_of_range if n > numberOfDigits or i+n > size().
         */
        void setValue(WORD value, size_t i, size_t n)
        {
            if(n > std::numeric_limits<WORD>::digits ||
               i + n > m_bitSize)
            {
                throw std::out_of_range("mylib::Bitset::setValue: invalid range");
            }

            size_t word{ index(i) };    // индекс слова, где находится начало
            size_t bit{ offset(i) };    // позиция бита внутри этого слова
            size_t shift{};             // сдвиг в value, откуда брать биты

            size_t numbers{ n };
            while(numbers > 0)
            {
                // сколько бит можно записать в текущее слово (либо до конца слова,
                // либо сколько осталось до конца поля)
                size_t m{ std::min(numbers, numberOfDigits - bit) };

                // записываем m бит из value (начиная со сдвига shift)
                // в текущее слово, начиная с позиции bit
                bit::setValue(m_words[word++], value >> shift, static_cast<int>(bit), static_cast<int>(m));
                shift += m;
                numbers -= m;

                bit = 0; // после первого слова все последующие пишем с нулевого бита
            }
        }

        /**
         * @brief Reverses the order of all bits in the bitset.
         * @note Performs a bit‑reversal of the entire sequence.
         * @exception noexcept – does not throw.
         */
        void reverse() noexcept
        {
            size_t nFill{ garbageBits() };
            m_bitSize += nFill;
            (*this) <<= static_cast<int>(nFill);
            // инвертирование слов на хранении
            if constexpr(HasReverse<decltype(m_words)>)
            {
                m_words.reverse();
            }
            else
            {
                std::reverse(m_words.begin(), m_words.end());
            }

            for(size_t i{}; i < wordsSize(); ++i)
            {
                m_words[i] = bit::reverseAllBits(m_words[i]);
            }

            //удаление мусора
            m_bitSize -= nFill;
            zeroOutReminder();
        }

        /**
         * @brief Counts the number of set bits in the bitset.
         * @return Total popcount.
         */
        int popcount() const noexcept
        {
            int sum{};
            for(size_t i{}; i < wordsSize(); ++i)
            {
                sum += bit::popcount<WORD>(m_words[i]);
            }

            return sum;
        }


        /**
         * @brief Writes bits from a string representation into the bitset.
         *
         * The string must contain only characters '0' and '1'. The first character of
         * the string is treated as the most significant bit (MSB) of the block being
         * written. It will be stored at the highest bit index within the written range:
         * position + str.size() - 1. The last character corresponds to the lowest bit
         * (position). If the bitset is smaller than needed, it is automatically
         * resized.
         *
         * @param str      String of '0' and '1' characters (MSB first).
         * @param position Starting bit index (LSB of the written block). Default 0.
         *
         * @throw std::invalid_argument if the string contains any character other
         *        than '0' or '1'.
         * @throw std::length_error if the resulting size exceeds the maximum allowed.
         * @exception Strong guarantee – on failure the bitset remains unchanged.
         *
         * @note If `position + str.size()` exceeds the current size, the bitset is
         *       extended (new bits are zero-initialised). If the string is empty,
         *       the function does nothing.
         *
         * @see appendFromString
         * @see toString
         */
        void setFromString(const std::string& str, size_t position = 0)
        {
            if (str.empty())
            {
                return;
            }
            // Проверка на валидные символы
            for (char ch : str)
            {
                if (ch != '0' && ch != '1')
                {
                    throw std::invalid_argument("Bitset::setFromString: string must contain only '0' and '1'");
                }
            }
            size_t len{ str.length() };
            size_t newBitSize{ position + len };
            if (newBitSize > m_bitSize)
            {
                size_t neededWords{ (newBitSize + numberOfDigits - 1) / numberOfDigits };
                if (neededWords > m_words.size())
                {
                    m_words.resize(neededWords);
                }

                m_bitSize = newBitSize;
                zeroOutReminder();
            }
            // Записываем биты: первый символ -> старший бит (position+len-1)
            for (size_t j{ 0 }; j < len; ++j)
            {
                bool val{ (str[j] == '1') };
                size_t bitPos{ position + (len - 1 - j) };
                set(bitPos, val); // set уже проверяет границы, но мы расширили
            }
        }


        /**
         * @brief Appends bits from a string representation to the end of the bitset.
         *
         * Equivalent to `setFromString(str, size())`. The string is interpreted in
         * the same way: MSB first, so the first character becomes the most significant
         * bit of the appended block (highest index).
         *
         * @param str String of '0' and '1' characters (MSB first).
         *
         * @throw std::invalid_argument if the string contains invalid characters.
         * @throw std::length_error if size overflow occurs.
         * @exception Strong guarantee.
         *
         * @see setFromString
         * @see toString
         */
        void appendFromString(const std::string& str)
        {
            setFromString(str, m_bitSize);
        }


        /**
         * @brief Converts the entire bitset to a string of '0' and '1' characters.
         *
         * The string is built from the most significant bit to the least significant
         * bit (MSB first). That is, the first character corresponds to the bit with
         * index `size() - 1`, the last character corresponds to bit 0.
         *
         * @return std::string containing '0' and '1' characters, length equals size().
         *         Returns an empty string if the bitset is empty.
         *
         * @note The method is `const` and does not modify the bitset.
         *
         * @see setFromString
         * @see appendFromString
         */
        std::string toString() const
        {
            std::string result;
            result.reserve(m_bitSize);
            for (size_t i{ m_bitSize }; i > 0; --i)
            {
                bool bit{ getBit(i - 1) }; // getBit без проверки границ, но i-1 корректно
                result.push_back(bit ? '1' : '0');
            }

            return result;
        }

    public:
        /**
         * @brief Proxy class allowing direct bit assignment and conversion.
         * @details Used by operator[] to provide lvalue access.
         */
        class BitReference final
        {
        private:
            WORD* m_blockPtr{ nullptr };
            int m_index{};

            bool get() const noexcept
            {
                return bit::get(*m_blockPtr, m_index);
            }

        public:
            /**
             * @brief Constructs a reference to a bit.
             * @param ptr Pointer to the WORD containing the bit.
             * @param offset Bit offset within the WORD (0..numberOfDigits-1).
             * @throw std::out_of_range if offset >= numberOfDigits.
             */
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

            /**
             * @brief Converts the referenced bit to bool.
             */
            explicit operator bool() const { return get(); }

            /**
             * @brief Assigns a bool value to the referenced bit.
             * @param value Value to set.
             * @return *this.
             */
            BitReference& operator=(bool value)
            {
                bit::set(*m_blockPtr, m_index, value);

                return *this;
            }

            /**
             * @brief Assigns from another BitReference.
             */
            BitReference& operator=(const BitReference& other)
            {
                *this = static_cast<bool>(other);
                return *this;
            }

            bool operator==(bool b) const noexcept
            {
                return get() == b;
            }

            bool operator!=(bool b) const noexcept
            {
                return !(get() == b);
            }

            friend bool operator==(bool b, const BitReference& ref) noexcept
            {
                return ref == b;
            }

            friend bool operator!=(bool b, const BitReference& ref) noexcept
            {
                return ref != b;
            }

            bool operator!() const noexcept
            {
                return !get();
            }

            bool operator==(const BitReference& other) const noexcept
            {
                return get() == other.get();
            }

            bool operator!=(const BitReference& other) const noexcept
            {
                return !(*this == other);
            }
        };
    };

    /**
     * @brief Deduction guide for constructing Bitset from a container.
     */
    template<typename CONTAINER>
    Bitset(const CONTAINER&) -> Bitset<typename CONTAINER::value_type>;

} // end namespace mylib

#endif // BITSET_H
