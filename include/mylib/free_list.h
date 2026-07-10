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
    };


} // end namespace mylib;


#endif // FREE_LIST_H
