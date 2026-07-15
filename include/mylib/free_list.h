#ifndef FREE_LIST_H
#define FREE_LIST_H

#include <algorithm>
#include <cstddef>

#include "mylib/list.h"
#include "mylib/memory.h"
#include "mylib/static_free_list.h"



namespace mylib
{
    template<typename T, typename ALLOCATOR = mylib::MySimpleAllocator<T>>
    class FreeList final
    {
    private:
        static constexpr const size_t MIN_BLOCK_SIZE    { 8 };
        static constexpr const size_t MAX_BLOCK_SIZE    { 8192 };
        static constexpr const size_t DEFAULT_SIZE      { 32 };

    public:
        using Block = StaticFreeList<T, ALLOCATOR>;
        using BlockAllocator = typename std::allocator_traits<ALLOCATOR>::template rebind_alloc<Block>;
        using BlockList = mylib::List<Block, BlockAllocator>;

    private:
        BlockList m_blocks{};
        size_t m_currentBlockSize{};
        size_t m_size{};
        size_t m_capacity{};

        ALLOCATOR m_alloc{};

        size_t m_numberOfEmptyBlock{};

        void createNewBlock();
        void release() noexcept;

    public:
        explicit FreeList(size_t initialSize = DEFAULT_SIZE, ALLOCATOR alloc = ALLOCATOR());

        FreeList(const FreeList&) = delete;
        FreeList& operator=(const FreeList&) = delete;

        FreeList(FreeList&& other) noexcept;
        FreeList& operator=(FreeList&& other) noexcept;

        ~FreeList() noexcept {}

        T* allocateRaw();
        template<typename... ARGS>
        T* emplace(ARGS&&... args);
        void remove(T* ptr);

        bool empty() const noexcept { return m_size == 0; }
        explicit operator bool() const noexcept { return !empty(); }
        size_t size() const noexcept { return m_size; }
        size_t capacity() const noexcept{ return m_capacity; }
        size_t blockCount() const noexcept { return m_blocks.size(); }
    };


} // end namespace mylib;



template<typename T, typename ALLOCATOR>
T* mylib::FreeList<T, ALLOCATOR>::allocateRaw()
{
    if(m_size == m_capacity)
    {
        createNewBlock();
    }

    bool blockWasEmpty{ m_blocks.front().empty() };

    typename Block::Node* node{ m_blocks.front().allocate() };

    if(blockWasEmpty)
    {
        --m_numberOfEmptyBlock;
    }

    node->userData = static_cast<void*>(m_blocks.begin().getNode());

    if(m_blocks.front().isFull())
    {
        m_blocks.moveToEnd(m_blocks.begin());
    }

    ++m_size;
    return &(node->value);
}



template<typename T, typename ALLOCATOR>
template<typename... ARGS>
T* mylib::FreeList<T, ALLOCATOR>::emplace(ARGS&&... args)
{
    T* place{ allocateRaw() };

    new (place) T(std::forward<ARGS>(args)...);

    return place;
}



template<typename T, typename ALLOCATOR>
void mylib::FreeList<T, ALLOCATOR>::
    createNewBlock()
{
    m_blocks.push_front(StaticFreeList<T, ALLOCATOR>{ m_currentBlockSize });
    m_capacity += m_currentBlockSize;
    m_currentBlockSize = std::min(MAX_BLOCK_SIZE, 2 * m_currentBlockSize);

    ++m_numberOfEmptyBlock;
}



template<typename T, typename ALLOCATOR>
mylib::FreeList<T, ALLOCATOR>::
    FreeList(size_t initialSize, ALLOCATOR alloc)
    : m_currentBlockSize{ MIN_BLOCK_SIZE }
    , m_size{ 0 }
    , m_capacity{ 0 }
    , m_alloc{ alloc }
    , m_numberOfEmptyBlock{ 0 }
{
    size_t neededCapacity{ std::max(initialSize, MIN_BLOCK_SIZE) };
    if(neededCapacity >= MAX_BLOCK_SIZE)
    {
        m_currentBlockSize = MAX_BLOCK_SIZE;

        // Количество блоков MAX_BLOCK_SIZE, чтобы покрыть neededCapacity с избытком
        size_t numBlocks{ (neededCapacity + MAX_BLOCK_SIZE - 1) / MAX_BLOCK_SIZE };
        for (size_t i{}; i < numBlocks; ++i)
        {
            createNewBlock();
        }
    }
    else
    {
        m_currentBlockSize = neededCapacity;
        createNewBlock();
    }
}



template<typename T, typename ALLOCATOR>
mylib::FreeList<T, ALLOCATOR>::
    FreeList(FreeList&& other) noexcept
    : m_blocks{ std::move(other.m_blocks) }
    , m_currentBlockSize{ other.m_currentBlockSize }
    , m_size{ other.m_size }
    , m_capacity{ other.m_capacity }
    , m_alloc{ std::move(other.m_alloc) }
    , m_numberOfEmptyBlock{ other.m_numberOfEmptyBlock }
{
    other.release();
}



template<typename T, typename ALLOCATOR>
mylib::FreeList<T, ALLOCATOR>&
    mylib::FreeList<T, ALLOCATOR>::
        operator=(FreeList&& other) noexcept
{
    if(this != &other)
    {
        m_blocks = std::move(other.m_blocks);
        m_currentBlockSize = other.m_currentBlockSize;
        m_size = other.m_size;
        m_capacity = other.m_capacity;
        m_alloc = std::move(other.m_alloc);
        m_numberOfEmptyBlock = other.m_numberOfEmptyBlock;

        other.release();
    }

    return *this;
}



template<typename T, typename ALLOCATOR>
void mylib::FreeList<T, ALLOCATOR>::
    release() noexcept
{
    m_currentBlockSize = MIN_BLOCK_SIZE;
    m_size = 0;
    m_capacity = 0;
    m_numberOfEmptyBlock = 0;
}



template<typename T, typename ALLOCATOR>
void mylib::FreeList<T, ALLOCATOR>::
    remove(T* ptr)
{
    if(ptr == nullptr )
    {
        return;
    }

    typename Block::Node* blockNode{ reinterpret_cast<Block::Node*>(ptr) };

    assert(blockNode->userData);

    typename BlockList::Iterator it{ reinterpret_cast<BlockList::BaseNode*>(blockNode->userData) };

    Block* block{ it.operator->() };
    block->remove(blockNode);
    --m_size;
    m_blocks.moveToBegin(it);

    if(block->empty())
    {
        ++m_numberOfEmptyBlock;
    }


    if(block->empty() && m_numberOfEmptyBlock > 1)
    {
        m_capacity -= block->capacity();
        --m_numberOfEmptyBlock;
        m_blocks.erase(m_blocks.begin());
    }
}


#endif // FREE_LIST_H
