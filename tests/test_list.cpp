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



TEST_CASE("Этап 1: Конструкторы, деструктор, size, empty, front/back", "[list][construction]")
{
    // Общие переменные, используемые во всех секциях
    const int defaultVal{ 42 };
    const std::string strVal{ "Hello" };

    SECTION("Конструктор по умолчанию")
    {
        mylib::List<int> lst;
        REQUIRE(lst.empty());
        REQUIRE(lst.size() == 0);
        REQUIRE_FALSE(lst.operator bool()); // explicit operator bool
        // front/back не вызываем, так как список пуст (assert не должен срабатывать в тестах)
    }


    SECTION("Конструктор от count и value (int)")
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
        SECTION("с нестандартным типом (std::string)")
        {
            mylib::List<std::string> lst(3, strVal);
            REQUIRE(lst.size() == 3);
            auto vec{ toVector(lst) };
            REQUIRE(vec == std::vector<std::string>(3, strVal));
        }
    }

    SECTION("Конструктор от initializer_list")
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

    SECTION("Конструктор из диапазона итераторов (шаблонный)")
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

    SECTION("Конструктор копирования")
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

    SECTION("Конструктор перемещения")
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

    SECTION("Деструктор (проверка отсутствия утечек)")
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
        SUCCEED("Деструктор отработал без падений");
    }

    SECTION("Методы front() и back() для непустого списка (включая константные)")
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
