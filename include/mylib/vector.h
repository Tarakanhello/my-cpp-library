#ifndef VECTOR_H
#define VECTOR_H

#include <initializer_list>
#include <vector>

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

        T* m_data;

    public:
        explicit Vector();
        explicit Vector(int size);
        explicit Vector(int size, const T& value = T());
        explicit Vector(const std::initializer_list<T>& list);

        T* getArray();
        const T* getArray() const;

        T* data();
        const T* data() const;

        int size() const;
        int getSize() const;

        T& operator[](int i);

        const T& operator[](int i) const;
    };

} // end namespace

#endif // VECTOR_H
