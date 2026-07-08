#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include <string>
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
