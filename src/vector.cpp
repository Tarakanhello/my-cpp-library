#include "mylib/vector.h"

#include "mylib/memory.h"


template<typename T>
mylib::Vector<T>::Vector()
    : m_capacity{ 0 }
    , m_size{ 0 }
    , m_data{ nullptr }
{}



template<typename T>
mylib::Vector<T>::Vector(size_t size, const T& value)
    : m_capacity{ 0 }
    , m_size{ size }
    , m_data{ nullptr }
{
    if(m_size == 0)
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
    other.reset();
}



template<typename T>
mylib::Vector<T>& mylib::Vector<T>::operator=(Vector<T>&& other) noexcept
{
    if(this != &other)
    {
        swap(other);
        other.reset();
    }

    return *this;
}



template<typename T>
mylib::Vector<T>::~Vector()
{
    deallocate();
}



template<typename T>
void mylib::Vector<T>::append(const T& item)
{
    resize(m_size + 1, item);
}



template<typename T>
size_t mylib::Vector<T>::calculateNewCapacity(size_t requiredSize) const
{
    size_t newCap{ (m_capacity == 0) ? MinCapacity : m_capacity };
    while(newCap < requiredSize)
    {
        newCap *=2;
    }

    return newCap;
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
T* mylib::Vector<T>::data()
{
    return m_data;
}



template<typename T>
const T* mylib::Vector<T>::data() const
{
    return m_data;
}



template<typename T>
void mylib::Vector<T>::deallocate()
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
bool mylib::Vector<T>::empty() const
{
    return 0 == m_size;
}




template<typename T>
void mylib::Vector<T>::reallocateBuffer(size_t newCapacity, size_t newSize, const T& value)
{
    T* newData{ memory::rawMemory<T>(newCapacity) };
    size_t oldSize{ m_size };
    size_t copyCount{ (newSize < oldSize) ? newSize : oldSize };

    size_t constructCount{ (newSize > oldSize) ? (newSize - oldSize) : 0 };

    // Копирование старых элементов
    size_t copied{ 0 };

    try
    {
        if(m_data != nullptr)
        {
            for(; copied < copyCount; ++copied)
            {
                new (&newData[copied]) T(m_data[copied]);
            }
        }

        // Создание новых элементов
        size_t created{ 0 };
        if(constructCount > 0)
        {
            try
            {
                for(; created < constructCount; ++created)
                {
                    new (&newData[oldSize + created]) T(value);
                }
            }
            catch(...)
            {
                // откат: уничтожить всё, что успели создать в новом буфере
                memory::rawDestruct(newData, oldSize + created);
                throw;
            }
        }

        // Успех: освободить старые данные
        if(m_data)
        {
            memory::rawDestruct(m_data, oldSize);
        }

        m_data = newData;
        m_capacity = newCapacity;
        m_size = newSize;
    }
    catch (...)
    {
        // откат: уничтожить скопированные объекты и освободить память
        memory::rawDestruct(newData, copied);
        throw;
    }
}



template<typename T>
void mylib::Vector<T>::reset()
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
size_t mylib::Vector<T>::size() const
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
T& mylib::Vector<T>::operator[](size_t i)
{
    assert(i < size());

    return m_data[i];
}



template<typename T>
const T& mylib::Vector<T>::operator[](size_t i) const
{
    assert(i < size());

    return m_data[i];
}
