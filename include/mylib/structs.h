#ifndef STRUCTS_H
#define STRUCTS_H

#include <algorithm>
#include <cassert>
#include <utility>


namespace mylib
{
    /**
     * @brief Шаблонная структура, представляющая пару "ключ-значение".
     *
     * @tparam KEY   Тип ключа.
     * @tparam VALUE Тип значения.
     */
    template <typename KEY, typename VALUE>
    struct KVPair
    {
        KEY key;
        VALUE value;


        /**
         * @brief Конструктор с параметрами по умолчанию.
         * @param theKey  Начальный ключ (по умолчанию создаётся значение по умолчанию для KEY).
         * @param theValue Начальное значение (по умолчанию создаётся значение по умолчанию для VALUE).
         */
        KVPair(const KEY& theKey = KEY(), const VALUE& theValue = VALUE())
            : key{ theKey }
            , value{ theValue }
        {}
    };

    /**
     * @brief Пустая структура-метка, может использоваться как фиктивный тип.
     */
    struct Empty {};

    /**
     * @brief Пространство имён для компараторов, трансформаций и вспомогательных функций сравнения.
     */
    namespace comparators
    {

    /**
     * @brief Компаратор по умолчанию, использующий операторы < и ==.
     * @tparam T Тип сравниваемых объектов.
     */
    template<typename T>
    struct DefaultComparator
    {
        /**
         * @brief Оператор "меньше".
         * @param left  Левый операнд.
         * @param right Правый операнд.
         * @return true если left < right, иначе false.
         */
        bool operator()(const T& left, const T& right) const
        {
            return left < right;
        }

        /**
         * @brief Проверка на равенство.
         * @param left  Левый операнд.
         * @param right Правый операнд.
         * @return true если left == right, иначе false.
         */
        bool isEqual(const T& left, const T& right) const
        {
            return left == right;
        }
    };

    /**
     * @brief Компаратор, обращающий порядок сравнения (по убыванию).
     * @tparam T         Тип сравниваемых объектов.
     * @tparam COMPARATOR Базовый компаратор (по умолчанию DefaultComparator<T>).
     */
    template<typename T, typename COMPARATOR = DefaultComparator<T> >
    struct ReverseComparator
    {
        COMPARATOR comparator;

        /**
         * @brief Конструктор.
         * @param theComparator Базовый компаратор (Компаратор по умолчанию, использующий операторы < и ==).
         */
        explicit ReverseComparator(const COMPARATOR& theComparator = COMPARATOR())
            : comparator{ theComparator }
        {}

        /**
         * @brief Оператор "меньше" с обратным порядком.
         * @return true если базовый компаратор считает, что right < left.
         */
        bool operator()(const T& left, const T& right) const
        {
            return comparator(right, left);
        }

        /**
         * @brief Равенство – делегируется базовому компаратору.
         */
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

        const T& operator()(std::size_t i) const
        {
            return array[i];
        }
    };

    template<typename T, typename COMPARATOR = DefaultComparator<T> >
    using IndexComparator = TransformComparator<std::size_t, IndexTransform<T>, COMPARATOR>;

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
        bool operator()(const VECTOR& left, const VECTOR& right, std::size_t i) const
        {
            return i < left.size() ? i < right.size() && left[i] < right[i] :
                                        i < right.size();
        }

        bool operator()(const VECTOR& left, const VECTOR& right) const
        {
            for(std::size_t i{}; i < std::min(left.size(), right.size()); ++i)
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

        bool isEqual(const VECTOR& left, const VECTOR& right, std::size_t i) const
        {
            return i < left.size() ? i < right.size() && left[i] == right[i] :
                                        i >= right.size();
        }

        bool isEqual(const VECTOR& left, const VECTOR& right) const
        {
            for(std::size_t i{}; i < std::min(left.size(), right.size()); ++i)
            {
                if(left[i] != right[i])
                {
                    return false;
                }
            }
            return left.size() == right.size();
        }


        std::size_t getSize(const VECTOR& vector) const
        {
            return vector.size();
        }
    };

    template<typename T, typename COMPARATOR = DefaultComparator<T>>
    std::size_t argMin(const T* array, std::size_t size, const COMPARATOR& comparator = COMPARATOR())
    {
        assert(size > 0);
        std::size_t best{};

        for(std::size_t i{ 1 }; i < size; ++i)
        {
            if(comparator(array[i], array[best]))
            {
                best = i;
            }
        }

        return best;
    }

    template<typename T, typename COMPARATOR = ReverseComparator<T>>
    std::size_t argMax(const T* array, std::size_t size, const COMPARATOR& comparator = COMPARATOR())
    {
        return argMin(array, size, comparator);
    }

    template<typename T>
    T& valMin(T* array, std::size_t size)
    {
        std::size_t index{ argMin(array, size) };

        return array[index];
    }

    template<typename T>
    const T& valMin(const T* array, std::size_t size)
    {
        std::size_t index{ argMin(array, size) };

        return array[index];
    }

    template<typename T>
    T& valMax(T* array, std::size_t size)
    {
        std::size_t index{ argMax(array, size) };

        return array[index];
    }

    template<typename T>
    const T& valMax(const T* array, std::size_t size)
    {
        std::size_t index{ argMax(array, size) };

        return array[index];
    }

    // индекс минимума массива с функцией преобразования
    template<typename T, typename FUNCTION>
    std::size_t argMinFunc(const T* array, std::size_t size, const FUNCTION& function)
    {
        using ResultType = decltype(function(array[0]));
        auto comp{ TransformComparator<T, FUNCTION, DefaultComparator<ResultType>>(function) };

        return argMin(array, size, comp);
    }

    template<typename T, typename FUNCTION>
    T& valMinFunc(T* array, std::size_t size, const FUNCTION& function)
    {
        std::size_t index{ argMinFunc(array, size, function) };

        return array[index];
    }

    } // end namespace comparators

    template<typename T>
    struct ArithmeticType
    {
        friend T operator+(const T& a, const T& b)
        {
            T result{ a };
            return result += b;
        }

        friend T operator-(const T& a, const T& b)
        {
            T result{ a };
            return result -= b;
        }

        friend T operator*(const T& a, const T& b)
        {
            T result{ a };
            return result *= b;
        }

        friend T operator<<(const T& a, int shift)
        {
            T result{ a };
            return result <<= shift;
        }

        friend T operator>>(const T& a, int shift)
        {
            T result{ a };
            return result >>= shift;
        }

        friend T operator%(const T& a, const T& b)
        {
            T result{ a };
            return result %= b;
        }

        friend T operator/(const T& a, const T& b)
        {
            T result{ a };
            return result /= b;
        }
    };

} // end namespace mylib


#endif // STRUCTS_H
