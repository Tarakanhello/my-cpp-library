#ifndef STRUCTS_H
#define STRUCTS_H

#include <algorithm>
#include <cstddef>
#include <cassert>
#include <compare>
#include <utility>

#include "mylib/memory.h"

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
        constexpr KVPair(const KEY& theKey = KEY(), const VALUE& theValue = VALUE())
            : key{ theKey }
            , value{ theValue }
        {}
    };

    /**
     * @brief Пустая структура-метка, может использоваться как фиктивный тип.
     */
    struct Empty {};


    template<typename T>
    struct BufferGuard
    {
        T* ptr;
        size_t count; // количество объектов в буфере
        bool committed;

        BufferGuard(T* thePtr, size_t theCount = 0) noexcept
            : ptr{ thePtr }
            , count{ theCount }
            , committed{ false }
        {}

        ~BufferGuard() noexcept
        {
            if(!committed && ptr)
            {
                memory::rawDestruct(ptr, count);
            }
        }

        void addConstructed(size_t n = 1) noexcept
        {
            count += n;
        }

        void commit() noexcept
        {
            committed = true;
            ptr = nullptr;
        }
    };


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
            constexpr bool operator()(const T& left, const T& right) const
            {
                return left < right;
            }

            /**
             * @brief Проверка на равенство.
             * @param left  Левый операнд.
             * @param right Правый операнд.
             * @return true если left == right, иначе false.
             */
            constexpr bool isEqual(const T& left, const T& right) const
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
            explicit constexpr ReverseComparator(const COMPARATOR& theComparator = COMPARATOR())
                : comparator{ theComparator }
            {}

            /**
             * @brief Оператор "меньше" с обратным порядком.
             * @return true если базовый компаратор считает, что right < left.
             */
            constexpr bool operator()(const T& left, const T& right) const
            {
                return comparator(right, left);
            }

            /**
             * @brief Равенство – делегируется базовому компаратору.
             */
            constexpr bool isEqual(const T& left, const T& right) const
            {
                return comparator.isEqual(right, left);
            }
        };

        /**
         * @brief Компаратор, преобразующий объекты перед сравнением.
         * @tparam T          Исходный тип.
         * @tparam TRANSFORM  Функтор преобразования (принимает T, возвращает некоторый тип U).
         * @tparam COMPARATOR Компаратор для преобразованных значений (по умолчанию DefaultComparator<U>).
         */
        template<typename T, typename TRANSFORM, typename COMPARATOR>
        struct TransformComparator
        {
            TRANSFORM transform;
            COMPARATOR comparator;

            /**
             * @brief Конструктор.
             * @param theTransform   Функтор преобразования (по умолчанию создаётся новый).
             * @param theComparator  Компаратор для преобразованных значений (по умолчанию новый).
             */
            explicit constexpr TransformComparator(const TRANSFORM& theTransform = TRANSFORM(),
                                         const COMPARATOR& theComparator = COMPARATOR())
                : transform{ theTransform }
                , comparator{ theComparator }
            {}

             /**
             * @brief Сравнение после применения transform.
             */
            constexpr bool operator()(const T& left, const T& right) const
            {
                return comparator(transform(left), transform(right));
            }

            /**
             * @brief Проверка равенства после применения transform.
             */
            constexpr bool isEqual(const T& left, const T& right) const
            {
                return comparator.isEqual(transform(left), transform(right));
            }
        };

        /**
         * @brief Функтор преобразования указателя в ссылку.
         * @tparam T Тип хранимого значения.
         *
         * @warning Оператор вызова содержит assert(ptr). Если передан нулевой указатель,
         *          программа аварийно завершится.
         */
        template<typename T>
        struct PointerTransform
        {
            const T& operator()(const T* item) const
            {
                assert(item);
                return *item;
            }
        };

        /**
         * @brief Удобный псевдоним для сравнения указателей через PointerTransform.
         * @tparam T Тип значения, на которое указывает указатель.
         * @tparam COMPARATOR Компаратор для значений T (по умолчанию DefaultComparator<T>).
         */
        template<typename T, typename COMPARATOR = DefaultComparator<T> >
        using PointerComparator = TransformComparator<const T*, PointerTransform<T>, COMPARATOR>;

        /**
         * @brief Функтор, преобразующий индекс в элемент массива.
         * @tparam T Тип элементов массива.
         */
        template<typename T>
        struct IndexTransform
        {
            const T* array;

            /**
             * @brief Конструктор, запоминающий массив.
             * @param theArray Указатель на массив (должен существовать дольше, чем объект трансформации).
             */
            explicit constexpr IndexTransform(const T* theArray)
                : array{ theArray }
            {}

            /**
             * @brief Доступ к элементу по индексу.
             * @param i Индекс.
             * @return Ссылка на i-й элемент.
             */
            constexpr const T& operator()(std::size_t i) const
            {
                return array[i];
            }
        };

        /**
         * @brief Компаратор для сравнения элементов массива по индексам.
         * @tparam T Тип элементов массива.
         * @tparam COMPARATOR Компаратор для значений T (по умолчанию DefaultComparator<T>).
         */
        template<typename T, typename COMPARATOR = DefaultComparator<T> >
        struct IndexComparator final : public TransformComparator<std::size_t, IndexTransform<T>, COMPARATOR>
        {
            explicit IndexComparator(const T* array, const COMPARATOR& comp = COMPARATOR())
                : TransformComparator<std::size_t, IndexTransform<T>, COMPARATOR>{ IndexTransform<T>{ array }, comp }
            {}
        };

        /**
         * @brief Функтор, извлекающий первый элемент из std::pair.
         * @tparam FIRST  Тип первого элемента пары.
         * @tparam SECOND Тип второго элемента пары.
         */
        template<typename FIRST, typename SECOND>
        struct PairFirstTransform
        {
            /**
             * @brief Возвращает pair.first.
             * @return Константная ссылка на поле first.
             */
            constexpr const FIRST& operator()(const std::pair<FIRST, SECOND>& pair) const
            {
                return pair.first;
            }
        };

        /**
         * @brief Компаратор, сравнивающий std::pair по первому элементу.
         * @tparam FIRST     Тип первого элемента.
         * @tparam SECOND    Тип второго элемента (не участвует в сравнении).
         * @tparam COMPARATOR Компаратор для FIRST.
         */
        template<typename FIRST, typename SECOND, typename COMPARATOR = DefaultComparator<FIRST> >
        using PairFirstComparator = TransformComparator<std::pair<FIRST, SECOND>,
                                                        PairFirstTransform<FIRST, SECOND>,
                                                        COMPARATOR>;

        /**
         * @brief Лексикографический компаратор для контейнеров, поддерживающих operator[] и size().
         * @tparam VECTOR Тип контейнера (например, std::vector, std::string).
         *
         * Предоставляет сравнение двух контейнеров как последовательностей,
         * а также вспомогательные методы для поэлементного сравнения.
         */
        template<typename VECTOR>
        struct LexicographicComparator
        {
            /**
             * @brief Сравнение элементов на позиции i (без проверки границ).
             * @return true если левый элемент меньше правого на позиции i,
             *         иначе false (если индексы вне диапазона – см. логику).
             */
            constexpr bool operator()(const VECTOR& left, const VECTOR& right, std::size_t i) const
            {
                return i < left.size() ? i < right.size() && left[i] < right[i] :
                                            i < right.size();
            }

            /**
             * @brief Лексикографическое сравнение двух контейнеров.
             * @return true если left < right в лексикографическом порядке.
             */
            constexpr bool operator()(const VECTOR& left, const VECTOR& right) const
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

            /**
             * @brief Проверка равенства элементов на позиции i.
             */
            constexpr bool isEqual(const VECTOR& left, const VECTOR& right, std::size_t i) const
            {
                return i < left.size() ? i < right.size() && left[i] == right[i] :
                                            i >= right.size();
            }

            /**
             * @brief Проверка полного равенства двух контейнеров.
             * @return true если left и right имеют одинаковый размер и все элементы равны.
             */
            constexpr bool isEqual(const VECTOR& left, const VECTOR& right) const
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


            /**
             * @brief Возвращает размер контейнера (нужен для некоторых алгоритмов).
             */
            constexpr std::size_t getSize(const VECTOR& vector) const
            {
                return vector.size();
            }


            /**
             * @brief Трёхстороннее сравнение двух контейнеров.
             * @return std::strong_ordering::less, equal или greater.
             */
            constexpr std::strong_ordering compare(const VECTOR& left, const VECTOR& right) const
            {
                size_t minSize{ std::min(left.size(), right.size()) };

                for(size_t i{}; i < minSize; ++i)
                {
                    if(left[i] < right[i])
                    {
                        return std::strong_ordering::less;
                    }
                    if(left[i] > right[i])
                    {
                        return std::strong_ordering::greater;
                    }
                }

                if(left.size() < right.size())
                {
                    return std::strong_ordering::less;
                }
                if(right.size() < left.size())
                {
                    return std::strong_ordering::greater;
                }

                return std::strong_ordering::equal;
            }
        };

        /**
         * @brief Находит индекс минимального элемента в массиве.
         * @tparam T Тип элементов.
         * @tparam COMPARATOR Тип компаратора (по умолчанию DefaultComparator<T>).
         * @param array Указатель на массив.
         * @param size  Количество элементов.
         * @param comparator Компаратор, определяющий отношение "меньше".
         * @pre size > 0, array != nullptr.
         * @return Индекс первого вхождения минимального элемента.
         */
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

        /**
         * @brief Находит индекс максимального элемента в массиве (через обратный компаратор).
         * @tparam T Тип элементов.
         * @tparam COMPARATOR Компаратор (по умолчанию ReverseComparator<T>).
         * @param array Указатель на массив.
         * @param size  Количество элементов.
         * @param comparator Компаратор (обычно игнорируется, т.к. используется ReverseComparator).
         * @return Индекс максимального элемента.
         */
        template<typename T, typename COMPARATOR = ReverseComparator<T>>
        std::size_t argMax(const T* array, std::size_t size, const COMPARATOR& comparator = COMPARATOR())
        {
            return argMin(array, size, comparator);
        }

        /**
         * @brief Возвращает ссылку на минимальный элемент массива (неконстантная версия).
         * @tparam T Тип элементов.
         * @param array Указатель на массив.
         * @param size  Количество элементов.
         * @return Ссылка на минимальный элемент.
         */
        template<typename T>
        T& valMin(T* array, std::size_t size)
        {
            std::size_t index{ argMin(array, size) };

            return array[index];
        }

        /// @copydoc valMin (константная версия)
        template<typename T>
        const T& valMin(const T* array, std::size_t size)
        {
            std::size_t index{ argMin(array, size) };

            return array[index];
        }

        /**
         * @brief Возвращает ссылку на максимальный элемент массива (неконстантная версия).
         */
        template<typename T>
        T& valMax(T* array, std::size_t size)
        {
            std::size_t index{ argMax(array, size) };

            return array[index];
        }

        /// @copydoc valMax (константная версия)
        template<typename T>
        const T& valMax(const T* array, std::size_t size)
        {
            std::size_t index{ argMax(array, size) };

            return array[index];
        }


        /**
         * @brief Находит индекс минимального элемента, используя функцию преобразования.
         * @tparam T Тип элементов.
         * @tparam FUNCTION Тип функтора, принимающего T и возвращающего некоторый тип для сравнения.
         * @param array Указатель на массив.
         * @param size  Количество элементов.
         * @param function Функтор преобразования (например, доступ к полю).
         * @return Индекс элемента, для которого значение function(element) минимально.
         */
        template<typename T, typename FUNCTION>
        std::size_t argMinFunc(const T* array, std::size_t size, const FUNCTION& function)
        {
            using ResultType = decltype(function(array[0]));
            auto comp{ TransformComparator<T, FUNCTION, DefaultComparator<ResultType>>(function) };

            return argMin(array, size, comp);
        }

        /**
         * @brief Возвращает ссылку на элемент, с минимальным значением.
         */
        template<typename T, typename FUNCTION>
        T& valMinFunc(T* array, std::size_t size, const FUNCTION& function)
        {
            std::size_t index{ argMinFunc(array, size, function) };

            return array[index];
        }

    } // end namespace comparators

    /**
     * @brief Класс-примесь (mixin), добавляющий арифметические операторы.
     * @tparam T Наследуемый тип (CRTP).
     *
     * Чтобы добавить своему классу операторы +, -, *, /, %, <<, >>,
     * достаточно унаследовать от ArithmeticType<YourClass> и реализовать
     * составные операторы +=, -=, *=, /=, %=, <<=, >>=.
     */
    template<typename T>
    struct ArithmeticType
    {
        friend constexpr T operator+(const T& a, const T& b)
        {
            T result{ a };
            return result += b;
        }

        friend constexpr T operator-(const T& a, const T& b)
        {
            T result{ a };
            return result -= b;
        }

        friend constexpr T operator*(const T& a, const T& b)
        {
            T result{ a };
            return result *= b;
        }

        friend constexpr T operator<<(const T& a, int shift)
        {
            T result{ a };
            return result <<= shift;
        }

        friend constexpr T operator>>(const T& a, int shift)
        {
            T result{ a };
            return result >>= shift;
        }

        friend constexpr T operator%(const T& a, const T& b)
        {
            T result{ a };
            return result %= b;
        }

        friend constexpr T operator/(const T& a, const T& b)
        {
            T result{ a };
            return result /= b;
        }
    };

} // end namespace mylib


#endif // STRUCTS_H
