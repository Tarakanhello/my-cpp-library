#include <catch2/catch_all.hpp>

#include "mylib/static_free_list.h"

namespace
{

    class TestObject
    {
    public:
        static int alive; // счётчик живых объектов

        TestObject() { ++alive; }
        TestObject(const TestObject&) { ++alive; }
        TestObject(TestObject&&) { ++alive; }
        ~TestObject() { --alive; }
    };

    int TestObject::alive{ 0 };

} // end namespace

// ============================================================================
// Тесты конструкторов и деструктора
// ============================================================================
TEST_CASE("StaticFreeList constructors and destructor", "[StaticFreeList][constructors]")
{
    // ----------------------------------------------------------------
    SECTION("Default constructor creates empty list")
    {
        mylib::StaticFreeList<int> list;
        REQUIRE(list.empty());
        REQUIRE(list.isFull());
        REQUIRE(list.capacity() == 0);
        REQUIRE_FALSE(static_cast<bool>(list)); // operator bool возвращает !empty()
    }

    // ----------------------------------------------------------------
    SECTION("Constructor with capacity = 0")
    {
        mylib::StaticFreeList<int> list{ 0 };
        REQUIRE(list.empty());
        REQUIRE(list.isFull()); // так как 0 == 0
        REQUIRE(list.capacity() == 0);
        REQUIRE_FALSE(static_cast<bool>(list));
    }

    // ----------------------------------------------------------------
    SECTION("Constructor with positive capacity")
    {
        const size_t capacity{ 5 };
        mylib::StaticFreeList<TestObject> list{ capacity };
        REQUIRE(list.empty());
        REQUIRE_FALSE(list.isFull());
        REQUIRE(list.capacity() == capacity);
        REQUIRE_FALSE(static_cast<bool>(list));

        // Проверяем, что можно выделить узлы (косвенно проверяет, что память выделена)
        auto* node1{ list.allocate() };
        new (&node1->value) TestObject();
        REQUIRE(node1 != nullptr);
        REQUIRE_FALSE(list.empty());
        REQUIRE_FALSE(list.isFull());

        auto* node2 = list.allocate();
        new (&node2->value) TestObject();
        REQUIRE(node2 != nullptr);
        REQUIRE_FALSE(list.empty());
        REQUIRE_FALSE(list.isFull());

        // Освобождаем для очистки (чтобы деструктор отработал чисто)
        list.remove(node1);
        list.remove(node2);
        // После удаления список снова пуст
        REQUIRE(list.empty());
    }

    // ----------------------------------------------------------------
    SECTION("Destructor with no allocated objects")
    {
        // Создаём и тут же уничтожаем – проверяем, что нет утечек
        // (непосредственно проверить утечки сложно, но санитайзеры помогут)
        {
            mylib::StaticFreeList<int> list{ 10 };
            // Ничего не выделяем
        } // деструктор вызывается
        SUCCEED("No crash or leak detected (use sanitizers for verification)");
    }

    // ----------------------------------------------------------------
    SECTION("Destructor destroys alive objects")
    {
        // Проверяем, что деструктор StaticFreeList вызывает деструкторы
        // всех живых объектов (сборка мусора)
        REQUIRE(TestObject::alive == 0);

        {
            mylib::StaticFreeList<TestObject> list{ 5 };
            REQUIRE(list.empty());

            // Выделяем 3 объекта
            auto* node1 = list.allocate();
            new (&node1->value) TestObject();
            auto* node2 = list.allocate();
            new (&node2->value) TestObject();
            auto* node3 = list.allocate();
            new (&node3->value) TestObject();
            REQUIRE(TestObject::alive == 3);
            REQUIRE_FALSE(list.empty());
            REQUIRE_FALSE(list.isFull()); // capacity 5, занято 3

            // Освобождаем один узел (он должен быть уничтожен)
            list.remove(node2);
            REQUIRE(TestObject::alive == 2); // один удалён
            REQUIRE_FALSE(list.empty());
        } // деструктор списка

        // Проверяем, что все объекты уничтожены
        REQUIRE(TestObject::alive == 0);
    }

    // ----------------------------------------------------------------
    SECTION("Destructor with all objects already freed")
    {
        REQUIRE(TestObject::alive == 0);

        {
            mylib::StaticFreeList<TestObject> list(5);
            auto* node1 = list.allocate();
            new (&node1->value) TestObject();
            auto* node2 = list.allocate();
            new (&node2->value) TestObject();

            REQUIRE(TestObject::alive == 2);

            // Освобождаем все
            list.remove(node1);
            list.remove(node2);
            REQUIRE(TestObject::alive == 0);
            REQUIRE(list.empty());
        } // деструктор списка

        REQUIRE(TestObject::alive == 0);
    }

    // ----------------------------------------------------------------
    SECTION("Move constructor transfers ownership")
    {
        mylib::StaticFreeList<TestObject> list1(5);
        auto* node = list1.allocate();
        new (&node->value) TestObject();
        REQUIRE_FALSE(list1.empty());
        REQUIRE(list1.capacity() == 5);

        mylib::StaticFreeList<TestObject> list2(std::move(list1));
        // После перемещения list1 должен стать пустым (release)
        REQUIRE(list1.empty());
        REQUIRE(list1.capacity() == 0); // capacity обнуляется в release

        // list2 должен владеть данными
        REQUIRE_FALSE(list2.empty());
        REQUIRE(list2.capacity() == 5);
        // Проверяем, что узел всё ещё жив (можно попытаться освободить его через list2)
        // Убедимся, что указатель валиден (просто проверяем, что remove не падает)
        list2.remove(node);
        REQUIRE(list2.empty());
    }

    // ----------------------------------------------------------------
    SECTION("Move assignment operator")
    {
        mylib::StaticFreeList<TestObject> list1(5);
        auto* node = list1.allocate();
        new (&node->value) TestObject();
        REQUIRE_FALSE(list1.empty());

        mylib::StaticFreeList<TestObject> list2(3);
        list2 = std::move(list1);
        // list1 должен стать пустым
        REQUIRE(list1.empty());
        REQUIRE(list1.capacity() == 0);

        // list2 владеет данными
        REQUIRE_FALSE(list2.empty());
        REQUIRE(list2.capacity() == 5);
        // Освобождаем узел через list2
        list2.remove(node);
        REQUIRE(list2.empty());
    }

    // ----------------------------------------------------------------
    SECTION("Move assignment destroys old data")
    {
        mylib::StaticFreeList<TestObject> list1(5);
        auto* n1 = list1.allocate();
        new (&n1->value) TestObject();
        auto* n2 = list1.allocate();
        new (&n2->value) TestObject();

        REQUIRE(TestObject::alive == 2);

        mylib::StaticFreeList<TestObject> list2(10);
        auto* n3 = list2.allocate(); // выделяем один объект в list2
        new (&n3->value) TestObject();
        REQUIRE(TestObject::alive == 3);

        // При перемещении list1 в list2, старые данные list2 должны быть уничтожены
        list2 = std::move(list1);

        // Старые данные list2 (один объект) должны быть уничтожены,
        // но данные из list1 (два объекта) теперь принадлежат list1
        // Общее количество живых объектов должно быть 2 (из list1)
        REQUIRE(TestObject::alive == 2);

        // Освобождаем оставшиеся объекты, чтобы не было утечек
        list2.remove(n1);
        list2.remove(n2);
        REQUIRE(TestObject::alive == 0);
    }
}
