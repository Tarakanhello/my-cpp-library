#ifndef VECTOR_H
#define VECTOR_H

#include <algorithm>
#include <cassert>
#include <initializer_list>
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
        void destroyElements(size_t from, size_t to) noexcept;
        void deallocate() noexcept;
        void reallocateBuffer(size_t newCapacity, size_t newSize, const T& value = T());
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

        T* data() noexcept;
        const T* data() const noexcept;

        size_t size() const noexcept;

        bool empty() const noexcept;

        void resize(size_t newSize, const T& value = T());

        size_t capacity() const noexcept { return m_capacity; }

        T& operator[](size_t i) noexcept;
        const T& operator[](size_t i) const noexcept;


        class iterator final
        {
        private:
            T* m_ptr;

        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type        = T;
            using difference_type   = std::ptrdiff_t;
            using pointer           = T*;
            using reference         = T&;

            explicit iterator(pointer ptr = nullptr) noexcept : m_ptr{ ptr } {}

            reference operator*()  const noexcept { return *m_ptr; }
            pointer   operator->() const noexcept { return  m_ptr; }

            iterator& operator++() noexcept { ++m_ptr; return *this; }
            iterator operator++(int) noexcept
            {
                iterator tmp(*this);
                ++m_ptr;
                return tmp;
            }

            iterator& operator--() noexcept { --m_ptr; return *this; }
            iterator operator--(int) noexcept
            {
                iterator tmp(*this);
                --m_ptr;
                return tmp;
            }


        };
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
bool mylib::Vector<T>::empty() const noexcept
{
    return 0 == m_size;
}


template<typename T>
void mylib::Vector<T>::reallocateBuffer(size_t newCapacity, size_t newSize, const T& value)
{
    T* newData{ memory::rawMemory<T>(newCapacity) };

    mylib::BufferGuard<T> guard{ newData, 0 };

    size_t oldSize{ m_size };
    size_t copyCount{ std::min(newSize, oldSize) };

    size_t constructCount{ (newSize > oldSize) ? (newSize - oldSize) : 0 };

    // Копирование старых элементов

    if(m_data != nullptr)
    {
        for(size_t i{}; i < copyCount; ++i)
        {
            new (&newData[i]) T(m_data[i]);
            guard.addConstructed();
        }
    }

    // Создание новых элементов

    for(size_t i{}; i < constructCount; ++i)
    {
        new (&newData[oldSize + i]) T(value);
        guard.addConstructed();
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
        }
    }
    else // Увеличение размера
    {
        if(newSize <= m_capacity)
        {
            constructElements(m_size, newSize, value);
        }
        else
        {
            size_t newCapacity{ calculateNewCapacity(newSize) };

            reallocateBuffer(newCapacity, newSize, value);
        }
    }

    m_size = newSize;
}



template<typename T>
size_t mylib::Vector<T>::size() const noexcept
{
    return m_size;
}



template<typename T>
void mylib::Vector<T>::swap(Vector& other) noexcept
{
    std::swap(m_data, other.m_data);
    std::swap(m_size, other.m_size);
    std::swap(m_capacity, other.m_capacity);
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


#endif // VECTOR_H
