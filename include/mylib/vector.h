#ifndef VECTOR_H
#define VECTOR_H

#include <algorithm>
#include <cassert>
#include <cmath>
#include <format>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <type_traits>
#include <utility>

#include "mylib/memory.h"
#include "mylib/structs.h"


namespace mylib
{
    /**
     * @brief Динамический массив, аналогичный std::vector.
     * @tparam T Тип хранимых элементов.
     *
     * Обеспечивает автоматическое управление памятью, поддержку итераторов
     * и строгую гарантию безопасности исключений для основных операций.
     *
     * @note Наследует ArithmeticType для возможности использования арифметических операций,
     *       но они не реализованы – наследование оставлено для расширяемости.
     */
    template<typename T, typename ALLOCATOR = mylib::MySimpleAllocator<T>>
    class Vector final : public mylib::ArithmeticType<Vector<T, ALLOCATOR>>
    {
    private:
        /** @brief Минимальная ёмкость при первом выделении памяти. */
        enum{ MinCapacity = 8 };

        ALLOCATOR m_allocator{};
        size_t m_capacity{};    ///< Текущая выделенная ёмкость (количество элементов).
        size_t m_size{};        ///< Текущий размер (количество элементов).
        T* m_data{ nullptr };   ///< Указатель на массив элементов.

        /**
         * @brief Вычисляет новую ёмкость, достаточную для хранения requiredSize элементов.
         * @param requiredSize Необходимый размер.
         * @return Новая ёмкость (не менее MinCapacity и не менее requiredSize, степень двойки).
         */
        constexpr size_t calculateNewCapacity(size_t requiredSize) const;

        /**
         * @brief Конструирует элементы в диапазоне [from, to) со значением value.
         * @param from Начальный индекс.
         * @param to   Конечный индекс (не включается).
         * @param value Значение для копирования.
         * @throw Любое исключение, брошенное конструктором T. При ошибке уже созданные элементы уничтожаются.
         */
        constexpr void constructElements(size_t from, size_t to, const T& value = T());

        /**
         * @brief Конструирует count элементов из диапазона, начиная с src, в позицию from.
         * @param from Индекс в массиве m_data, куда копировать.
         * @param src  Указатель на исходный массив.
         * @param count Количество элементов.
         * @throw Любое исключение, брошенное конструктором T. При ошибке созданные элементы уничтожаются.
         */
        constexpr void constructElementsFromRange(size_t from, const T* src, size_t count);

        /**
         * @brief Освобождает память и уничтожает все элементы.
         * @note noexcept – гарантирует, что не бросит исключение.
         */
        void deallocate() noexcept;

        /**
         * @brief Уничтожает элементы в диапазоне [from, to).
         * @param from Начальный индекс.
         * @param to   Конечный индекс (не включается).
         * @note noexcept – вызов деструкторов не должен бросать исключения.
         */
        constexpr void destroyElements(size_t from, size_t to) noexcept;

        static constexpr size_t maxSize() noexcept
        {
            return std::numeric_limits<size_t>::max() / sizeof(T);
        }

        /**
         * @brief Перевыделяет буфер без создания новых элементов (newSize == m_size).
         * @param newCapacity Новая ёмкость (должна быть >= newSize).
         * @param newSize     Новый размер (должен совпадать с текущим m_size).
         * @throws std::bad_alloc при нехватке памяти.
         * @note Обеспечивает строгую гарантию безопасности исключений.
         */
        void reallocateBuffer(size_t newCapacity, size_t newSize);

        /**
         * @brief Перевыделяет буфер с новой ёмкостью и размером.
         * @tparam ARGS Типы аргументов для создания новых элементов.
         * @param newCapacity Новая ёмкость (должна быть >= newSize).
         * @param newSize     Новый размер.
         * @param args        Аргументы для конструирования первого нового элемента (если newSize > oldSize).
         *
         * Обеспечивает строгую гарантию безопасности исключений:
         * - старые данные не повреждаются при сбое,
         * - при успехе старые данные освобождаются.
         */
        template<typename... ARGS>
        void reallocateBuffer(size_t newCapacity, size_t newSize, ARGS&&... args);

        /**
         * @brief Освобождает указатель и обнуляет размер/ёмкость (без уничтожения элементов).
         * @note Используется в move-операциях для "обнуления" перемещаемого объекта.
         */
        constexpr void release() noexcept;

        /**
         * @brief Меняет содержимое с другим вектором за O(1).
         * @param other Другой вектор.
         */
        constexpr void swap(Vector& other) noexcept;

    public:
        /**
         * @brief Конструктор по умолчанию. Создаёт пустой вектор.
         */
        constexpr Vector() noexcept;

        /**
         * @brief Конструктор, создающий вектор с size элементами, инициализированными значением value.
         * @param size Количество элементов.
         * @param value Значение для инициализации (по умолчанию T()).
         */
        Vector(size_t size, const T& value = T(), const ALLOCATOR& alloc = ALLOCATOR());

        /**
         * @brief Конструктор из std::initializer_list.
         * @param list Список инициализации.
         */
        Vector(const std::initializer_list<T>& list, const ALLOCATOR& alloc = ALLOCATOR());

        /**
         * @brief Конструктор копирования.
         * @param other Вектор для копирования.
         */
        Vector(const Vector<T, ALLOCATOR>& other);

        /**
         * @brief Оператор присваивания копированием (через copy-and-swap).
         * @param other Вектор для копирования.
         * @return Ссылка на *this.
         */
        Vector<T, ALLOCATOR>& operator=(const Vector<T, ALLOCATOR>& other);

        /**
         * @brief Конструктор перемещения.
         * @param other Вектор, из которого перемещаются данные.
         */
        Vector(Vector<T, ALLOCATOR>&& other) noexcept;

        /**
         * @brief Оператор присваивания перемещением.
         * @param other Вектор, из которого перемещаются данные.
         * @return Ссылка на *this.
         */
        Vector<T, ALLOCATOR>& operator=(Vector<T, ALLOCATOR>&& other) noexcept;

        /**
         * @brief Деструктор. Уничтожает все элементы и освобождает память.
         */
        ~Vector() noexcept;

        /**
         * @brief Добавляет элемент в конец (перегрузка для lvalue).
         * @param item Элемент для добавления.
         */
        void append(const T& item);

        void appendVector(const Vector& vec);

        /**
         * @brief Доступ к элементу с проверкой границ.
         * @param i Индекс элемента.
         * @return Ссылка на элемент.
         * @throw std::out_of_range если i >= size().
         */
        constexpr T& at(size_t i);
        constexpr const T& at(size_t i) const;

        /**
         * @brief Возвращает текущую ёмкость.
         * @return Количество элементов, которое может вместить вектор без перераспределения.
         */
        constexpr size_t capacity() const noexcept { return m_capacity; }

        void clear() noexcept { deallocate(); }

        /**
         * @brief Возвращает ссылку на последний элемент.
         * @return Ссылка на последний элемент.
         * @pre Вектор не должен быть пустым (проверяется через assert).
         */
        constexpr T& back() noexcept
        {
            assert(!empty());
            return m_data[m_size - 1];
        }
        constexpr const T& back() const noexcept
        {
            assert(!empty());
            return m_data[m_size - 1];
        }

        /**
         * @brief Возвращает указатель на внутренний массив.
         * @return Указатель на данные (может быть nullptr для пустого вектора).
         */
        constexpr T* data() noexcept;
        constexpr const T* data() const noexcept;

        /**
         * @brief Конструирует элемент в конце вектора из переданных аргументов.
         * @tparam ARGS Типы аргументов конструктора T.
         * @param args Аргументы для конструктора T.
         */
        template<typename... ARGS>
        void emplace_back(ARGS&&... args);

        /**
         * @brief Проверяет, пуст ли вектор.
         * @return true, если size() == 0.
         */
        constexpr bool empty() const noexcept;

        /**
         * @brief Доступ к элементу без проверки границ.
         * @param i Индекс элемента.
         * @return Ссылка на элемент.
         * @pre i < size() (проверяется через assert в отладочной сборке).
         */
        constexpr T& operator[](size_t i) noexcept;
        constexpr const T& operator[](size_t i) const noexcept;

        /**
         * @brief Проверяет равенство двух векторов.
         * @param other Вектор для сравнения.
         * @return true, если size() равны и все элементы равны.
         * @note Использует LexicographicComparator::isEqual.
         */
        constexpr bool operator==(const Vector<T, ALLOCATOR>& other) const;

        /**
         * @brief Трёхстороннее сравнение (лексикографическое).
         * @param other Вектор для сравнения.
         * @return std::strong_ordering::less, equal или greater.
         * @note Использует LexicographicComparator::compare, что даёт один проход.
         */
        constexpr auto operator<=>(const Vector<T, ALLOCATOR>& other) const;

        /**
         * @brief Поэлементно прибавляет другой вектор к текущему.
         * @param other Вектор, который нужно прибавить.
         * @return Ссылка на *this.
         * @throw std::invalid_argument если размеры векторов не совпадают.
         */
        constexpr Vector& operator+=(const Vector<T, ALLOCATOR>& other);

        /**
         * @brief Поэлементно вычитает другой вектор из текущего.
         * @param other Вектор, который нужно вычесть.
         * @return Ссылка на *this.
         * @throw std::invalid_argument если размеры векторов не совпадают.
         */
        constexpr Vector& operator-=(const Vector<T, ALLOCATOR>& other);

        /**
         * @brief Поэлементно умножает текущий вектор на скаляр.
         * @param scalar Скалярный множитель.
         * @return Ссылка на *this.
         */
        constexpr Vector& operator*=(const T& scalar);

        /**
         * @brief Унарный минус – возвращает вектор с отрицательными значениями каждого элемента.
         * @return Новый вектор, содержащий -element для каждого элемента исходного.
         */
        constexpr Vector operator-() const;

        explicit constexpr operator bool() const noexcept { return !empty(); }

        /**
         * @brief Вычисляет евклидову норму вектора.
         * @return Значение нормы (квадратный корень из скалярного произведения вектора на себя).
         * @note Доступно только для векторов типа double (static_assert).
         */
        double norm() const;

        T pop_back() noexcept;

        /**
         * @brief Вставляет элемент в конец (перегрузка для rvalue).
         * @param element Элемент для добавления.
         */
        void push_back(const T& element);

        /**
         * @brief Вставляет элемент в конец (перегрузка для rvalue).
         * @param element Элемент для добавления (перемещается).
         */
        void push_back(T&& element);

        /**
         * @brief Резервирует память для как минимум newCap элементов.
         * @param newCap Желаемая ёмкость.
         * @note Если newCap <= текущей ёмкости, ничего не делает.
         * @throw std::bad_alloc при нехватке памяти.
         */
        void reserve(size_t newCap);

        /**
         * @brief Изменяет размер вектора.
         * @param newSize Новый размер.
         * @param value   Значение для инициализации новых элементов (если newSize > текущего).
         * @note Если newSize меньше текущего, лишние элементы уничтожаются.
         *       При уменьшении ёмкость может быть уменьшена, если newSize * 4 < capacity.
         * @throw Любое исключение при конструировании элементов.
         */
        void resize(size_t newSize, const T& value = T());

        void reverse();
        void reverse(size_t start, size_t end);

        /**
         * @brief Уменьшает ёмкость до размера (shrink-to-fit).
         * @throw std::bad_alloc при нехватке памяти (если требуется перераспределение).
         */
        void shrink_to_fit();

        /**
         * @brief Возвращает текущий размер.
         * @return Количество элементов.
         */
        constexpr size_t size() const noexcept;

        // ИТЕРАТОРЫ
        class Iterator;
        class ConstIterator;
        // Обратные итераторы – используем std::reverse_iterator
        using reverse_iterator       = std::reverse_iterator<Iterator>;
        using const_reverse_iterator = std::reverse_iterator<ConstIterator>;

        /**
         * @brief Возвращает итератор на первый элемент.
         */
        constexpr Iterator begin() noexcept;

        /**
         * @brief Возвращает константный итератор на первый элемент.
         */
        constexpr ConstIterator begin() const noexcept;

        /**
         * @brief Возвращает константный итератор на первый элемент.
         */
        constexpr ConstIterator cbegin() const noexcept;

        /**
         * @brief Возвращает итератор на элемент, следующий за последним.
         */
        constexpr Iterator end() noexcept;

        /**
         * @brief Возвращает константный итератор на элемент, следующий за последним.
         */
        constexpr ConstIterator end() const noexcept;

        /**
         * @brief Возвращает константный итератор на элемент, следующий за последним.
         */
        constexpr ConstIterator cend() const noexcept;

        /**
         * @brief Возвращает обратный итератор на элемент, следующий за последним.
         */
        constexpr reverse_iterator rbegin() noexcept;

        /**
         * @brief Возвращает константный обратный итератор на элемент, следующий за последним.
         */
        constexpr const_reverse_iterator rbegin() const noexcept;

        /**
         * @brief Возвращает константный обратный итератор на элемент, следующий за последним.
         */
        constexpr const_reverse_iterator crbegin() const noexcept;

        /**
         * @brief Возвращает обратный итератор на первый элемент.
         */
        constexpr reverse_iterator rend() noexcept;

        /**
         * @brief Возвращает константный обратный итератор на первый элемент.
         */
        constexpr const_reverse_iterator rend() const noexcept;

        /**
         * @brief Возвращает константный обратный итератор на первый элемент.
         */
        constexpr const_reverse_iterator crend() const noexcept;
    };



    /**
     * @brief Умножение вектора на скаляр (вектор * скаляр).
     * @param vec Вектор.
     * @param scalar Скалярный множитель.
     * @return Новый вектор, каждый элемент которого равен vec[i] * scalar.
     */
    template<typename T, typename ALLOCATOR>
    constexpr Vector<T, ALLOCATOR> operator*(const  Vector<T, ALLOCATOR>& vec, const T& scalar)
    {
        Vector result{ vec };

        return result *= scalar;
    }


    /**
     * @brief Умножение скаляра на вектор (скаляр * вектор).
     * @param scalar Скалярный множитель.
     * @param vec Вектор.
     * @return Новый вектор, каждый элемент которого равен scalar * vec[i].
     */
    template<typename T, typename ALLOCATOR>
    constexpr Vector<T, ALLOCATOR> operator*(const T& scalar, const  Vector<T, ALLOCATOR>& vec)
    {
        return vec * scalar;
    }



    /**
     * @brief Вычисляет скалярное произведение (dot product) двух векторов.
     * @param a Первый вектор.
     * @param b Второй вектор.
     * @return Скалярное произведение (сумма произведений соответствующих элементов).
     * @throw std::invalid_argument если размеры векторов не совпадают.
     */
    template<typename T, typename ALLOCATOR>
    constexpr T dot(const Vector<T, ALLOCATOR>& a, const Vector<T, ALLOCATOR>& b)
    {
        if(a.size() != b.size())
        {
            throw std::invalid_argument("size mismatch");
        }

        T result{};

        for(size_t i{}; i < a.size(); ++i)
        {
            result += a[i] * b[i];
        }

        return result;
    }

} // end namespace



template<typename T, typename ALLOCATOR>
constexpr mylib::Vector<T, ALLOCATOR>::Vector() noexcept
    : m_capacity{ 0 }
    , m_size{ 0 }
    , m_data{ nullptr }
    , m_allocator{}
{}



template<typename T, typename ALLOCATOR>
mylib::Vector<T, ALLOCATOR>::Vector(size_t size, const T& value, const ALLOCATOR& alloc)
    : m_capacity{ 0 }
    , m_size{ 0 }
    , m_data{ nullptr }
    , m_allocator{ alloc }
{
    if(size == 0)
        return;

    size_t newCap{ calculateNewCapacity(size) };
    reallocateBuffer(newCap, size, value);
}



template<typename T, typename ALLOCATOR>
mylib::Vector<T, ALLOCATOR>::Vector(const std::initializer_list<T>& list, const ALLOCATOR& alloc)
    : m_capacity{ 0 }
    , m_size{ 0 }
    , m_data{ nullptr }
    , m_allocator{ alloc }
{
    if(list.size() == 0)
    {
        return;
    }

    m_capacity = calculateNewCapacity(list.size());
    m_data = m_allocator.allocate(m_capacity);
    try
    {
        constructElementsFromRange(0, list.begin(), list.size());
        m_size = list.size();
    }
    catch (...)
    {
        m_allocator.deallocate(m_data, m_capacity);
        m_data = nullptr;
        m_capacity = 0;
        throw;
    }
}



template<typename T, typename ALLOCATOR>
mylib::Vector<T, ALLOCATOR>::Vector(const Vector<T, ALLOCATOR>& other)
    : m_capacity{ other.m_capacity }
    , m_size{ other.m_size }
    , m_allocator{ other.m_allocator }
{
    if(m_capacity == 0)
    {
        return;
    }

    m_data = m_allocator.allocate(m_capacity);
    try
    {
        constructElementsFromRange(0, other.m_data, other.m_size);
    }
    catch (...)
    {
        m_allocator.deallocate(m_data, m_capacity);
        m_data = nullptr;
        m_capacity = 0;
        m_size = 0;
        throw;
    }
}



template<typename T, typename ALLOCATOR>
mylib::Vector<T, ALLOCATOR>& mylib::Vector<T, ALLOCATOR>::operator=(const Vector<T, ALLOCATOR>& other)
{
    if(this != &other)
    {
        Vector temp(other);
        swap(temp);
    }

    return *this;
}



template<typename T, typename ALLOCATOR>
mylib::Vector<T, ALLOCATOR>::Vector(Vector<T, ALLOCATOR>&& other) noexcept
    : m_capacity{ other.m_capacity }
    , m_size{ other.m_size }
    , m_data{ other.m_data }
    , m_allocator{ std::move(other.m_allocator) }
{
    other.release();
}



template<typename T, typename ALLOCATOR>
mylib::Vector<T, ALLOCATOR>& mylib::Vector<T, ALLOCATOR>::operator=(Vector<T, ALLOCATOR>&& other) noexcept
{
    if(this != &other)
    {
        swap(other);
        other.deallocate();
    }

    return *this;
}



template<typename T, typename ALLOCATOR>
mylib::Vector<T, ALLOCATOR>::~Vector() noexcept
{
    deallocate();
}



template<typename T, typename ALLOCATOR>
void mylib::Vector<T, ALLOCATOR>::append(const T& item)
{
    push_back(item);
}



template<typename T, typename ALLOCATOR>
void mylib::Vector<T, ALLOCATOR>::appendVector(const Vector<T, ALLOCATOR>& vec)
{
    if(this == &vec)
    {
        Vector copy{ vec };
        appendVector(copy);
        return;
    }
    if (m_size > maxSize() - vec.size())
    {
        throw std::length_error("Vector size overflow");
    }
    reserve(calculateNewCapacity(m_size + vec.size()));

    constructElementsFromRange(m_size, vec.data(), vec.size());

    m_size = m_size + vec.size();
}



template<typename T, typename ALLOCATOR>
constexpr T& mylib::Vector<T, ALLOCATOR>::at(size_t i)
{
    if(!(i < m_size))
        throw std::out_of_range(std::format("An index must be less then the size. "
                                            "You have index: {}, size: {}", i, m_size));
    return m_data[i];
}



template<typename T, typename ALLOCATOR>
constexpr const T& mylib::Vector<T, ALLOCATOR>::at(size_t i) const
{
    if(!(i < m_size))
        throw std::out_of_range(std::format("An index must be less then the size."
                                            "You have index: {}, size: {}", i, m_size));
    return m_data[i];
}



template<typename T, typename ALLOCATOR>
constexpr size_t mylib::Vector<T, ALLOCATOR>::calculateNewCapacity(size_t requiredSize) const
{
    if(requiredSize > maxSize())
    {
        throw std::length_error("Vector size exceeds maximum possible size");
    }

    size_t reqCap{ MinCapacity };
    while(reqCap < requiredSize )
    {
        if(reqCap > maxSize() / 2)
        {
            throw std::length_error("Vector capacity overflow");
        }
        reqCap *= 2;
    }

    return reqCap;
}



template<typename T, typename ALLOCATOR>
constexpr void mylib::Vector<T, ALLOCATOR>::constructElements(size_t from, size_t to, const T& value)
{
    size_t i{ from };
    try
    {
        for(; i < to; ++i)
        {
            new (&m_data[i]) T(value);
        }
    }
    catch(...)
    {
        destroyElements(from, i);
        throw;
    }
}



template<typename T, typename ALLOCATOR>
constexpr void mylib::Vector<T, ALLOCATOR>::constructElementsFromRange(size_t from, const T* src, size_t count)
{
    size_t i{ 0 };
    try
    {
        for(; i < count; ++i)
        {
            new (&m_data[from + i]) T(src[i]);
        }
    }
    catch(...)
    {
        destroyElements(from, from + i);
        throw;
    }
}



template<typename T, typename ALLOCATOR>
constexpr T* mylib::Vector<T, ALLOCATOR>::data() noexcept
{
    return m_data;
}



template<typename T, typename ALLOCATOR>
constexpr const T* mylib::Vector<T, ALLOCATOR>::data() const noexcept
{
    return m_data;
}



template<typename T, typename ALLOCATOR>
void mylib::Vector<T, ALLOCATOR>::deallocate() noexcept
{
    if(m_data)
    {
        destroyElements(0, m_size);
        m_allocator.deallocate(m_data, m_capacity);
        m_data = nullptr;
    }

    m_size = 0;
    m_capacity = 0;
}



template<typename T, typename ALLOCATOR>
constexpr void mylib::Vector<T, ALLOCATOR>::destroyElements(size_t from, size_t to) noexcept
{
    for(size_t i{ from }; i < to; ++i)
    {
        m_data[i].~T();
    }
}



template<typename T, typename ALLOCATOR>
template<typename... ARGS>
void mylib::Vector<T, ALLOCATOR>::emplace_back(ARGS&&... args)
{
    if(m_size == m_capacity)
    {
        if (m_size == maxSize())
        {
            throw std::length_error("Vector max size reached");
        }

        reallocateBuffer(calculateNewCapacity(m_size + 1), m_size + 1, std::forward<ARGS>(args)...);
        return;
    }

    new (&m_data[m_size]) T(std::forward<ARGS>(args)...);
    ++m_size;
}


template<typename T, typename ALLOCATOR>
constexpr bool mylib::Vector<T, ALLOCATOR>::empty() const noexcept
{
    return 0 == m_size;
}



template<typename T, typename ALLOCATOR>
constexpr T& mylib::Vector<T, ALLOCATOR>::operator[](size_t i) noexcept
{
    assert(i < size());

    return m_data[i];
}



template<typename T, typename ALLOCATOR>
constexpr const T& mylib::Vector<T, ALLOCATOR>::operator[](size_t i) const noexcept
{
    assert(i < size());

    return m_data[i];
}



template<typename T, typename ALLOCATOR>
constexpr bool mylib::Vector<T, ALLOCATOR>::operator==(const Vector<T, ALLOCATOR>& other) const
{
    return comparators::LexicographicComparator<Vector<T, ALLOCATOR>>{}.isEqual(*this, other);
}




template<typename T, typename ALLOCATOR>
constexpr auto mylib::Vector<T, ALLOCATOR>::operator<=>(const Vector<T, ALLOCATOR>& other) const
{
    return comparators::LexicographicComparator<Vector<T, ALLOCATOR>>{}.compare(*this, other);
}



template<typename T, typename ALLOCATOR>
constexpr mylib::Vector<T, ALLOCATOR>& mylib::Vector<T, ALLOCATOR>::operator+=(const Vector<T, ALLOCATOR>& other)
{
    if(size() != other.size())
    {
        throw std::invalid_argument("size mismatch");
    }

    for(size_t i{}; i < size(); ++i)
    {
        m_data[i] += other[i];
    }

    return *this;
}



template<typename T, typename ALLOCATOR>
constexpr mylib::Vector<T, ALLOCATOR>& mylib::Vector<T, ALLOCATOR>::operator-=(const Vector<T, ALLOCATOR>& other)
{
    if(size() != other.size())
    {
        throw std::invalid_argument("size mismatch");
    }

    for(size_t i{}; i < size(); ++i)
    {
        m_data[i] -= other[i];
    }

    return *this;
}



template<typename T, typename ALLOCATOR>
constexpr mylib::Vector<T, ALLOCATOR>&  mylib::Vector<T, ALLOCATOR>::operator*=(const T& scalar)
{
    for(size_t i{}; i < size(); ++i)
    {
        m_data[i] *= scalar;
    }

    return *this;
}



template<typename T, typename ALLOCATOR>
constexpr mylib::Vector<T, ALLOCATOR> mylib::Vector<T, ALLOCATOR>::operator-() const
{
    Vector result{ *this };
    return result *= -1;
}



template<typename T, typename ALLOCATOR>
double mylib::Vector<T, ALLOCATOR>::norm() const
{
    static_assert(std::is_same_v<T, double>, "norm is only available for double vectors");
    return std::sqrt(dot(*this, *this));
}



template<typename T, typename ALLOCATOR>
T mylib::Vector<T, ALLOCATOR>::pop_back() noexcept
{
    assert(!empty());

    T temp{ back() };

    resize(size() - 1);

    if(empty())
    {
        clear();
    }

    return temp;
}



template<typename T, typename ALLOCATOR>
void mylib::Vector<T, ALLOCATOR>::push_back(const T& element)
{
    if(m_size == m_capacity)
    {
        if (m_size == maxSize())
        {
            throw std::length_error("Vector max size reached");
        }
        reallocateBuffer(calculateNewCapacity(m_size + 1), m_size + 1, element);
        return;
    }

    new (&m_data[m_size]) T{ element };
    ++m_size;
}



template<typename T, typename ALLOCATOR>
void mylib::Vector<T, ALLOCATOR>::push_back(T&& element)
{
    if(m_size == m_capacity)
    {
        if (m_size == maxSize())
        {
            throw std::length_error("Vector max size reached");
        }
        reallocateBuffer(calculateNewCapacity(m_size + 1), m_size + 1, std::forward<T>(element));
        return;
    }

    new (&m_data[m_size]) T{ std::move(element) };
    ++m_size;
}



template<typename T, typename ALLOCATOR>
void mylib::Vector<T, ALLOCATOR>::reallocateBuffer(size_t newCapacity, size_t newSize)
{
    if(newCapacity == m_capacity && newSize == m_size)
    {
        return; // ничего не делаем
    }

    T* newData{ m_allocator.allocate(newCapacity) };
    mylib::BufferGuard<T> guard{ newData, 0 };

    size_t copyCount{ std::min(newSize, m_size) };
    if(m_data)
    {
        for (size_t i{}; i < copyCount; ++i)
        {
            new (&newData[i]) T(std::move_if_noexcept(m_data[i]));
            guard.addConstructed();
        }
    }

    // Если всё успешно – уничтожаем старые элементы и освобождаем старую память
    if(m_data)
    {
        destroyElements(0, m_size);
        m_allocator.deallocate(m_data, m_capacity);
    }

    guard.commit(); // предотвращает двойное уничтожение

    m_data = newData;
    m_capacity = newCapacity;
    m_size = newSize;
}


template<typename T, typename ALLOCATOR>
template<typename... ARGS>
void mylib::Vector<T, ALLOCATOR>::reallocateBuffer(size_t newCapacity, size_t newSize, ARGS&&... args)
{
    if (newSize > maxSize())
    {
        throw std::length_error("Vector size exceeds maximum possible size");
    }

    T* newData{ m_allocator.allocate(newCapacity) };

    mylib::BufferGuard<T> guard{ newData, 0 };

    size_t oldSize{ m_size };
    size_t copyCount{ std::min(newSize, oldSize) };

    size_t newElementsCount{ (newSize > oldSize) ? (newSize - oldSize) : 0 };

    // Копирование старых элементов

    if(m_data)
    {
        for(size_t i{}; i < copyCount; ++i)
        {
            new (&newData[i]) T(std::move_if_noexcept(m_data[i]));
            guard.addConstructed();
        }
    }

    // Создание новых элементов
    if(newElementsCount > 0)
    {
        // Создаём первый новый элемент из переданных аргументов
        new (&newData[oldSize]) T(std::forward<ARGS>(args)...);
        guard.addConstructed();

        // Копируем первый элемент для остальных новых элементов (если их больше одного)
        for(size_t i{ 1 }; i < newElementsCount; ++i)
        {
            new (&newData[oldSize + i]) T{ newData[oldSize] }; // копия первого
            guard.addConstructed();
        }
    }

    // Успех: освободить старые данные
    if(m_data)
    {
        destroyElements(0, oldSize);
        m_allocator.deallocate(m_data, m_capacity);
    }

    guard.commit();

    m_data = newData;
    m_capacity = newCapacity;
    m_size = newSize;
}



template<typename T, typename ALLOCATOR>
constexpr void mylib::Vector<T, ALLOCATOR>::release() noexcept
{
    m_data = nullptr;
    m_size = 0;
    m_capacity = 0;
}



template<typename T, typename ALLOCATOR>
void mylib::Vector<T, ALLOCATOR>::reserve(size_t newCap)
{
    if (newCap > maxSize())
    {
        throw std::length_error("Vector capacity exceeds maximum possible size");
    }

    if(m_capacity < newCap)
    {
        reallocateBuffer(newCap, m_size);
    }
}


template<typename T, typename ALLOCATOR>
void mylib::Vector<T, ALLOCATOR>::resize(size_t newSize, const T& value)
{
    if(newSize == m_size)
    {
        return;
    }
    else if(newSize < m_size) // Уменьшение размера
    {
        // Проверяем, нужно ли уменьшить ёмкость
        if(m_capacity > MinCapacity && newSize < m_capacity / 4)
        {
            size_t newCapacity{ calculateNewCapacity(newSize) };
            reallocateBuffer(newCapacity, newSize, value);
        }
        else
        {
            destroyElements(newSize, m_size);
            m_size = newSize;
        }
    }
    else // Увеличение размера
    {
        if(newSize <= m_capacity)
        {
            constructElements(m_size, newSize, value);
            m_size = newSize;
        }
        else
        {
            size_t newCapacity{ calculateNewCapacity(newSize) };

            reallocateBuffer(newCapacity, newSize, value);
        }
    }
}



template<typename T, typename ALLOCATOR>
void mylib::Vector<T, ALLOCATOR>::reverse()
{
    reverse(0, m_size);
}




template<typename T, typename ALLOCATOR>
void mylib::Vector<T, ALLOCATOR>::reverse(size_t start, size_t end)
{
    if( start >= end || end > m_size)
    {
        if(start == end)
        {
            return;
        }

        throw std::out_of_range("...");
    }

    while(start < end)
    {
        --end;
        std::swap(m_data[start], m_data[end]);
        ++start;
    }
}



template<typename T, typename ALLOCATOR>
constexpr size_t mylib::Vector<T, ALLOCATOR>::size() const noexcept
{
    return m_size;
}




template<typename T, typename ALLOCATOR>
void mylib::Vector<T, ALLOCATOR>::shrink_to_fit()
{
    if(m_capacity > m_size)
    {
        reallocateBuffer(m_size, m_size);
    }
}


template<typename T, typename ALLOCATOR>
constexpr void mylib::Vector<T, ALLOCATOR>::swap(Vector& other) noexcept
{
    std::swap(m_data, other.m_data);
    std::swap(m_size, other.m_size);
    std::swap(m_capacity, other.m_capacity);
    std::swap(m_allocator, other.m_allocator);
}



template<typename T, typename ALLOCATOR>
class mylib::Vector<T, ALLOCATOR>::Iterator final
{
private:
    T* m_ptr;

public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type        = T;
    using difference_type   = std::ptrdiff_t;
    using pointer           = T*;
    using reference         = T&;

    explicit constexpr  Iterator(T* ptr = nullptr) noexcept : m_ptr{ ptr } {}

    // Разыменование
    constexpr T& operator*()  const noexcept { return *m_ptr; }
    constexpr T* operator->() const noexcept { return  m_ptr; }


    // Инкремент/декремент
    constexpr Iterator& operator++() noexcept
    {
        ++m_ptr;
        return *this;
    }

    constexpr Iterator operator++(int) noexcept
    {
        Iterator tmp(*this);
        ++m_ptr;
        return tmp;
    }

    constexpr Iterator& operator--() noexcept
    {
        --m_ptr;
        return *this;
    }

    constexpr Iterator operator--(int) noexcept
    {
        Iterator tmp(*this);
        --m_ptr;
        return tmp;
    }


    // Арифметика
    constexpr Iterator& operator+=(std::ptrdiff_t n) noexcept
    {
        m_ptr += n;
        return *this;
    }

    constexpr Iterator& operator-=(std::ptrdiff_t n) noexcept
    {
        m_ptr -= n;
        return *this;
    }

    constexpr Iterator operator+(std::ptrdiff_t n) noexcept { return Iterator(m_ptr + n); }
    constexpr Iterator operator-(std::ptrdiff_t n) noexcept { return Iterator(m_ptr - n); }
    friend constexpr  Iterator operator+(std::ptrdiff_t n, const Iterator& it) noexcept { return it + n; }

    constexpr std::ptrdiff_t operator-(const Iterator& other) const noexcept { return m_ptr - other.m_ptr; }

    // Сравнение
    constexpr auto operator<=>(const Iterator& other) const noexcept { return m_ptr <=> other.m_ptr; }
    constexpr bool operator==(const Iterator& other) const noexcept { return m_ptr == other.m_ptr; }

    // Доступ по индексу
    constexpr T& operator[](std::ptrdiff_t n) const noexcept { return m_ptr[n]; }
};




template<typename T, typename ALLOCATOR>
class mylib::Vector<T, ALLOCATOR>::ConstIterator final
{
private:
    const T* m_ptr;

public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type        = T;
    using difference_type   = std::ptrdiff_t;
    using pointer           = const T*;
    using reference         = const T&;

    explicit constexpr  ConstIterator(const T* ptr = nullptr) noexcept : m_ptr{ ptr } {}

    constexpr ConstIterator(const Iterator& other) noexcept : m_ptr{ other.operator->() } {}

    // Разыменование
    constexpr const T& operator*()  const noexcept { return *m_ptr; }
    constexpr const T* operator->() const noexcept { return  m_ptr; }


    // Инкремент/декремент
    constexpr ConstIterator& operator++() noexcept
    {
        ++m_ptr;
        return *this;
    }

    constexpr ConstIterator operator++(int) noexcept
    {
        ConstIterator tmp(*this);
        ++m_ptr;
        return tmp;
    }

    constexpr ConstIterator& operator--() noexcept
    {
        --m_ptr;
        return *this;
    }

    constexpr ConstIterator operator--(int) noexcept
    {
        ConstIterator tmp(*this);
        --m_ptr;
        return tmp;
    }


    // Арифметика
    constexpr ConstIterator& operator+=(std::ptrdiff_t n) noexcept
    {
        m_ptr += n;
        return *this;
    }

    constexpr ConstIterator& operator-=(std::ptrdiff_t n) noexcept
    {
        m_ptr -= n;
        return *this;
    }

    constexpr ConstIterator operator+(std::ptrdiff_t n) noexcept { return ConstIterator(m_ptr + n); }
    constexpr ConstIterator operator-(std::ptrdiff_t n) noexcept { return ConstIterator(m_ptr - n); }
    friend constexpr ConstIterator operator+(std::ptrdiff_t n, const ConstIterator& it) noexcept { return it + n; }

    constexpr std::ptrdiff_t operator-(const ConstIterator& other) const noexcept { return m_ptr - other.m_ptr; }

    // Сравнение
    constexpr auto operator<=>(const ConstIterator& other) const noexcept { return m_ptr <=> other.m_ptr; }
    constexpr bool operator== (const ConstIterator& other) const noexcept { return m_ptr == other.m_ptr; }

    // Доступ по индексу
    constexpr const T& operator[](std::ptrdiff_t n) const noexcept { return m_ptr[n]; }
};



template<typename T, typename ALLOCATOR>
constexpr mylib::Vector<T, ALLOCATOR>::Iterator mylib::Vector<T, ALLOCATOR>::begin() noexcept
{
    return Iterator{ m_data };
}



template<typename T, typename ALLOCATOR>
constexpr mylib::Vector<T, ALLOCATOR>::ConstIterator mylib::Vector<T, ALLOCATOR>::begin() const noexcept
{
    return ConstIterator{ m_data };
}



template<typename T, typename ALLOCATOR>
constexpr mylib::Vector<T, ALLOCATOR>::ConstIterator mylib::Vector<T, ALLOCATOR>::cbegin() const noexcept
{
    return ConstIterator{ m_data };
}



template<typename T, typename ALLOCATOR>
constexpr mylib::Vector<T, ALLOCATOR>::Iterator mylib::Vector<T, ALLOCATOR>::end() noexcept
{
    return Iterator{ m_data + m_size };
}



template<typename T, typename ALLOCATOR>
constexpr mylib::Vector<T, ALLOCATOR>::ConstIterator mylib::Vector<T, ALLOCATOR>::end() const noexcept
{
    return ConstIterator{ m_data + m_size };
}



template<typename T, typename ALLOCATOR>
constexpr mylib::Vector<T, ALLOCATOR>::ConstIterator mylib::Vector<T, ALLOCATOR>::cend() const noexcept
{
    return ConstIterator{ m_data + m_size };
}



template<typename T, typename ALLOCATOR>
constexpr mylib::Vector<T, ALLOCATOR>::reverse_iterator mylib::Vector<T, ALLOCATOR>::rbegin() noexcept
{
    return reverse_iterator{ end() };
}



template<typename T, typename ALLOCATOR>
constexpr mylib::Vector<T, ALLOCATOR>::const_reverse_iterator mylib::Vector<T, ALLOCATOR>::rbegin() const noexcept
{
    return const_reverse_iterator{ end() };
}



template<typename T, typename ALLOCATOR>
constexpr mylib::Vector<T, ALLOCATOR>::const_reverse_iterator mylib::Vector<T, ALLOCATOR>::crbegin() const noexcept
{
    return const_reverse_iterator{ cend() };
}



template<typename T, typename ALLOCATOR>
constexpr mylib::Vector<T, ALLOCATOR>::reverse_iterator mylib::Vector<T, ALLOCATOR>::rend() noexcept
{
    return reverse_iterator{ begin() };
}



template<typename T, typename ALLOCATOR>
constexpr mylib::Vector<T, ALLOCATOR>::const_reverse_iterator mylib::Vector<T, ALLOCATOR>::rend() const noexcept
{
    return const_reverse_iterator{ begin() };
}



template<typename T, typename ALLOCATOR>
constexpr mylib::Vector<T, ALLOCATOR>::const_reverse_iterator mylib::Vector<T, ALLOCATOR>::crend() const noexcept
{
    return const_reverse_iterator{ cbegin() };
}


#endif // VECTOR_H
