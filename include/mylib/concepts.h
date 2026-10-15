#ifndef CONCEPTS_H
#define CONCEPTS_H

namespace mylib
{

/**
 * @brief Concept that detects a container exposing a noexcept reverse() method.
 *
 * Used by Bitset::reverse() to decide whether to call the container's own
 * reverse() (O(1) for Vector) or fall back to std::reverse.
 */
template<typename CONTAINER>
concept HasReverse = requires(CONTAINER& c)
{
    { c.reverse() } noexcept;
};

} // end namespace

#endif // CONCEPTS_H
