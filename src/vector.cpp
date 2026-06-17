#include "mylib/vector.h"

#include "mylib/memory.h"


template<typename T>
mylib::Vector<T>::Vector()
    : m_capacity{ 0 }
    , m_size{ 0 }
    , m_data{ nullptr }
{}



template<typename T>
mylib::Vector<T>::Vector(int size, const T& value)
    : m_capacity{ 0 }
    , m_size{ size }
    , m_data{ nullptr }
{
    if(m_size == 0)
        return;

    updateCapacity();
    allocate(value);
}



template<typename T>
mylib::Vector<T>::Vector(const std::initializer_list<T>& list)
    : m_data{ nullptr }
{
    m_size = list.size();
    updateCapacity();
    allocate(list);
}


template<typename T>
mylib::Vector<T>::Vector(const Vector<T>& other)
    : m_capacity{ other.m_capacity }
    , m_size{ other.m_size }
{
    allocate(other);
}


template<typename T>
mylib::Vector<T>& mylib::Vector<T>::operator=(const Vector<T>& other)
{
    if(this != &other)
    {
        reallocate(other);
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
        deallocate();
        m_capacity = other.m_capacity;
        m_size = other.m_size;
        m_data = other.m_data;

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
void mylib::Vector<T>::allocate(const T& value)
{
    m_data = memory::rawMemory<T>(m_capacity);

    for(int i{ 0 }; i < m_size; ++i)
    {
        new (&m_data[i]) T(value);
    }
}



template<typename T>
template<typename Z>
void mylib::Vector<T>::allocate(const Z& values)
{
    if(m_capacity == 0)
    {
        m_data =  nullptr;

        return;
    }

    m_data = memory::rawMemory<T>(m_capacity);

    for(int i{ 0 }; i < m_size; ++i)
    {
        new (&m_data[i]) T(values[i]);
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
        for(int i{ 0 }; i < m_size; ++i)
        {
            m_data[i].~T();
        }

        memory::rawDelete(m_data);

        m_data = nullptr;
    }

    m_size = 0;
    m_capacity = 0;
}




template<typename T>
bool mylib::Vector<T>::empty() const
{
    return 0 == m_size;
}



template<typename T>
T* mylib::Vector<T>::getArray()
{
    return data();
}



template<typename T>
const T* mylib::Vector<T>::getArray() const
{
    return data();
}



template<typename T>
int mylib::Vector<T>::getSize() const
{
    return size();
}



template<typename T>
void mylib::Vector<T>::reallocate(const Vector<T>& other)
{
    T* newData{ memory::rawMemory<T>(other.m_capacity) };

    for(int i{ 0 }; i < other.m_size; ++i)
    {
        new (&newData[i]) T(other.m_data[i]);
    }

    deallocate();

    m_data = newData;
    m_size = other.m_size;
    m_capacity = other.m_capacity;

    newData = nullptr;
}


template<typename T>
void mylib::Vector<T>::reset()
{
    m_data = nullptr;
    m_size = 0;
    m_capacity = 0;
}




template<typename T>
int mylib::Vector<T>::size() const
{
    return m_size;
}



template<typename T>
void mylib::Vector<T>::updateCapacity()
{
    m_capacity = (m_capacity == 0 ? MinCapacity : m_capacity);
    while(m_capacity < m_size)
    {
        m_capacity *= 2;
    }
}



template<typename T>
T& mylib::Vector<T>::operator[](int i)
{
    assert(i >=0 && i < size());

    return m_data[i];
}



template<typename T>
const T& mylib::Vector<T>::operator[](int i) const
{
    assert(i >=0 && i < size());

    return m_data[i];
}
