#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include <algorithm>
#include <iterator>
#include <list>
#include <numeric>
#include <queue>
#include <random>
#include <stack>
#include <string>
#include <type_traits>
#include <vector>
#include <memory>
#include <utility>

#include "mylib/mylib.h"   // предполагается, что здесь определён mylib::Stack

using namespace Catch::Matchers;

namespace
{

// Вспомогательная функция: превращает стек в вектор (порядок от основания к вершине)
template <typename T, typename Alloc>
std::vector<T> toVector(const mylib::Stack<T, Alloc>& st)
{
    std::vector<T> vec;
    vec.reserve(st.size());
    auto copy = st;                  // копия стека
    while (!copy.empty())
    {
        vec.push_back(copy.top());
        copy.pop();
    }
    std::reverse(vec.begin(), vec.end());   // восстанавливаем порядок вставки
    return vec;
}

// Проверка инварианта: size <= capacity
template <typename T, typename Alloc>
bool isConsistent(const mylib::Stack<T, Alloc>& st)
{
    return st.size() <= st.capacity();
}

// -------------------------------------------------------------------
// Тип с подсчётом копирований и возможностью выбрасывать исключения
// -------------------------------------------------------------------
struct ThrowOnCopy
{
    int value;
    static int copyCount;
    static int moveCount;
    static bool throwOnCopy;

    ThrowOnCopy(int v = 0) : value(v) {}

    ThrowOnCopy(const ThrowOnCopy& other) : value(other.value)
    {
        ++copyCount;
        if (throwOnCopy)
        {
            throw std::runtime_error("copy constructor throws");
        }
    }

    ThrowOnCopy(ThrowOnCopy&& other) noexcept : value(other.value)
    {
        ++moveCount;
    }

    ThrowOnCopy& operator=(const ThrowOnCopy& other)
    {
        ++copyCount;
        if (throwOnCopy)
        {
            throw std::runtime_error("copy assignment throws");
        }
        value = other.value;
        return *this;
    }

    ThrowOnCopy& operator=(ThrowOnCopy&& other) noexcept
    {
        ++moveCount;
        value = other.value;
        return *this;
    }

    bool operator==(const ThrowOnCopy& other) const
    {
        return value == other.value;
    }

    bool operator!=(const ThrowOnCopy& other) const
    {
        return !(*this == other);
    }

    static void resetCounters()
    {
        copyCount = 0;
        moveCount = 0;
        throwOnCopy = false;
    }
};

int ThrowOnCopy::copyCount = 0;
int ThrowOnCopy::moveCount = 0;
bool ThrowOnCopy::throwOnCopy = false;

std::ostream& operator<<(std::ostream& os, const ThrowOnCopy& t)
{
    os << t.value;
    return os;
}

// -------------------------------------------------------------------
// Аллокатор, который может выбрасывать исключения при выделении
// -------------------------------------------------------------------
struct ThrowingAllocatorBase
{
    static inline bool throwOnAllocate = false;
};

template <typename T>
class ThrowingAllocator : public ThrowingAllocatorBase
{
public:
    using value_type = T;
    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type;

    ThrowingAllocator() = default;

    template <typename U>
    ThrowingAllocator(const ThrowingAllocator<U>&) noexcept
    {
    }

    T* allocate(size_t n)
    {
        if (throwOnAllocate)
        {
            throw std::bad_alloc();
        }
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }

    void deallocate(T* p, size_t) noexcept
    {
        ::operator delete(p);
    }

    bool operator==(const ThrowingAllocator&) const { return true; }
    bool operator!=(const ThrowingAllocator&) const { return false; }
};

} // namespace anonymous

// ============================================================================
// Тесты для mylib::Stack
// ============================================================================

TEST_CASE("Stage 1: Constructors, capacity, empty, size", "[stack][construction]")
{
    SECTION("Default constructor")
    {
        mylib::Stack<int> st;
        REQUIRE(st.empty());
        REQUIRE(st.size() == 0);
        REQUIRE(st.capacity() == 8);        // начальная ёмкость 8
        REQUIRE(isConsistent(st));
    }

    SECTION("Constructor with capacity")
    {
        mylib::Stack<int> st(5);
        REQUIRE(st.empty());
        REQUIRE(st.size() == 0);
        REQUIRE(st.capacity() == 5);
        REQUIRE(isConsistent(st));

        mylib::Stack<int> st0(0);
        REQUIRE(st0.empty());
        REQUIRE(st0.capacity() == 0);
    }

    SECTION("Copy constructor")
    {
        mylib::Stack<int> st1;
        st1.push(1);
        st1.push(2);
        st1.push(3);
        mylib::Stack<int> st2(st1);
        REQUIRE(st2.size() == 3);
        REQUIRE(toVector(st2) == std::vector<int>{1, 2, 3});
        REQUIRE(st2.capacity() == st1.capacity());
        // Изменение копии не влияет на оригинал
        st2.push(4);
        REQUIRE(st1.size() == 3);
        REQUIRE(st2.size() == 4);
        REQUIRE(toVector(st1) == std::vector<int>{1, 2, 3});
        REQUIRE(toVector(st2) == std::vector<int>{1, 2, 3, 4});
    }

    SECTION("Move constructor")
    {
        mylib::Stack<int> st1;
        st1.push(1);
        st1.push(2);
        mylib::Stack<int> st2(std::move(st1));
        REQUIRE(st2.size() == 2);
        REQUIRE(toVector(st2) == std::vector<int>{1, 2});
        REQUIRE(st1.empty());
        REQUIRE(st1.size() == 0);
        // st1 можно использовать заново
        st1.push(10);
        REQUIRE(st1.size() == 1);
        REQUIRE(st1.top() == 10);
    }

    SECTION("Destructor – no explicit check (memory leak detection via sanitizers)")
    {
        {
            mylib::Stack<int> st(1000);
            for (int i = 0; i < 1000; ++i) st.push(i);
            REQUIRE(st.size() == 1000);
        }
        SUCCEED("Destructor completed without crashes");
    }
}

TEST_CASE("Stage 2: push, pop, top", "[stack][modifiers]")
{
    SECTION("push(const T&)")
    {
        mylib::Stack<int> st;
        int val = 42;
        st.push(val);
        REQUIRE(st.size() == 1);
        REQUIRE(st.top() == 42);
        REQUIRE(toVector(st) == std::vector<int>{42});
    }

    SECTION("push(T&&)")
    {
        mylib::Stack<std::string> st;
        std::string s = "hello";
        st.push(std::move(s));
        REQUIRE(st.size() == 1);
        REQUIRE(st.top() == "hello");
        // после перемещения s находится в допустимом, но неопределённом состоянии
    }

    SECTION("Capacity growth (doubling)")
    {
        mylib::Stack<int> st;   // capacity = 8
        for (int i = 0; i < 20; ++i)
        {
            st.push(i);
        }
        REQUIRE(st.size() == 20);
        // 8 -> 16 -> 32
        REQUIRE(st.capacity() == 32);
        REQUIRE(st.top() == 19);
        std::vector<int> expected(20);
        std::iota(expected.begin(), expected.end(), 0);
        REQUIRE(toVector(st) == expected);
    }

    SECTION("pop()")
    {
        mylib::Stack<int> st;
        st.push(1);
        st.push(2);
        st.push(3);
        st.pop();
        REQUIRE(st.size() == 2);
        REQUIRE(st.top() == 2);
        REQUIRE(toVector(st) == std::vector<int>{1, 2});
        st.pop();
        st.pop();
        REQUIRE(st.empty());
        // ёмкость не уменьшается после pop
        REQUIRE(st.capacity() == 8);
    }

    SECTION("top() – non‑const version")
    {
        mylib::Stack<int> st;
        st.push(10);
        st.top() = 20;
        REQUIRE(st.top() == 20);
        REQUIRE(toVector(st) == std::vector<int>{20});
    }

    SECTION("top() – const version")
    {
        const mylib::Stack<int> st = [] {
            mylib::Stack<int> s;
            s.push(5);
            return s;
        }();
        REQUIRE(st.top() == 5);
        // st.top() возвращает const int&
    }

    // Попытка вызова top/pop на пустом стеке приводит к assert – не тестируем.
}

TEST_CASE("Stage 3: clear and shrink_to_fit", "[stack][clear][shrink]")
{
    SECTION("clear()")
    {
        mylib::Stack<int> st;
        for (int i = 0; i < 10; ++i) st.push(i);
        REQUIRE(st.size() == 10);
        REQUIRE(st.capacity() == 16);
        st.clear();
        REQUIRE(st.empty());
        REQUIRE(st.size() == 0);
        REQUIRE(st.capacity() == 16);      // ёмкость не уменьшается
        // можно снова добавлять
        st.push(100);
        REQUIRE(st.size() == 1);
        REQUIRE(st.top() == 100);
    }

    SECTION("shrink_to_fit()")
    {
        mylib::Stack<int> st;
        for (int i = 0; i < 20; ++i) st.push(i);
        REQUIRE(st.capacity() == 32);
        // удаляем половину
        for (int i = 0; i < 10; ++i) st.pop();
        REQUIRE(st.size() == 10);
        REQUIRE(st.capacity() == 32);
        st.shrink_to_fit();
        REQUIRE(st.capacity() == 10);
        REQUIRE(toVector(st) == std::vector<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
    }

    SECTION("shrink_to_fit() on empty stack")
    {
        mylib::Stack<int> st;
        st.push(1);
        st.push(2);
        st.clear();
        st.shrink_to_fit();
        REQUIRE(st.capacity() == 0);
        REQUIRE(st.empty());
        // можно снова добавлять
        st.push(5);
        REQUIRE(st.capacity() >= 1);
        REQUIRE(st.top() == 5);
    }
}

TEST_CASE("Stage 4: Assignment operators (copy and move)", "[stack][assignment]")
{
    SECTION("Copy assignment")
    {
        mylib::Stack<int> src;
        src.push(1);
        src.push(2);
        mylib::Stack<int> dst;
        dst.push(10);
        dst = src;
        REQUIRE(dst.size() == 2);
        REQUIRE(toVector(dst) == std::vector<int>{1, 2});
        REQUIRE(dst.capacity() == src.capacity());
        // самоприсваивание
        dst = dst;
        REQUIRE(dst.size() == 2);
        REQUIRE(toVector(dst) == std::vector<int>{1, 2});
        // присваивание пустого стека
        mylib::Stack<int> empty;
        dst = empty;
        REQUIRE(dst.empty());
        REQUIRE(dst.size() == 0);
    }

    SECTION("Move assignment")
    {
        mylib::Stack<int> src;
        src.push(1);
        src.push(2);
        mylib::Stack<int> dst;
        dst.push(10);
        dst = std::move(src);
        REQUIRE(dst.size() == 2);
        REQUIRE(toVector(dst) == std::vector<int>{1, 2});
        REQUIRE(src.empty());
        // самоприсваивание перемещением
        dst = std::move(dst);
        REQUIRE(dst.size() == 2);
        REQUIRE(toVector(dst) == std::vector<int>{1, 2});
        // перемещение из пустого
        mylib::Stack<int> empty;
        dst = std::move(empty);
        REQUIRE(dst.empty());
        REQUIRE(empty.empty());
    }
}

TEST_CASE("Stage 5: Exception safety", "[stack][exceptions]")
{
    SECTION("Allocator throws on allocate during push")
    {
        using Stack = mylib::Stack<int, ThrowingAllocator<int>>;
        Stack st;
        for (int i = 0; i < 8; ++i)
            st.push(i);
        REQUIRE(st.size() == 8);
        REQUIRE(st.capacity() == 8);

        ThrowingAllocator<int>::throwOnAllocate = true;
        REQUIRE_THROWS_AS(st.push(42), std::bad_alloc);
        REQUIRE(st.size() == 8);
        ThrowingAllocator<int>::throwOnAllocate = false;
        st.push(100);
        REQUIRE(st.size() == 9);
        REQUIRE(st.top() == 100);
    }

    SECTION("ThrowOnCopy during push (copy constructor throws)")
    {
        mylib::Stack<ThrowOnCopy> st;
        ThrowOnCopy::resetCounters();
        ThrowOnCopy::throwOnCopy = true;
        ThrowOnCopy val(42);
        REQUIRE_THROWS_AS(st.push(val), std::runtime_error);
        REQUIRE(st.empty());
        ThrowOnCopy::throwOnCopy = false;
        st.push(ThrowOnCopy(10));
        REQUIRE(st.size() == 1);
        REQUIRE(st.top().value == 10);
    }

    SECTION("Strong exception safety during reallocation")
    {
        mylib::Stack<ThrowOnCopy> st;
        ThrowOnCopy::resetCounters();
        ThrowOnCopy::throwOnCopy = false;
        // заполняем до предела (ёмкость 8)
        for (int i = 0; i < 8; ++i)
            st.push(ThrowOnCopy(i));
        REQUIRE(st.size() == 8);
        REQUIRE(st.capacity() == 8);

        // следующий push вызовет реаллокацию, которая копирует существующие элементы
        ThrowOnCopy::throwOnCopy = true;
        ThrowOnCopy newVal(99);
        REQUIRE_THROWS_AS(st.push(newVal), std::runtime_error);

        // Сбрасываем флаг, чтобы последующие операции (например, toVector) не выбрасывали
        ThrowOnCopy::throwOnCopy = false;

        // стек должен остаться неизменным
        REQUIRE(st.size() == 8);
        REQUIRE(st.capacity() == 8);
        auto vec = toVector(st);
        std::vector<int> values;
        for (const auto& x : vec)
            values.push_back(x.value);
        REQUIRE(values == std::vector<int>{0, 1, 2, 3, 4, 5, 6, 7});

        ThrowOnCopy::throwOnCopy = false;
        st.push(ThrowOnCopy(8));
        REQUIRE(st.size() == 9);
        REQUIRE(st.capacity() == 16);   // удвоение
    }
}

TEST_CASE("Stage 6: Move‑only types", "[stack][moveonly]")
{
    SECTION("push and pop with std::unique_ptr")
    {
        mylib::Stack<std::unique_ptr<int>> st;
        st.push(std::make_unique<int>(5));
        st.push(std::make_unique<int>(6));
        REQUIRE(st.size() == 2);
        // забираем верхний элемент через перемещение из top
        std::unique_ptr<int> p = std::move(st.top());
        st.pop();
        REQUIRE(st.size() == 1);
        REQUIRE(*p == 6);
        REQUIRE(*st.top() == 5);
    }
}

TEST_CASE("Stage 7: Integration – random operations compared to vector", "[stack][integration]")
{
    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> distOp(0, 2); // 0=push, 1=pop, 2=top
    std::uniform_int_distribution<int> distVal(1, 100);

    mylib::Stack<int> st;
    std::vector<int> ref;   // эталон, используем push_back/pop_back

    const int NUM_OPS = 1000;
    for (int i = 0; i < NUM_OPS; ++i)
    {
        int op = distOp(rng);
        if (op == 0)        // push
        {
            int val = distVal(rng);
            st.push(val);
            ref.push_back(val);
        }
        else if (op == 1)   // pop
        {
            if (!st.empty())
            {
                st.pop();
                ref.pop_back();
            }
        }
        else                // top (чтение)
        {
            if (!st.empty())
            {
                REQUIRE(st.top() == ref.back());
            }
        }
        REQUIRE(st.size() == ref.size());
        REQUIRE(isConsistent(st));
        // периодически проверяем полное содержимое
        if (i % 100 == 0)
        {
            REQUIRE(toVector(st) == ref);
        }
    }
}

TEST_CASE("Stage 8: Additional checks for capacity and shrink", "[stack][capacity]")
{
    SECTION("Capacity after copy and move")
    {
        mylib::Stack<int> st;
        for (int i = 0; i < 10; ++i) st.push(i);
        REQUIRE(st.capacity() == 16);

        mylib::Stack<int> copy(st);
        REQUIRE(copy.capacity() == 16);

        mylib::Stack<int> moved(std::move(st));
        REQUIRE(moved.capacity() == 16);
        REQUIRE(st.capacity() == 0);   // moved‑from usually has zero capacity
    }

    SECTION("shrink_to_fit after clear and push again")
    {
        mylib::Stack<int> st;
        for (int i = 0; i < 20; ++i) st.push(i);
        st.clear();
        st.shrink_to_fit();
        REQUIRE(st.capacity() == 0);
        st.push(1);
        st.push(2);
        REQUIRE(st.capacity() >= 2);
        REQUIRE(toVector(st) == std::vector<int>{1, 2});
    }
}

TEST_CASE("Stage 9: Stack with std::string (non‑trivial type)", "[stack][string]")
{
    mylib::Stack<std::string> st;
    st.push("first");
    st.push("second");
    REQUIRE(st.size() == 2);
    REQUIRE(st.top() == "second");
    st.pop();
    REQUIRE(st.top() == "first");
    st.push("third");
    std::vector<std::string> expected{"first", "third"};
    REQUIRE(toVector(st) == expected);
}
