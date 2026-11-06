#ifndef INSERTIONSORT_H
#define INSERTIONSORT_H

#include <concepts>
#include <cstddef>
#include <iterator>
#include <utility>

#include "mylib/core/concepts.h"
#include "mylib/core/structs.h"

namespace mylib::algorithms
{


/**
 * @brief Sorts a range of elements in a raw array using the insertion sort algorithm.
 *
 * This overload works with raw pointers and explicit index boundaries.
 * The algorithm is stable and performs in O(N^2) in the worst case,
 * and O(N) in the best case (when the array is already sorted).
 *
 * @tparam T The type of elements in the array. Must satisfy LessAndEqualComparable.
 * @tparam COMPARATOR The type of the comparison function object.
 *
 * @param vector Pointer to the first element of the array.
 * @param left The starting index of the range to sort (inclusive).
 * @param right The ending index of the range to sort (inclusive).
 * @param comp The comparator used to determine order. Defaults to DefaultComparator.
 */
template<LessAndEqualComparable T, typename COMPARATOR = comparators::DefaultComparator<T>>
void insertSort(T* vector, size_t left, size_t right, const COMPARATOR& comp = COMPARATOR())
{
    if (left >= right)
    {
        return;
    }
    for(size_t i{ left + 1 }; i <= right; ++i)
    {
        if(comp(vector[i], vector[i - 1]))
        {
            auto movedValue{ std::move(vector[i]) };
            size_t j{ i };
            while(j > left && comp(movedValue, vector[j-1]))
            {
                vector[j] = std::move(vector[j-1]);
                --j;
            }
            vector[j] = std::move(movedValue);
        }
    }
}




/**
 * @brief Sorts a range of elements defined by iterators using the insertion sort algorithm.
 *
 * This overload works with any container providing bidirectional iterators (e.g., std::vector, std::list).
 * The algorithm is stable and performs in O(N^2) in the worst case,
 * and O(N) in the best case (when the range is already sorted).
 * It uses std::move to optimize performance for heavy types (like std::string).
 *
 * @tparam IT The bidirectional iterator type.
 * @tparam COMPARATOR The type of the comparison function object.
 *
 * @param begin Iterator to the beginning of the range to sort (inclusive).
 * @param end Iterator to the end of the range to sort (exclusive).
 * @param comp The comparator used to determine order. Defaults to DefaultComparator.
 */
template<std::bidirectional_iterator IT,
         typename COMPARATOR = comparators::DefaultComparator<std::iter_value_t<IT>>>
    requires std::permutable<IT>
void insertSort(IT begin, IT end, const COMPARATOR& comp = COMPARATOR())
{
    if(begin == end)
    {
        return;
    }


    for(auto it{ std::next(begin) }; it != end; ++it)
    {
        if(comp(*it, *(std::prev(it))))
        {
            auto movedValue{ std::move(*it) };
            auto it_j{ it };
            while(it_j != begin && comp(movedValue, *(std::prev(it_j))))
            {
                *it_j = std::move(*(std::prev(it_j)));
                --it_j;
            }
            *it_j = std::move(movedValue);
        }
    }
}


} // end namespace mylib::algorithms

#endif // INSERTIONSORT_H
