#ifndef FREE_LIST_H
#define FREE_LIST_H

#include "mylib/list.h"
#include "mylib/memory.h"
#include "mylib/vector.h"

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

        using BLOCK_ALLOCATOR = typename std::allocator_traits<ALLOCATOR>::template rebind_alloc<Block>;
        using ITEM_ALLOCATOR  = typename std::allocator_traits<ALLOCATOR>::template rebind_alloc<Block::Item>;

        mylib::List<Block, BLOCK_ALLOCATOR> m_blocks;

    };


    template<typename T, typename ALLOCATOR>
    struct FreeList<T, ALLOCATOR>::
        Block final
    {


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
