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

    // ================================================================
    //  Bit access helpers
    // ================================================================

    /**
         * @brief Returns the value of a specific bit (no bounds check).
         */
    bool getBit(size_t i) const;

    /**
         * @brief Returns the word index for a given bit index.
         */
    size_t index(size_t index) const noexcept;

    /**
         * @brief Returns the bit offset within a word for a given bit index.
         */
    size_t offset(size_t index) const noexcept;

    // ================================================================
    //  Sizing helpers
    // ================================================================

    /**
         * @brief Changes the number of bits in the bitset, growing or shrinking it.
         *
         * If `newSize` is greater than the current size, the bitset is extended with
         * zero-initialised bits. If `newSize` is smaller, the trailing bits are
         * discarded and the underlying storage may be compacted. If the number of
         * words required for `newSize` is less than the current capacity by a
         * significant margin, the storage is reallocated to fit.
         *
         * @param newSize New number of bits.
         *
         * @throw std::length_error if `newSize` exceeds the maximum representable size.
         * @throw std::bad_alloc    if memory allocation fails (only when growing).
         *
         * @note On failure the bitset remains unchanged (strong guarantee).
         * @note Bits are zero-initialised in the extended region.
         * @note After the call, `size() == newSize` and all unused bits in the last
         *       word are cleared.
         *
         * @see prepend
         * @see setFromString
         */
    void resize(size_t newSize);

    /**
         * @brief Computes the number of words needed to store m_bitSize bits.
         */
    size_t wordsNeeded() const noexcept;

    static void overflowCheck(size_t current, size_t checkedSize, std::string_view source = "")
    {
        if (current > std::numeric_limits<size_t>::max() - checkedSize)
        {
            throw std::length_error(std::format("Bitset::{}: size overflow", source));
        }
    }

    // ================================================================
    //  String helpers
    // ================================================================

    /**
         * @brief Writes bits without validating. Assumes str is valid and
         *        position + str.size() <= m_bitSize.
         */
    void setFromStringUnchecked(std::string_view str, size_t position) noexcept;

    /**
         * @brief Validates that the string contains only '0' and '1'.
         * @throw std::invalid_argument on invalid character.
         */
    static void validateBinaryString(std::string_view str);

    // ================================================================
    //  Storage maintenance
    // ================================================================

    /**
         * @brief Clears the unused bits in the last word.
         */
    void zeroOutReminder();

public:
    // ================================================================
    //  Construction
    // ================================================================

    /**
         * @brief Default constructor creating an empty bitset.
         * @param initialSize Initial number of bits (all zero). Default 0.
         */
    explicit Bitset(size_t initialSize = 0);

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
         * @see prependFromString
         * @see setFromString
         * @see toString
         */
    explicit Bitset(const std::string& str);

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
    Bitset(const CONTAINER& container);

    // ================================================================
    //  Size and capacity
    // ================================================================

    /**
         * @brief Returns the number of garbage (unused) bits in the last word.
         * @return numberOfDigits - lastWordBits(), or 0 if empty.
         */
    size_t garbageBits() const noexcept;

    /**
         * @brief Returns the number of valid bits in the last word.
         * @return Number of bits in the last word (1..numberOfDigits).
         * @pre m_bitSize > 0.
         */
    size_t lastWordBits() const noexcept;

    /**
         * @brief Returns the number of bits in the bitset.
         */
    size_t size() const noexcept;

    /**
         * @brief Returns the number of words currently used.
         */
    size_t wordsSize() const noexcept;

    // ================================================================
    //  Element access
    // ================================================================

    /**
         * @brief Returns a const reference to the underlying storage container.
         */
    const Container& get() const noexcept;

    /**
         * @brief Returns a pointer to the raw word array.
         */
    const WORD* getData() const noexcept;

    /**
         * @brief Extracts a field of bits starting at position i, length n.
         * @param i Starting bit index.
         * @param n Number of bits to extract (must be <= numberOfDigits).
         * @return The extracted value in the low-order bits of the result.
         * @throw std::out_of_range if n > numberOfDigits or i+n > size().
         */
    WORD getValue(size_t i, size_t n) const;

    /**
         * @brief Mutable access to a bit via proxy reference.
         * @param i Bit index (0-based).
         * @return BitReference allowing assignment and conversion to bool.
         * @throw std::out_of_range if i >= size().
         */
    BitReference operator[](size_t i);

    /**
         * @brief Const access to a bit.
         * @param i Bit index.
         * @return true if the bit is set, false otherwise.
         * @throw std::out_of_range if i >= size().
         */
    bool operator[](size_t i) const;

    // ================================================================
    //  Bit modification
    // ================================================================

    /**
         * @brief Clears all bits to zero.
         */
    void clear() noexcept;

    /**
         * @brief Flips (inverts) all bits in the bitset.
         */
    void flip() noexcept;

    /**
         * @brief Reverses the order of all bits in the bitset.
         * @note Performs a bit‑reversal of the entire sequence.
         * @exception noexcept – does not throw.
         */
    void reverse() noexcept;

    /**
         * @brief Sets a specific bit to a given value.
         * @param i Bit index.
         * @param value Value to set (true=1, false=0). Default true.
         * @throw std::out_of_range if i >= size().
         */
    void set(size_t i, bool value = true);

    /**
         * @brief Sets all bits to a given value.
         * @param value Value to set (true=1, false=0). Default true.
         */
    void setAll(bool value = true) noexcept;

    /**
         * @brief Writes a value into a field of bits at position i, length n.
         * @param value The value to write (only low-order n bits are used).
         * @param i Starting bit index.
         * @param n Number of bits to overwrite (must be <= numberOfDigits).
         * @throw std::out_of_range if n > numberOfDigits or i+n > size().
         */
    void setValue(WORD value, size_t i, size_t n);

    // ================================================================
    //  Insertion / removal
    // ================================================================

    /**
         * @brief Appends a single bit to the end of the string representation.
         *
         * The new bit becomes the last character of toString() (bit 0). All existing
         * bits are shifted to higher indices.
         *
         * @param value Value of the new bit.
         * @exception Strong guarantee – on failure the bitset remains unchanged.
         *
         * @note Implemented as: prepend a dummy 0-bit, shift left by 1, then set bit 0.
         *       The dummy bit absorbs the shift, so no real bit is lost.
         */
    void append(bool value);

    /**
         * @brief Removes the first bit (same as removeFirst).
         */
    void pop_front();

    /**
         * @brief Prepends a single bit to the front of the bitset.
         *
         * In the string representation (MSB first), the new bit becomes the first
         * character of toString(). Internally the bit is added at index size().
         *
         * @param value Value of the new bit.
         * @exception Strong exception guarantee – on failure the bitset remains unchanged.
         */
    void prepend(bool value);

    /**
         * @brief Prepends a block of bits from a WORD value.
         *
         * The low-order `size` bits of `value` are placed at the highest indices,
         * so they appear at the front of the string representation.
         *
         * @param value The word containing the bits to prepend.
         * @param size Number of low-order bits to take from value (1..numberOfDigits).
         * @throw std::out_of_range if size == 0 or size > numberOfDigits.
         * @exception Strong guarantee – no change on failure.
         */
    void prepend(WORD value, size_t size);

    /**
         * @brief Prepends another bitset to the front.
         *
         * After the operation, the string representation is
         * `other.toString() + this->toString()`.
         *
         * @param other Bitset to prepend.
         * @note Handles self-prepend by making a temporary copy.
         * @exception Strong guarantee – no change on failure.
         */
    void prepend(const Bitset& other);

    /**
         * @brief Appends bits from a string representation to the end of the bitset.
         *
         * Equivalent to concatenating `str` to the right of `toString()`. The string
         * is interpreted MSB-first: its first character becomes the most significant
         * bit of the appended block.
         *
         * @param str String of '0' and '1' characters (MSB first).
         *
         * @throw std::invalid_argument if the string contains any character other
         *        than '0' or '1'.
         * @throw std::length_error if the resulting size exceeds the maximum allowed.
         * @exception Strong guarantee – on failure the bitset remains unchanged.
         *
         * @see setFromString
         * @see toString
         */
    void push_back(std::string_view str);

    /**
         * @brief Appends a block of `size` low-order bits from `word` to the end of
         *        the string representation.
         *
         * The first character of the appended block corresponds to bit `size-1` of
         * `word`, the last — to bit 0. After the call, the string representation is
         * `this->toString() + blockString`, where `blockString` is the `size`-bit
         * binary representation of `word` (MSB first).
         *
         * @param word Value whose low-order `size` bits are appended.
         * @param size Number of bits to append (1..numberOfDigits). If 0, no-op.
         *
         * @throw std::out_of_range if size > numberOfDigits.
         * @exception Strong guarantee – on failure the bitset remains unchanged.
         *
         * @note Implemented via prepend(0, size), then <<= size, then setValue at 0.
         */
    void push_back(WORD word, size_t size);

    /**
         * @brief Appends another bitset to the end of this one.
         * @param bitset Bitset to append.
         */
    void push_back(const Bitset& other);

    /**
         * @brief Removes the first bit (the leading character of toString()).
         *
         * Internally removes the bit with index size() - 1.
         *
         * @pre m_bitSize > 0 (asserted in debug build).
         */
    void removeFirst();

    // ================================================================
    //  String conversion
    // ================================================================

    /**
         * @brief Checks whether the bitset is equal to a binary string representation.
         *
         * The string must consist only of '0' and '1' characters and must have the same
         * length as the bitset. The first character corresponds to the most significant
         * bit (MSB), the last character to the least significant bit (LSB).
         *
         * @param str Binary string to compare with (MSB first).
         * @return true if the bitset has the same size and all bits match the string;
         *         false otherwise, including when the string contains invalid characters
         *         or has a different length.
         *
         * @note This method does not throw exceptions; it returns false for any
         *       mismatch, including invalid input.
         * @see toString
         * @see setFromString
         */
    bool equals(std::string_view str) const noexcept;

    /**
         * @brief Prepends bits from a string representation to the front of the bitset.
         *
         * Equivalent to `setFromString(str, size())`. The string is interpreted in
         * the same way: MSB first, so the first character becomes the most significant
         * bit of the prepended block (highest index). After the call the string
         * representation is `str + this->toString()`.
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
    void prependFromString(std::string_view str);

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
         * @see prependFromString
         * @see toString
         */
    void setFromString(std::string_view str, size_t position = 0);

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
         * @see prependFromString
         */
    std::string toString() const;

    // ================================================================
    //  Comparison
    // ================================================================

    /**
         * @brief Three-way comparison (lexicographic order).
         * @return std::strong_ordering::less/equal/greater.
         * @note Compares by size first, then by words.
         */
    auto operator<=>(const Bitset& other) const noexcept;

    /**
         * @brief Equality comparison.
         * @return true if both bitsets have the same size and identical bits.
         */
    bool operator==(const Bitset& other) const noexcept;

    // ================================================================
    //  Bitwise operations
    // ================================================================

    /**
         * @brief Bitwise AND assignment.
         * @param other Bitset of the same size.
         * @return *this.
         * @throw std::length_error if sizes differ.
         */
    Bitset& operator&=(const Bitset& other);

    /**
         * @brief Bitwise XOR assignment.
         * @param other Bitset of the same size.
         * @return *this.
         * @throw std::length_error if sizes differ.
         */
    Bitset& operator^=(const Bitset& other);

    /**
         * @brief Bitwise OR assignment.
         * @param other Bitset of the same size.
         * @return *this.
         * @throw std::length_error if sizes differ.
         */
    Bitset& operator|=(const Bitset& other);

    // ================================================================
    //  Shift operations
    // ================================================================

    /**
         * @brief Left shift assignment.
         * @param shift Number of bits to shift (moves left).
         * @return *this.
         * @note Shift amount is reduced modulo m_bitSize.
         */
    Bitset& operator<<=(size_t shift) noexcept;

    /**
         * @brief Right shift assignment.
         * @param shift Number of bits to shift (moves right).
         * @return *this.
         * @note Shift amount is reduced modulo m_bitSize.
         */
    Bitset& operator>>=(size_t shift) noexcept;

    // ================================================================
    //  Queries
    // ================================================================

    /**
         * @brief Checks if the bitset is non‑zero.
         * @return true if at least one bit is set.
         */
    explicit operator bool() const noexcept;

    /**
         * @brief Checks if all bits are zero.
         * @return true if no bit is set.
         */
    bool isZero() const noexcept;

    /**
         * @brief Counts the number of set bits in the bitset.
         * @return Total popcount.
         */
    int popcount() const noexcept;

    // ================================================================
    //  Nested types
    // ================================================================

    /**
         * @brief Proxy class allowing direct bit assignment and conversion.
         * @details Used by operator[] to provide lvalue access.
         */
    class BitReference final
    {
    private:
        WORD* m_blockPtr{ nullptr };
        int m_index{};

        /**
             * @brief Returns the referenced bit value.
             */
        bool get() const noexcept;

    public:
        /**
             * @brief Constructs a reference to a bit.
             * @param ptr Pointer to the WORD containing the bit.
             * @param offset Bit offset within the WORD (0..numberOfDigits-1).
             * @throw std::out_of_range if offset >= numberOfDigits.
             */
        BitReference(WORD* ptr, size_t offset);

        /**
             * @brief Converts the referenced bit to bool.
             */
        explicit operator bool() const noexcept;

        /**
             * @brief Logical NOT of the referenced bit.
             */
        bool operator!() const noexcept;

        /**
             * @brief Assigns a bool value to the referenced bit.
             * @param value Value to set.
             * @return *this.
             */
        BitReference& operator=(bool value);

        /**
             * @brief Assigns from another BitReference.
             */
        BitReference& operator=(const BitReference& other);

        /**
             * @brief Compares the referenced bit with a bool.
             */
        bool operator==(bool b) const noexcept;

        /**
             * @brief Compares two BitReference objects.
             */
        bool operator==(const BitReference& other) const noexcept;

        /**
             * @brief Compares the referenced bit with a bool (not equal).
             */
        bool operator!=(bool b) const noexcept;

        /**
             * @brief Compares two BitReference objects (not equal).
             */
        bool operator!=(const BitReference& other) const noexcept;

        friend bool operator==(bool b, const BitReference& ref) noexcept
        {
            return ref == b;
        }

        friend bool operator!=(bool b, const BitReference& ref) noexcept
        {
            return ref != b;
        }
    };
};

// ====================================================================
//  Bitset<WORD> — method definitions (alphabetical order)
// ====================================================================

// ---- append ---------------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
void Bitset<WORD>::append(bool value)
{
    prepend(false);
    *this <<= 1;
    set(0, value);
}

// ---- Bitset(const CONTAINER&) ---------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
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
Bitset<WORD>::Bitset(const CONTAINER& container)
    : m_bitSize{ numberOfDigits * container.size() }
    , m_words(container.size())
{
    std::ranges::copy(container, m_words.begin());
    zeroOutReminder();
}

// ---- Bitset(size_t) -------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
Bitset<WORD>::Bitset(size_t initialSize)
    : m_bitSize{ initialSize }
    , m_words(wordsNeeded())
{}

// ---- Bitset(const std::string&) -------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
Bitset<WORD>::Bitset(const std::string& str)
{
    prependFromString(str);
}

// ---- clear ----------------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
void Bitset<WORD>::clear() noexcept
{
    setAll(false);
}

// ---- equals ---------------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
bool Bitset<WORD>::equals(std::string_view str) const noexcept
{
    if(str.size() != m_bitSize)
    {
        return false;
    }

    for(size_t i{}; i < m_bitSize; ++i)
    {
        char c{ str[i] };
        if(c != '0' && c != '1')
        {
            return false;
        }

        // str[0] — MSB (индекс m_bitSize - 1), str[m_bitSize - 1] — LSB (индекс 0)
        size_t bitIndex{ m_bitSize - 1 - i };

        bool bitValue{ c == '1' };
        if(getBit(bitIndex) != bitValue)
        {
            return false;
        }
    }

    return true;
}

// ---- flip -----------------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
void Bitset<WORD>::flip() noexcept
{
    for(size_t i{}; i < wordsSize(); ++i)
    {
        m_words[i] = ~m_words[i];
    }

    zeroOutReminder();
}

// ---- garbageBits ----------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
size_t Bitset<WORD>::garbageBits() const noexcept
{
    return m_bitSize > 0 ? numberOfDigits - lastWordBits() : 0;
}

// ---- get ------------------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
const typename Bitset<WORD>::Container& Bitset<WORD>::get() const noexcept
{
    return m_words;
}

// ---- getBit ---------------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
bool Bitset<WORD>::getBit(size_t i) const
{
    assert(i < m_bitSize);

    return bit::get(m_words[index(i)], offset(i));
}

// ---- getData --------------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
const WORD* Bitset<WORD>::getData() const noexcept
{
    return m_words.data();
}

// ---- getValue -------------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
WORD Bitset<WORD>::getValue(size_t i, size_t n) const
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

// ---- index ----------------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
size_t Bitset<WORD>::index(size_t index) const noexcept
{
    return index / numberOfDigits;
}

// ---- isZero ---------------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
bool Bitset<WORD>::isZero() const noexcept
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

// ---- lastWordBits ---------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
size_t Bitset<WORD>::lastWordBits() const noexcept
{
    assert(m_bitSize > 0);

    size_t result{ offset(m_bitSize) };

    return result == 0 ? numberOfDigits : result;
}

// ---- offset ---------------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
size_t Bitset<WORD>::offset(size_t index) const noexcept
{
    return index % numberOfDigits;
}

// ---- operator bool --------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
Bitset<WORD>::operator bool() const noexcept
{
    return !isZero();
}

// ---- operator&= -----------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
Bitset<WORD>& Bitset<WORD>::operator&=(const Bitset& other)
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

// ---- operator<=> ----------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
auto Bitset<WORD>::operator<=>(const Bitset& other) const noexcept
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

// ---- operator== -----------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
bool Bitset<WORD>::operator==(const Bitset& other) const noexcept
{
    return m_words == other.m_words;
}

// ---- operator|= -----------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
Bitset<WORD>& Bitset<WORD>::operator|=(const Bitset& other)
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

// ---- operator^= -----------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
Bitset<WORD>& Bitset<WORD>::operator^=(const Bitset& other)
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

// ---- operator[] (non-const) -----------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
typename Bitset<WORD>::BitReference Bitset<WORD>::operator[](size_t i)
{
    if(i >= m_bitSize)
    {
        throw std::out_of_range("mylib::BitSet::operator[]: index must be < m_bitSize");
    }

    return BitReference(&m_words[index(i)], offset(i));
}

// ---- operator[] (const) ---------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
bool Bitset<WORD>::operator[](size_t i) const
{
    if (i >= m_bitSize)
    {
        throw std::out_of_range("mylib::BitSet::operator[] const: index out of range");
    }
    return getBit(i);
}

// ---- operator<<= ----------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
Bitset<WORD>& Bitset<WORD>::operator<<=(size_t shift) noexcept
{
    if(m_bitSize == 0)
    {
        return *this;   // ничего не делаем, если набор пуст
    }

    const size_t normalShift{ shift % m_bitSize };
    const size_t wordShift{ index(normalShift) };
    const size_t bitShift{ offset(normalShift) };

    if(wordShift > 0) // сдвиг по словам
    {
        for(size_t i{ wordsSize() }; i-- > wordShift; )
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
            WORD tempCarry{ static_cast<WORD>(m_words[i] >> (numberOfDigits - bitShift)) };
            m_words[i] <<= bitShift;
            m_words[i] |= carry;
            carry = tempCarry;
        }
    }

    zeroOutReminder();
    return *this;
}

// ---- operator>>= ----------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
Bitset<WORD>& Bitset<WORD>::operator>>=(size_t shift) noexcept
{
    if(m_bitSize == 0)
    {
        return *this;   // ничего не делаем, если набор пуст
    }

    const size_t normalShift{ shift % m_bitSize };
    const size_t wordShift{ index(normalShift) };
    const size_t bitShift{ offset(normalShift) };

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
        for(size_t i{ wordsSize() - wordShift }; i-- > 0; )
        {
            WORD tempCarry{ static_cast<WORD>(m_words[i] << (numberOfDigits - bitShift)) };
            m_words[i] >>= bitShift;
            m_words[i] |= carry;
            carry = tempCarry;
        }
    }

    zeroOutReminder();
    return *this;
}

// ---- pop_front ------------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
void Bitset<WORD>::pop_front()
{
    removeFirst();
}

// ---- popcount -------------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
int Bitset<WORD>::popcount() const noexcept
{
    int sum{};
    for(size_t i{}; i < wordsSize(); ++i)
    {
        sum += bit::popcount<WORD>(m_words[i]);
    }

    return sum;
}

// ---- prepend(bool) --------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
void Bitset<WORD>::prepend(bool value)
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

// ---- prepend(WORD, size_t) ------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
void Bitset<WORD>::prepend(WORD value, size_t size)
{
    if(0 == size)
    {
        return;
    }

    if (size > std::numeric_limits<WORD>::digits)
    {
        throw std::out_of_range("mylib::Bitset::prepend: size exceeds WORD bits");
    }

    size_t start{ m_bitSize };

    overflowCheck(m_bitSize, size, "prepend(WORD, size_t)");
    resize(m_bitSize + size);

    setValue(value, start, size);
}

// ---- prepend(const Bitset&) -----------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
void Bitset<WORD>::prepend(const Bitset& other)
{
    if(this == &other)
    {
        Bitset temp{ other };
        prepend(temp);
        return;
    }

    if(other.size() == 0)
    {
        return;
    }

    // Копируем биты из other в текущий набор
    size_t destWord{ index(m_bitSize) };
    size_t destBit{ offset(m_bitSize) };

    size_t srcPos{ 0 };  // текущая позиция в other
    size_t srcBits{ other.size() };

    // Вычисляем новый размер и выделяем память
    overflowCheck(m_bitSize, other.size(), "prepend(const Bitset&)");
    resize(m_bitSize + other.size());

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
}

// ---- prependFromString ----------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
void Bitset<WORD>::prependFromString(std::string_view str)
{
    setFromString(str, m_bitSize);
}

// ---- push_back(std::string_view) ------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
void Bitset<WORD>::push_back(std::string_view str)
{
    if (str.empty())
    {
        return;
    }
    validateBinaryString(str);

    overflowCheck(m_bitSize, str.size(), "push_back(std::string_view)");
    resize(m_bitSize + str.size());
    *this <<= str.size();
    setFromStringUnchecked(str, 0);
}

// ---- push_back(WORD, size_t) ----------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
void Bitset<WORD>::push_back(WORD word, size_t size)
{
    if(0 == size)
    {
        return;
    }

    prepend(static_cast<WORD>(0), size);

    *this <<= static_cast<int>(size);

    setValue(word, 0, size);
}

// ---- push_back(const Bitset&) ---------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
void Bitset<WORD>::push_back(const Bitset& other)
{
    if(this == &other)
    {
        Bitset temp{ other };
        push_back(temp);
        return;
    }
    if(other.size() == 0)
    {
        return;
    }

    overflowCheck(m_bitSize, other.size(), "push_back(const Bitset&)");
    resize(m_bitSize + other.size());
    *this <<= other.size();

    for(size_t i{}; i < other.wordsSize(); ++i)
    {
        m_words[i] |= other.m_words[i];
    }
}

// ---- removeFirst ----------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
void Bitset<WORD>::removeFirst()
{
    assert(m_bitSize > 0);
    if(lastWordBits() == 1)
    {
        m_words.pop_back();
    }

    --m_bitSize;
    zeroOutReminder();
}

// ---- resize ---------------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
void Bitset<WORD>::resize(size_t newSize)
{
    const size_t needed{ static_cast<size_t>(math::ceiling(newSize, numberOfDigits)) };

    if(needed != m_words.size())
    {
        m_words.resize(needed);
    }

    m_bitSize = newSize;
    zeroOutReminder();
}

// ---- reverse --------------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
void Bitset<WORD>::reverse() noexcept
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

// ---- set ------------------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
void Bitset<WORD>::set(size_t i, bool value)
{
    if (i >= m_bitSize)
    {
        throw std::out_of_range("mylib::BitSet::set: index out of range");
    }

    bit::set(m_words[index(i)], offset(i), value);
}

// ---- setAll ---------------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
void Bitset<WORD>::setAll(bool value) noexcept
{
    for(size_t i{}; i < wordsSize(); ++i)
    {
        m_words[i] = value ? static_cast<WORD>(bit::FULL) : static_cast<WORD>(bit::ZERO);
    }

    zeroOutReminder();
}

// ---- setFromString --------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
void Bitset<WORD>::setFromString(std::string_view str, size_t position)
{
    if (str.empty())
    {
        return;
    }
    // Проверка на валидные символы
    validateBinaryString(str);
    overflowCheck(position, str.length(), "setFromString");

    if(position + str.length() > m_bitSize)
    {
        resize(position + str.length());
    }

    // Записываем биты: первый символ -> старший бит (position+len-1)
    setFromStringUnchecked(str, position);
}

// ---- setFromStringUnchecked -----------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
void Bitset<WORD>::setFromStringUnchecked(std::string_view str, size_t position) noexcept
{
    for(size_t i{}; i < str.size(); ++i)
    {
        size_t bitPos = position + (str.size() - 1 - i);
        bit::set(m_words[index(bitPos)], offset(bitPos), str[i] == '1');
    }
}

// ---- setValue -------------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
void Bitset<WORD>::setValue(WORD value, size_t i, size_t n)
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
        bit::setValue(m_words[word++], static_cast<WORD>(value >> shift), static_cast<int>(bit), static_cast<int>(m));
        shift += m;
        numbers -= m;

        bit = 0; // после первого слова все последующие пишем с нулевого бита
    }
}

// ---- size -----------------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
size_t Bitset<WORD>::size() const noexcept
{
    return m_bitSize;
}

// ---- toString -------------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
std::string Bitset<WORD>::toString() const
{
    std::string result;
    result.reserve(m_bitSize);
    for (size_t i{ m_bitSize }; i > 0; --i)
    {
        bool b{ getBit(i - 1) }; // getBit без проверки границ, но i-1 корректно
        result.push_back(b ? '1' : '0');
    }

    return result;
}

// ---- validateBinaryString -------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
void Bitset<WORD>::validateBinaryString(std::string_view str)
{
    for(char ch : str)
    {
        if(ch != '0' && ch != '1')
        {
            throw std::invalid_argument(
                "mylib::Bitset: string must contain only '0' and '1'");
        }
    }
}

// ---- wordsNeeded ----------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
size_t Bitset<WORD>::wordsNeeded() const noexcept
{
    return static_cast<size_t>(math::ceiling(m_bitSize, numberOfDigits));
}

// ---- wordsSize ------------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
size_t Bitset<WORD>::wordsSize() const noexcept
{
    return m_words.size();
}

// ---- zeroOutReminder ------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
void Bitset<WORD>::zeroOutReminder()
{
    if(m_bitSize > 0)
    {
        m_words.back() &= bit::lowerMask(lastWordBits());
    }
}

// ====================================================================
//  Bitset<WORD>::BitReference — method definitions (alphabetical)
// ====================================================================

// ---- BitReference(WORD*, size_t) ------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
Bitset<WORD>::BitReference::BitReference(WORD* ptr, size_t offset)
    : m_blockPtr{ ptr }
{
    size_t limit{ std::numeric_limits<WORD>::digits };
    if(offset >= limit)
    {
        throw std::out_of_range(std::format(
            "mylib::Bitset::BitReference(WORD, size_t): offset must be in [0, {}]",
            limit - 1));
    }

    m_index = static_cast<int>(offset);
}

// ---- get ------------------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
bool Bitset<WORD>::BitReference::get() const noexcept
{
    return bit::get(*m_blockPtr, m_index);
}

// ---- operator bool --------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
Bitset<WORD>::BitReference::operator bool() const noexcept
{
    return get();
}

// ---- operator! ------------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
bool Bitset<WORD>::BitReference::operator!() const noexcept
{
    return !get();
}

// ---- operator=(bool) ------------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
typename Bitset<WORD>::BitReference&
Bitset<WORD>::BitReference::operator=(bool value)
{
    bit::set(*m_blockPtr, m_index, value);
    return *this;
}

// ---- operator=(const BitReference&) ---------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
typename Bitset<WORD>::BitReference&
Bitset<WORD>::BitReference::operator=(const BitReference& other)
{
    *this = static_cast<bool>(other);
    return *this;
}

// ---- operator==(bool) -----------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
bool Bitset<WORD>::BitReference::operator==(bool b) const noexcept
{
    return get() == b;
}

// ---- operator==(const BitReference&) --------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
bool Bitset<WORD>::BitReference::operator==(const BitReference& other) const noexcept
{
    return get() == other.get();
}

// ---- operator!=(bool) -----------------------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
bool Bitset<WORD>::BitReference::operator!=(bool b) const noexcept
{
    return !(get() == b);
}

// ---- operator!=(const BitReference&) --------------------------------
template<typename WORD>
    requires std::unsigned_integral<WORD>
bool Bitset<WORD>::BitReference::operator!=(const BitReference& other) const noexcept
{
    return !(*this == other);
}

/**
     * @brief Deduction guide for constructing Bitset from a container.
     */
template<typename CONTAINER>
Bitset(const CONTAINER&) -> Bitset<typename CONTAINER::value_type>;

} // end namespace mylib

#endif // BITSET_H
