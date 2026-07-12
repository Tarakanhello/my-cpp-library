#ifndef FREE_LIST_H
#define FREE_LIST_H

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
        enum class BlockSizes
        {
            MIN_BLOCK_SIZE = 8,
            MAX_BLOCK_SIZE = 8192,
            DEFAULT_SIZE = 32,
            UNDEFAULT_SIZE = 0
        };

    private:
        using Block = StaticFreeList<T, ALLOCATOR>;
        using BlockAllocator = typename std::allocator_traits<ALLOCATOR>::template rebind_alloc<Block>;
        using BlockList = mylib::List<Block, BlockAllocator>;

        BlockList m_blocks{};
        size_t m_currentBlockSize{ BlockSizes::MIN_BLOCK_SIZE };
        size_t m_totalSize{};


        Block* getFirstNonFullBlock();
        void createNewBlock();
        void moveBlockToFront(typename BlockList::iterator it);

    public:
        FreeList() = default;
        explicit FreeList(size_t initialSize = BlockSizes::DEFAULT_SIZE, ALLOCATOR alloc = MySimpleAllocator<T>());

        FreeList(const FreeList&) = delete;
        FreeList& operator=(const FreeList&) = delete;

        FreeList(FreeList&& other) noexcept;
        FreeList& operator=(FreeList&& other) noexcept;

        ~FreeList() noexcept;

        T* allocate();
        void remove(T* ptr);

        bool empty() const noexcept { return m_blocks.empty(); }
        size_t size() const noexcept { return m_totalSize; }
        size_t blockCount() const noexcept { return m_blocks.size(); }
    };


} // end namespace mylib;


#endif // FREE_LIST_H
