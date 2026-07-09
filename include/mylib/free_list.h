#ifndef FREE_LIST_H
#define FREE_LIST_H

#include "mylib/list.h"
#include "mylib/memory.h"

namespace mylib
{
    template<typename T, typename ALLOCATOR = mylib::MySimpleAllocator<T>>
    class FreeList final
    {
    private:
        static constexpr size_t MIN_BLOCK_SIZE{ 8 };
        static constexpr size_t MAX_BLOCK_SIZE{ 8192 };
        static constexpr size_t DEFAULT_SIZE{ 32 };

        struct Block;

        List<Block, ALLOCATOR> m_blocks;

        T* allocate()
        {

        }

    };


    template<typename T, typename ALLOCATOR>
    struct FreeList<T, ALLOCATOR>::
        Block final
    {
        struct Item;

        using ITEM_ALLOCATOR = typename std::allocator_traits<ALLOCATOR>::template rebind_alloc<Item>;
        using ALLOC_TRAITS = std::allocator_traits<ITEM_ALLOCATOR>;

        Item* returned; // head of free Items;
        Item* unused;   // unused part

        ITEM_ALLOCATOR itemAllocator;

        // size <= maxSize <= capacity
        size_t size{};
        size_t maxSize{};
        size_t capacity{};

        Block(size_t theCapacity, ALLOCATOR alloc = ALLOCATOR())
            : capacity{ theCapacity }
            , size{}
            , maxSize{}
            , returned{ nullptr }
            , itemAllocator{ alloc }
            , unused{ ALLOC_TRAITS::allocate(itemAllocator, theCapacity) }
        {}


        bool isFull() const { return size == capacity; }
        bool empty() const { return size == 0; }

        explicit operator bool() const noexcept { return !empty(); }
    };


    template<typename T, typename ALLOCATOR>
    struct FreeList<T, ALLOCATOR>::Block::
        Item final
    {
        T item;

        union
        {
            Item* next; // *this is free, pointer to next free Item;
            void* userData; // *this is full, pointer to owner of this Item;
        };
    };

} // end namespace mylib;


#endif // FREE_LIST_H
