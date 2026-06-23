#ifndef VECTOR_H
#define VECTOR_H

#include <algorithm>
#include <cassert>
#include <format>
#include <initializer_list>
#include <iterator>
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
    template<typename T>
    class Vector final : public mylib::ArithmeticType<Vector<T>>
    {
    private:
        /** @brief Минимальная ёмкость при первом выделении памяти. */
        enum{ MinCapacity = 8 };

        size_t m_capacity{};    ///< Текущая выделенная ёмкость (количество элементов).
        size_t m_size{};        ///< Текущий размер (количество элементов).
        T* m_data{ nullptr };   ///< Указатель на массив элементов.

        /**
         * @brief Вычисляет новую ёмкость, достаточную для хранения requiredSize элементов.
         * @param requiredSize Необходимый размер.
         * @return Новая ёмкость (не менее MinCapacity и не менее requiredSize, степень двойки).
         */
        size_t calculateNewCapacity(size_t requiredSize) const noexcept;

        /**
         * @brief Конструирует элементы в диапазоне [from, to) со значением value.
         * @param from Начальный индекс.
         * @param to   Конечный индекс (не включается).
         * @param value Значение для копирования.
         * @throw Любое исключение, брошенное конструктором T. При ошибке уже созданные элементы уничтожаются.
         */
        void constructElements(size_t from, size_t to, const T& value = T());

        /**
         * @brief Конструирует count элементов из диапазона, начиная с src, в позицию from.
         * @param from Индекс в массиве m_data, куда копировать.
         * @param src  Указатель на исходный массив.
         * @param count Количество элементов.
         * @throw Любое исключение, брошенное конструктором T. При ошибке созданные элементы уничтожаются.
         */
        void constructElementsFromRange(size_t from, const T* src, size_t count);

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
        void destroyElements(size_t from, size_t to) noexcept;

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
        void release() noexcept;

        /**
         * @brief Меняет содержимое с другим вектором за O(1).
         * @param other Другой вектор.
         */
        void swap(Vector& other) noexcept;

    public:
        /**
         * @brief Конструктор по умолчанию. Создаёт пустой вектор.
         */
        Vector() noexcept;

        /**
         * @brief Конструктор, создающий вектор с size элементами, инициализированными значением value.
         * @param size Количество элементов.
         * @param value Значение для инициализации (по умолчанию T()).
         */
        explicit Vector(size_t size, const T& value = T());

        /**
         * @brief Конструктор из std::initializer_list.
         * @param list Список инициализации.
         */
        Vector(const std::initializer_list<T>& list);

        /**
         * @brief Конструктор копирования.
         * @param other Вектор для копирования.
         */
        Vector(const Vector<T>& other);

        /**
         * @brief Оператор присваивания копированием (через copy-and-swap).
         * @param other Вектор для копирования.
         * @return Ссылка на *this.
         */
        Vector<T>& operator=(const Vector<T>& other);

        /**
         * @brief Конструктор перемещения.
         * @param other Вектор, из которого перемещаются данные.
         */
        Vector(Vector<T>&& other) noexcept;

        /**
         * @brief Оператор присваивания перемещением.
         * @param other Вектор, из которого перемещаются данные.
         * @return Ссылка на *this.
         */
        Vector<T>& operator=(Vector<T>&& other) noexcept;

        /**
         * @brief Деструктор. Уничтожает все элементы и освобождает память.
         */
        ~Vector() noexcept;

        /**
         * @brief Добавляет элемент в конец (перегрузка для lvalue).
         * @param item Элемент для добавления.
         * @note Использует resize, что может быть неоптимально, но надёжно.
         */
        void append(const T& item);

        /**
         * @brief Доступ к элементу с проверкой границ.
         * @param i Индекс элемента.
         * @return Ссылка на элемент.
         * @throw std::out_of_range если i >= size().
         */
        T& at(size_t i);
        const T& at(size_t i) const;

        /**
         * @brief Возвращает текущую ёмкость.
         * @return Количество элементов, которое может вместить вектор без перераспределения.
         */
        size_t capacity() const noexcept { return m_capacity; }

        /**
         * @brief Возвращает указатель на внутренний массив.
         * @return Указатель на данные (может быть nullptr для пустого вектора).
         */
        T* data() noexcept;
        const T* data() const noexcept;

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
        bool empty() const noexcept;

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
         * @brief Изменяет размер вектора.
         * @param newSize Новый размер.
         * @param value   Значение для инициализации новых элементов (если newSize > текущего).
         * @note Если newSize меньше текущего, лишние элементы уничтожаются.
         *       При уменьшении ёмкость может быть уменьшена, если newSize * 4 < capacity.
         * @throw Любое исключение при конструировании элементов.
         */
        void resize(size_t newSize, const T& value = T());

        /**
         * @brief Резервирует память для как минимум newCap элементов.
         * @param newCap Желаемая ёмкость.
         * @note Если newCap <= текущей ёмкости, ничего не делает.
         * @throw std::bad_alloc при нехватке памяти.
         */
        void reserve(size_t newCap);

        /**
         * @brief Уменьшает ёмкость до размера (shrink-to-fit).
         * @throw std::bad_alloc при нехватке памяти (если требуется перераспределение).
         */
        void shrink_to_fit();

        /**
         * @brief Возвращает текущий размер.
         * @return Количество элементов.
         */
        size_t size() const noexcept;

        /**
         * @brief Доступ к элементу без проверки границ.
         * @param i Индекс элемента.
         * @return Ссылка на элемент.
         * @pre i < size() (проверяется через assert в отладочной сборке).
         */
        T& operator[](size_t i) noexcept;
        const T& operator[](size_t i) const noexcept;

        /**
         * @brief Проверяет равенство двух векторов.
         * @param other Вектор для сравнения.
         * @return true, если size() равны и все элементы равны.
         * @note Использует LexicographicComparator::isEqual.
         */
        bool operator==(const Vector<T>& other) const;

        /**
         * @brief Трёхстороннее сравнение (лексикографическое).
         * @param other Вектор для сравнения.
         * @return std::strong_ordering::less, equal или greater.
         * @note Использует LexicographicComparator::compare, что даёт один проход.
         */
        auto operator<=>(const Vector<T>& other) const;

        // ИТЕРАТОРЫ
        class iterator;
        class const_iterator;
        // Обратные итераторы – используем std::reverse_iterator
        using reverse_iterator       = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        iterator begin() noexcept;
        const_iterator begin() const noexcept;
        const_iterator cbegin() const noexcept;

        iterator end() noexcept;
        const_iterator end() const noexcept;
        const_iterator cend() const noexcept;

        reverse_iterator rbegin() noexcept;
        const_reverse_iterator rbegin() const noexcept;
        const_reverse_iterator crbegin() const noexcept;

        reverse_iterator rend() noexcept;
        const_reverse_iterator rend() const noexcept;
        const_reverse_iterator crend() const noexcept;
    };

} // end namespace



template<typename T>
mylib::Vector<T>::Vector() noexcept
    : m_capacity{ 0 }
    , m_size{ 0 }
    , m_data{ nullptr }
{}



template<typename T>
mylib::Vector<T>::Vector(size_t size, const T& value)
    : m_capacity{ 0 }
    , m_size{ 0 }
    , m_data{ nullptr }
{
    if(size == 0)
        return;

    size_t newCap{ calculateNewCapacity(size) };
    reallocateBuffer(newCap, size, value);
}



template<typename T>
mylib::Vector<T>::Vector(const std::initializer_list<T>& list)
    : m_capacity{ 0 }
    , m_size{ 0 }
    , m_data{ nullptr }
{
    if(list.size() == 0)
    {
        return;
    }

    m_capacity = calculateNewCapacity(list.size());
    m_data = memory::rawMemory<T>(m_capacity);
    try
    {
        constructElementsFromRange(0, list.begin(), list.size());
        m_size = list.size();
    }
    catch (...)
    {
        memory::rawDelete(m_data);
        m_data = nullptr;
        m_capacity = 0;
        throw;
    }
}



template<typename T>
mylib::Vector<T>::Vector(const Vector<T>& other)
    : m_capacity{ other.m_capacity }
    , m_size{ other.m_size }
{
    if(m_capacity == 0)
    {
        return;
    }

    m_data = memory::rawMemory<T>(m_capacity);
    try
    {
        constructElementsFromRange(0, other.m_data, other.m_size);
    }
    catch (...)
    {
        memory::rawDelete(m_data);
        m_data = nullptr;
        m_capacity = 0;
        m_size = 0;
        throw;
    }
}



template<typename T>
mylib::Vector<T>& mylib::Vector<T>::operator=(const Vector<T>& other)
{
    if(this != &other)
    {
        Vector temp(other);
        swap(temp);
    }

    return *this;
}



template<typename T>
mylib::Vector<T>::Vector(Vector<T>&& other) noexcept
    : m_capacity{ other.m_capacity }
    , m_size{ other.m_size }
    , m_data{ other.m_data }
{
    other.release();
}



template<typename T>
mylib::Vector<T>& mylib::Vector<T>::operator=(Vector<T>&& other) noexcept
{
    if(this != &other)
    {
        swap(other);
        other.deallocate();
    }

    return *this;
}



template<typename T>
mylib::Vector<T>::~Vector() noexcept
{
    deallocate();
}



template<typename T>
void mylib::Vector<T>::append(const T& item)
{
    resize(m_size + 1, item);
}



template<typename T>
T& mylib::Vector<T>::at(size_t i)
{
    if(!(i < m_size))
        throw std::out_of_range(std::format("An index must be less then the size. "
                                            "You have index: {}, size: {}", i, m_size));
    return m_data[i];
}



template<typename T>
const T& mylib::Vector<T>::at(size_t i) const
{
    if(!(i < m_size))
        throw std::out_of_range(std::format("An index must be less then the size."
                                            "You have index: {}, size: {}", i, m_size));
    return m_data[i];
}



template<typename T>
size_t mylib::Vector<T>::calculateNewCapacity(size_t requiredSize) const noexcept
{
    size_t reqCap{ MinCapacity };
    while(reqCap < requiredSize )
    {
        reqCap *= 2;
    }

    return reqCap;
}



template<typename T>
void mylib::Vector<T>::constructElements(size_t from, size_t to, const T& value)
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



template<typename T>
void mylib::Vector<T>::constructElementsFromRange(size_t from, const T* src, size_t count)
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



template<typename T>
T* mylib::Vector<T>::data() noexcept
{
    return m_data;
}



template<typename T>
const T* mylib::Vector<T>::data() const noexcept
{
    return m_data;
}



template<typename T>
void mylib::Vector<T>::deallocate() noexcept
{
    if(m_data)
    {
        memory::rawDestruct(m_data, m_size);
        m_data = nullptr;
    }

    m_size = 0;
    m_capacity = 0;
}



template<typename T>
void mylib::Vector<T>::destroyElements(size_t from, size_t to) noexcept
{
    for(size_t i{ from }; i < to; ++i)
    {
        m_data[i].~T();
    }
}



template<typename T>
template<typename... ARGS>
void mylib::Vector<T>::emplace_back(ARGS&&... args)
{
    if(m_size == m_capacity)
    {
        reallocateBuffer(calculateNewCapacity(m_size + 1), m_size + 1, std::forward<ARGS>(args)...);
        return;
    }

    new (&m_data[m_size]) T(std::forward<ARGS>(args)...);
    ++m_size;
}


template<typename T>
bool mylib::Vector<T>::empty() const noexcept
{
    return 0 == m_size;
}



template<typename T>
T& mylib::Vector<T>::operator[](size_t i) noexcept
{
    assert(i < size());

    return m_data[i];
}



template<typename T>
const T& mylib::Vector<T>::operator[](size_t i) const noexcept
{
    assert(i < size());

    return m_data[i];
}



template<typename T>
bool mylib::Vector<T>::operator==(const Vector<T>& other) const
{
    return comparators::LexicographicComparator<Vector<T>>{}.isEqual(*this, other);
}




template<typename T>
auto mylib::Vector<T>::operator<=>(const Vector<T>& other) const
{
    return comparators::LexicographicComparator<Vector<T>>{}.compare(*this, other);
}



template<typename T>
void mylib::Vector<T>::push_back(const T& element)
{
    if(m_size == m_capacity)
    {
        reallocateBuffer(calculateNewCapacity(m_size + 1), m_size + 1, element);
        return;
    }

    new (&m_data[m_size]) T{ element };
    ++m_size;
}



template<typename T>
void mylib::Vector<T>::push_back(T&& element)
{
    if(m_size == m_capacity)
    {
        reallocateBuffer(calculateNewCapacity(m_size + 1), m_size + 1, std::forward<T>(element));
        return;
    }

    new (&m_data[m_size]) T{ std::move(element) };
    ++m_size;
}



template<typename T>
template<typename... ARGS>
void mylib::Vector<T>::reallocateBuffer(size_t newCapacity, size_t newSize, ARGS&&... args)
{
    T* newData{ memory::rawMemory<T>(newCapacity) };

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
        memory::rawDestruct(m_data, oldSize);
    }

    guard.commit();

    m_data = newData;
    m_capacity = newCapacity;
    m_size = newSize;
}



template<typename T>
void mylib::Vector<T>::release() noexcept
{
    m_data = nullptr;
    m_size = 0;
    m_capacity = 0;
}



template<typename T>
void mylib::Vector<T>::reserve(size_t newCap)
{
    if(m_capacity < newCap)
    {
        reallocateBuffer(newCap, m_size, T{});
    }
}


template<typename T>
void mylib::Vector<T>::resize(size_t newSize, const T& value)
{
    if(newSize == m_size)
    {
        return;
    }
    else if(newSize < m_size) // Уменьшение размера
    {
        // Проверяем, нужно ли уменьшить ёмкость
        if(m_capacity > MinCapacity && newSize * 4 < m_capacity)
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



template<typename T>
size_t mylib::Vector<T>::size() const noexcept
{
    return m_size;
}




template<typename T>
void mylib::Vector<T>::shrink_to_fit()
{
    if(m_capacity > m_size)
    {
        reallocateBuffer(m_size, m_size, T{});
    }
}


template<typename T>
void mylib::Vector<T>::swap(Vector& other) noexcept
{
    std::swap(m_data, other.m_data);
    std::swap(m_size, other.m_size);
    std::swap(m_capacity, other.m_capacity);
}



template<typename T>
class mylib::Vector<T>::iterator final
{
private:
    T* m_ptr;

public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type        = T;
    using difference_type   = std::ptrdiff_t;
    using pointer           = T*;
    using reference         = T&;

    explicit iterator(T* ptr = nullptr) noexcept : m_ptr{ ptr } {}

    // Разыменование
    T& operator*()  const noexcept { return *m_ptr; }
    T* operator->() const noexcept { return  m_ptr; }


    // Инкремент/декремент
    iterator& operator++() noexcept
    {
        ++m_ptr;
        return *this;
    }

    iterator operator++(int) noexcept
    {
        iterator tmp(*this);
        ++m_ptr;
        return tmp;
    }

    iterator& operator--() noexcept
    {
        --m_ptr;
        return *this;
    }

    iterator operator--(int) noexcept
    {
        iterator tmp(*this);
        --m_ptr;
        return tmp;
    }


    // Арифметика
    iterator& operator+=(std::ptrdiff_t n) noexcept
    {
        m_ptr += n;
        return *this;
    }

    iterator& operator-=(std::ptrdiff_t n) noexcept
    {
        m_ptr -= n;
        return *this;
    }

    iterator operator+(std::ptrdiff_t n) noexcept { return iterator(m_ptr + n); }
    iterator operator-(std::ptrdiff_t n) noexcept { return iterator(m_ptr - n); }
    friend iterator operator+(std::ptrdiff_t n, const iterator& it) noexcept { return it + n; }

    std::ptrdiff_t operator-(const iterator& other) const noexcept { return m_ptr - other.m_ptr; }

    // Сравнение
    auto operator<=>(const iterator& other) const noexcept { return m_ptr <=> other.m_ptr; }
    bool operator==(const iterator& other) const noexcept { return m_ptr == other.m_ptr; }

    // Доступ по индексу
    T& operator[](std::ptrdiff_t n) const noexcept { return m_ptr[n]; }
};




template<typename T>
class mylib::Vector<T>::const_iterator final
{
private:
    const T* m_ptr;

public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type        = T;
    using difference_type   = std::ptrdiff_t;
    using pointer           = const T*;
    using reference         = const T&;

    explicit const_iterator(const T* ptr = nullptr) noexcept : m_ptr{ ptr } {}

    const_iterator(const iterator& other) noexcept : m_ptr{ other.operator->() } {}

    // Разыменование
    const T& operator*()  const noexcept { return *m_ptr; }
    const T* operator->() const noexcept { return  m_ptr; }


    // Инкремент/декремент
    const_iterator& operator++() noexcept
    {
        ++m_ptr;
        return *this;
    }

    const_iterator operator++(int) noexcept
    {
        const_iterator tmp(*this);
        ++m_ptr;
        return tmp;
    }

    const_iterator& operator--() noexcept
    {
        --m_ptr;
        return *this;
    }

    const_iterator operator--(int) noexcept
    {
        const_iterator tmp(*this);
        --m_ptr;
        return tmp;
    }


    // Арифметика
    const_iterator& operator+=(std::ptrdiff_t n) noexcept
    {
        m_ptr += n;
        return *this;
    }

    const_iterator& operator-=(std::ptrdiff_t n) noexcept
    {
        m_ptr -= n;
        return *this;
    }

    const_iterator operator+(std::ptrdiff_t n) noexcept { return const_iterator(m_ptr + n); }
    const_iterator operator-(std::ptrdiff_t n) noexcept { return const_iterator(m_ptr - n); }
    friend const_iterator operator+(std::ptrdiff_t n, const const_iterator& it) noexcept { return it + n; }

    std::ptrdiff_t operator-(const const_iterator& other) const noexcept { return m_ptr - other.m_ptr; }

    // Сравнение
    auto operator<=>(const const_iterator& other) const noexcept { return m_ptr <=> other.m_ptr; }
    bool operator== (const const_iterator& other) const noexcept { return m_ptr == other.m_ptr; }

    // Доступ по индексу
    const T& operator[](std::ptrdiff_t n) const noexcept { return m_ptr[n]; }
};



template<typename T>
mylib::Vector<T>::iterator mylib::Vector<T>::begin() noexcept
{
    return iterator{ m_data };
}



template<typename T>
mylib::Vector<T>::const_iterator mylib::Vector<T>::begin() const noexcept
{
    return const_iterator{ m_data };
}



template<typename T>
mylib::Vector<T>::const_iterator mylib::Vector<T>::cbegin() const noexcept
{
    return const_iterator{ m_data };
}



template<typename T>
mylib::Vector<T>::iterator mylib::Vector<T>::end() noexcept
{
    return iterator{ m_data + m_size };
}



template<typename T>
mylib::Vector<T>::const_iterator mylib::Vector<T>::end() const noexcept
{
    return const_iterator{ m_data + m_size };
}



template<typename T>
mylib::Vector<T>::const_iterator mylib::Vector<T>::cend() const noexcept
{
    return const_iterator{ m_data + m_size };
}



template<typename T>
mylib::Vector<T>::reverse_iterator mylib::Vector<T>::rbegin() noexcept
{
    return reverse_iterator{ end() };
}



template<typename T>
mylib::Vector<T>::const_reverse_iterator mylib::Vector<T>::rbegin() const noexcept
{
    return const_reverse_iterator{ end() };
}



template<typename T>
mylib::Vector<T>::const_reverse_iterator mylib::Vector<T>::crbegin() const noexcept
{
    return const_reverse_iterator{ cend() };
}



template<typename T>
mylib::Vector<T>::reverse_iterator mylib::Vector<T>::rend() noexcept
{
    return reverse_iterator{ begin() };
}



template<typename T>
mylib::Vector<T>::const_reverse_iterator mylib::Vector<T>::rend() const noexcept
{
    return const_reverse_iterator{ begin() };
}



template<typename T>
mylib::Vector<T>::const_reverse_iterator mylib::Vector<T>::crend() const noexcept
{
    return const_reverse_iterator{ cbegin() };
}


#endif // VECTOR_H
