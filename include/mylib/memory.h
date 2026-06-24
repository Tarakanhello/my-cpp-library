#ifndef MEMORY_H_INCLUDED
#define MEMORY_H_INCLUDED

#include <new>
#include <cstddef>

namespace mylib
{
    namespace memory
    {
        template<typename T>
        T* rawMemory(std::size_t size)
        {
            if(size == 0)
            {
                return nullptr;
            }

            return static_cast<T*>(::operator new(sizeof(T) * size,
                                                  std::align_val_t{ alignof(T) }));
        }

        template<typename T>
        void rawDelete(T* array)
        {
            if(array)
            {
                ::operator delete(array, std::align_val_t{ alignof(T) });
                array = nullptr;
            }
        }

        template<typename T>
        void rawDestruct(T* array, size_t size)
        {
            if(array)
            {
                for(size_t i{}; i < size; ++i)
                {
                    array[i].~T();
                }

                rawDelete(array);
            }
        }
    } // end namespace memory

    template<typename T>
    class MySimpleAllocator
    {
    public:
        using value_type = T;

        MySimpleAllocator() noexcept = default;

        template<typename U>
        MySimpleAllocator(const MySimpleAllocator<U>&) noexcept {}

        T* allocate(size_t size)
        {
            if(size == 0)
            {
                return nullptr;
            }

            return static_cast<T*>(::operator new(sizeof(T) * size,
                                                   std::align_val_t{ alignof(T) }));
        }


        void deallocate(T* array, [[maybe_unused]] size_t capacity)
        {
            if(array)
            {
                ::operator delete(array, std::align_val_t{ alignof(T) });
                array = nullptr;
            }
        }
    };


} // end namespace mylib

#endif // MEMORY_H_INCLUDED
