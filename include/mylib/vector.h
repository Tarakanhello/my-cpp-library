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

        int m_capacity{};
        int m_size{};


        T* m_data{ nullptr };

        void updateCapacity();

        void allocate(const T& value = T());

        template<typename Z>
        void allocate(const Z& values);

        void deallocate();
        void reallocate(const Vector<T>& other);
        void reset();
        void resize(int newSize);

    public:
        Vector();
        explicit Vector(int size, const T& value = T());
        explicit Vector(const std::initializer_list<T>& list);

        Vector(const Vector<T>& other);
        Vector<T>& operator=(const Vector<T>& other);

        Vector(Vector<T>&& other) noexcept;
        Vector<T>& operator=(Vector<T>&& other) noexcept;

        ~Vector();

        T* getArray();
        const T* getArray() const;

        T* data();
        const T* data() const;

        int size() const;
        int getSize() const;

        bool empty() const;

        T& operator[](int i);
        const T& operator[](int i) const;
    };

} // end namespace

#endif // VECTOR_H
