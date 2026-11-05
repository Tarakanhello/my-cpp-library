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
    else if (comp(*it_i), *(it_k))
    {
        return it_k;
    }

    return it_i;
}



template<std::bidirectional_iterator IT, typename COMPARATOR = comparators::DefaultComparator<std::iter_value_t<IT>>>
void partition(IT begin, IT end, IT& it_i, IT& it_j, const COMPARATOR& comp = COMPARATOR())
{
    auto value{ *pickPivot(begin, end, comp) };

    auto leftBound { std::prev(begin) };
    auto rightBound{ end };

    std::optional<IT> lastLeftEqual { std::nullopt };
    std::optional<IT> lastRightEqual{ std::nullopt };

    auto isEqual{ [&comp](const auto& a, const auto& b){
        return !comp(a, b) && !comp(b, a);
    } };

    while(true)
    {
        while(comp(*(++leftBound), value));
        while(comp(value, *(--rightBound)));

        if(leftBound == rightBound || leftBound == std::next(rightBound))
        {
            break;
        }

        std::swap(*leftBound, *rightBound);

        if(isEqual(*leftBound, value))
        {
            if(!lastLeftEqual.has_value())
            {
                lastLeftEqual.emplace(begin);
                std::swap(*(lastLeftEqual.value()), *leftBound);
            }
            else
            {
                lastLeftEqual = std::next(lastLeftEqual.value());
                std::swap(*(lastLeftEqual.value()), *leftBound);
            }
        }
        if(isEqual(*rightBound, value))
        {
            if(!lastRightEqual.has_value())
            {
                lastRightEqual.emplace(std::prev(end));
                std::swap(*(lastRightEqual.value()), *rightBound);
            }
            else
            {
                lastRightEqual = std::prev(lastRightEqual.value());
                std::swap(*(lastRightEqual.value()), *rightBound);
            }
        }
    }

    if(leftBound == rightBound)
    {
        ++leftBound;
        --rightBound;
    }

    if(lastLeftEqual.has_value())
    {
        for(auto it_k{ begin }; it_k != std::next(lastLeftEqual.value()); ++it_k )
        {
            std::swap(*it_k, *rightBound);
            --rightBound;
        }
    }

    if(lastRightEqual.has_value())
    {
        for(auto it_k{ std::prev(end) }; it_k != std::prev(lastRightEqual.value()); --it_k )
        {
            std::swap(*it_k, *leftBound);
            ++leftBound;
        }
    }

    it_i = leftBound;
    it_j = rightBound;
}

}

#endif // QUICK_SORT_H
