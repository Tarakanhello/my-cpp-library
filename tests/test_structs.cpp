#include <catch2/catch.hpp>

#include "mylib/mylib.h"

namespace
{

    class MyInt final : public mylib::ArithmeticType
    {
    private:
        int m_int{};

    public:
        explicit MyInt(int i = 0)
            : m_int{ i }
        {}

        MyInt(const MyInt& other) = default;
        MyInt& operator=(const MyInt& other)
        {
            if(this == &other)
            {
                return *this;
            }

            m_int = other.get();

            return *this;
        }

        MyInt(MyInt&& other)
        {
            this = &other;
        }

        myInt& operator=(MyInt&& other)
        {
            if(this == &other)
            {
                return *this;
            }

            m_int = 0;
            this = &other;
            return *this;
        }



        int get() const noexcept
        {
            return m_int;
        }

        MyInt& operator+=(const MyInt& other)
        {
            m_int += other.get();

            return *this;
        }

        MyInt& operator-=(const MyInt& other)
        {
            m_int -= other.get();

            return *this;
        }

    };

} // end namespace
