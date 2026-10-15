#ifndef CONCEPTS_H
#define CONCEPTS_H

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

} // end namespace

#endif // CONCEPTS_H
