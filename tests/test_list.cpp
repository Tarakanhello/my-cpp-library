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

#include "mylib/mylib.h"

using namespace Catch::Matchers;

namespace
{

    template <typename T>
    std::vector<T> toVector(const mylib::List<T>& lst)
    {
        std::vector<T> vec;
        vec.reserve(lst.size());
        for (auto it = lst.begin(); it != lst.end(); ++it)
        {
            vec.push_back(*it);
        }
        return vec;
    }


    template <typename T>
    bool isConsistent(const mylib::List<T>& lst)
    {
        size_t count{ 0 };
        if (lst.empty())
        {
            // For empty list, begin() == end()
        }
        auto it{ lst.begin() };
        while (it != lst.end())
        {
            ++count;
            ++it;
        }
        return (count == lst.size());
    }

    // -------------------------------------------------------------------
    // 1. Тип с подсчётом копирований и возможностью выбрасывать исключения
    // -------------------------------------------------------------------
    struct ThrowOnCopy
    {
        int value;
        static int copyCount;
        static int moveCount;
        static bool throwOnCopy;   // флаг: выбрасывать ли исключение при копировании

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
            // перемещение не выбрасывает
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

    // Специализация для вывода в Catch (для REQUIRE)
    std::ostream& operator<<(std::ostream& os, const ThrowOnCopy& t)
    {
        os << t.value;
        return os;
    }

    // -------------------------------------------------------------------
    // 2. Аллокатор, который может выбрасывать исключения при выделении
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
        ThrowingAllocator(const ThrowingAllocator<U>&) noexcept {}

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

} //end namespace



TEST_CASE("Stage 1: Constructors, destructor, size, empty, front/back", "[list][construction]")
{
    // Общие переменные, используемые во всех секциях
    const int defaultVal{ 42 };
    const std::string strVal{ "Hello" };

    SECTION("Default constructor")
    {
        mylib::List<int> lst;
        REQUIRE(lst.empty());
        REQUIRE(lst.size() == 0);
        REQUIRE_FALSE(lst); // explicit operator bool
        // front/back не вызываем, так как список пуст (assert не должен срабатывать в тестах)
    }


    SECTION("Count and value constructor (int)")
    {
        SECTION("count = 0")
        {
            mylib::List<int> lst(0, 5);
            REQUIRE(lst.empty());
            REQUIRE(lst.size() == 0);
        }
        SECTION("count = 1")
        {
            mylib::List<int> lst(1, 42);
            REQUIRE_FALSE(lst.empty());
            REQUIRE(lst.size() == 1);
            REQUIRE(lst.front() == 42);
            REQUIRE(lst.back() == 42);
            // Проверка через итераторы
            auto vec{ toVector(lst) };
            REQUIRE(vec == std::vector<int>{42});
        }
        SECTION("count = 5")
        {
            mylib::List<int> lst(5, 7);
            REQUIRE(lst.size() == 5);
            auto vec{ toVector(lst) };
            REQUIRE(vec == std::vector<int>(5, 7));
            // Проверка front/back
            REQUIRE(lst.front() == 7);
            REQUIRE(lst.back() == 7);
        }
        SECTION("with non‑default type (std::string)")
        {
            mylib::List<std::string> lst(3, strVal);
            REQUIRE(lst.size() == 3);
            auto vec{ toVector(lst) };
            REQUIRE(vec == std::vector<std::string>(3, strVal));
        }
    }

    SECTION("Initializer_list constructor")
    {
        mylib::List<int> lst{ 1, 2, 3, 4, 5 };
        REQUIRE(lst.size() == 5);
        REQUIRE_FALSE(lst.empty());
        auto vec{ toVector(lst) };
        REQUIRE(vec == std::vector<int>{1, 2, 3, 4, 5});
        REQUIRE(lst.front() == 1);
        REQUIRE(lst.back() == 5);
        // Проверка константных версий front/back
        const auto& constLst{ lst };
        REQUIRE(constLst.front() == 1);
        REQUIRE(constLst.back() == 5);
    }

    SECTION("Range constructor (from iterators)")
    {
        std::vector<int> src{ 10, 20, 30, 40 };
        mylib::List<int> lst(src.begin(), src.end());
        REQUIRE(lst.size() == 4);
        auto vec{ toVector(lst) };
        REQUIRE(vec == src);
        // Из другого списка
        mylib::List<int> another{ 100, 200 };
        mylib::List<int> lst2(another.begin(), another.end());
        REQUIRE(toVector(lst2) == std::vector<int>{100, 200});
    }

    SECTION("Copy constructor")
    {
        mylib::List<int> original{ 1, 2, 3 };
        mylib::List<int> copy(original);
        REQUIRE(copy.size() == original.size());
        REQUIRE(toVector(copy) == toVector(original));
        // Изменение копии не влияет на оригинал
        copy.push_back(4);
        REQUIRE(original.size() == 3);
        REQUIRE(toVector(original) == std::vector<int>{1, 2, 3});
        REQUIRE(toVector(copy) == std::vector<int>{1, 2, 3, 4});
        // Проверка, что копия имеет свой собственный аллокатор (неявно)
    }

    SECTION("Move constructor")
    {
        mylib::List<int> original{ 5, 6, 7 };
        mylib::List<int> moved(std::move(original));
        REQUIRE(moved.size() == 3);
        REQUIRE(toVector(moved) == std::vector<int>{5, 6, 7});
        // Исходный список должен быть пустым (валидным)
        REQUIRE(original.empty());
        REQUIRE(original.size() == 0);
        // Можно проверить, что original можно использовать заново
        original.push_back(10);
        REQUIRE(original.size() == 1);
        REQUIRE(original.front() == 10);
    }

    SECTION("Destructor (memory leak check)")
    {
        // В данном тесте мы просто создаём объект в блоке, деструктор вызовется автоматически.
        // Утечки можно проверить с помощью санитайзеров, здесь мы просто проверяем, что
        // после уничтожения список корректно освобождает память.
        // Для этого можно создать список с большим количеством элементов и позволить ему выйти из области видимости.
        {
            mylib::List<int> big(1000, 1);
            REQUIRE(big.size() == 1000);
        }
        // Если бы была утечка, санитайзер бы сообщил. В коде теста мы ничего не проверяем, кроме того, что деструктор отработал.
        SUCCEED("Destructor completed without crashes");
    }

    SECTION("front() and back() for non‑empty list (including const versions)")
    {
        mylib::List<int> lst{ 10, 20, 30 };
        // Не константные версии
        REQUIRE(lst.front() == 10);
        REQUIRE(lst.back() == 30);
        // Изменяем через ссылки
        lst.front() = 100;
        lst.back() = 300;
        REQUIRE(toVector(lst) == std::vector<int>{100, 20, 300});
        // Константные версии
        const mylib::List<int>& constLst{ lst };
        REQUIRE(constLst.front() == 100);
        REQUIRE(constLst.back() == 300);
        // Проверка, что constLst.front() возвращает константную ссылку (нельзя присвоить)
        // Это проверяется на этапе компиляции.
    }

    SECTION("explicit operator bool")
    {
        mylib::List<int> empty;
        REQUIRE_FALSE(static_cast<bool>(empty));
        mylib::List<int> nonEmpty{ 1 };
        REQUIRE(static_cast<bool>(nonEmpty));
    }

}




TEST_CASE("Stage 2: push_front, push_back, pop_front, pop_back", "[list][modifiers]")
{
    // Common data
    const int val1{ 10 };
    const int val2{ 20 };
    const int val3{ 30 };
    const std::string s1{ "one" };
    const std::string s2{ "two" };

    SECTION("push_front(const T&)")
    {
        mylib::List<int> lst;
        lst.push_front(val1);
        REQUIRE(lst.size() == 1);
        REQUIRE(lst.front() == val1);
        REQUIRE(lst.back() == val1);

        lst.push_front(val2);
        REQUIRE(lst.size() == 2);
        REQUIRE(toVector(lst) == std::vector<int>{val2, val1});
        REQUIRE(lst.front() == val2);
        REQUIRE(lst.back() == val1);
    }

    SECTION("push_front(T&&)")
    {
        mylib::List<std::string> lst;
        std::string temp{ s1 };
        lst.push_front(std::move(temp));
        REQUIRE(lst.size() == 1);
        REQUIRE(lst.front() == s1);

        lst.push_front(std::string(s2));
        REQUIRE(toVector(lst) == std::vector<std::string>{s2, s1});
    }

    SECTION("push_back(const T&)")
    {
        mylib::List<int> lst;
        lst.push_back(val1);
        REQUIRE(lst.size() == 1);
        REQUIRE(lst.front() == val1);
        REQUIRE(lst.back() == val1);

        lst.push_back(val2);
        REQUIRE(lst.size() == 2);
        REQUIRE(toVector(lst) == std::vector<int>{val1, val2});
        REQUIRE(lst.front() == val1);
        REQUIRE(lst.back() == val2);
    }

    SECTION("push_back(T&&)")
    {
        mylib::List<std::string> lst;
        std::string temp{ s1 };
        lst.push_back(std::move(temp));
        REQUIRE(lst.size() == 1);
        REQUIRE(lst.back() == s1);
        lst.push_back(std::string(s2));
        REQUIRE(toVector(lst) == std::vector<std::string>{s1, s2});
    }

    SECTION("Combination of push_front and push_back")
    {
        mylib::List<int> lst;
        lst.push_back(1);
        lst.push_front(0);
        lst.push_back(2);
        lst.push_front(-1);
        REQUIRE(toVector(lst) == std::vector<int>{-1, 0, 1, 2});
        REQUIRE(lst.size() == 4);
    }

    SECTION("pop_front()")
    {
        mylib::List<int> lst{ 1, 2, 3 };

        int val{ lst.pop_front() };
        REQUIRE(val == 1);
        REQUIRE(lst.size() == 2);
        REQUIRE(toVector(lst) == std::vector<int>{2, 3});
        REQUIRE(lst.front() == 2);
        REQUIRE(lst.back() == 3);

        val = lst.pop_front();
        REQUIRE(val == 2);
        REQUIRE(lst.size() == 1);
        REQUIRE(toVector(lst) == std::vector<int>{3});
        REQUIRE(lst.front() == 3);
        REQUIRE(lst.back() == 3);

        val = lst.pop_front();
        REQUIRE(val == 3);
        REQUIRE(lst.empty());
        REQUIRE(lst.size() == 0);
    }

    SECTION("pop_back()")
    {
        mylib::List<int> lst{ 1, 2, 3 };

        int val{ lst.pop_back() };
        REQUIRE(val == 3);
        REQUIRE(lst.size() == 2);
        REQUIRE(toVector(lst) == std::vector<int>{1, 2});
        REQUIRE(lst.front() == 1);
        REQUIRE(lst.back() == 2);

        val = lst.pop_back();
        REQUIRE(val == 2);
        REQUIRE(lst.size() == 1);
        REQUIRE(toVector(lst) == std::vector<int>{1});
        REQUIRE(lst.front() == 1);
        REQUIRE(lst.back() == 1);

        val = lst.pop_back();
        REQUIRE(val == 1);
        REQUIRE(lst.empty());
    }

    SECTION("Mixed pop_front and pop_back")
    {
        mylib::List<int> lst{ 1, 2, 3, 4, 5 };
        REQUIRE(lst.pop_front() == 1);
        REQUIRE(lst.pop_back() == 5);
        REQUIRE(toVector(lst) == std::vector<int>{2, 3, 4});
        REQUIRE(lst.pop_front() == 2);
        REQUIRE(lst.pop_back() == 4);
        REQUIRE(toVector(lst) == std::vector<int>{3});
        REQUIRE(lst.pop_front() == 3);
        REQUIRE(lst.empty());
    }

    SECTION("Move semantics in pop (return by value)")
    {
        mylib::List<std::string> lst;
        lst.push_back(std::string("move me"));
        std::string result{ lst.pop_back() };
        REQUIRE(result == "move me");
        REQUIRE(lst.empty());


        mylib::List<std::string> lst2;
        lst2.push_front(std::string("front"));
        std::string result2{ lst2.pop_front() };
        REQUIRE(result2 == "front");
        REQUIRE(lst2.empty());
    }

    // Примечание: тесты на вызов pop из пустого списка опущены, так как они приводят к assert.
}



TEST_CASE("Stage 3: Iterators (mutable, const, reverse)", "[list][iterators]")
{
    // Общие данные
    mylib::List<int> lst{ 10, 20, 30, 40, 50 };
    const mylib::List<int> constLst{ 1, 2, 3, 4, 5 };

    SECTION("begin() and end() for empty list")
    {
        mylib::List<int> empty;
        REQUIRE(empty.begin() == empty.end());
        REQUIRE(empty.cbegin() == empty.cend());
    }

    SECTION("Iterate forward with mutable iterators")
    {
        std::vector<int> expected{ 10, 20, 30, 40, 50 };
        std::vector<int> actual;
        for (auto it = lst.begin(); it != lst.end(); ++it)
        {
            actual.push_back(*it);
        }
        REQUIRE(actual == expected);
    }

    SECTION("Iterate forward with const iterators (cbegin/cend)")
    {
        std::vector<int> expected{ 1, 2, 3, 4, 5 };
        std::vector<int> actual;
        for (auto it = constLst.cbegin(); it != constLst.cend(); ++it)
        {
            actual.push_back(*it);
        }
        REQUIRE(actual == expected);
    }

    SECTION("Mutable iterators allow modification")
    {
        for (auto it = lst.begin(); it != lst.end(); ++it)
        {
            *it *= 2;
        }
        REQUIRE(toVector(lst) == std::vector<int>{20, 40, 60, 80, 100});
    }

    SECTION("Const iterators provide read-only access")
    {

        auto it = constLst.begin();
        REQUIRE(*it == 1);
        // *it = 10; // ошибка компиляции
    }

    SECTION("Reverse iterators (rbegin/rend)")
    {
        std::vector<int> expected{ 50, 40, 30, 20, 10 };
        std::vector<int> actual;
        for (auto it = lst.rbegin(); it != lst.rend(); ++it)
        {
            actual.push_back(*it);
        }
        REQUIRE(actual == expected);
    }

    SECTION("Const reverse iterators (crbegin/crend)")
    {
        std::vector<int> expected{ 5, 4, 3, 2, 1 };
        std::vector<int> actual;
        for (auto it = constLst.crbegin(); it != constLst.crend(); ++it)
        {
            actual.push_back(*it);
        }
        REQUIRE(actual == expected);
    }

    SECTION("Reverse iterators allow modification (through rbegin)")
    {
        for (auto it = lst.rbegin(); it != lst.rend(); ++it)
        {
            *it += 1;
        }
        REQUIRE(toVector(lst) == std::vector<int>{ 11, 21, 31, 41, 51 });
    }

    SECTION("Iterator increment/decrement operations")
    {
        auto it{ lst.begin() };
        REQUIRE(*it == 10);
        ++it;                       // префиксный инкремент
        REQUIRE(*it == 20);
        it++;                       // постфиксный инкремент
        REQUIRE(*it == 30);
        --it;                       // префиксный декремент
        REQUIRE(*it == 20);
        it--;                       // постфиксный декремент
        REQUIRE(*it == 10);
    }

    SECTION("Iterator comparison (==, !=)")
    {
        auto it1{ lst.begin() };
        auto it2{ lst.begin() };
        REQUIRE(it1 == it2);
        ++it2;
        REQUIRE(it1 != it2);
    }

    SECTION("Using algorithms with iterators")
    {
        // std::find
        auto found{ std::find(lst.begin(), lst.end(), 30) };
        REQUIRE(found != lst.end());
        REQUIRE(*found == 30);
        auto notFound{ std::find(lst.begin(), lst.end(), 99) };
        REQUIRE(notFound == lst.end());

        // std::distance
        auto dist{ std::distance(lst.begin(), lst.end()) };
        REQUIRE(dist == 5);
        dist = std::distance(lst.begin(), found);
        REQUIRE(dist == 2);

        // std::advance
        auto it{ lst.begin() };
        std::advance(it, 3);
        REQUIRE(*it == 40);


        auto it2{ std::next(lst.begin(), 2) };
        REQUIRE(*it2 == 30);
        auto it3{std::prev(it2) };
        REQUIRE(*it3 == 20);

        // std::reverse (работает с двунаправленными итераторами)
        std::reverse(lst.begin(), lst.end());
        REQUIRE(toVector(lst) == std::vector<int>{50, 40, 30, 20, 10});
        // обратно
        std::reverse(lst.begin(), lst.end());
        REQUIRE(toVector(lst) == std::vector<int>{10, 20, 30, 40, 50});
    }

    SECTION("Iterator conversion: Iterator -> ConstIterator")
    {
        mylib::List<int>::Iterator it{ lst.begin() };
        mylib::List<int>::ConstIterator cit{ it }; // неявное преобразование
        REQUIRE(cit == lst.cbegin());
        // cit можно разыменовать только для чтения
        REQUIRE(*cit == 10);
        // *cit = 100; // ошибка компиляции
    }

    SECTION("ConstIterator can be compared with Iterator")
    {
        auto it{ lst.begin() };
        auto cit{ lst.cbegin() };
        REQUIRE(it == cit); // сравнение разных типов допустимо
        REQUIRE(cit == it);
    }

    SECTION("Post-increment and post-decrement return old value")
    {
        auto it{ lst.begin() };
        auto old{ it++ };
        REQUIRE(*old == 10);
        REQUIRE(*it == 20);
        old = it--;
        REQUIRE(*old == 20);
        REQUIRE(*it == 10);
    }
}




TEST_CASE("Stage 4: Insert and erase by position", "[list][insert][erase]")
{
    // Общие данные
    const int valA{ 100 };
    const int valB{ 200 };
    const std::string strVal{ "inserted" };

    SECTION("insert at beginning (pos == begin())")
    {
        mylib::List<int> lst{ 1, 2, 3 };
        auto it{ lst.insert(lst.begin(), valA) };
        REQUIRE(lst.size() == 4);
        REQUIRE(toVector(lst) == std::vector<int>{valA, 1, 2, 3});
        // Проверяем возвращаемый итератор
        REQUIRE(it == lst.begin());
        REQUIRE(*it == valA);
        // Проверяем, что итератор указывает на вставленный элемент
        ++it;
        REQUIRE(*it == 1);
    }

    SECTION("insert at end (pos == end())")
    {
        mylib::List<int> lst{ 1, 2, 3 };
        auto it{ lst.insert(lst.end(), valA) };
        REQUIRE(lst.size() == 4);
        REQUIRE(toVector(lst) == std::vector<int>{1, 2, 3, valA});
        // Возвращённый итератор указывает на последний элемент
        REQUIRE(it == std::prev(lst.end()));
        REQUIRE(*it == valA);
    }

    SECTION("insert in the middle")
    {
        mylib::List<int> lst{ 1, 2, 4, 5 };
        auto pos{ std::next(lst.begin(), 2) }; // указывает на 4 (третий элемент)
        auto it{ lst.insert(pos, valA) };
        REQUIRE(lst.size() == 5);
        REQUIRE(toVector(lst) == std::vector<int>{1, 2, valA, 4, 5});
        // Проверяем возвращаемый итератор
        REQUIRE(*it == valA);
        REQUIRE(it == std::next(lst.begin(), 2));
    }

    SECTION("insert with move (T&&)")
    {
        mylib::List<std::string> lst{ "one", "three" };
        std::string temp{ "two" };
        auto pos{ std::next(lst.begin(), 1) }; // перед "three"
        auto it{ lst.insert(pos, std::move(temp)) };
        REQUIRE(lst.size() == 3);
        REQUIRE(toVector(lst) == std::vector<std::string>{"one", "two", "three"});
        REQUIRE(*it == "two");
        // temp находится в перемещённом состоянии, но мы его не используем
    }

    SECTION("erase first element (pos == begin())")
    {
        mylib::List<int> lst{ 10, 20, 30 };
        auto it{ lst.erase(lst.begin()) };
        REQUIRE(lst.size() == 2);
        REQUIRE(toVector(lst) == std::vector<int>{20, 30});
        REQUIRE(it == lst.begin());
        REQUIRE(*it == 20);
    }

    SECTION("erase last element (pos == --end())")
    {
        mylib::List<int> lst{ 10, 20, 30 };
        auto it{ lst.erase(std::prev(lst.end())) };
        REQUIRE(lst.size() == 2);
        REQUIRE(toVector(lst) == std::vector<int>{ 10, 20 });
        REQUIRE(it == lst.end());
        // Проверка, что end() достижим и не разыменовывается
        REQUIRE(it == lst.end());
    }

    SECTION("erase from the middle")
    {
        mylib::List<int> lst{ 1, 2, 3, 4, 5 };
        auto pos{ std::next(lst.begin(), 2) }; // указывает на 3
        auto it{ lst.erase(pos) };
        REQUIRE(lst.size() == 4);
        REQUIRE(toVector(lst) == std::vector<int>{1, 2, 4, 5});
        REQUIRE(it == std::next(lst.begin(), 2));
        REQUIRE(*it == 4);
    }

    SECTION("erase all elements one by one")
    {
        mylib::List<int> lst{ 1, 2, 3 };
        auto it{ lst.erase(lst.begin()) };
        REQUIRE(lst.size() == 2);
        it = lst.erase(it); // удаляем следующий (2)
        REQUIRE(lst.size() == 1);
        REQUIRE(toVector(lst) == std::vector<int>{3});
        it = lst.erase(it);
        REQUIRE(lst.empty());
        REQUIRE(it == lst.end());
    }

    SECTION("Check return value after insert and erase in sequence")
    {
        mylib::List<int> lst{ 1, 2, 3, 4 };
        auto it{ lst.insert(std::next(lst.begin(), 2), 99) }; // вставляем 99 перед 3
        REQUIRE(toVector(lst) == std::vector<int>{1, 2, 99, 3, 4});
        // Теперь удалим вставленный элемент
        it = lst.erase(it); // it указывает на 99
        REQUIRE(lst.size() == 4);
        REQUIRE(toVector(lst) == std::vector<int>{1, 2, 3, 4});
        // it должен указывать на следующий элемент после удалённого, т.е. на 3
        REQUIRE(*it == 3);
        REQUIRE(it == std::next(lst.begin(), 2));
    }

    SECTION("Insert before end() equivalent to push_back")
    {
        mylib::List<int> lst{ 1, 2 };
        auto it{ lst.insert(lst.end(), 3) };
        REQUIRE(toVector(lst) == std::vector<int>{1, 2, 3});
        REQUIRE(it == std::prev(lst.end()));
    }

    SECTION("Insert at begin() equivalent to push_front")
    {
        mylib::List<int> lst{ 1, 2 };
        auto it{ lst.insert(lst.begin(), 0) };
        REQUIRE(toVector(lst) == std::vector<int>{0, 1, 2});
        REQUIRE(it == lst.begin());
    }

    SECTION("Insert using const_iterator (conversion)")
    {
        mylib::List<int> lst{ 1, 2, 3, 4 };
        mylib::List<int>::ConstIterator pos{ std::next(lst.cbegin(), 2) }; // указывает на 3
        auto it{ lst.insert(pos, 99) }; // insert принимает ConstIterator
        REQUIRE(toVector(lst) == std::vector<int>{1, 2, 99, 3, 4});
        REQUIRE(*it == 99);
    }

    SECTION("Erase using const_iterator")
    {
        mylib::List<int> lst{ 1, 2, 3, 4 };
        mylib::List<int>::ConstIterator pos{ std::next(lst.cbegin(), 2) }; // указывает на 3
        auto it{ lst.erase(pos) }; // erase принимает ConstIterator
        REQUIRE(toVector(lst) == std::vector<int>{1, 2, 4});
        REQUIRE(*it == 4);
    }

    // Дополнительно: проверка, что после erase итератор не указывает на удалённый элемент
    SECTION("Iterator after erase is valid")
    {
        mylib::List<int> lst{ 1, 2, 3 };
        auto it{ lst.begin() };
        ++it; // указывает на 2
        auto after{ lst.erase(it) };
        REQUIRE(after == std::next(lst.begin())); // теперь указывает на 3
        REQUIRE(*after == 3);
    }
}



TEST_CASE("Stage 5: Assignment operators and swap", "[list][assignment][swap]")
{
    // Общие данные
    mylib::List<int> source{ 1, 2, 3, 4, 5 };
    mylib::List<int> empty;

    SECTION("Copy assignment operator (operator=(const List&))")
    {
        mylib::List<int> target{ 10, 20, 30 };
        target = source;
        REQUIRE(target.size() == source.size());
        REQUIRE(toVector(target) == toVector(source));
        // Проверяем, что изменения в target не влияют на source
        target.push_back(6);
        REQUIRE(source.size() == 5);
        REQUIRE(toVector(source) == std::vector<int>{ 1, 2, 3, 4, 5 });
        REQUIRE(toVector(target) == std::vector<int>{ 1, 2, 3, 4, 5, 6 });
    }

    SECTION("Copy assignment: self-assignment")
    {
        mylib::List<int> lst{ 1, 2, 3 };
        // Самоприсваивание должно работать корректно без повреждения данных
        lst = lst;
        REQUIRE(lst.size() == 3);
        REQUIRE(toVector(lst) == std::vector<int>{ 1, 2, 3 });
        // Проверяем, что объект остался валидным
        lst.push_back(4);
        REQUIRE(toVector(lst) == std::vector<int>{ 1, 2, 3, 4 });
    }

    SECTION("Copy assignment: assigning empty list to non-empty")
    {
        mylib::List<int> target{ 1, 2, 3 };
        target = empty;
        REQUIRE(target.empty());
        REQUIRE(target.size() == 0);
        // Можно повторно заполнить
        target.push_back(42);
        REQUIRE(toVector(target) == std::vector<int>{ 42 });
    }

    SECTION("Copy assignment: assigning to empty list")
    {
        mylib::List<int> target;
        target = source;
        REQUIRE(target.size() == source.size());
        REQUIRE(toVector(target) == toVector(source));
    }

    SECTION("Move assignment operator (operator=(List&&))")
    {
        mylib::List<int> target{ 10, 20, 30 };
        mylib::List<int> movedFrom{ 1, 2, 3, 4, 5 };
        target = std::move(movedFrom);
        REQUIRE(target.size() == 5);
        REQUIRE(toVector(target) == std::vector<int>{ 1, 2, 3, 4, 5 });
        // Исходный объект должен быть пустым (валидным)
        REQUIRE(movedFrom.empty());
        REQUIRE(movedFrom.size() == 0);
        // Можно использовать movedFrom заново
        movedFrom.push_back(100);
        REQUIRE(movedFrom.size() == 1);
        REQUIRE(movedFrom.front() == 100);
        // Проверяем, что старые данные target были освобождены
    }

    SECTION("Move assignment: self-assignment")
    {
        mylib::List<int> lst{ 1, 2, 3 };
        // Самоприсваивание перемещением не должно ничего делать
        lst = std::move(lst);
        REQUIRE(lst.size() == 3);
        REQUIRE(toVector(lst) == std::vector<int>{ 1, 2, 3 });
        // Проверяем, что объект остался валидным
        lst.push_back(4);
        REQUIRE(toVector(lst) == std::vector<int>{ 1, 2, 3, 4 });
    }

    SECTION("Move assignment: from empty to non-empty")
    {
        mylib::List<int> target{ 1, 2, 3 };
        mylib::List<int> emptySrc;
        target = std::move(emptySrc);
        REQUIRE(target.empty());
        REQUIRE(emptySrc.empty()); // перемещение из пустого списка в пустой – оба пусты
    }

    SECTION("swap: swap two non-empty lists")
    {
        mylib::List<int> a{ 1, 2, 3 };
        mylib::List<int> b{ 10, 20, 30, 40 };
        a.swap(b);
        REQUIRE(toVector(a) == std::vector<int>{ 10, 20, 30, 40 });
        REQUIRE(toVector(b) == std::vector<int>{ 1, 2, 3 });
        REQUIRE(a.size() == 4);
        REQUIRE(b.size() == 3);
    }

    SECTION("swap: with empty list")
    {
        mylib::List<int> a{ 1, 2 };
        mylib::List<int> b;
        a.swap(b);
        REQUIRE(a.empty());
        REQUIRE(toVector(b) == std::vector<int>{ 1, 2 });
        REQUIRE(a.size() == 0);
        REQUIRE(b.size() == 2);
    }

    SECTION("swap: both empty")
    {
        mylib::List<int> a;
        mylib::List<int> b;
        a.swap(b);
        REQUIRE(a.empty());
        REQUIRE(b.empty());
    }

    SECTION("swap: self-swap (a.swap(a))")
    {
        mylib::List<int> a{ 1, 2, 3 };
        a.swap(a);
        REQUIRE(toVector(a) == std::vector<int>{ 1, 2, 3 });
        // Проверяем, что ничего не сломалось
        a.push_back(4);
        REQUIRE(toVector(a) == std::vector<int>{ 1, 2, 3, 4 });
    }

    SECTION("Copy assignment after swap")
    {
        mylib::List<int> a{ 1, 2 };
        mylib::List<int> b{ 3, 4, 5 };
        a.swap(b);
        REQUIRE(toVector(a) == std::vector<int>{ 3, 4, 5 });
        REQUIRE(toVector(b) == std::vector<int>{ 1, 2 });
        // Затем присваиваем b = a
        b = a;
        REQUIRE(toVector(b) == toVector(a));
        REQUIRE(toVector(b) == std::vector<int>{ 3, 4, 5 });
        // Убеждаемся, что a не изменился
        REQUIRE(toVector(a) == std::vector<int>{ 3, 4, 5 });
    }

    SECTION("Move assignment after move construction")
    {
        mylib::List<int> a{ 1, 2, 3 };
        mylib::List<int> b{ std::move(a) };
        REQUIRE(toVector(b) == std::vector<int>{ 1, 2, 3 });
        REQUIRE(a.empty());
        // Перемещаем b в c
        mylib::List<int> c;
        c = std::move(b);
        REQUIRE(toVector(c) == std::vector<int>{ 1, 2, 3 });
        REQUIRE(b.empty());
        // a, b, c - все валидны, но a и b пусты
        a.push_back(10); // NOLINT(clang-analyzer-cplusplus.Move)
        REQUIRE(toVector(a) == std::vector<int>{ 10 });
        b.push_back(20); // NOLINT(clang-analyzer-cplusplus.Move)
        REQUIRE(toVector(b) == std::vector<int>{ 20 });
    }
}



TEST_CASE("Stage 6: resize and clear", "[list][resize][clear]")
{
    // Общие данные
    const int defaultValue{ 0 };
    const int customValue{ 42 };
    const std::string strDefault{ "" };
    const std::string strCustom{ "filled" };

    SECTION("resize: increase size with default value")
    {
        mylib::List<int> lst{ 1, 2, 3 };
        lst.resize(5);
        REQUIRE(lst.size() == 5);
        REQUIRE(toVector(lst) == std::vector<int>{ 1, 2, 3, 0, 0 });
        // front/back работают корректно
        REQUIRE(lst.front() == 1);
        REQUIRE(lst.back() == 0);
    }

    SECTION("resize: increase size with custom value")
    {
        mylib::List<int> lst{ 1, 2, 3 };
        lst.resize(5, customValue);
        REQUIRE(lst.size() == 5);
        REQUIRE(toVector(lst) == std::vector<int>{ 1, 2, 3, customValue, customValue });
    }

    SECTION("resize: decrease size (truncate)")
    {
        mylib::List<int> lst{ 1, 2, 3, 4, 5 };
        lst.resize(3);
        REQUIRE(lst.size() == 3);
        REQUIRE(toVector(lst) == std::vector<int>{ 1, 2, 3 });
    }

    SECTION("resize: decrease size to zero")
    {
        mylib::List<int> lst{ 1, 2, 3 };
        lst.resize(0);
        REQUIRE(lst.empty());
        REQUIRE(lst.size() == 0);
        lst.push_back(10);
        REQUIRE(toVector(lst) == std::vector<int>{ 10 });
    }

    SECTION("resize: same size (no change)")
    {
        mylib::List<int> lst{ 1, 2, 3 };
        lst.resize(3);
        REQUIRE(lst.size() == 3);
        REQUIRE(toVector(lst) == std::vector<int>{ 1, 2, 3 });
        // Проверяем, что добавление с тем же размером не меняет значения
        lst.resize(3, customValue);
        REQUIRE(toVector(lst) == std::vector<int>{ 1, 2, 3 });
    }

    SECTION("resize: from empty list with default value")
    {
        mylib::List<int> lst;
        lst.resize(4);
        REQUIRE(lst.size() == 4);
        REQUIRE(toVector(lst) == std::vector<int>(4, 0));
    }

    SECTION("resize: from empty list with custom value")
    {
        mylib::List<int> lst;
        lst.resize(4, customValue);
        REQUIRE(lst.size() == 4);
        REQUIRE(toVector(lst) == std::vector<int>(4, customValue));
    }

    SECTION("resize: with non-default type (std::string)")
    {
        mylib::List<std::string> lst{ "a", "b" };
        lst.resize(4, strCustom);
        REQUIRE(lst.size() == 4);
        REQUIRE(toVector(lst) == std::vector<std::string>{ "a", "b", strCustom, strCustom });
        lst.resize(2);
        REQUIRE(toVector(lst) == std::vector<std::string>{ "a", "b" });
    }

    SECTION("resize: large increase (performance)")
    {
        mylib::List<int> lst;
        lst.resize(10000, 7);
        REQUIRE(lst.size() == 10000);
        // Проверяем только первый и последний элемент для скорости
        REQUIRE(lst.front() == 7);
        // Не можем легко получить последний, но можем через итераторы
        auto it{ lst.begin() };
        std::advance(it, 9999);
        REQUIRE(*it == 7);
    }

    SECTION("clear: non-empty list")
    {
        mylib::List<int> lst{ 1, 2, 3, 4, 5 };
        lst.clear();
        REQUIRE(lst.empty());
        REQUIRE(lst.size() == 0);
        // Проверяем, что можно снова использовать
        lst.push_back(10);
        lst.push_back(20);
        REQUIRE(toVector(lst) == std::vector<int>{ 10, 20 });
    }

    SECTION("clear: empty list")
    {
        mylib::List<int> lst;
        lst.clear();
        REQUIRE(lst.empty());
        REQUIRE(lst.size() == 0);
        // Ничего не сломалось
    }

    SECTION("clear: after resize")
    {
        mylib::List<int> lst{ 1, 2, 3 };
        lst.resize(5, 9);
        lst.clear();
        REQUIRE(lst.empty());
        lst.push_back(100);
        REQUIRE(toVector(lst) == std::vector<int>{ 100 });
    }

    SECTION("clear: check memory deallocation (indirectly)")
    {
        // Создаем список, добавляем много элементов, очищаем, затем добавляем новые.
        {
            mylib::List<int> big(1000, 5);
            big.clear();
            REQUIRE(big.empty());
            big.resize(500, 3);
            REQUIRE(big.size() == 500);
        }
        SUCCEED("clear and reuse completed without crashes");
    }

    SECTION("resize with custom value after clear")
    {
        mylib::List<int> lst{ 1, 2, 3 };
        lst.clear();
        lst.resize(3, customValue);
        REQUIRE(toVector(lst) == std::vector<int>(3, customValue));
    }

    SECTION("resize(0) is equivalent to clear")
    {
        mylib::List<int> lst{ 1, 2, 3 };
        lst.resize(0);
        REQUIRE(lst.empty());
        // После resize(0) список пуст, как и после clear
        lst.push_back(5);
        REQUIRE(toVector(lst) == std::vector<int>{ 5 });
    }
}



TEST_CASE("Stage 7: Edge cases, allocator, exception safety", "[list][edge][allocator][exceptions]")
{
    SECTION("Large number of elements (100k)")
    {
        const size_t N{ 100000 };
        mylib::List<int> lst;
        for(size_t i{}; i < N; ++i)
        {
            lst.push_back(static_cast<int>(i));
        }
        REQUIRE(lst.size() == N);
        // Проверяем первый и последний
        REQUIRE(lst.front() == 0);
        auto it{ lst.begin() };
        std::advance(it, N - 1);
        REQUIRE(*it == static_cast<int>(N - 1));
        // Проверяем, что можно пройтись по всем без сбоев
        size_t count{ 0 };
        for(auto x : lst)
        {
            REQUIRE(x == static_cast<int>(count));
            ++count;
        }
        REQUIRE(count == N);
        // Очистка большого списка должна работать
        lst.clear();
        REQUIRE(lst.empty());
    }



    SECTION("Non-trivial type (std::string)")
    {
        mylib::List<std::string> lst;
        for (int i{}; i < 1000; ++i)
        {
            lst.push_back(std::to_string(i));
        }
        REQUIRE(lst.size() == 1000);
        REQUIRE(lst.front() == "0");
        REQUIRE(lst.back() == "999");
        // Копирование и перемещение
        auto copy{ lst };
        REQUIRE(copy.size() == lst.size());
        auto moved{ std::move(lst) };
        REQUIRE(moved.size() == 1000);
        REQUIRE(lst.empty()); // после перемещения оригинал пуст
    }

    SECTION("Copy/move counts for element type")
    {
        ThrowOnCopy::resetCounters();
        ThrowOnCopy::throwOnCopy = false;
        mylib::List<ThrowOnCopy> lst;
        // Добавляем элементы
        ThrowOnCopy a(1);
        ThrowOnCopy b(2);
        lst.push_back(a);     // копирование
        lst.push_back(std::move(b)); // перемещение
        REQUIRE(lst.size() == 2);

        REQUIRE(ThrowOnCopy::copyCount >= 1);
        REQUIRE(ThrowOnCopy::moveCount >= 1);
    }

    SECTION("Exception safety: insert throws during copy")
    {
        ThrowOnCopy::resetCounters();
        ThrowOnCopy::throwOnCopy = true; // копирование будет выбрасывать

        mylib::List<ThrowOnCopy> lst;
        ThrowOnCopy val(42);

        REQUIRE_THROWS_AS(lst.push_back(val), std::runtime_error);
        // Список должен быть пустым и валидным
        REQUIRE(lst.empty());
        REQUIRE(lst.size() == 0);

        // Вставляем в середину (insert)
        mylib::List<ThrowOnCopy> lst2;
        lst2.push_back(ThrowOnCopy(10));
        lst2.push_back(ThrowOnCopy(30)); // [10, 30]
        auto pos{ std::next(lst2.begin()) }; // позиция перед 30
        REQUIRE_THROWS_AS(lst2.insert(pos, val), std::runtime_error);
        // Список должен остаться без изменений
        ThrowOnCopy::throwOnCopy = false;
        REQUIRE(lst2.size() == 2);
        REQUIRE(toVector(lst2)[0].value == 10);
        REQUIRE(toVector(lst2)[1].value == 30);

        // Отключаем исключения для остальных тестов
        ThrowOnCopy::throwOnCopy = false;
    }

    SECTION("Allocator throws on allocate")
    {
        using AllocList = mylib::List<int, ThrowingAllocator<int>>;
        ThrowingAllocator<int>::throwOnAllocate = true;

        AllocList lst;
        // Попытка вставки должна выбросить std::bad_alloc
        REQUIRE_THROWS_AS(lst.push_back(42), std::bad_alloc);
        // Список должен остаться пустым
        REQUIRE(lst.empty());

        // Отключаем исключения для остальных проверок
        ThrowingAllocator<int>::throwOnAllocate = false;
        // После отключения должно работать
        lst.push_back(100);
        REQUIRE(lst.size() == 1);
        REQUIRE(lst.front() == 100);
    }

    SECTION("Allocator propagation on copy/move/swap")
    {
        mylib::List<int> src{ 1, 2, 3 };
        mylib::List<int> dst;
        dst = src; // copy assignment
        REQUIRE(toVector(dst) == toVector(src));

        mylib::List<int> moved{ std::move(src) };
        REQUIRE(toVector(moved) == std::vector<int>{ 1, 2, 3 });
        REQUIRE(src.empty());

        // swap
        mylib::List<int> a{ 10, 20 };
        mylib::List<int> b{ 30, 40, 50 };
        a.swap(b);
        REQUIRE(toVector(a) == std::vector<int>{ 30, 40, 50 });
        REQUIRE(toVector(b) == std::vector<int>{ 10, 20 });
    }

    SECTION("Strong exception safety for insert (non-throwing move)")
    {
        // Тип, который бросает только при копировании, но перемещение безопасно
        ThrowOnCopy::resetCounters();

        ThrowOnCopy::throwOnCopy = false;
        mylib::List<ThrowOnCopy> lst{ ThrowOnCopy(1), ThrowOnCopy(2), ThrowOnCopy(3) };
        auto oldSize{ lst.size() };
        // Теперь включаем флаг и проверяем insert
        ThrowOnCopy::throwOnCopy = true;
        ThrowOnCopy::throwOnCopy = true;
        ThrowOnCopy val(99);
        auto pos{ std::next(lst.begin()) };
        REQUIRE_THROWS_AS(lst.insert(pos, val), std::runtime_error);
        ThrowOnCopy::throwOnCopy = false;
        // Размер и содержимое не должны измениться
        REQUIRE(lst.size() == oldSize);
        REQUIRE(toVector(lst)[0].value == 1);
        REQUIRE(toVector(lst)[1].value == 2);
        REQUIRE(toVector(lst)[2].value == 3);
        // Отключаем исключения
    }

    SECTION("List remains usable after exception")
    {
        ThrowOnCopy::resetCounters();
        ThrowOnCopy::throwOnCopy = true;
        mylib::List<ThrowOnCopy> lst;
        ThrowOnCopy val(5);
        // Первая вставка выбросит исключение
        REQUIRE_THROWS_AS(lst.push_back(val), std::runtime_error);
        REQUIRE(lst.empty());
        // Отключаем исключения
        ThrowOnCopy::throwOnCopy = false;
        // Теперь список должен работать
        lst.push_back(ThrowOnCopy(10));
        lst.push_back(ThrowOnCopy(20));
        REQUIRE(lst.size() == 2);
        REQUIRE(toVector(lst)[0].value == 10);
        REQUIRE(toVector(lst)[1].value == 20);
        // Можно также проверить, что вставка с копированием работает
        ThrowOnCopy val2(30);
        lst.push_back(val2);
        REQUIRE(lst.size() == 3);
        REQUIRE(toVector(lst)[2].value == 30);
    }
}



TEST_CASE("Stage 8: Integration tests (random operations, iterator validity, invariants)",
          "[list][integration]")
{

    SECTION("Random mix of push/pop operations compared to vector")
    {
        // Use a fixed seed for reproducibility
        std::mt19937 rng(12345);
        std::uniform_int_distribution<int> distOp(0, 3); // 0=push_front, 1=push_back, 2=pop_front, 3=pop_back
        std::uniform_int_distribution<int> distValue(1, 100);

        mylib::List<int> lst;
        std::vector<int> ref;

        const int NUM_OPERATIONS{ 1000 };

        for (int i{}; i < NUM_OPERATIONS; ++i)
        {
            int op{ distOp(rng) };
            if (op == 0)
            { // push_front
                int val{ distValue(rng) };
                lst.push_front(val);
                ref.insert(ref.begin(), val);
            }
            else if (op == 1)
            { // push_back
                int val{ distValue(rng) };
                lst.push_back(val);
                ref.push_back(val);
            }
            else if (op == 2)
            { // pop_front
                if (!lst.empty())
                {
                    int val = lst.pop_front();
                    REQUIRE(val == ref.front());
                    ref.erase(ref.begin());
                }
            }
            else
            { // pop_back
                if (!lst.empty())
                {
                    int val = lst.pop_back();
                    REQUIRE(val == ref.back());
                    ref.pop_back();
                }
            }

            // After each operation, check that list is consistent and matches reference vector
            REQUIRE(lst.size() == ref.size());
            REQUIRE(toVector(lst) == ref);
            REQUIRE(isConsistent(lst));
        }
    }

    SECTION("Random insert and erase operations")
    {
        std::mt19937 rng(56789);
        std::uniform_int_distribution<int> distValue(1, 100);
        std::uniform_int_distribution<int> distPos(0, 0); // will be adjusted

        mylib::List<int> lst;
        std::vector<int> ref;

        const int NUM_OPERATIONS = 500;

        for (int i{}; i < NUM_OPERATIONS; ++i)
        {
            // Decide insert or erase
            if (ref.empty() || (std::uniform_int_distribution<int>(0, 1)(rng) == 0))
            {
                // Insert
                int val{ distValue(rng) };
                // Choose a position: 0 .. ref.size() inclusive
                int pos{ std::uniform_int_distribution<int>(0, static_cast<int>(ref.size()))(rng) };
                auto it{ lst.begin() };
                std::advance(it, pos);
                auto newIt = lst.insert(it, val);
                // Check returned iterator points to inserted element
                REQUIRE(*newIt == val);
                ref.insert(ref.begin() + pos, val);
            }
            else
            {
                // Erase
                int pos{ std::uniform_int_distribution<int>(0, static_cast<int>(ref.size()) - 1)(rng) };
                auto it{ lst.begin() };
                std::advance(it, pos);
                auto nextIt{ lst.erase(it) };
                ref.erase(ref.begin() + pos);
                // Returned iterator should point to the next element (or end)
                if (pos < static_cast<int>(ref.size()))
                {
                    REQUIRE(*nextIt == ref[pos]);
                }
                else
                {
                    REQUIRE(nextIt == lst.end());
                }
            }

            REQUIRE(lst.size() == ref.size());
            REQUIRE(toVector(lst) == ref);
            REQUIRE(isConsistent(lst));
        }
    }

    SECTION("Mixed operations with front/back and resize")
    {
        mylib::List<int> lst;
        std::vector<int> ref;

        // Initial fill
        for (int i{}; i < 10; ++i)
        {
            lst.push_back(i);
            ref.push_back(i);
        }

        // Resize larger
        lst.resize(15, 42);
        ref.resize(15, 42);
        REQUIRE(toVector(lst) == ref);
        REQUIRE(isConsistent(lst));

        // Resize smaller
        lst.resize(7);
        ref.resize(7);
        REQUIRE(toVector(lst) == ref);
        REQUIRE(isConsistent(lst));

        // push/pop mixed
        lst.push_front(100);
        ref.insert(ref.begin(), 100);
        lst.push_back(200);
        ref.push_back(200);
        lst.pop_front();
        ref.erase(ref.begin());
        lst.pop_back();
        ref.pop_back();

        REQUIRE(toVector(lst) == ref);
        REQUIRE(isConsistent(lst));
    }

    SECTION("Iterator validity after insert and erase")
    {
        mylib::List<int> lst{ 1, 2, 3, 4, 5 };
        auto it1{ lst.begin() };
        auto it2{ std::next(it1, 2) }; // points to 3
        auto it3{ std::prev(lst.end()) }; // points to 5

        // Insert before it2 (pos = 3)
        auto itNew{ lst.insert(it2, 99) };
        // Check that it1 is still valid (points to 1)
        REQUIRE(*it1 == 1);
        // it2 is now invalid? Actually in a list, insert does not invalidate iterators, except those to erased.
        // But it2 points to the same node? In our case, inserting before it2 does not change it2's node.
        // However, check standard: in list, insert does not invalidate iterators.
        REQUIRE(*it2 == 3); // it2 still points to node containing 3
        // it3 points to 5 (unchanged)
        REQUIRE(*it3 == 5);
        // Check new iterator points to inserted
        REQUIRE(*itNew == 99);

        // Erase the inserted element (at position itNew)
        auto itAfterErase{ lst.erase(itNew) };
        // it1, it2, it3 should still be valid
        REQUIRE(*it1 == 1);
        REQUIRE(*it2 == 3);
        REQUIRE(*it3 == 5);
        // itAfterErase should point to the element after erased (which was it2)
        REQUIRE(*itAfterErase == 3);

        // Erase the middle element (2)
        auto itToErase{ std::next(lst.begin()) }; // points to 2
        auto itAfter{ lst.erase(itToErase) };
        // it1 still valid, it2 now points to 4? Actually it2 originally pointed to 3, but after erasing 2,
        // the node 3 is still there; it2 is still valid and points to 3.
        REQUIRE(*it1 == 1);
        REQUIRE(*it2 == 3);
        // it3 points to 5
        REQUIRE(*it3 == 5);
        REQUIRE(*itAfter == 3); // after erasing 2, next is 3

        // Check overall consistency
        REQUIRE(toVector(lst) == std::vector<int>{1, 3, 4, 5});
        REQUIRE(isConsistent(lst));
    }

    SECTION("Combine multiple operations with swap and move")
    {
        mylib::List<int> a{ 1, 2, 3 };
        mylib::List<int> b{ 10, 20, 30, 40 };

        // Swap
        a.swap(b);
        REQUIRE(toVector(a) == std::vector<int>{ 10, 20, 30, 40 });
        REQUIRE(toVector(b) == std::vector<int>{ 1, 2, 3 });

        // Move assignment
        mylib::List<int> c;
        c = std::move(a);
        REQUIRE(toVector(c) == std::vector<int>{ 10, 20, 30, 40 });
        REQUIRE(a.empty());

        // Modify c and ensure b not affected
        c.push_back(50);
        b.push_front(0);
        REQUIRE(toVector(c) == std::vector<int>{ 10, 20, 30, 40, 50 });
        REQUIRE(toVector(b) == std::vector<int>{ 0, 1, 2, 3 });

        // Move construction
        mylib::List<int> d{ std::move(b) };
        REQUIRE(toVector(d) == std::vector<int>{ 0, 1, 2, 3 });
        REQUIRE(b.empty());
    }

    SECTION("Check invariant after many operations on strings")
    {
        mylib::List<std::string> lst;
        std::vector<std::string> ref;

        // Perform random operations with strings
        std::mt19937 rng(98765);
        std::uniform_int_distribution<int> distOp(0, 3);
        std::uniform_int_distribution<int> distLen(1, 5);

        for (int i{}; i < 200; ++i)
        {
            int op = distOp(rng);
            if (op == 0)
            { // push_front
                std::string val(distLen(rng), 'a' + rng() % 26);
                lst.push_front(val);
                ref.insert(ref.begin(), val);
            }
            else if (op == 1)
            { // push_back
                std::string val(distLen(rng), 'a' + rng() % 26);
                lst.push_back(val);
                ref.push_back(val);
            }
            else if (op == 2)
            { // pop_front
                if (!lst.empty())
                {
                    std::string val{ lst.pop_front() };
                    REQUIRE(val == ref.front());
                    ref.erase(ref.begin());
                }
            }
            else
            { // pop_back
                if (!lst.empty())
                {
                    std::string val = lst.pop_back();
                    REQUIRE(val == ref.back());
                    ref.pop_back();
                }
            }

            REQUIRE(lst.size() == ref.size());
            REQUIRE(toVector(lst) == ref);
            REQUIRE(isConsistent(lst));
        }
    }

    SECTION("Clear after operations and reuse") {
        mylib::List<int> lst;
        for (int i{}; i < 50; ++i)
        {
            lst.push_back(i);
        }
        REQUIRE(lst.size() == 50);

        lst.clear();
        REQUIRE(lst.empty());
        REQUIRE(isConsistent(lst));

        // Re-populate
        for (int i{ 10 }; i < 20; ++i)
        {
            lst.push_back(i);
        }
        std::vector<int> expected(10);
        std::iota(expected.begin(), expected.end(), 10);
        REQUIRE(toVector(lst) == expected);
        REQUIRE(isConsistent(lst));

        // Resize from empty
        lst.resize(5, 99);
        expected = { 10, 11, 12, 13, 14, 99, 99, 99, 99, 99 }; // actually after resize(5) with value 99,
        // but we already had 10 elements, resize to 5 would truncate, not increase.
        // Wait: we had 10 elements (10..19). resize(5) will truncate to first 5.
        lst.resize(5);
        expected = { 10, 11, 12, 13, 14 };
        REQUIRE(toVector(lst) == expected);
        REQUIRE(isConsistent(lst));
    }
}



TEST_CASE("Stage 9: STL compatibility (algorithms, adapters, concepts)", "[list][stl]") {

    SECTION("Non-modifying algorithms")
    {
        mylib::List<int> lst{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

        // std::find
        auto found{ std::find(lst.begin(), lst.end(), 5) };
        REQUIRE(found != lst.end());
        REQUIRE(*found == 5);
        auto notFound{ std::find(lst.begin(), lst.end(), 99) };
        REQUIRE(notFound == lst.end());

        // std::for_each
        int sum{};
        std::for_each(lst.begin(), lst.end(), [&sum](int x) { sum += x; });
        REQUIRE(sum == 55); // 1+2+...+10

        // std::count
        auto cnt{ std::count(lst.begin(), lst.end(), 3) };
        REQUIRE(cnt == 1);

        // std::accumulate (from <numeric>)
        auto acc{ std::accumulate(lst.begin(), lst.end(), 0) };
        REQUIRE(acc == 55);
        auto accMul{ std::accumulate(lst.begin(), lst.end(), 1, std::multiplies<int>()) };
        REQUIRE(accMul == 3628800); // 10!

        // std::equal – compare with vector
        std::vector<int> vec{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        REQUIRE(std::equal(lst.begin(), lst.end(), vec.begin()));

        // std::lexicographical_compare
        mylib::List<int> lst2{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 11 };
        REQUIRE(std::lexicographical_compare(lst.begin(), lst.end(), lst2.begin(), lst2.end()));
        REQUIRE_FALSE(std::lexicographical_compare(lst2.begin(), lst2.end(), lst.begin(), lst.end()));
    }

    SECTION("Modifying algorithms (compatible with bidirectional iterators)")
    {
        mylib::List<int> lst{ 1, 2, 3, 4, 5 };

        // std::reverse
        std::reverse(lst.begin(), lst.end());
        REQUIRE(toVector(lst) == std::vector<int>{ 5, 4, 3, 2, 1 });
        // reverse back
        std::reverse(lst.begin(), lst.end());
        REQUIRE(toVector(lst) == std::vector<int>{ 1, 2, 3, 4, 5 });

        // std::copy – into vector
        std::vector<int> vec(5);
        std::copy(lst.begin(), lst.end(), vec.begin());
        REQUIRE(vec == std::vector<int>{ 1, 2, 3, 4, 5 });

        // std::move – into another list (via inserter)
        mylib::List<int> movedTo;
        std::move(lst.begin(), lst.end(), std::back_inserter(movedTo));
        // After move, elements of lst are in moved-from state, but we can check movedTo
        REQUIRE(toVector(movedTo) == std::vector<int>{1, 2, 3, 4, 5});
        // lst is still valid but its elements are unspecified; we clear it
        lst.clear();

        // std::fill
        mylib::List<int> fillList(5, 0);
        std::fill(fillList.begin(), fillList.end(), 7);
        REQUIRE(toVector(fillList) == std::vector<int>(5, 7));

        // std::generate
        std::generate(fillList.begin(), fillList.end(), []() { static int n = 0; return ++n; });
        REQUIRE(toVector(fillList) == std::vector<int>{1, 2, 3, 4, 5});

        // std::replace
        mylib::List<int> repList = {1, 2, 3, 2, 4, 2, 5};
        std::replace(repList.begin(), repList.end(), 2, 99);
        REQUIRE(toVector(repList) == std::vector<int>{1, 99, 3, 99, 4, 99, 5});

        // std::transform
        mylib::List<int> transList = {1, 2, 3, 4, 5};
        std::transform(transList.begin(), transList.end(), transList.begin(),
                       [](int x) { return x * 2; });
        REQUIRE(toVector(transList) == std::vector<int>{2, 4, 6, 8, 10});
    }

    SECTION("Algorithms with std::back_inserter")
    {
        mylib::List<int> lst = {1, 2, 3};
        mylib::List<int> dest;
        std::copy(lst.begin(), lst.end(), std::back_inserter(dest));
        REQUIRE(toVector(dest) == std::vector<int>{1, 2, 3});
        // Also test with std::back_inserter on empty list
        mylib::List<int> dest2;
        std::fill_n(std::back_inserter(dest2), 5, 42);
        REQUIRE(toVector(dest2) == std::vector<int>(5, 42));
    }

    SECTION("Constructing STL containers from List iterators")
    {
        mylib::List<int> lst = {10, 20, 30, 40, 50};

        // Construct std::vector from List iterators
        std::vector<int> vec(lst.begin(), lst.end());
        REQUIRE(vec == std::vector<int>{10, 20, 30, 40, 50});

        // Construct std::list from List iterators
        std::list<int> stdList(lst.begin(), lst.end());
        REQUIRE(std::equal(lst.begin(), lst.end(), stdList.begin()));

        // Construct our List from std::list iterators (back)
        mylib::List<int> fromStdList(stdList.begin(), stdList.end());
        REQUIRE(toVector(fromStdList) == std::vector<int>{10, 20, 30, 40, 50});
    }

    SECTION("Range-based for loop (already tested, but ensure compatibility)")
    {
        mylib::List<int> lst = {1, 2, 3};
        int sum = 0;
        for (auto x : lst) {
            sum += x;
        }
        REQUIRE(sum == 6);

        for (auto& x : lst) {
            x *= 2;
        }
        REQUIRE(toVector(lst) == std::vector<int>{2, 4, 6});

        const auto& constLst = lst;
        for (auto x : constLst) {
            // x is read-only
            REQUIRE(x % 2 == 0);
        }
    }

    SECTION("STL adapters: std::stack and std::queue")
    {
        // std::stack requires: back(), push_back(), pop_back(), size(), empty()
        // our List provides all of them
        mylib::List<int> lst;
        std::stack<int, mylib::List<int>> stk(lst); // uses our list as underlying container
        stk.push(10);
        stk.push(20);
        stk.push(30);
        REQUIRE(stk.size() == 3);
        REQUIRE(stk.top() == 30);
        stk.pop();
        REQUIRE(stk.top() == 20);
        stk.pop();
        REQUIRE(stk.top() == 10);
        stk.pop();
        REQUIRE(stk.empty());

        // std::queue requires: push_back(), pop_front(), front(), back(), size(), empty()
        mylib::List<int> lst2;
        std::queue<int, mylib::List<int>> que(lst2);
        que.push(1);
        que.push(2);
        que.push(3);
        REQUIRE(que.size() == 3);
        REQUIRE(que.front() == 1);
        REQUIRE(que.back() == 3);
        que.pop();
        REQUIRE(que.front() == 2);
        que.pop();
        REQUIRE(que.front() == 3);
        que.pop();
        REQUIRE(que.empty());
    }

    SECTION("Iterator traits and category")
    {
        using Iter = mylib::List<int>::Iterator;
        using traits = std::iterator_traits<Iter>;
        // Check that iterator_category is bidirectional_iterator_tag
        static_assert(std::is_same_v<traits::iterator_category, std::bidirectional_iterator_tag>,
                      "Iterator must be bidirectional");
        // Check that value_type, pointer, reference are correct
        static_assert(std::is_same_v<traits::value_type, int>);
        static_assert(std::is_same_v<traits::pointer, int*>);
        static_assert(std::is_same_v<traits::reference, int&>);

        // Check const iterator as well
        using ConstIter = mylib::List<int>::ConstIterator;
        using traitsConst = std::iterator_traits<ConstIter>;
        static_assert(std::is_same_v<traitsConst::iterator_category, std::bidirectional_iterator_tag>);
        static_assert(std::is_same_v<traitsConst::value_type, int>);
        static_assert(std::is_same_v<traitsConst::pointer, const int*>);
        static_assert(std::is_same_v<traitsConst::reference, const int&>);
    }

    SECTION("std::sort should NOT compile (bidirectional iterators only)")
    {
        // This is a compile-time check; in runtime we just note that we cannot call std::sort.
        // The test is only to document that sort is not supported.
        // If someone tries to compile `std::sort(lst.begin(), lst.end())`, it will fail.
        // We can add a comment and optionally a static_assert to guard against misuse.
        // But we cannot "test" compilation failure in Catch2 directly, so we just leave a comment.
        SUCCEED("std::sort is not required and will not compile, as expected.");
    }

    SECTION("std::reverse_iterator works via rbegin/rend (already tested, but double-check)")
    {
        mylib::List<int> lst = {1, 2, 3, 4, 5};
        auto rit = lst.rbegin();
        REQUIRE(*rit == 5);
        ++rit;
        REQUIRE(*rit == 4);
        // Compare with std::reverse_iterator from end
        auto stdRit = std::reverse_iterator(lst.end());
        REQUIRE(*stdRit == 5);
    }

    SECTION("Algorithms with std::inserter for insert at specific positions")
    {
        mylib::List<int> src = {10, 20, 30};
        mylib::List<int> dst = {1, 2, 3};
        // Insert src at position 2 (before 3)
        auto pos = std::next(dst.begin(), 2);
        std::copy(src.begin(), src.end(), std::inserter(dst, pos));
        // Expected: {1, 2, 10, 20, 30, 3}
        REQUIRE(toVector(dst) == std::vector<int>{1, 2, 10, 20, 30, 3});
    }

    SECTION("std::move with move-only types (if applicable)")
    {
        // This is a bit advanced; we can skip for now if we don't have move-only types.
        // But we can test with std::unique_ptr (which is move-only).
        // Our list supports move-only types because insert/emplace use perfect forwarding.
        // We'll test minimal: create list of unique_ptr and move them.
        mylib::List<std::unique_ptr<int>> src;
        src.push_back(std::make_unique<int>(1));
        src.push_back(std::make_unique<int>(2));
        src.push_back(std::make_unique<int>(3));

        mylib::List<std::unique_ptr<int>> dst;
        std::move(src.begin(), src.end(), std::back_inserter(dst));

        REQUIRE(dst.size() == 3);
        REQUIRE(*dst.front() == 1);
        REQUIRE(*dst.back() == 3);
        // src elements are moved-from, but we can check that size is still 3? Actually moving
        // the elements does not change src.size() because the nodes are still there with moved-from
        // unique_ptrs. But we can clear src.
        src.clear();
        REQUIRE(src.empty());
        // Verify dst still has correct values
        int sum = 0;
        for (auto& ptr : dst) {
            sum += *ptr;
        }
        REQUIRE(sum == 6);
    }

    SECTION("std::ranges algorithms")
    {
        mylib::List<int> lst = {1, 2, 3, 4, 5};

        // std::ranges::find
        auto it = std::ranges::find(lst, 3);
        REQUIRE(it != lst.end());
        REQUIRE(*it == 3);

        // std::ranges::reverse
        std::ranges::reverse(lst);
        REQUIRE(toVector(lst) == std::vector<int>{5, 4, 3, 2, 1});

        // std::ranges::copy
        std::vector<int> vec(5);
        std::ranges::copy(lst, vec.begin());
        REQUIRE(vec == std::vector<int>{5, 4, 3, 2, 1});

        // std::ranges::for_each
        int sum = 0;
        std::ranges::for_each(lst, [&sum](int x) { sum += x; });
        REQUIRE(sum == 15);
    }
}



// ============================================================================
// Тесты для moveToBegin
// ============================================================================
TEST_CASE("List::moveToBegin()", "[List][moveToBegin]")
{
    mylib::List<int> list;

    // ----------------------------------------------------------------
    SECTION("Move single element to begin – no effect")
    {
        list.push_back(42);
        REQUIRE(list.size() == 1);

        auto it = list.begin();
        auto newIt = list.moveToBegin(it);
        REQUIRE(newIt == list.begin());
        REQUIRE(*list.begin() == 42);
        REQUIRE(list.size() == 1);
    }

    // ----------------------------------------------------------------
    SECTION("Move middle element to begin")
    {
        list.push_back(1);
        list.push_back(2);
        list.push_back(3); // list: 1,2,3

        auto it = list.begin();
        ++it; // указывает на 2
        auto movedIt = list.moveToBegin(it);
        REQUIRE(movedIt == list.begin());
        REQUIRE(*movedIt == 2);
        REQUIRE(*std::next(movedIt) == 1);
        REQUIRE(*std::next(std::next(movedIt)) == 3);
        REQUIRE(list.size() == 3);
    }

    // ----------------------------------------------------------------
    SECTION("Move last element to begin")
    {
        list.push_back(1);
        list.push_back(2);
        list.push_back(3); // 1,2,3
        auto it = list.end();
        --it; // указывает на 3
        auto movedIt = list.moveToBegin(it);
        REQUIRE(movedIt == list.begin());
        REQUIRE(*movedIt == 3);
        REQUIRE(*std::next(movedIt) == 1);
        REQUIRE(*std::next(std::next(movedIt)) == 2);
        REQUIRE(list.size() == 3);
    }

    // ----------------------------------------------------------------
    SECTION("Move first element to begin – should be no-op")
    {
        list.push_back(1);
        list.push_back(2);
        list.push_back(3);
        auto it = list.begin(); // 1
        auto movedIt = list.moveToBegin(it);
        REQUIRE(movedIt == list.begin());
        REQUIRE(*movedIt == 1);
        // Проверяем, что порядок не изменился
        auto itCheck = list.begin();
        REQUIRE(*itCheck == 1);
        ++itCheck;
        REQUIRE(*itCheck == 2);
        ++itCheck;
        REQUIRE(*itCheck == 3);
        REQUIRE(list.size() == 3);
    }

    // ----------------------------------------------------------------
    SECTION("Move element in a list with multiple same values")
    {
        list.push_back(1);
        list.push_back(1);
        list.push_back(2); // 1,1,2
        auto it = list.begin();
        ++it; // указывает на второй 1
        auto movedIt = list.moveToBegin(it);
        REQUIRE(movedIt == list.begin());
        REQUIRE(*movedIt == 1);
        // Проверяем, что теперь на первом месте тот же элемент (по значению, но это может быть тот же узел)
        // Мы не можем проверить идентичность узла, но можем проверить размер и порядок
        auto it2 = list.begin();
        REQUIRE(*it2 == 1);
        ++it2;
        REQUIRE(*it2 == 1);
        ++it2;
        REQUIRE(*it2 == 2);
        REQUIRE(list.size() == 3);
    }


    // ----------------------------------------------------------------
    SECTION("Iterator validity after moveToBegin")
    {
        list.push_back(1);
        list.push_back(2);
        list.push_back(3);
        auto it = list.begin();
        ++it; // указывает на 2
        auto it2 = list.end();
        --it2; // указывает на 3
        auto movedIt = list.moveToBegin(it);
        // Итератор it остался валидным и теперь указывает на тот же узел, который теперь в начале
        REQUIRE(*it == 2); // if iterator is not invalidated
        // it2 остался валидным и указывает на 3, но порядок изменился
        REQUIRE(*it2 == 3);
        // Проверяем, что список перестроен
        auto itCheck = list.begin();
        REQUIRE(*itCheck == 2);
        ++itCheck;
        REQUIRE(*itCheck == 1);
        ++itCheck;
        REQUIRE(*itCheck == 3);
        REQUIRE(list.size() == 3);
    }
}



// ============================================================================
// Тесты для moveToEnd
// ============================================================================
TEST_CASE("List::moveToEnd()", "[List][moveToEnd]")
{
    mylib::List<int> list;

    // ----------------------------------------------------------------
    SECTION("Move single element to end – no effect")
    {
        list.push_back(42);
        REQUIRE(list.size() == 1);

        auto it = list.begin();
        list.moveToEnd(it);
        REQUIRE(list.size() == 1);
        REQUIRE(*list.begin() == 42);
    }

    // ----------------------------------------------------------------
    SECTION("Move first element to end")
    {
        list.push_back(1);
        list.push_back(2);
        list.push_back(3); // 1,2,3
        auto it = list.begin(); // указывает на 1
        list.moveToEnd(it);
        REQUIRE(list.size() == 3);
        // Ожидаемый порядок: 2,3,1
        auto itCheck = list.begin();
        REQUIRE(*itCheck == 2);
        ++itCheck;
        REQUIRE(*itCheck == 3);
        ++itCheck;
        REQUIRE(*itCheck == 1);
    }

    // ----------------------------------------------------------------
    SECTION("Move middle element to end")
    {
        list.push_back(1);
        list.push_back(2);
        list.push_back(3);
        list.push_back(4); // 1,2,3,4
        auto it = list.begin();
        ++it; // указывает на 2
        list.moveToEnd(it);
        REQUIRE(list.size() == 4);
        // Ожидаемый порядок: 1,3,4,2
        auto itCheck = list.begin();
        REQUIRE(*itCheck == 1);
        ++itCheck;
        REQUIRE(*itCheck == 3);
        ++itCheck;
        REQUIRE(*itCheck == 4);
        ++itCheck;
        REQUIRE(*itCheck == 2);
    }

    // ----------------------------------------------------------------
    SECTION("Move last element to end – should be no-op")
    {
        list.push_back(1);
        list.push_back(2);
        list.push_back(3);
        auto it = list.end();
        --it; // указывает на 3
        list.moveToEnd(it);
        REQUIRE(list.size() == 3);
        // Порядок не должен измениться
        auto itCheck = list.begin();
        REQUIRE(*itCheck == 1);
        ++itCheck;
        REQUIRE(*itCheck == 2);
        ++itCheck;
        REQUIRE(*itCheck == 3);
    }

    // ----------------------------------------------------------------
    SECTION("Iterator validity after moveToEnd")
    {
        list.push_back(1);
        list.push_back(2);
        list.push_back(3);
        list.push_back(4);
        auto it = list.begin();
        ++it; // указывает на 2
        auto it2 = list.begin(); // указывает на 1
        list.moveToEnd(it);
        // Итератор it остался валидным и теперь указывает на последний элемент
        REQUIRE(*it == 2);
        // it2 остался валидным, но теперь он указывает на первый элемент (который теперь 1? нет, порядок изменился)
        // Но мы не можем проверить, что он указывает на тот же узел, но можем проверить значение
        REQUIRE(*it2 == 1);
        // Проверяем, что список перестроен: 1,3,4,2
        auto itCheck = list.begin();
        REQUIRE(*itCheck == 1);
        ++itCheck;
        REQUIRE(*itCheck == 3);
        ++itCheck;
        REQUIRE(*itCheck == 4);
        ++itCheck;
        REQUIRE(*itCheck == 2);
    }
}
