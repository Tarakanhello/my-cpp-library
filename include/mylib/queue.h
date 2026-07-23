#ifndef QUEUE_H
#define QUEUE_H

#include <cstddef>
#include <utility>

#include "mylib/memory.h"
#include "mylib/vector.h"


namespace mylib
{

template<typename T, typename ALLOCATOR = mylib::MySimpleAllocator<T>>
class Queue final
{
private:
    /**
     * @brief Начальная ёмкость стека по умолчанию.
     */
    constexpr static const size_t initialCapacity{ 8 };

    Vector<T, ALLOCATOR> m_data{};

    size_t m_indexOfFrontElement{}; // индекс первого живого элемента
    size_t m_size{};                // количество живых элементов

    // Сдвигает все живые элементы в начало вектора и урезает размер до m_size
    void shift()
    {
        if (m_indexOfFrontElement > 0 && m_size > 0) {
            for (size_t i{}; i < m_size; ++i)
            {
                m_data[i] = std::move(m_data[m_indexOfFrontElement + i]);
            }
            // Уничтожаем старые элементы (которые остались после m_size)
            for (size_t i{ m_size }; i < m_indexOfFrontElement + m_size; ++i)
            {
                m_data[i].~T();
            }
            m_indexOfFrontElement = 0;
            m_data.resize(m_size); // физический размер = логический
        }
        else if (m_indexOfFrontElement > 0)
        {
            // Если очередь пуста, просто сбрасываем состояние
            m_indexOfFrontElement = 0;
            m_data.resize(0);
            m_size = 0;
        }
    }

    void release()
    {
        m_indexOfFrontElement = 0;
        m_size = 0;
    }

public:
    Queue()
        : m_indexOfFrontElement{ 0 }
        , m_size{ 0 }
    {
        m_data.reserve(initialCapacity);
    }

    explicit Queue(const ALLOCATOR& alloc)
        : m_data(initialCapacity, alloc)
        , m_indexOfFrontElement{ 0 }
        , m_size{ 0 }
    {
        m_data.reserve(initialCapacity);
    }

    Queue(const Queue& other)
        : m_data{ other.m_data }
        , m_indexOfFrontElement{ other.m_indexOfFrontElement }
        , m_size{ other.m_size }
    {}

    Queue(Queue&& other) noexcept
        : m_data{ std::move(other.m_data) }
        , m_indexOfFrontElement{ other.m_indexOfFrontElement }
        , m_size{ other.m_size }
    {
        other.release();
    }

    ~Queue() noexcept = default;

    Queue& operator=(const Queue& other)
    {
        if(this != &other)
        {
            m_data = other.m_data;
            m_indexOfFrontElement = other.m_indexOfFrontElement;
            m_size = other.m_size;
        }
        return *this;
    }

    Queue& operator=(Queue&& other) noexcept
    {
        if(this != &other)
        {
            m_data = std::move(other.m_data);
            m_indexOfFrontElement = other.m_indexOfFrontElement;
            m_size = other.m_size;

            other.release();
        }
        return *this;
    }

    template<typename... ARGS>
    void push(ARGS&&... args)
    {
        if(m_data.size() == m_data.capacity())
        {
            if(m_indexOfFrontElement > 0)
            {
                // Если есть мёртвые элементы в начале, сдвигаем их, чтобы освободить место в конце
                shift();
            }
            // Если после сдвига всё ещё нет места – увеличиваем ёмкость
            if(m_data.size() == m_data.capacity())
            {
                m_data.reserve(m_data.capacity() * 2);
            }
        }
        // Добавляем новый элемент в конец вектора (emplace_back использует perfect forwarding)
        m_data.emplace_back(std::forward<ARGS>(args)...);

        ++m_size;
    }

    void pop()
    {
        if(empty())
        {
            throw std::out_of_range("Queue::pop() on empty queue");
        }
        // Явно уничтожаем элемент
        m_data[m_indexOfFrontElement].~T();
        ++m_indexOfFrontElement;
        --m_size;

        if(m_indexOfFrontElement > 0 &&
            m_indexOfFrontElement > m_data.capacity() / 2 &&
            m_size < m_data.capacity() / 2)
        {
            shift();
        }
    }

    T& front()
    {
        if(empty())
        {
            throw std::out_of_range("Queue::front() on empty queue");
        }
        return m_data[m_indexOfFrontElement];
    }

    const T& front() const
    {
        if(empty())
        {
            throw std::out_of_range("Queue::front() on empty queue");
        }

        return m_data[m_indexOfFrontElement];
    }

    T& back()
    {
        if(empty())
        {
            throw std::out_of_range("Queue::back() on empty queue");
        }

        return m_data[m_indexOfFrontElement + size() - 1];
    }

    const T& back() const
    {
        if(empty())
        {
            throw std::out_of_range("Queue::back() on empty queue");
        }

        return m_data[m_indexOfFrontElement + size() - 1];
    }

    bool empty() const noexcept { return size() == 0; }
    explicit operator bool() const noexcept { return !empty(); }
    size_t size() const noexcept { return m_size; }
    void clear() noexcept
    {
        m_data.clear();
        release();
    }

    bool operator==(const Queue& other) const
    {
        if(size() != other.size())
        {
            return false;
        }
        for(size_t i{}; i < size(); ++i)
        {
            if(m_data[m_indexOfFrontElement + i] != other.m_data[m_indexOfFrontElement + i])
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
            if(m_data[m_indexOfFrontElement + i] < other.m_data[m_indexOfFrontElement + i])
            {
                return std::strong_ordering::less;
            }
            if(m_data[m_indexOfFrontElement + i] > other.m_data[m_indexOfFrontElement + i])
            {
                return std::strong_ordering::greater;
            }
        }
        return std::strong_ordering::equal;
    }
};

} // end namespace

#endif // QUEUE_H
