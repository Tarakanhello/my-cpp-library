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


    // Вспомогательный аллокатор для подсчёта вызовов allocate/deallocate
    template<typename T>
    class TrackingAllocator : public std::allocator<T>
    {
    public:
        using Base = std::allocator<T>;

        static std::size_t allocate_count;
        static std::size_t deallocate_count;

        TrackingAllocator() = default;
        template<typename U>
        TrackingAllocator(const TrackingAllocator<U>&) noexcept {}

        T* allocate(std::size_t n)
        {
            ++allocate_count;
            return Base::allocate(n);
        }

        void deallocate(T* p, std::size_t n)
        {
            ++deallocate_count;
            Base::deallocate(p, n);
        }

        // Для перепривязки
        template<typename U>
        struct rebind {
            using other = TrackingAllocator<U>;
        };
    };

    template<typename T>
    std::size_t TrackingAllocator<T>::allocate_count = 0;
    template<typename T>
    std::size_t TrackingAllocator<T>::deallocate_count = 0;

    // Аллокатор, который может бросать исключения при выделении
    template<typename T>
    class ThrowingAllocator : public std::allocator<T>
    {
    public:
        using Base = std::allocator<T>;
        static bool should_throw;

        ThrowingAllocator() = default;
        template<typename U>
        ThrowingAllocator(const ThrowingAllocator<U>&) noexcept {}

        T* allocate(std::size_t n)
        {
            if (should_throw) {
                throw std::bad_alloc();
            }
            return Base::allocate(n);
        }

        void deallocate(T* p, std::size_t n)
        {
            Base::deallocate(p, n);
        }

        template<typename U>
        struct rebind {
            using other = ThrowingAllocator<U>;
        };
    };

    template<typename T>
    bool ThrowingAllocator<T>::should_throw = false;

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



// ============================================================================
// Тесты allocate() и remove()
// ============================================================================
TEST_CASE("StaticFreeList allocate and remove", "[StaticFreeList][allocate][remove]")
{
    // ----------------------------------------------------------------
    SECTION("Allocate on empty list returns valid node and updates state")
    {
        mylib::StaticFreeList<TestObject> list(3);
        REQUIRE(list.empty());
        REQUIRE_FALSE(list.isFull());

        auto* node = list.allocate();
        REQUIRE(node != nullptr);
        // Объект ещё не сконструирован, поэтому alive не увеличился
        REQUIRE(TestObject::alive == 0);
        // конструируем объект
        new (&node->value) TestObject();
        REQUIRE(TestObject::alive == 1);

        REQUIRE_FALSE(list.empty());
        REQUIRE_FALSE(list.isFull());
        REQUIRE(static_cast<bool>(list) == true);

        // Освобождаем и проверяем, что alive уменьшился
        list.remove(node);
        REQUIRE(TestObject::alive == 0);
        REQUIRE(list.empty());
        REQUIRE_FALSE(list.isFull());
        REQUIRE_FALSE(static_cast<bool>(list));
    }

    // ----------------------------------------------------------------
    SECTION("Allocate fills block and isFull becomes true")
    {
        const size_t capacity = 3;
        mylib::StaticFreeList<TestObject> list(capacity);

        for (size_t i = 0; i < capacity; ++i) {
            auto* node = list.allocate();
            REQUIRE(node != nullptr);
            new (&node->value) TestObject();
            REQUIRE(TestObject::alive == static_cast<int>(i + 1));
            if (i + 1 < capacity) {
                REQUIRE_FALSE(list.isFull());
            }
        }
        REQUIRE(list.isFull());
        REQUIRE_FALSE(list.empty());

        // Освобождаем все для очистки
        // Но у нас нет доступа к узлам, если мы не сохранили их. Сохраним в вектор.
        // Поэтому этот тест лучше переписать с сохранением узлов.
    }

    // Более надёжный тест с сохранением узлов
    SECTION("Allocate fills block and then remove frees")
    {
        const size_t capacity = 3;
        mylib::StaticFreeList<TestObject> list(capacity);
        std::vector<typename mylib::StaticFreeList<TestObject>::Node*> nodes;
        nodes.reserve(capacity);

        for (size_t i = 0; i < capacity; ++i) {
            auto* node = list.allocate();
            REQUIRE(node != nullptr);
            new (&node->value) TestObject();
            nodes.push_back(node);
        }
        REQUIRE(list.isFull());
        REQUIRE(TestObject::alive == static_cast<int>(capacity));

        // Освобождаем все
        for (auto* node : nodes) {
            list.remove(node);
        }
        REQUIRE(TestObject::alive == 0);
        REQUIRE(list.empty());
        REQUIRE_FALSE(list.isFull());
    }

    // ----------------------------------------------------------------
    SECTION("Allocate reuses freed nodes (LIFO order)")
    {
        mylib::StaticFreeList<TestObject> list(3);
        auto* node1 = list.allocate();
        new (&node1->value) TestObject();
        auto* node2 = list.allocate();
        new (&node2->value) TestObject();
        auto* node3 = list.allocate();
        new (&node3->value) TestObject();
        REQUIRE(TestObject::alive == 3);

        // Освобождаем node2, затем node1
        list.remove(node2);
        list.remove(node1);
        REQUIRE(TestObject::alive == 1); // только node3 жив

        // Теперь выделяем снова – должен вернуться node1 (LIFO, так как он был освобождён последним)
        auto* new_node = list.allocate();
        REQUIRE(new_node == node1); // должен быть тот же адрес
        // Объект ещё не сконструирован, поэтому alive остаётся 1
        new (&new_node->value) TestObject();
        REQUIRE(TestObject::alive == 2); // node3 + новый

        // Освобождаем оставшиеся
        list.remove(node3);
        list.remove(new_node);
        REQUIRE(TestObject::alive == 0);
        REQUIRE(list.empty());
    }

    // ----------------------------------------------------------------
    SECTION("Remove(nullptr) does nothing")
    {
        mylib::StaticFreeList<TestObject> list(3);
        REQUIRE(list.empty());
        list.remove(nullptr); // не должно упасть
        REQUIRE(list.empty());
        // Проверяем, что можно выделить и освободить нормально
        auto* node = list.allocate();
        new (&node->value) TestObject();
        REQUIRE(TestObject::alive == 1);
        list.remove(node);
        REQUIRE(TestObject::alive == 0);
        REQUIRE(list.empty());
    }

    // ----------------------------------------------------------------
    SECTION("Mixed allocate and remove operations maintain invariants")
    {
        const size_t capacity = 5;
        mylib::StaticFreeList<TestObject> list(capacity);
        std::vector<typename mylib::StaticFreeList<TestObject>::Node*> allocated;

        // Выделим 3 узла
        for (int i = 0; i < 3; ++i) {
            auto* node = list.allocate();
            new (&node->value) TestObject();
            allocated.push_back(node);
        }
        REQUIRE(TestObject::alive == 3);
        REQUIRE_FALSE(list.isFull());

        // Освободим один (например, второй)
        list.remove(allocated[1]);
        REQUIRE(TestObject::alive == 2);
        allocated.erase(allocated.begin() + 1);

        // Теперь выделим снова – должен переиспользоваться освобождённый
        auto* new_node = list.allocate();
        REQUIRE(new_node != nullptr);
        new (&new_node->value) TestObject();
        allocated.push_back(new_node);
        REQUIRE(TestObject::alive == 3);

        // Освободим все
        for (auto* node : allocated) {
            list.remove(node);
        }
        REQUIRE(TestObject::alive == 0);
        REQUIRE(list.empty());
        REQUIRE_FALSE(list.isFull());
    }

    // ----------------------------------------------------------------
    SECTION("Capacity and constructed size after many operations")
    {
        const size_t capacity = 4;
        mylib::StaticFreeList<TestObject> list(capacity);
        std::vector<typename mylib::StaticFreeList<TestObject>::Node*> nodes;

        // Выделим все 4
        for (size_t i = 0; i < capacity; ++i) {
            auto* node = list.allocate();
            new (&node->value) TestObject();
            nodes.push_back(node);
        }
        REQUIRE(list.isFull());
        REQUIRE(TestObject::alive == static_cast<int>(capacity));

        // Освободим 2 (например, первые два)
        list.remove(nodes[0]);
        list.remove(nodes[2]);
        REQUIRE(TestObject::alive == 2); // nodes[1] и nodes[3] живы
        REQUIRE_FALSE(list.isFull());
        REQUIRE_FALSE(list.empty());

        // Выделим ещё 2 – они должны переиспользовать freed
        auto* new1 = list.allocate();
        new (&new1->value) TestObject();
        auto* new2 = list.allocate();
        new (&new2->value) TestObject();
        REQUIRE(TestObject::alive == 4); // все 4 живы
        REQUIRE(list.isFull());

        // Освободим все
        list.remove(nodes[1]);
        list.remove(nodes[3]);
        list.remove(new1);
        list.remove(new2);
        REQUIRE(TestObject::alive == 0);
        REQUIRE(list.empty());
        REQUIRE_FALSE(list.isFull());
    }

    // ----------------------------------------------------------------
    SECTION("operator bool reflects emptiness")
    {
        mylib::StaticFreeList<TestObject> list(2);
        REQUIRE_FALSE(static_cast<bool>(list));

        auto* node = list.allocate();
        new (&node->value) TestObject();
        REQUIRE(static_cast<bool>(list));

        list.remove(node);
        REQUIRE_FALSE(static_cast<bool>(list));
    }
}



// ============================================================================
// Тесты сборки мусора (деструктор)
// ============================================================================
TEST_CASE("StaticFreeList garbage collection (destructor behavior)", "[StaticFreeList][destructor][garbage]")
{
    // ----------------------------------------------------------------
    SECTION("Destructor destroys all alive objects when some are freed")
    {
        REQUIRE(TestObject::alive == 0);

        {
            mylib::StaticFreeList<TestObject> list(10);
            std::vector<typename mylib::StaticFreeList<TestObject>::Node*> nodes;

            // Выделяем 5 объектов
            for (int i = 0; i < 5; ++i) {
                auto* node = list.allocate();
                new (&node->value) TestObject();
                nodes.push_back(node);
            }
            REQUIRE(TestObject::alive == 5);

            // Освобождаем 2 (например, индексы 1 и 3)
            list.remove(nodes[1]);
            list.remove(nodes[3]);
            REQUIRE(TestObject::alive == 3); // остались 0,2,4

            // Уничтожаем список – должны уничтожиться только 3 живых объекта
        } // деструктор списка

        REQUIRE(TestObject::alive == 0);
    }

    // ----------------------------------------------------------------
    SECTION("Destructor does nothing when list is empty")
    {
        REQUIRE(TestObject::alive == 0);

        {
            mylib::StaticFreeList<TestObject> list(5);
            // ничего не выделяем
        } // деструктор

        REQUIRE(TestObject::alive == 0);
    }

    // ----------------------------------------------------------------
    SECTION("Destructor does nothing when all objects already freed")
    {
        REQUIRE(TestObject::alive == 0);

        {
            mylib::StaticFreeList<TestObject> list(5);
            auto* n1 = list.allocate();
            new (&n1->value) TestObject();
            auto* n2 = list.allocate();
            new (&n2->value) TestObject();
            REQUIRE(TestObject::alive == 2);

            list.remove(n1);
            list.remove(n2);
            REQUIRE(TestObject::alive == 0);
        } // деструктор

        REQUIRE(TestObject::alive == 0);
    }

    // ----------------------------------------------------------------
    SECTION("Destructor destroys all objects when block is full and none freed")
    {
        const size_t capacity = 4;
        REQUIRE(TestObject::alive == 0);

        {
            mylib::StaticFreeList<TestObject> list(capacity);
            std::vector<typename mylib::StaticFreeList<TestObject>::Node*> nodes;

            for (size_t i = 0; i < capacity; ++i) {
                auto* node = list.allocate();
                new (&node->value) TestObject();
                nodes.push_back(node);
            }
            REQUIRE(TestObject::alive == static_cast<int>(capacity));
            REQUIRE(list.isFull());

            // Не освобождаем ничего
        } // деструктор должен уничтожить все 4

        REQUIRE(TestObject::alive == 0);
    }

    // ----------------------------------------------------------------
    SECTION("Destructor after move: source list becomes empty and doesn't destroy objects")
    {
        REQUIRE(TestObject::alive == 0);

        {
            mylib::StaticFreeList<TestObject> list1(5);
            auto* node = list1.allocate();
            new (&node->value) TestObject();
            REQUIRE(TestObject::alive == 1);

            mylib::StaticFreeList<TestObject> list2(std::move(list1));
            // list1 теперь пуст, его деструктор не должен ничего трогать
            REQUIRE(list1.empty());
            REQUIRE(TestObject::alive == 1); // объект всё ещё жив в list2

            // Уничтожаем list2 – объект должен быть уничтожен
        } // деструктор list2

        REQUIRE(TestObject::alive == 0);
    }

    // ----------------------------------------------------------------
    SECTION("Destructor after move assignment: old data destroyed, new data transferred")
    {
        REQUIRE(TestObject::alive == 0);

        mylib::StaticFreeList<TestObject> list1(5);
        auto* n1 = list1.allocate();
        new (&n1->value) TestObject();
        auto* n2 = list1.allocate();
        new (&n2->value) TestObject();
        REQUIRE(TestObject::alive == 2);

        mylib::StaticFreeList<TestObject> list2(10);
        auto* n3 = list2.allocate();
        new (&n3->value) TestObject();
        REQUIRE(TestObject::alive == 3);

        // Перемещаем list1 в list2 – старые данные list2 (n3) должны быть уничтожены
        list2 = std::move(list1);
        // Теперь n3 уничтожен, остались n1 и n2
        REQUIRE(TestObject::alive == 2);

        // Уничтожаем list2 – должны уничтожиться n1 и n2
        // (list1 уже пуст, его деструктор ничего не делает)
        list2.remove(n1);
        list2.remove(n2);
        REQUIRE(TestObject::alive == 0);
    }

    // ----------------------------------------------------------------
    SECTION("Destructor handles mixed allocated/freed nodes correctly")
    {
        REQUIRE(TestObject::alive == 0);

        {
            mylib::StaticFreeList<TestObject> list(6);
            std::vector<typename mylib::StaticFreeList<TestObject>::Node*> nodes;

            // Выделяем все 6
            for (int i = 0; i < 6; ++i) {
                auto* node = list.allocate();
                new (&node->value) TestObject();
                nodes.push_back(node);
            }
            REQUIRE(TestObject::alive == 6);

            // Освобождаем узлы с индексами 1, 3, 5 (чётные оставляем живыми)
            list.remove(nodes[1]);
            list.remove(nodes[3]);
            list.remove(nodes[5]);
            REQUIRE(TestObject::alive == 3); // живы 0,2,4

            // Уничтожаем список – должны уничтожиться только 0,2,4
        } // деструктор

        REQUIRE(TestObject::alive == 0);
    }
}



// ============================================================================
// Тесты с разными аллокаторами
// ============================================================================
TEST_CASE("StaticFreeList with different allocators", "[StaticFreeList][allocators]")
{
    // ----------------------------------------------------------------
    SECTION("Uses default MySimpleAllocator (implicitly tested in previous stages)")
    {
        // Просто создаём и используем, как раньше
        mylib::StaticFreeList<TestObject> list(5);
        auto* node = list.allocate();
        new (&node->value) TestObject();
        REQUIRE(TestObject::alive == 1);
        list.remove(node);
        REQUIRE(TestObject::alive == 0);
    }

    // ----------------------------------------------------------------
    SECTION("Works with std::allocator")
    {
        mylib::StaticFreeList<TestObject, std::allocator<TestObject>> list(5);
        auto* node = list.allocate();
        new (&node->value) TestObject();
        REQUIRE(TestObject::alive == 1);
        list.remove(node);
        REQUIRE(TestObject::alive == 0);
    }

    // ----------------------------------------------------------------
    SECTION("Works with TrackingAllocator and tracks calls")
    {
        // Очищаем счётчики
        TrackingAllocator<typename mylib::StaticFreeList<TestObject, TrackingAllocator<TestObject>>::Node>::allocate_count = 0;
        TrackingAllocator<typename mylib::StaticFreeList<TestObject, TrackingAllocator<TestObject>>::Node>::deallocate_count = 0;

        using ListType = mylib::StaticFreeList<TestObject, TrackingAllocator<TestObject>>;
        {
            ListType list(10);
            // При создании должен быть вызван allocate для массива Node
            REQUIRE(TrackingAllocator<typename ListType::Node>::allocate_count == 1);

            // Выделяем несколько узлов
            auto* n1 = list.allocate();
            new (&n1->value) TestObject();
            auto* n2 = list.allocate();
            new (&n2->value) TestObject();
            // Дополнительных выделений памяти быть не должно
            REQUIRE(TrackingAllocator<typename ListType::Node>::allocate_count == 1);

            // Освобождаем один узел
            list.remove(n1);
            // deallocate не должен вызываться при освобождении узла (только при уничтожении блока)
            REQUIRE(TrackingAllocator<typename ListType::Node>::deallocate_count == 0);

            // Уничтожаем список – должен быть вызван deallocate для массива
        }
        REQUIRE(TrackingAllocator<typename ListType::Node>::deallocate_count == 1);
        // Проверяем, что количество allocate и deallocate совпадает
        REQUIRE(TrackingAllocator<typename ListType::Node>::allocate_count ==
                TrackingAllocator<typename ListType::Node>::deallocate_count);
    }

    // ----------------------------------------------------------------
    SECTION("Handles allocation failure (ThrowingAllocator)")
    {
        using ListType = mylib::StaticFreeList<TestObject, ThrowingAllocator<TestObject>>;

        // Разрешаем аллокатору бросать исключение
        ThrowingAllocator<typename ListType::Node>::should_throw = true;

        // Попытка создать объект с capacity > 0 должна привести к исключению
        // и объект не должен быть создан (RAII)
        bool exception_caught = false;
        try {
            ListType list(10);
        } catch (const std::bad_alloc&) {
            exception_caught = true;
        }
        REQUIRE(exception_caught);

        // Убеждаемся, что объект не создан, проверить можно через счётчик, но проще через флаг
        // Теперь отключаем выбрасывание исключений и создаём нормально
        ThrowingAllocator<typename ListType::Node>::should_throw = false;
        {
            ListType list(10);
            auto* node = list.allocate();
            new (&node->value) TestObject();
            REQUIRE(TestObject::alive == 1);
            list.remove(node);
            REQUIRE(TestObject::alive == 0);
        }
    }

    // ----------------------------------------------------------------
    SECTION("TrackingAllocator with move operations")
    {
        // Очищаем счётчики
        using ListType = mylib::StaticFreeList<TestObject, TrackingAllocator<TestObject>>;
        TrackingAllocator<typename ListType::Node>::allocate_count = 0;
        TrackingAllocator<typename ListType::Node>::deallocate_count = 0;

        {
            ListType list1(5);
            REQUIRE(TrackingAllocator<typename ListType::Node>::allocate_count == 1);

            auto* node = list1.allocate();
            new (&node->value) TestObject();
            REQUIRE(TestObject::alive == 1);

            // Перемещаем
            ListType list2(std::move(list1));
            // При перемещении не должно быть новых выделений или освобождений
            REQUIRE(TrackingAllocator<typename ListType::Node>::allocate_count == 1);
            REQUIRE(TrackingAllocator<typename ListType::Node>::deallocate_count == 0);

            // Уничтожаем list2 – освобождается память
        }
        REQUIRE(TrackingAllocator<typename ListType::Node>::deallocate_count == 1);
        REQUIRE(TestObject::alive == 0);

        // Проверяем баланс
        REQUIRE(TrackingAllocator<typename ListType::Node>::allocate_count ==
                TrackingAllocator<typename ListType::Node>::deallocate_count);
    }

    // ----------------------------------------------------------------
    SECTION("Allocator rebinding works")
    {
        // Тест проверяет, что StaticFreeList правильно перепривязывает аллокатор
        // от типа T к типу Node. Это уже проверяется косвенно, но можно добавить
        // явную проверку компиляции (она должна пройти).
        using ListType = mylib::StaticFreeList<TestObject, std::allocator<TestObject>>;
        ListType list(3);
        auto* node = list.allocate();
        new (&node->value) TestObject();
        list.remove(node);
        // Если компилируется – значит rebind работает.
        SUCCEED("Rebinding compiles successfully");
    }
}
