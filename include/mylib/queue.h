#ifndef QUEUE_H
#define QUEUE_H

#include <cstddef>
#include <stdexcept>
#include <utility>

#include "mylib/memory.h"
#include "mylib/structs.h"


namespace mylib
{

template<typename T, typename ALLOCATOR = mylib::MySimpleAllocator<T>>
class Queue final
{
private:
    enum { MinCapacity = 8 };

    using AllocTraits = std::allocator_traits<ALLOCATOR>;

    T* m_data{ nullptr };

    size_t m_front{};
    size_t m_size{};
    size_t m_capacity{};

    ALLOCATOR m_alloc{};

    T* allocate(size_t size)
    {
        return AllocTraits::allocate(m_alloc, size);
    }

    template<typename... ARGS>
    void constructElement(T* elementPtr, ARGS&&... args)
    {
        AllocTraits::construct(m_alloc, elementPtr, std::forward<ARGS>(args)...);
        ++m_size;
    }

    constexpr void deallocate() noexcept
    {
        if(m_data)
        {
            AllocTraits::deallocate(m_alloc, m_data, m_capacity);
        }
    }

    constexpr void destroyAll() noexcept
    {
        for(size_t i{}; i < size(); ++i)
        {
            AllocTraits::destroy(m_alloc, m_data + offset(i));
        }
        m_size = 0;
    }

    constexpr void destroyElement(T* elementPtr) noexcept
    {
        AllocTraits::destroy(m_alloc, elementPtr);
        --m_size;
    }

    static constexpr size_t maxSize() noexcept
    {
        return std::numeric_limits<size_t>::max() / sizeof(T);
    }

    constexpr size_t offset(size_t i) const noexcept
    {
        return (m_front + i) % m_capacity;
    }

    constexpr void release() noexcept
    {
        m_data = nullptr;
        m_front = 0;
        m_size = 0;
        m_capacity = 0;
    }

    constexpr void swap(Queue& other) noexcept
    {
        std::swap(m_data, other.m_data);
        std::swap(m_front, other.m_front);
        std::swap(m_size, other.m_size);
        std::swap(m_capacity, other.m_capacity);
        std::swap(m_alloc, other.m_alloc);
    }

public:
    explicit Queue(ALLOCATOR alloc = ALLOCATOR())
        : m_data{ nullptr }
        , m_front{ 0 }
        , m_size{ 0 }
        , m_capacity{ 0 }
        , m_alloc{ alloc }
    {}

    explicit Queue(size_t capacity, ALLOCATOR alloc = ALLOCATOR())
        : m_data{ nullptr }
        , m_front{ 0 }
        , m_size{ 0 }
        , m_alloc{ alloc }
    {
        m_capacity = std::max(static_cast<size_t>(MinCapacity), capacity);
        m_data = allocate(m_capacity);
    }

    Queue(const Queue& other)
        : m_front{}
        , m_size{}
        , m_capacity{ other.m_capacity }
        , m_alloc{ other.m_alloc }
    {
        try
        {
            m_data = allocate(m_capacity);
            BufferGuard guard{ m_data };
            for(size_t i{}; i < other.m_size; ++i)
            {
                constructElement(m_data + i, *(other.m_data + other.offset(i)));
                guard.addConstructed();
            }
            guard.commit();
        }
        catch(...)
        {
            if(m_data)
            {
                deallocate();
            }

            release();
            throw;
        }
    }

    Queue(Queue&& other) noexcept
        : m_data{ other.m_data }
        , m_front{ other.m_front }
        , m_size{ other.m_size }
        , m_capacity{ other.m_capacity }
        , m_alloc{ std::move(other.m_alloc) }
    {
        other.release();
    }

    ~Queue() noexcept
    {
        destroyAll();
        deallocate();
        release();
    }

    Queue& operator=(const Queue& other)
    {
        if(this != &other)
        {
            Queue temp{ other };
            swap(temp);
        }
        return *this;
    }

    Queue& operator=(Queue&& other) noexcept
    {
        if(this != &other)
        {
            swap(other);

            other.destroyAll();
            other.deallocate();
            other.release();
        }
        return *this;
    }

    template<typename... ARGS>
    void push(ARGS&&... args)
    {
        if(!m_data)
        {
            m_data = allocate(MinCapacity);
            m_capacity = MinCapacity;
        }

        if(m_size == m_capacity)
        {
            size_t newCap{ calculateNewCapacity(m_capacity * 2, maxSize(), MinCapacity, "mylib::Queue") };
            Queue temp(newCap, m_alloc);
            for(size_t i{}; i < size(); ++i)
            {
                temp.constructElement(temp.m_data + i, std::move(*(m_data + offset(i))));
            }
            swap(temp);
            constructElement(m_data + offset(m_size), std::forward<ARGS>(args)...);
        }
        else
        {
            constructElement(m_data + offset(m_size), std::forward<ARGS>(args)...);
        }
    }

    void pop()
    {
        if(empty())
        {
            throw std::out_of_range("Queue::pop() on empty queue");
        }
        destroyElement(m_data + m_front);
        m_front = offset(1);
    }

    T& front()
    {
        if(empty())
        {
            throw std::out_of_range("Queue::front() on empty queue");
        }

        return *(m_data + m_front);
    }

    const T& front() const
    {
        if(empty())
        {
            throw std::out_of_range("Queue::front() on empty queue");
        }
        return *(m_data + m_front);
    }

    T& back()
    {
        if(empty())
        {
            throw std::out_of_range("Queue::back() on empty queue");
        }

        return *(m_data + offset(size() - 1));
    }

    const T& back() const
    {
        if(empty())
        {
            throw std::out_of_range("Queue::back() on empty queue");
        }

        return *(m_data + offset(size() - 1));
    }

    bool empty() const noexcept { return size() == 0; }
    explicit operator bool() const noexcept { return !empty(); }
    size_t size() const noexcept { return m_size; }
    void clear() noexcept
    {
        destroyAll();
    }

    bool operator==(const Queue& other) const
    {
        if(size() != other.size())
        {
            return false;
        }
        for(size_t i{}; i < size(); ++i)
        {
            if(*(m_data + offset(i)) != *(other.m_data + offset(i)))
            {
                return false;
            }
        }

        return true;
    }

    auto operator<=>(const Queue& other) const
    {
        if(size() != other.size())
        {
            return size() <=> other.size();
        }

        for(size_t i{}; i < size(); ++i)
        {
            if(*(m_data + offset(i)) < *(other.m_data + offset(i)))
            {
                return std::strong_ordering::less;
            }
            if(*(m_data + offset(i)) > *(other.m_data + offset(i)))
            {
                return std::strong_ordering::greater;
            }
        }
        return std::strong_ordering::equal;
    }
};

} // end namespace

#endif // QUEUE_H
