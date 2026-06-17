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

    updateCapacity(size);
    allocate(value);
}



template<typename T>
mylib::Vector<T>::Vector(const std::initializer_list<T>& list)
    : m_capacity{ 0 }
    , m_size{ 0 }
    , m_data{ nullptr }
{
    m_size = list.size();
    updateCapacity(m_size);
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

    for(size_t i{ 0 }; i < m_size; ++i)
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

    for(size_t i{ 0 }; i < m_size; ++i)
    {
        new (&m_data[i]) T(values[i]);
    }
}


template<typename T>
void mylib::Vector<T>::append(const T& item)
{
    if(m_size >= m_capacity)
    {
        resize(m_size + 1);
        m_data[m_size - 1] = item;
    }
    else
    {
        new (&m_data[m_size]) T(item);
        ++m_size;
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
bool mylib::Vector<T>::empty() const
{
    return 0 == m_size;
}



template<typename T>
void mylib::Vector<T>::reallocate(const Vector<T>& other)
{
    T* newData{ memory::rawMemory<T>(other.m_capacity) };
    size_t i{ 0 };
    try
    {
        for(; i < other.m_size; ++i)
        {
            new (&newData[i]) T(other.m_data[i]);
        }
    }
    catch(...)
    {
        memory::rawDestruct(newData, i);
        throw;
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
void mylib::Vector<T>::resize(size_t newSize)
{
    if(newSize == m_size)
    {
        return;
    }
    else if(newSize < m_size)
    {
        if(m_capacity > MinCapacity && newSize * 4 < m_capacity)
        {
            // TO DO
        }
        else
        {
            for(size_t i{ newSize }; i < m_size; ++i)
            {
                m_data[i].~T();
            }
        }
    }
    else
    {
        if(newSize <= m_capacity)
        {
            size_t i{ m_size };
            try
            {
                for(; i < newSize; ++i)
                {
                    new (&m_data[i]) T();
                }
            }
            catch (...)
            {
                for(size_t j{ m_size }; j < i; ++j)
                {
                    m_data[j].~T();
                }
                throw;
            }
        }
        else
        {
            updateCapacity(newSize);

            T* newData{ memory::rawMemory<T>(m_capacity) };
            size_t copied{ 0 }; // сколько старых элементов скопировано
            try
            {
                for(; copied < m_size; ++copied)
                {
                    new (&newData[copied]) T(m_data[copied]);
                }
            }
            catch(...)
            {
                memory::rawDestruct(newData, copied);
                throw;
            }

            size_t created{ m_size };
            try
            {
                for(; created < newSize; ++created)
                {
                    new (&newData[created]) T();
                }
            }
            catch(...)
            {
                memory::rawDestruct(newData, created);
                throw;
            }

            if (m_data)
            {
                memory::rawDestruct(m_data, m_size);
            }

            m_data = newData;
        }
    }

    m_size = newSize;
}

template<typename T>
void mylib::Vector<T>::resize()
{
    T* oldItems{ m_data };
    m_capacity = std::max(2 * m_size, MinCapacity);

    m_data = memory::rawMemory<T>(m_capacity);

    for(size_t i{}; i < m_size; ++i)
    {
        new (&m_data[i]) T(oldItems[i]);
    }
    memory::rawDestruct(oldItems, m_size);
}

template<typename T>
size_t mylib::Vector<T>::size() const
{
    return m_size;
}



template<typename T>
void mylib::Vector<T>::updateCapacity(size_t size)
{
    m_capacity = (m_capacity == 0 ? MinCapacity : m_capacity);
    while(m_capacity < size)
    {
        m_capacity *= 2;
    }
}



template<typename T>
T& mylib::Vector<T>::operator[](size_t i)
{
    assert(i >=0 && i < size());

    return m_data[i];
}



template<typename T>
const T& mylib::Vector<T>::operator[](size_t i) const
{
    assert(i < size());

    return m_data[i];
}
