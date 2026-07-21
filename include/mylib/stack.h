#ifndef STACK_H
#define STACK_H


#include <cstddef>
#include <utility>

#include "mylib/memory.h"
#include "mylib/vector.h"

namespace
{

constexpr const size_t initialCapacity{ 8 };

} // end namespace



namespace mylib
{

template<typename T, typename ALLOCATOR = mylib::MySimpleAllocator<T>>
class Stack final
{
private:
    Vector<T, ALLOCATOR> m_vector{};

public:
    explicit Stack(ALLOCATOR alloc = ALLOCATOR())
        : m_vector(initialCapacity, alloc)
    {}

    explicit Stack(size_t capacity, ALLOCATOR alloc = ALLOCATOR())
        : m_vector(capacity, alloc)
    {}

    Stack(const Stack& other)
        : m_vector{ other.m_vector }
    {}

    Stack(Stack&& other) noexcept
        : m_vector{ std::move(other.m_vector) }
    {}

    ~Stack() noexcept = default;

    Stack& operator=(const Stack& other)
    {
        if(this != &other)
        {
            m_vector = other.m_vector;
        }

        return *this;
    }

    Stack& operator=(Stack&& other) noexcept
    {
        if(this != &other)
        {
            m_vector = std::move(other.m_vector);
        }

        return *this;
    }

    template<typename... ARGS>
    void push(ARGS&&... args)
    {
        m_vector.push_back(std::forward<ARGS>(args)...);
    }

    void pop() noexcept
    {
        m_vector.resize(m_vector.size() - 1);
    }

    T& top() noexcept
    {
        return m_vector.back();
    }

    const T& top() const noexcept
    {
        return m_vector.back();
    }

    constexpr bool empty() const noexcept
    {
        return m_vector.empty();
    }

    explicit constexpr operator bool() const noexcept
    {
        return !empty();
    }

    size_t size() const noexcept
    {
        return m_vector.size();
    }

    size_t capacity() const noexcept
    {
        return m_vector.capacity();
    }

    void clear() noexcept
    {
        m_vector.clear();
    }

    void shrink_to_fit()
    {
        m_vector.shrink_to_fit();
    }

    bool operator<=>(const Stack& other) const
    {
        return m_vector <=> other.m_vector;
    }
};

} // end namespace


#endif // STACK_H
