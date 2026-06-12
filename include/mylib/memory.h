#ifndef MEMORY_H_INCLUDED
#define MEMORY_H_INCLUDED

#include <new>
#include <cstddef>

namespace memory
{
    template<typename T>
    T* rawMemory(std::size_t size)
    {
        return static_cast<T*>(::operator new(sizeof(T) * size,
                                              std::align_val_t{ alignof(T) }));
    }

    template<typename T>
    void rawDelete(T* array)
    {
        ::operator delete(array, std::align_val_t{ alignof(T) });
    }
}

#endif // MEMORY_H_INCLUDED
