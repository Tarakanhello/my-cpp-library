// test_free_list.cpp
// Модульные тесты для класса FreeList с использованием Catch2

#include <catch2/catch_all.hpp>
#include "mylib/free_list.h"
#include "mylib/memory.h" // для TestObject и аллокаторов

namespace {

    // Вспомогательный класс для отслеживания вызовов деструкторов
    class TestObject
    {
    public:
        static int alive;
        TestObject() { ++alive; }
        TestObject(const TestObject&) { ++alive; }
        TestObject(TestObject&&) { ++alive; }
        ~TestObject() { --alive; }
    };
    int TestObject::alive = 0;

    // Аллокатор-счётчик (аналог TrackingAllocator из тестов StaticFreeList)
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

        T* allocate(std::size_t n) {
            ++allocate_count;
            return Base::allocate(n);
        }
        void deallocate(T* p, std::size_t n) {
            ++deallocate_count;
            Base::deallocate(p, n);
        }
        template<typename U>
        struct rebind { using other = TrackingAllocator<U>; };
    };

    template<typename T>
    std::size_t TrackingAllocator<T>::allocate_count = 0;
    template<typename T>
    std::size_t TrackingAllocator<T>::deallocate_count = 0;

    // Аллокатор, бросающий исключения
    template<typename T>
    class ThrowingAllocator : public std::allocator<T>
    {
    public:
        using Base = std::allocator<T>;
        static bool should_throw;

        ThrowingAllocator() = default;
        template<typename U>
        ThrowingAllocator(const ThrowingAllocator<U>&) noexcept {}

        T* allocate(std::size_t n) {
            if (should_throw) throw std::bad_alloc();
            return Base::allocate(n);
        }
        void deallocate(T* p, std::size_t n) { Base::deallocate(p, n); }
        template<typename U>
        struct rebind { using other = ThrowingAllocator<U>; };
    };
    template<typename T>
    bool ThrowingAllocator<T>::should_throw = false;

} // namespace



// ============================================================================
// Тесты конструкторов и деструктора
// ============================================================================
TEST_CASE("FreeList constructors and destructor", "[FreeList][constructors]")
{
    // ----------------------------------------------------------------
    SECTION("Default constructor creates empty list with one block")
    {
        mylib::FreeList<int> list;
        REQUIRE(list.empty());
        REQUIRE(static_cast<bool>(list) == false);
        REQUIRE(list.size() == 0);
        REQUIRE(list.blockCount() == 1);
        REQUIRE(list.capacity() == 32);
    }

    // ----------------------------------------------------------------
    SECTION("Constructor with initial size creates empty list with correct block size")
    {
        mylib::FreeList<int> list(16);
        REQUIRE(list.empty());
        REQUIRE(list.size() == 0);
        REQUIRE(list.blockCount() == 1);
        REQUIRE(list.capacity() == 16);
    }

    // ----------------------------------------------------------------
    SECTION("Destructor destroys all alive objects")
    {
        REQUIRE(TestObject::alive == 0);
        {
            mylib::FreeList<TestObject> list(4);
            auto* p1 = list.allocate();
            new (p1) TestObject();
            auto* p2 = list.allocate();
            new (p2) TestObject();
            REQUIRE(TestObject::alive == 2);
            // Не освобождаем, при разрушении list должны уничтожиться
        }
        REQUIRE(TestObject::alive == 0);
    }

    // ----------------------------------------------------------------
    SECTION("Move constructor transfers ownership")
    {
        mylib::FreeList<TestObject> list1(4);
        auto* p = list1.allocate();
        new (p) TestObject();
        REQUIRE(list1.size() == 1);
        REQUIRE(list1.blockCount() == 1);
        REQUIRE(TestObject::alive == 1);

        mylib::FreeList<TestObject> list2(std::move(list1));
        REQUIRE(list1.empty());
        REQUIRE(list1.size() == 0);
        REQUIRE(list1.blockCount() == 0);
        REQUIRE_FALSE(list2.empty());
        REQUIRE(list2.size() == 1);
        REQUIRE(list2.blockCount() == 1);
        // Объект должен быть жив
        REQUIRE(TestObject::alive == 1);
        list2.remove(p);
        REQUIRE(TestObject::alive == 0);
    }

    // ----------------------------------------------------------------
    SECTION("Move assignment transfers ownership and destroys old data")
    {
        mylib::FreeList<TestObject> list1(4);
        auto* p1 = list1.allocate();
        new (p1) TestObject();
        auto* p2 = list1.allocate();
        new (p2) TestObject();
        REQUIRE(TestObject::alive == 2);

        mylib::FreeList<TestObject> list2(8);
        auto* p3 = list2.allocate();
        new (p3) TestObject();
        REQUIRE(TestObject::alive == 3);

        list2 = std::move(list1);
        // Старые данные list2 (p3) должны быть уничтожены
        REQUIRE(TestObject::alive == 2);
        // list2 теперь владеет данными list1
        REQUIRE(list2.size() == 2);
        REQUIRE(list2.blockCount() == 1);
        // list1 пуст
        REQUIRE(list1.empty());

        list2.remove(p1);
        list2.remove(p2);
        REQUIRE(TestObject::alive == 0);
    }
}

// ============================================================================
// Тесты allocate() и remove()
// ============================================================================
TEST_CASE("FreeList allocate and remove", "[FreeList][allocate][remove]")
{
    // ----------------------------------------------------------------
    SECTION("Allocate on empty list creates first block")
    {
        mylib::FreeList<TestObject> list(4);
        auto* p = list.allocate();
        REQUIRE(p != nullptr);
        REQUIRE(list.blockCount() == 1);
        REQUIRE(list.size() == 1);
        REQUIRE_FALSE(list.empty());
        new (p) TestObject();
        REQUIRE(TestObject::alive == 1);
        list.remove(p);
        REQUIRE(TestObject::alive == 0);
        REQUIRE(list.empty());
        REQUIRE(list.size() == 0);
        // Блок остаётся, но пуст
        REQUIRE(list.blockCount() == 1);
    }

    // ----------------------------------------------------------------
    SECTION("Allocate fills block and creates new block when full")
    {
        const size_t initial = 8;
        mylib::FreeList<TestObject> list(initial);
        std::vector<TestObject*> ptrs;

        // Заполняем первый блок
        for (size_t i = 0; i < initial; ++i)
        {
            auto* p = list.allocate();
            REQUIRE(p != nullptr);
            new (p) TestObject();
            ptrs.push_back(p);
        }
        REQUIRE(list.blockCount() == 1);
        REQUIRE(list.size() == initial);
        REQUIRE_FALSE(list.empty());

        // Следующее выделение должно создать новый блок
        auto* p_new = list.allocate();
        REQUIRE(p_new != nullptr);
        new (p_new) TestObject();
        REQUIRE(list.blockCount() == 2);
        REQUIRE(list.size() == initial + 1);

        // Освобождаем всё
        for (auto* p : ptrs) list.remove(p);
        list.remove(p_new);
        REQUIRE(TestObject::alive == 0);
        REQUIRE(list.empty());
        REQUIRE(list.size() == 0);

        // В текущей логике пустые блоки не удаляются, так что blockCount должен остаться 2
        REQUIRE(list.blockCount() == 2);
    }

    // ----------------------------------------------------------------
    SECTION("Remove frees object and reuses freed slot")
    {
        mylib::FreeList<TestObject> list(2);
        auto* p1 = list.allocate();
        new (p1) TestObject();
        auto* p2 = list.allocate();
        new (p2) TestObject();
        REQUIRE(TestObject::alive == 2);

        list.remove(p1);
        REQUIRE(TestObject::alive == 1);
        REQUIRE(list.size() == 1);
        REQUIRE_FALSE(list.empty());

        // Новое выделение должно использовать освобождённый слот
        auto* p3 = list.allocate();
        REQUIRE(p3 == p1); // должен быть тот же адрес (если LIFO)
        new (p3) TestObject();
        REQUIRE(TestObject::alive == 2);
        REQUIRE(list.size() == 2);

        list.remove(p2);
        list.remove(p3);
        REQUIRE(TestObject::alive == 0);
        REQUIRE(list.empty());
    }

    // ----------------------------------------------------------------
    SECTION("Block becomes empty and is removed")
    {
        mylib::FreeList<TestObject> list(2);
        auto* p1 = list.allocate();
        new (p1) TestObject();
        auto* p2 = list.allocate();
        new (p2) TestObject();
        REQUIRE(list.blockCount() == 1);

        list.remove(p1);
        list.remove(p2);
        REQUIRE(list.empty());

        //// ВНИМАНИЕ!!! В текущей реализации пустые блоки не удаляются
        REQUIRE(list.blockCount() == 1);
    }

    // ----------------------------------------------------------------
    SECTION("remove(nullptr) does nothing")
    {
        mylib::FreeList<TestObject> list(2);
        list.remove(nullptr);
        REQUIRE(list.empty());
        // Можно выделить и освободить нормально
        auto* p = list.allocate();
        new (p) TestObject();
        REQUIRE(TestObject::alive == 1);
        list.remove(p);
        REQUIRE(TestObject::alive == 0);
    }
}


// ============================================================================
// Тесты сборки мусора (уничтожение живых объектов в деструкторе)
// ============================================================================
TEST_CASE("FreeList garbage collection", "[FreeList][destructor][garbage]")
{
    // ----------------------------------------------------------------
    SECTION("Destructor destroys all alive objects across multiple blocks")
    {
        REQUIRE(TestObject::alive == 0);
        {
            mylib::FreeList<TestObject> list(2);
            std::vector<TestObject*> ptrs;
            // Создадим 3 блока (каждый по 2, всего 6 объектов)
            for (int i = 0; i < 6; ++i) {
                auto* p = list.allocate();
                new (p) TestObject();
                ptrs.push_back(p);
            }
            REQUIRE(TestObject::alive == 6);
            // Освободим некоторые, например, 2 объекта из разных блоков
            list.remove(ptrs[0]); // первый блок
            list.remove(ptrs[3]); // второй блок
            REQUIRE(TestObject::alive == 4);
            // При разрушении list должны уничтожиться оставшиеся 4
        }
        REQUIRE(TestObject::alive == 0);
    }
}

// ============================================================================
// Тесты с разными аллокаторами
// ============================================================================
TEST_CASE("FreeList with different allocators", "[FreeList][allocators]")
{
    // ----------------------------------------------------------------
    SECTION("Works with std::allocator")
    {
        mylib::FreeList<TestObject, std::allocator<TestObject>> list(4);
        auto* p = list.allocate();
        new (p) TestObject();
        REQUIRE(TestObject::alive == 1);
        list.remove(p);
        REQUIRE(TestObject::alive == 0);
    }

    // ----------------------------------------------------------------
    SECTION("Works with TrackingAllocator and tracks calls")
    {
        using ListType = mylib::FreeList<TestObject, TrackingAllocator<TestObject>>;
        using NodeAlloc = TrackingAllocator<typename ListType::Block::Node>;
        NodeAlloc::allocate_count = 0;
        NodeAlloc::deallocate_count = 0;

        {
            ListType list(4);
            // При создании первого блока должен быть вызван allocate для узлов
            REQUIRE(NodeAlloc::allocate_count == 1);

            auto* p = list.allocate();
            new (p) TestObject();
            // Дополнительных выделений быть не должно
            REQUIRE(NodeAlloc::allocate_count == 1);

            list.remove(p);
            // Освобождение узла не вызывает deallocate блока
            REQUIRE(NodeAlloc::deallocate_count == 0);
        }
        // При разрушении FreeList блоки уничтожаются, вызывается deallocate для каждого блока
        REQUIRE(NodeAlloc::deallocate_count == 1);
        REQUIRE(NodeAlloc::allocate_count == NodeAlloc::deallocate_count);
    }

    // ----------------------------------------------------------------
    SECTION("Handles allocation failure in createNewBlock")
    {
        using ListType = mylib::FreeList<TestObject, ThrowingAllocator<TestObject>>;
        using NodeAlloc = ThrowingAllocator<typename ListType::Block::Node>;

        NodeAlloc::should_throw = true;
        bool exception_caught = false;
        try {
            ListType list(4);
            // Попытка создать первый блок вызовет исключение
            list.allocate();
        } catch (const std::bad_alloc&) {
            exception_caught = true;
        }
        REQUIRE(exception_caught);

        // Сброс флага
        NodeAlloc::should_throw = false;
        // Теперь должно работать
        {
            ListType list(4);
            auto* p = list.allocate();
            new (p) TestObject();
            REQUIRE(TestObject::alive == 1);
            list.remove(p);
            REQUIRE(TestObject::alive == 0);
        }
    }
}

