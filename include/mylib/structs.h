#ifndef STRUCTS_H
#define STRUCTS_H

#include <algorithm>
#include <utility>


namespace mylib
{
    template <typename KEY, typename VALUE>
    struct KVPair
    {
        KEY key;
        VALUE value;

        KVPair(const KEY& theKey = KEY(), const VALUE& theValue = VALUE())
            : key{ theKey }
            , value{ theValue }
        {}
    };

    struct Empty {};

    namespace comparators
    {
    template<typename T>
    bool operator==(const T& left, const T& right)
    {
        return left <= right && left >= right;
    }

    template<typename T>
    bool operator!=(const T& left, const T& right)
    {
        return !(left == right);
    }

    template<typename T>
    struct DefaultComparator
    {
        bool operator()(const T& left, const T& right) const
        {
            return left < right;
        }

        bool isEqual(const T& left, const T& right) const
        {
            return left == right;
        }
    };

    template<typename T, typename COMPARATOR = DefaultComparator<T> >
    struct ReverseComparator
    {
        COMPARATOR comparator;
        explicit ReverseComparator(const COMPARATOR& theComparator = COMPARATOR())
            : comparator{ theComparator }
        {}

        bool operator()(const T& left, const T& right) const
        {
            return comparator(right, left);
        }

        bool isEqual(const T& left, const T& right) const
        {
            return comparator.isEqual(right, left);
        }
    };

    template<typename T, typename TRANSFORM, typename COMPARATOR>
    struct TransformComparator
    {
        TRANSFORM transform;
        COMPARATOR comparator;

        explicit TransformComparator(const TRANSFORM& theTransform = TRANSFORM(),
                                     const COMPARATOR& theComparator = COMPARATOR())
            : transform{ theTransform }
            , comparator{ theComparator }
        {}

        // требуется, если преобразование конструируется по умолчанию,
        // а компаратор нет.
        explicit TransformComparator(const COMPARATOR& theComparator)
            : comparator{ theComparator }
        {}

        bool operator()(const T& left, const T& right) const
        {
            return comparator(transform(left), transform(right));
        }

        bool isEqual(const T& left, const T& right) const
        {
            return comparator.isEqual(transform(left), transform(right));
        }
    };

    template<typename T>
    struct PointerTransform
    {
        const T& operator()(const T* item) const
        {
            assert(item); // здесь приложение упадет при ошибке. Насколько это оправдано, или лучше выбросить исключение?
            return *item;
        }
    };

    template<typename T, typename COMPARATOR = DefaultComparator<T> >
    using PointerComparator = TransformComparator<const T*, PointerTransform<T>, COMPARATOR>;

    template<typename T>
    struct IndexTransform
    {
        const T* array;
        explicit IndexTransform(const T* theArray)
            : array{ theArray }
        {}

        const T& operator()(int i) const
        {
            return array[i];
        }
    };

    template<typename T, typename COMPARATOR = DefaultComparator<T> >
    using IndexComparator = TransformComparator<int, IndexTransform<T>, COMPARATOR>;

    template<typename FIRST, typename SECOND>
    struct PairFirstTransform
    {
        const FIRST& operator()(const std::pair<FIRST, SECOND>& pair)
        {
            return pair.first;
        }
    };

    template<typename FIRST, typename SECOND, typename COMPARATOR = DefaultComparator<FIRST> >
    using PairFirstComparator = TransformComparator<std::pair<FIRST, SECOND>,
                                                    PairFirstTransform<FIRST, SECOND>,
                                                    COMPARATOR>;

    template<typename VECTOR>
    struct LexicographicComparator
    {
        bool operator()(const VECTOR& left, const VECTOR& right, int i) const
        {
            return i < left.size() ? i < right.size() && left[i] < right[i] :
                                        i < right.size();
        }

        bool operator()(const VECTOR& left, const VECTOR& right) const
        {
            for(int i{}; i < std::min(left.size(), right.size()); ++i)
            {
                if(left[i] < right[i])
                {
                    return true;
                }
                if(right[i] < left[i])
                {
                    return false;
                }
            }
            return left.size() < right.size();
        }

        bool isEqual(const VECTOR& left, const VECTOR& right, int i) const
        {
            return i < left.size() ? i < right.size() && left[i] == right[i] :
                                        i >= right.size();
        }

        bool isEqual(const VECTOR& left, const VECTOR& right) const
        {
            for(int i{}; i < std::min(left.size(), right.size()); ++i)
            {
                if(left[i] != right[i])
                {
                    return false;
                }
            }
            return left.size() == right.size();
        }


        int size(const VECTOR& vector) const
        {
            return vector.size();
        }
    };

    } // end namespace comparators
} // end namespace mylib


#endif // STRUCTS_H
