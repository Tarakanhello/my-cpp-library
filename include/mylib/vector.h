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
    template<typename T>
    class Vector final : public mylib::ArithmeticType<Vector<T>>
    {
    private:
        enum{ MinCapacity = 8 };

        size_t m_capacity{};
        size_t m_size{};

        T* m_data{ nullptr };

        size_t calculateNewCapacity(size_t requiredSize) const noexcept;
        void constructElements(size_t from, size_t to, const T& value = T());
        void constructElementsFromRange(size_t from, const T* src, size_t count);
        void deallocate() noexcept;
        void destroyElements(size_t from, size_t to) noexcept;

        template<typename... ARGS>
        void reallocateBuffer(size_t newCapacity, size_t newSize, ARGS&&... args);
        void release() noexcept;
        void swap(Vector& other) noexcept;

    public:
        Vector() noexcept;
        explicit Vector(size_t size, const T& value = T());
        Vector(const std::initializer_list<T>& list);
        Vector(const Vector<T>& other);
        Vector<T>& operator=(const Vector<T>& other);
        Vector(Vector<T>&& other) noexcept;
        Vector<T>& operator=(Vector<T>&& other) noexcept;
        ~Vector() noexcept;

        void append(const T& item);
        T& at(size_t i);
        const T& at(size_t i) const;
        size_t capacity() const noexcept { return m_capacity; }
        T* data() noexcept;
        const T* data() const noexcept;

        template<typename... ARGS>
        void emplace_back(ARGS&&... args);
        bool empty() const noexcept;
        void push_back(const T& element);
        void push_back(T&& element);
        void resize(size_t newSize, const T& value = T());
        void reserve(size_t newCap);
        void shrink_to_fit();
        size_t size() const noexcept;

        T& operator[](size_t i) noexcept;
        const T& operator[](size_t i) const noexcept;
        bool operator==(const Vector<T>& other) const noexcept;
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
bool mylib::Vector<T>::operator==(const Vector<T>& other) const noexcept
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
