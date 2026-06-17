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

        void updateCapacity(size_t size);

        void allocate(const T& value = T());

        template<typename Z>
        void allocate(const Z& values);

        void deallocate();
        void reallocate(const Vector<T>& other);
        void reset();
        void resize(size_t newSize);

    public:
        Vector();
        explicit Vector(size_t size, const T& value = T());
        explicit Vector(const std::initializer_list<T>& list);

        Vector(const Vector<T>& other);
        Vector<T>& operator=(const Vector<T>& other);

        Vector(Vector<T>&& other) noexcept;
        Vector<T>& operator=(Vector<T>&& other) noexcept;

        ~Vector();



        T* data();
        const T* data() const;

        size_t size() const;

        bool empty() const;

        T& operator[](size_t i);
        const T& operator[](size_t i) const;
    };

} // end namespace

#endif // VECTOR_H
