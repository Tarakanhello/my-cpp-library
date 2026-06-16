#include "mylib/vector.h"

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
int mylib::Vector<T>::size() const
{
    return m_size;
}

template<typename T>
int mylib::Vector<T>::getSize() const
{
    return size();
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
