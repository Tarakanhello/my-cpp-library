#ifndef SPEED_TESTER_H
#define SPEED_TESTER_H

#include <chrono>
#include <concepts>
#include <utility>

namespace mylib::utility
{

/**
 * @brief A utility functor for measuring the execution time of a callable.
 *
 * @tparam FUNCTION The type of the callable. Must be invocable with no arguments.
 */
template<std::invocable<> FUNCTION>
struct SpeedTester
{
private:
    // mutable позволяет вызывать не-const operator()
    // даже внутри const-методов SpeedTester
    mutable FUNCTION m_f;

public:
    explicit SpeedTester(FUNCTION f)
        : m_f(std::move(f))
    {}

    double operator()() const // <-- Оставляем const, чтобы SpeedTester был "чистым"
    {
        using namespace std::chrono;

        const auto start = steady_clock::now();

        m_f(); // Вызывается ЛЮБАЯ функция (даже mutable)

        const auto end = steady_clock::now();

        return duration<double>(end - start).count();
    }
};

template<typename FUNCTION>
SpeedTester(FUNCTION) -> SpeedTester<FUNCTION>;
}


#endif // SPEED_TESTER_H
