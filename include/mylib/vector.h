#ifndef VECTOR_H
#define VECTOR_H

#include <initializer_list>

#include "mylib/structs.h"

namespace mylib
{
    template<typename T>
    class Vector final : public mylib::ArithmeticType<Vector<T>>
    {
    private:
        enum{ MinCapacity = 8 };

        size_t m_capacity{};
        size_t m_size{};


        T* m_data{ nullptr };



        size_t calculateNewCapacity(size_t requiredSize) const;
        void constructElements(size_t from, size_t to, const T& value = T());
        void constructElementsFromRange(size_t from, const T* src, size_t count);
        void destroyElements(size_t from, size_t to) noexcept;
        void deallocate();
        void reallocateBuffer(size_t newCapacity, size_t newSize, const T& value = T());
        void reset();
        void resize(size_t newSize, const T& value = T());
        void swap(Vector& other) noexcept;

    public:
        Vector();
        explicit Vector(size_t size, const T& value = T());
        explicit Vector(const std::initializer_list<T>& list);

        Vector(const Vector<T>& other);
        Vector<T>& operator=(const Vector<T>& other);

        Vector(Vector<T>&& other) noexcept;
        Vector<T>& operator=(Vector<T>&& other) noexcept;

        ~Vector();

        void append(const T& item);

        T* data();
        const T* data() const;

        size_t size() const;

        bool empty() const;

        T& operator[](size_t i);
        const T& operator[](size_t i) const;
    };

} // end namespace

#endif // VECTOR_H
