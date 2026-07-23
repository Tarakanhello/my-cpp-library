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

#include "mylib/mylib.h"
using namespace Catch::Matchers;

namespace
{

// Вспомогательная функция: превращает очередь в вектор (порядок от front к back)
template <typename T, typename Alloc>
std::vector<T> toVector(const mylib::Queue<T, Alloc>& q)
{
    std::vector<T> vec;
    vec.reserve(q.size());
    auto copy = q;                  // копия очереди
    while (!copy.empty())
    {
        vec.push_back(copy.front());
        copy.pop();
    }
    return vec;
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
// Тесты для mylib::Queue
// ============================================================================

TEST_CASE("Queue: Constructors, empty, size", "[queue][construction]")
{
    SECTION("Default constructor")
    {
        mylib::Queue<int> q;
        REQUIRE(q.empty());
        REQUIRE(q.size() == 0);
    }

    SECTION("Constructor with allocator")
    {
        mylib::MySimpleAllocator<int> alloc;
        mylib::Queue<int, mylib::MySimpleAllocator<int>> q(alloc);
        REQUIRE(q.empty());
        REQUIRE(q.size() == 0);
    }

    SECTION("Copy constructor")
    {
        mylib::Queue<int> q1;
        q1.push(1);
        q1.push(2);
        q1.push(3);

        mylib::Queue<int> q2(q1);
        REQUIRE(q2.size() == 3);
        REQUIRE(toVector(q2) == std::vector<int>{1, 2, 3});
        // Изменение копии не влияет на оригинал
        q2.push(4);
        REQUIRE(q1.size() == 3);
        REQUIRE(q2.size() == 4);
        REQUIRE(toVector(q1) == std::vector<int>{1, 2, 3});
        REQUIRE(toVector(q2) == std::vector<int>{1, 2, 3, 4});
    }

    SECTION("Move constructor")
    {
        mylib::Queue<int> q1;
        q1.push(1);
        q1.push(2);
        mylib::Queue<int> q2(std::move(q1));
        REQUIRE(q2.size() == 2);
        REQUIRE(q2.front() == 1);
        REQUIRE(q2.back() == 2);
        REQUIRE(toVector(q2) == std::vector<int>{1, 2});
        REQUIRE(q1.empty());
        REQUIRE(q1.size() == 0);
        // q1 можно использовать заново
        q1.push(10);
        REQUIRE(q1.size() == 1);
        REQUIRE(q1.front() == 10);
        REQUIRE(q1.back() == 10);
    }

    SECTION("Destructor – no explicit check (memory leak detection via sanitizers)")
    {
        {
            mylib::Queue<int> q;
            for (int i = 0; i < 1000; ++i)
            {
                q.push(i);
            }
            REQUIRE(q.size() == 1000);
        }
        SUCCEED("Destructor completed without crashes");
    }

}

TEST_CASE("Queue: push, pop, front, back", "[queue][modifiers]")
{
    SECTION("push(const T&)")
    {
        mylib::Queue<int> q;
        int val = 42;
        q.push(val);
        REQUIRE(q.size() == 1);
        REQUIRE(q.front() == 42);
        REQUIRE(q.back() == 42);
        REQUIRE(toVector(q) == std::vector<int>{42});
    }

    SECTION("push(T&&)")
    {
        mylib::Queue<std::string> q;
        std::string s = "hello";
        q.push(std::move(s));
        REQUIRE(q.size() == 1);
        REQUIRE(q.front() == "hello");
        REQUIRE(q.back() == "hello");
        // после перемещения s находится в допустимом, но неопределённом состоянии
    }

    SECTION("push multiple elements")
    {
        mylib::Queue<int> q;
        q.push(1);
        q.push(2);
        q.push(3);
        REQUIRE(q.size() == 3);
        REQUIRE(q.front() == 1);
        REQUIRE(q.back() == 3);
        REQUIRE(toVector(q) == std::vector<int>{1, 2, 3});
    }

    SECTION("pop()")
    {
        mylib::Queue<int> q;
        q.push(1);
        q.push(2);
        q.push(3);
        q.pop();
        REQUIRE(q.size() == 2);
        REQUIRE(q.front() == 2);
        REQUIRE(q.back() == 3);
        REQUIRE(toVector(q) == std::vector<int>{2, 3});
        q.pop();
        q.pop();
        REQUIRE(q.empty());
    }

    SECTION("front() – non‑const version")
    {
        mylib::Queue<int> q;
        q.push(10);
        q.front() = 20;
        REQUIRE(q.front() == 20);
        REQUIRE(toVector(q) == std::vector<int>{20});
    }

    SECTION("front() – const version")
    {
        const mylib::Queue<int> q = [] {
            mylib::Queue<int> qq;
            qq.push(5);
            return qq;
        }();
        REQUIRE(q.front() == 5);
    }

    SECTION("back() – non‑const version")
    {
        mylib::Queue<int> q;
        q.push(10);
        q.push(20);
        q.back() = 30;
        REQUIRE(q.back() == 30);
        REQUIRE(toVector(q) == std::vector<int>{10, 30});
    }

    SECTION("back() – const version")
    {
        const mylib::Queue<int> q = [] {
            mylib::Queue<int> qq;
            qq.push(5);
            qq.push(7);
            return qq;
        }();
        REQUIRE(q.back() == 7);
    }

    SECTION("pop() front() back() in empty queue")
    {
        mylib::Queue<int> q;
        REQUIRE_THROWS_AS(q.pop(), std::out_of_range);
        REQUIRE_THROWS_AS(q.front(), std::out_of_range);
        REQUIRE_THROWS_AS(q.back(), std::out_of_range);
    }
}

TEST_CASE("Queue: clear", "[queue][clear]")
{
    SECTION("clear()")
    {
        mylib::Queue<int> q;
        for (int i = 0; i < 10; ++i) q.push(i);
        REQUIRE(q.size() == 10);
        q.clear();
        REQUIRE(q.empty());
        REQUIRE(q.size() == 0);
        // можно снова добавлять
        q.push(100);
        REQUIRE(q.size() == 1);
        REQUIRE(q.front() == 100);
        REQUIRE(q.back() == 100);
    }
}

TEST_CASE("Queue: Assignment operators (copy and move)", "[queue][assignment]")
{
    SECTION("Copy assignment")
    {
        mylib::Queue<int> src;
        src.push(1);
        src.push(2);
        mylib::Queue<int> dst;
        dst.push(10);
        dst = src;
        REQUIRE(dst.size() == 2);
        REQUIRE(toVector(dst) == std::vector<int>{1, 2});
        // самоприсваивание
        dst = dst;
        REQUIRE(dst.size() == 2);
        REQUIRE(toVector(dst) == std::vector<int>{1, 2});
        // присваивание пустой очереди
        mylib::Queue<int> empty;
        dst = empty;
        REQUIRE(dst.empty());
        REQUIRE(dst.size() == 0);
    }

    SECTION("Move assignment")
    {
        mylib::Queue<int> src;
        src.push(1);
        src.push(2);
        mylib::Queue<int> dst;
        dst.push(10);
        dst = std::move(src);
        REQUIRE(dst.size() == 2);
        REQUIRE(toVector(dst) == std::vector<int>{1, 2});
        REQUIRE(src.empty());
        // самоприсваивание перемещением
        dst = std::move(dst);
        REQUIRE(dst.size() == 2);
        REQUIRE(toVector(dst) == std::vector<int>{1, 2});
        // перемещение из пустой
        mylib::Queue<int> empty;
        dst = std::move(empty);
        REQUIRE(dst.empty());
        REQUIRE(empty.empty());
    }
}

TEST_CASE("Queue: Comparisons (== and <=>)", "[queue][comparison]")
{
    SECTION("operator==")
    {
        mylib::Queue<int> q1, q2;
        REQUIRE(q1 == q2);

        q1.push(1);
        REQUIRE(q1 != q2);

        q2.push(1);
        REQUIRE(q1 == q2);

        q1.push(2);
        q2.push(3);
        REQUIRE(q1 != q2);
    }

    SECTION("operator<=> (lexicographical)")
    {
        mylib::Queue<int> q1, q2;
        REQUIRE((q1 <=> q2) == 0);

        q1.push(1);
        REQUIRE((q1 <=> q2) > 0);   // 1 > empty

        q2.push(1);
        REQUIRE((q1 <=> q2) == 0);

        q1.push(2);
        q2.push(3);
        REQUIRE((q1 <=> q2) < 0);   // 1,2 < 1,3

        q1.push(4);
        q2.push(3); // теперь q1: 1,2,4 ; q2: 1,3,3 -> q1 < q2
        REQUIRE((q1 <=> q2) < 0);
    }
}

TEST_CASE("Queue: Exception safety", "[queue][exceptions]")
{
    SECTION("Allocator throws on allocate during push")
    {
        using Queue = mylib::Queue<int, ThrowingAllocator<int>>;
        Queue q;
        for (int i = 0; i < 8; ++i)
            q.push(i);
        REQUIRE(q.size() == 8);

        ThrowingAllocator<int>::throwOnAllocate = true;
        REQUIRE_THROWS_AS(q.push(42), std::bad_alloc);
        REQUIRE(q.size() == 8);          // состояние не изменилось
        ThrowingAllocator<int>::throwOnAllocate = false;
        q.push(100);
        REQUIRE(q.size() == 9);
        REQUIRE(q.back() == 100);
    }

    SECTION("ThrowOnCopy during push (copy constructor throws)")
    {
        mylib::Queue<ThrowOnCopy> q;
        ThrowOnCopy::resetCounters();
        ThrowOnCopy::throwOnCopy = true;
        ThrowOnCopy val(42);
        REQUIRE_THROWS_AS(q.push(val), std::runtime_error);
        REQUIRE(q.empty());
        ThrowOnCopy::throwOnCopy = false;
        q.push(ThrowOnCopy(10));
        REQUIRE(q.size() == 1);
        REQUIRE(q.front().value == 10);
        REQUIRE(q.back().value == 10);
    }

    // Для очереди сложнее проверить strong exception safety при внутренней реаллокации,
    // но можно попытаться: заполнить до размера, при котором оба стека имеют определенные размеры,
    // и при push вызвать исключение при копировании.
    SECTION("Strong exception safety during internal reallocation")
    {
        mylib::Queue<ThrowOnCopy> q;
        ThrowOnCopy::resetCounters();
        ThrowOnCopy::throwOnCopy = false;
        // Добавим некоторое количество элементов, чтобы при следующем push произошла реаллокация в одном из стеков.
        // Точное число зависит от реализации, но мы можем просто много добавить.
        for (int i = 0; i < 20; ++i)
            q.push(ThrowOnCopy(i));
        REQUIRE(q.size() == 20);

        ThrowOnCopy::throwOnCopy = true;
        ThrowOnCopy newVal(99);
        REQUIRE_THROWS_AS(q.push(newVal), std::runtime_error);

        // Сбрасываем флаг, чтобы последующие операции (например, toVector) не выбрасывали
        ThrowOnCopy::throwOnCopy = false;

        // очередь должна остаться неизменной
        REQUIRE(q.size() == 20);
        auto vec = toVector(q);
        std::vector<int> values;
        for (const auto& x : vec)
            values.push_back(x.value);
        std::vector<int> expected(20);
        std::iota(expected.begin(), expected.end(), 0);
        REQUIRE(values == expected);

        ThrowOnCopy::throwOnCopy = false;
        q.push(ThrowOnCopy(20));
        REQUIRE(q.size() == 21);
        REQUIRE(q.back().value == 20);
    }
}

TEST_CASE("Queue: Move‑only types", "[queue][moveonly]")
{
    SECTION("push and pop with std::unique_ptr")
    {
        mylib::Queue<std::unique_ptr<int>> q;
        q.push(std::make_unique<int>(5));
        q.push(std::make_unique<int>(6));
        REQUIRE(q.size() == 2);
        // забираем первый элемент через перемещение из front
        std::unique_ptr<int> p = std::move(q.front());
        q.pop();
        REQUIRE(q.size() == 1);
        REQUIRE(*p == 5);
        REQUIRE(*q.front() == 6);
        REQUIRE(*q.back() == 6);
    }
}

TEST_CASE("Queue: Integration – random operations compared to vector", "[queue][integration]")
{
    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> distOp(0, 2); // 0=push, 1=pop, 2=front/back
    std::uniform_int_distribution<int> distVal(1, 100);

    mylib::Queue<int> q;
    std::vector<int> ref;   // эталон, используем push_back/pop_front (но для сравнения удобнее хранить в векторе и удалять первый)

    const int NUM_OPS = 1000;
    for (int i = 0; i < NUM_OPS; ++i)
    {
        int op = distOp(rng);
        if (op == 0)        // push
        {
            int val = distVal(rng);
            q.push(val);
            ref.push_back(val);
        }
        else if (op == 1)   // pop
        {
            if (!q.empty())
            {
                q.pop();
                ref.erase(ref.begin());
            }
        }
        else                // front и back (чтение)
        {
            if (!q.empty())
            {
                REQUIRE(q.front() == ref.front());
                REQUIRE(q.back() == ref.back());
            }
        }
        REQUIRE(q.size() == ref.size());
        // периодически проверяем полное содержимое
        if (i % 100 == 0)
        {
            REQUIRE(toVector(q) == ref);
        }
    }

}

TEST_CASE("Queue: Mixed operations with stacks", "[queue][internal]")
{
    // Проверяем, что после чередования push/pop порядок сохраняется.
    mylib::Queue<int> q;
    q.push(1);
    q.push(2);
    q.push(3);
    REQUIRE(toVector(q) == std::vector<int>{1, 2, 3});

    q.pop();
    REQUIRE(toVector(q) == std::vector<int>{2, 3});

    q.push(4);
    REQUIRE(toVector(q) == std::vector<int>{2, 3, 4});

    q.pop();
    q.pop();
    REQUIRE(toVector(q) == std::vector<int>{4});

    q.pop();
    REQUIRE(q.empty());

    // Проверка back после pop
    q.push(10);
    q.push(20);
    REQUIRE(q.back() == 20);
    q.pop();
    REQUIRE(q.back() == 20); // теперь front=20, back=20
    q.pop();
    REQUIRE(q.empty());
}

TEST_CASE("Queue: Const correctness", "[queue][const]")
{
    mylib::Queue<int> q;
    q.push(1);
    q.push(2);
    const auto& cq = q;

    REQUIRE(cq.front() == 1);
    REQUIRE(cq.back() == 2);
    REQUIRE(cq.size() == 2);
    REQUIRE(!cq.empty());

    // Сравнение константных очередей
    mylib::Queue<int> q2;
    q2.push(1);
    q2.push(2);
    const auto& cq2 = q2;
    REQUIRE(cq == cq2);
    REQUIRE((cq <=> cq2) == 0);
}

TEST_CASE("Queue: Allocator propagation", "[queue][allocator]")
{
    // Проверяем, что конструктор с аллокатором работает и что аллокатор копируется/перемещается.
    using Alloc = mylib::MySimpleAllocator<int>;
    Alloc alloc;
    mylib::Queue<int, Alloc> q(alloc);
    REQUIRE(q.empty());

    // Копирование очереди с аллокатором
    q.push(1);
    mylib::Queue<int, Alloc> q2(q);
    REQUIRE(toVector(q2) == std::vector<int>{1});

    // Присваивание
    mylib::Queue<int, Alloc> q3;
    q3 = q;
    REQUIRE(toVector(q3) == std::vector<int>{1});
}

TEST_CASE("Queue: Large data and performance (smoke)", "[queue][performance]")
{
    const int N = 100000;
    mylib::Queue<int> q;
    for (int i = 0; i < N; ++i)
        q.push(i);
    REQUIRE(q.size() == N);
    REQUIRE(q.front() == 0);
    REQUIRE(q.back() == N - 1);

    for (int i = 0; i < N / 2; ++i)
        q.pop();
    REQUIRE(q.size() == N / 2);
    REQUIRE(q.front() == N / 2);

    // Добавим ещё
    for (int i = 0; i < N / 2; ++i)
        q.push(i + N);
    REQUIRE(q.size() == N);
    REQUIRE(q.front() == N / 2);
    REQUIRE(q.back() == N + N/2 - 1);

    // Проверим несколько случайных элементов в середине
    auto vec = toVector(q);
    REQUIRE(vec.size() == N);
    REQUIRE(vec[0] == N / 2);
    REQUIRE(vec[vec.size() - 1] == N + N/2 - 1);
}

