#ifndef MEMORY_H_INCLUDED
#define MEMORY_H_INCLUDED

#include <cstddef>
#include <limits>
#include <new>
#include <utility>

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

            if(size > std::numeric_limits<size_t>::max() / sizeof(T))
            {
                throw std::bad_alloc();
            }
            if(auto p {static_cast<T*>(::operator new(sizeof(T) * size,
                                                   std::align_val_t{ alignof(T) })) })
            {
                return p;
            }

            throw std::bad_alloc();
        }


        void deallocate(T* array, [[maybe_unused]] size_t capacity)
        {
            if(array)
            {
                ::operator delete(array, std::align_val_t{ alignof(T) });
                array = nullptr;
            }
        }

        // Конструирование объекта в выделенной памяти (placement new)
        template< typename U, typename... ARGS>
        void construct(U* place, ARGS&&... args)
        {
            new (place) U(std::forward<ARGS>(args)...);
        }

        // Уничтожение объекта без освобождения памяти
        template<typename U>
        void destroy(U* p) noexcept
        {
            p->~U();
        }

    };

    template<typename T, typename U>
    bool operator==(const MySimpleAllocator<T>&, const MySimpleAllocator<U>&) { return true; }

    template<typename T, typename U>
    bool operator!=(const MySimpleAllocator<T>&, const MySimpleAllocator<U>&) { return false; }
} // end namespace mylib

#endif // MEMORY_H_INCLUDED
