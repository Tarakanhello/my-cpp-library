#ifndef QUICK_SORT_H
#define QUICK_SORT_H

#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <utility>

#include "mylib/core/structs.h"
#include "mylib/random/distributions.h"

namespace mylib::algorithms
{

/**
 * @brief Selects an iterator to a pivot element using the "Median of Three" strategy.
 *
 * This function chooses three random elements within the given range,
 * determines the median among them, and returns an iterator pointing to it.
 * This helps to avoid the O(N^2) worst-case time complexity of Quicksort
 * on already sorted arrays.
 *
 * @tparam IT The forward access iterator type.
 * @tparam COMPARATOR The type of the comparison function object.
 *
 * @param begin Iterator to the beginning of the range (inclusive).
 * @param end Iterator to the end of the range (exclusive).
 * @param comp The comparator used to determine order.
 *
 * @return IT An iterator pointing to the selected pivot element.
 *
 * @throws std::invalid_argument If the range is empty or invalid (begin >= end).
 */
template<std::forward_iterator IT, typename COMPARATOR = comparators::DefaultComparator<std::iter_value_t<IT>>>
IT pickPivot(IT begin, IT end, const COMPARATOR& comp = COMPARATOR())
{
    if(begin == end)
    {
        throw std::invalid_argument("mylib::algorithms::pickPivot(IT, IT, COMPARATOR): Wrong range");
    }

    auto distance{ std::distance(begin, end) };
    auto maxIdx{ distance - 1 };

    auto i { random::GlobalRNG().inRange( std::ptrdiff_t{ 0 }, maxIdx) };
    auto j { random::GlobalRNG().inRange( std::ptrdiff_t{ 0 }, maxIdx) };
    auto k { random::GlobalRNG().inRange( std::ptrdiff_t{ 0 }, maxIdx) };


    auto it_i{  std::next(begin, i) };
    auto it_j{  std::next(begin, j) };
    auto it_k{  std::next(begin, k) };

    if(comp(*it_j, *it_i))
    {
        std::swap(it_i , it_j);
    }

    if(comp(*it_j, *it_k))
    {
        return it_j;
    }
    else if (comp(*it_i, *it_k))
    {
        return it_k;
    }

    return it_i;
}



/**
 * @brief Reorders elements in the range [begin, end) based on a unary predicate.
 *
 * This function partitions the range into two groups: elements for which the
 * predicate returns `true` are placed before the elements for which it returns `false`.
 * The relative order of elements within each group is not preserved (the algorithm is unstable).
 *
 * The implementation automatically selects the most efficient strategy based on the
 * iterator category:
 * - For bidirectional (or stronger) iterators, it uses the Hoare partition scheme.
 * - For forward iterators, it uses a Lomuto-like partition scheme.
 *
 * @tparam IT The forward iterator type. Must satisfy std::forward_iterator.
 * @tparam PREDICATE The type of the unary predicate. It must accept the value type
 *                    of the iterator and return a boolean-convertible value.
 *
 * @param begin Iterator to the beginning of the range to partition (inclusive).
 * @param end Iterator to the end of the range to partition (exclusive).
 * @param pred Unary predicate that returns `true` if the element should be placed
 *             in the first group (left side of the partition).
 *
 * @return IT An iterator pointing to the first element that does NOT satisfy the predicate.
 *            If all elements satisfy the predicate, returns `end`.
 *
 * @note Time complexity: O(N) comparisons and O(N) swaps (where N = distance(begin, end)).
 * @note Post-condition: All elements in the range [begin, return_value) satisfy `pred`.
 * @note Post-condition: All elements in the range [return_value, end) do NOT satisfy `pred`.
 * @note The predicate must not modify the elements it receives.
 */
template<std::forward_iterator IT, typename PREDICATE>
IT partition(IT begin, IT end, const PREDICATE& pred)
{
    if(begin == end)
    {
        return begin;
    }

    // Branch 1: Bidirectional iterators (std::vector, std::list, std::deque)
    // Uses the Hoare partition scheme (two pointers moving towards each other)
    if constexpr(std::bidirectional_iterator<IT>)
    {
        while(true)
        {
            while(begin != end && pred(*begin))
            {
                ++begin;
            }

            if(begin == end)
            {
                break;
            }

            --end;

            while(begin != end && !pred(*end))
            {
                --end;
            }

            if(begin == end)
            {
                break;
            }

            std::swap(*begin, *end);

            ++begin;
        }
    }
    // Branch 2: Forward-only iterators (std::forward_list)
    // Uses a Lomuto-like scheme (a boundary flag and a scanner)
    else
    {
        // `flag` points to the first element that does NOT satisfy the predicate.
        // Initialized to `end` to act as a sentinel (meaning "no such element found yet").
        IT flag{ end };

        while(begin != end)
        {
            if(pred(*begin))
            {
                if(flag != end)
                {
                    std::swap(*flag, *begin);
                    ++flag;
                }
                ++begin;
            }
            else
            {
                // If we found an element that does NOT satisfy the predicate,
                // and the flag hasn't been set yet, this is our partition point.
                if(flag == end)
                {
                    flag = begin;
                }
                ++begin;
            }

        }

        begin = flag;
    }

    return begin;
}

template<std::bidirectional_iterator IT, typename COMPARATOR = comparators::DefaultComparator<std::iter_value_t<IT>>>
std::pair<IT, IT> partition3(IT begin, IT end, const COMPARATOR& comp = COMPARATOR())
{
    auto value{ *pickPivot(begin, end, comp) };

    auto leftBound { std::prev(begin) };
    auto rightBound{ end };


    return { leftBound, rightBound };
}

}

#endif // QUICK_SORT_H
