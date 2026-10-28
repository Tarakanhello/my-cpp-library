#ifndef CONCEPTS_H
#define CONCEPTS_H

#include <concepts>
#include <cstdint>
#include <type_traits>

namespace mylib
{

/**
 * @brief Concept that detects a container exposing a noexcept reverse() method.
 */
template<typename CONTAINER>
concept HasReverse = requires(CONTAINER& c)
{
    { c.reverse() } noexcept;
};



/**
 * @brief Concept that detects whether CONTAINER is constructible from
 *        an iterator pair [first, last).
 */
template<typename CONTAINER, typename ITERATOR>
concept ConstructibleFromRange = requires(ITERATOR first, ITERATOR last)
{
    CONTAINER(first, last);
};



/**
 * @brief Concept that detects whether the type is an unsigned integer of exactly 32 or 64 bits.
 */
template<typename UIntType>
concept UInt_32_or_64 = std::is_unsigned_v<UIntType> && (sizeof(UIntType) == 4 || sizeof(UIntType) == 8);



/**
 * @brief Concept that detects whether the type provides a generate() method
 *        suitable for seeding a random engine.
 *        (Концепт, проверяющий наличие метода generate(), подходящего для инициализации генератора).
 */
template<typename T>
concept SeedSequence = requires(T& seq, uint32_t* ptr)
{
    // Требуется, чтобы метод generate принимал два указателя (начала и конца)
    // и возвращал void
    { seq.generate(ptr, ptr) } -> std::same_as<void>;
};

} // end namespace

#endif // CONCEPTS_H
