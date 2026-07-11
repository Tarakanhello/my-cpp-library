#ifndef STATIC_FREE_LIST_H
#define STATIC_FREE_LIST_H


#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>

#include "mylib/memory.h"
#include "mylib/vector.h"

namespace mylib
{
    template<typename T, typename ALLOCATOR = mylib::MySimpleAllocator<T>>
    class StaticFreeList final
    {
    public:
        struct Node;

    private:
        using NODE_ALLOCATOR = typename std::allocator_traits<ALLOCATOR>::template rebind_alloc<Node>;
        using ALLOC_TRAITS = std::allocator_traits<NODE_ALLOCATOR>;

        Node* allocateNodes() { return ALLOC_TRAITS::allocate(m_nodeAllocator, m_capacity); }
        void deallocateNodes() noexcept { ALLOC_TRAITS::deallocate(m_nodeAllocator, m_nodes, m_capacity); }

    private:
        // m_currentSize <= m_constructedSize <= m_capacity
        size_t m_currentSize{};     // текущий размер
        size_t m_constructedSize{}; // количество сконструированных элементов
        size_t m_capacity{};        // всестимость

        Node* m_returned{ nullptr }; // head of free Node;
        Node* m_nodes{ nullptr };    // array of Nodes;

        NODE_ALLOCATOR m_nodeAllocator{};

        void destroyList() noexcept;
        void release() noexcept;

    public:
        StaticFreeList() = default;
        explicit StaticFreeList(size_t capacity, ALLOCATOR alloc = ALLOCATOR());
        StaticFreeList(const StaticFreeList&) = delete;
        StaticFreeList& operator=(const StaticFreeList&) = delete;
        StaticFreeList(StaticFreeList&& other) noexcept;
        StaticFreeList& operator=(StaticFreeList&& other) noexcept;
        ~StaticFreeList() noexcept;

        size_t capacity() const noexcept { return m_capacity; }
        bool isFull() const noexcept { return m_currentSize == m_capacity; }
        bool empty() const noexcept { return m_currentSize == 0; }

        explicit operator bool() const noexcept { return !empty(); }

        Node* allocate() noexcept;
        void remove(Node* node) noexcept;
    }; // end StaticFreeList class



    template<typename T, typename ALLOCATOR>
    struct StaticFreeList<T, ALLOCATOR>::
        Node final
    {
        T value;

        union
        {
            Node* next;
            void* userData;
        };
    };

} // end namespace mylib



template<typename T, typename ALLOCATOR>
mylib::StaticFreeList<T, ALLOCATOR>::Node* mylib::StaticFreeList<T, ALLOCATOR>::
    allocate() noexcept
{
    assert(!isFull());

    Node* result{ m_returned };

    if(result)
    {
        m_returned = m_returned->next;
    }
    else
    {
        result = &(m_nodes[m_constructedSize++]);
    }
    ++m_currentSize;

    return result;
}



template<typename T, typename ALLOCATOR>
void mylib::StaticFreeList<T, ALLOCATOR>::
    destroyList() noexcept
{
    if(!empty())
    {
        try
        {
            mylib::Vector<bool> toDestruct(m_constructedSize, true);
            Node* returned{ m_returned };
            while(returned)
            {
                toDestruct[returned - m_nodes] = false;
                returned = returned->next;
            }

            for(size_t i{}; i < m_constructedSize; ++i)
            {
                if(toDestruct[i])
                {
                    m_nodes[i].value.~T();
                }
            }
        }
        catch(...)
        {
            // fallback O(N²) – надёжно, но медленно
            Node* curNode{ m_nodes };
            for(size_t i{}; i < m_constructedSize; ++i)
            {
                bool isReturned{ false };
                Node* returned{ m_returned };
                while(returned)
                {
                    if(returned == curNode)
                    {
                        isReturned = true;
                        break;
                    }
                    returned = returned->next;
                }
                if(!isReturned)
                {
                    curNode->value.~T();
                }

                ++curNode;
            }
        }
    }

    if(m_nodes)
        deallocateNodes();
}



template<typename T, typename ALLOCATOR>
void mylib::StaticFreeList<T, ALLOCATOR>::
    release() noexcept
{
    m_currentSize = 0;
    m_constructedSize = 0;
    m_capacity = 0;
    m_returned = nullptr;
    m_nodes = nullptr;
}



template<typename T, typename ALLOCATOR>
void mylib::StaticFreeList<T, ALLOCATOR>::
    remove(Node* node) noexcept
{
    if(node)
    {
        assert(node - m_nodes >= 0 && node - m_nodes < m_constructedSize);
        node->value.~T();
        node->next = m_returned;
        m_returned = node;
        --m_currentSize;
    }
}



template<typename T, typename ALLOCATOR>
mylib::StaticFreeList<T, ALLOCATOR>::
    StaticFreeList(size_t capacity, ALLOCATOR alloc)
    : m_capacity{ capacity }
    , m_currentSize{}
    , m_constructedSize{}
    , m_returned{ nullptr }
    , m_nodeAllocator{ alloc }
    , m_nodes{ capacity > 0 ? allocateNodes() : nullptr }
{}



template<typename T, typename ALLOCATOR>
mylib::StaticFreeList<T, ALLOCATOR>::
    StaticFreeList(StaticFreeList&& other) noexcept
{
    if(this != &other)
    {
        m_capacity = other.m_capacity;
        m_currentSize = other.m_currentSize;
        m_constructedSize = other.m_constructedSize;
        m_returned = other.m_returned;
        m_nodeAllocator = std::move(other.m_nodeAllocator);
        m_nodes = other.m_nodes;

        other.release();
    }
}



template<typename T, typename ALLOCATOR>
mylib::StaticFreeList<T, ALLOCATOR>& mylib::StaticFreeList<T, ALLOCATOR>::
    operator=(StaticFreeList&& other) noexcept
{
    if(this != &other)
    {
        destroyList();

        m_capacity = other.m_capacity;
        m_currentSize = other.m_currentSize;
        m_constructedSize = other.m_constructedSize;
        m_returned = other.m_returned;
        m_nodeAllocator = std::move(other.m_nodeAllocator);
        m_nodes = other.m_nodes;

        other.release();
    }

    return *this;
}



template<typename T, typename ALLOCATOR>
mylib::StaticFreeList<T, ALLOCATOR>::
    ~StaticFreeList() noexcept
{
    destroyList();
}



#endif // STATIC_FREE_LIST_H
