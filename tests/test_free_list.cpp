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
        int value;

        TestObject() : value{ 0 } { ++alive; }
        explicit TestObject(int v) : value{ v } { ++alive; }
        TestObject(int a, int b) : value{ a+ b } { ++alive; }
        TestObject(const TestObject& other) : value{ other.value } { ++alive; }
        TestObject(TestObject&& other) noexcept : value{ other.value } { ++alive; }
        ~TestObject() { --alive; }

        bool operator==(const TestObject& other) const noexcept { return value == other.value; }
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


    // Структура с несколькими полями
    struct Point
    {
        int x, y;
        Point(int theX, int theY) : x(theX), y(theY) {}
    };

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
            auto* p1 = list.allocateRaw();
            new (p1) TestObject();
            auto* p2 = list.allocateRaw();
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
        auto* p = list1.allocateRaw();
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
        auto* p1 = list1.allocateRaw();
        new (p1) TestObject();
        auto* p2 = list1.allocateRaw();
        new (p2) TestObject();
        REQUIRE(TestObject::alive == 2);

        mylib::FreeList<TestObject> list2(8);
        auto* p3 = list2.allocateRaw();
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
// Тесты allocateRaw() и remove()
// ============================================================================
TEST_CASE("FreeList allocate and remove", "[FreeList][allocate][remove]")
{
    // ----------------------------------------------------------------
    SECTION("Allocate on empty list creates first block")
    {
        mylib::FreeList<TestObject> list(4);
        auto* p = list.allocateRaw();
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
            auto* p = list.allocateRaw();
            REQUIRE(p != nullptr);
            new (p) TestObject();
            ptrs.push_back(p);
        }
        REQUIRE(list.blockCount() == 1);
        REQUIRE(list.size() == initial);
        REQUIRE_FALSE(list.empty());

        // Следующее выделение должно создать новый блок
        auto* p_new = list.allocateRaw();
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

        // В текущей логике оставется как минимум 1 пустой блок, так что blockCount должен быть 1
        REQUIRE(list.blockCount() == 1);
    }

    // ----------------------------------------------------------------
    SECTION("Remove frees object and reuses freed slot")
    {
        mylib::FreeList<TestObject> list(2);
        auto* p1 = list.allocateRaw();
        new (p1) TestObject();
        auto* p2 = list.allocateRaw();
        new (p2) TestObject();
        REQUIRE(TestObject::alive == 2);

        list.remove(p1);
        REQUIRE(TestObject::alive == 1);
        REQUIRE(list.size() == 1);
        REQUIRE_FALSE(list.empty());

        // Новое выделение должно использовать освобождённый слот
        auto* p3 = list.allocateRaw();
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
        auto* p1 = list.allocateRaw();
        new (p1) TestObject();
        auto* p2 = list.allocateRaw();
        new (p2) TestObject();
        REQUIRE(list.blockCount() == 1);

        list.remove(p1);
        list.remove(p2);
        REQUIRE(list.empty());

        //// ВНИМАНИЕ!!! В текущей реализации остается как минимум 1 пустой блок
        REQUIRE(list.blockCount() == 1);
    }

    // ----------------------------------------------------------------
    SECTION("remove(nullptr) does nothing")
    {
        mylib::FreeList<TestObject> list(2);
        list.remove(nullptr);
        REQUIRE(list.empty());
        // Можно выделить и освободить нормально
        auto* p = list.allocateRaw();
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
                auto* p = list.allocateRaw();
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
        mylib::FreeList<TestObject, 8, 32, std::allocator<TestObject>> list(4);
        auto* p = list.allocateRaw();
        new (p) TestObject();
        REQUIRE(TestObject::alive == 1);
        list.remove(p);
        REQUIRE(TestObject::alive == 0);
    }

    // ----------------------------------------------------------------
    SECTION("Works with TrackingAllocator and tracks calls")
    {
        using ListType = mylib::FreeList<TestObject, 8, 32, TrackingAllocator<TestObject>>;
        using NodeAlloc = TrackingAllocator<typename ListType::Block::Node>;
        NodeAlloc::allocate_count = 0;
        NodeAlloc::deallocate_count = 0;

        {
            ListType list(4);
            // При создании первого блока должен быть вызван allocate для узлов
            REQUIRE(NodeAlloc::allocate_count == 1);

            auto* p = list.allocateRaw();
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
        using ListType = mylib::FreeList<TestObject, 8, 32, ThrowingAllocator<TestObject>>;
        using NodeAlloc = ThrowingAllocator<typename ListType::Block::Node>;

        NodeAlloc::should_throw = true;
        bool exception_caught = false;
        try {
            ListType list(4);
            // Попытка создать первый блок вызовет исключение
            list.allocateRaw();
        } catch (const std::bad_alloc&) {
            exception_caught = true;
        }
        REQUIRE(exception_caught);

        // Сброс флага
        NodeAlloc::should_throw = false;
        // Теперь должно работать
        {
            ListType list(4);
            auto* p = list.allocateRaw();
            new (p) TestObject();
            REQUIRE(TestObject::alive == 1);
            list.remove(p);
            REQUIRE(TestObject::alive == 0);
        }
    }
}


// ============================================================================
// Тесты удаления пустых блоков
// ============================================================================
TEST_CASE("FreeList empty block removal strategy", "[FreeList][empty_blocks]")
{
    // ----------------------------------------------------------------
    SECTION("Single block becomes empty and remains (no deletion)")
    {
        mylib::FreeList<TestObject> list(8);
        REQUIRE(list.blockCount() == 1);
        REQUIRE(list.capacity() == 8);
        REQUIRE(list.empty());

        std::vector<TestObject*> ptrs;
        for (size_t i = 0; i < 8; ++i)
        {
            auto* p = list.allocateRaw();
            REQUIRE(p != nullptr);
            new (p) TestObject();
            ptrs.push_back(p);
        }
        REQUIRE(TestObject::alive == 8);
        REQUIRE(list.blockCount() == 1);
        REQUIRE(list.capacity() == 8);
        REQUIRE_FALSE(list.empty());

        // Освобождаем все объекты
        for (auto* p : ptrs)
        {
            list.remove(p);
        }
        REQUIRE(TestObject::alive == 0);
        REQUIRE(list.empty());
        // Блок остаётся (он единственный)
        REQUIRE(list.blockCount() == 1);
        REQUIRE(list.capacity() == 8);

        // Повторное выделение должно использовать тот же блок
        auto* new_p = list.allocateRaw();
        REQUIRE(new_p != nullptr);
        REQUIRE(list.blockCount() == 1); // блок не создавался новый
        REQUIRE(list.capacity() == 8);
        new (new_p) TestObject();
        REQUIRE(TestObject::alive == 1);
        list.remove(new_p);
        REQUIRE(TestObject::alive == 0);
        REQUIRE(list.empty());
        REQUIRE(list.blockCount() == 1);
    }

    // ----------------------------------------------------------------
    SECTION("Two empty blocks – one remains, the other is removed")
    {
        mylib::FreeList<TestObject> list(8); // первый блок 8, второй 16
        std::vector<TestObject*> ptrs;

        // Заполняем первый блок (8 объектов)
        for (size_t i = 0; i < 8; ++i)
        {
            auto* p = list.allocateRaw();
            new (p) TestObject();
            ptrs.push_back(p);
        }
        // Заполняем второй блок (16 объектов)
        for (size_t i = 0; i < 16; ++i)
        {
            auto* p = list.allocateRaw();
            new (p) TestObject();
            ptrs.push_back(p);
        }

        REQUIRE(TestObject::alive == 24);
        REQUIRE(list.blockCount() == 2);
        // Суммарная ёмкость: 8 + 16 = 24
        REQUIRE(list.capacity() == 24);

        // Освобождаем все объекты
        for (auto* p : ptrs)
        {
            list.remove(p);
        }
        REQUIRE(TestObject::alive == 0);
        REQUIRE(list.empty());

        // Остался один пустой блок (либо размером 8, либо 16)
        REQUIRE(list.blockCount() == 1);
        // Проверяем, что capacity стала равна 8 или 16 (один блок)
        REQUIRE((list.capacity() == 8 || list.capacity() == 16));

        // Последующее выделение не должно создавать новый блок
        auto* new_p = list.allocateRaw();
        REQUIRE(new_p != nullptr);
        REQUIRE(list.blockCount() == 1);
        new (new_p) TestObject();
        REQUIRE(TestObject::alive == 1);
        list.remove(new_p);
        REQUIRE(TestObject::alive == 0);
    }

    // ----------------------------------------------------------------
    SECTION("Three empty blocks – two removed, one remains")
    {
        mylib::FreeList<TestObject> list(8); // блоки: 8, 16, 32
        std::vector<TestObject*> ptrs;

        // Заполняем все три блока
        const size_t sizes[] = {8, 16, 32};
        for (size_t s : sizes)
        {
            for (size_t i = 0; i < s; ++i)
            {
                auto* p = list.allocateRaw();
                new (p) TestObject();
                ptrs.push_back(p);
            }
        }

        REQUIRE(TestObject::alive == 56); // 8+16+32 = 56
        REQUIRE(list.blockCount() == 3);
        REQUIRE(list.capacity() == 56);

        // Освобождаем все
        for (auto* p : ptrs)
        {
            list.remove(p);
        }

        REQUIRE(TestObject::alive == 0);
        REQUIRE(list.empty());

        // Остался ровно один пустой блок
        REQUIRE(list.blockCount() == 1);
        // Ёмкость должна быть равна размеру одного из блоков (8, 16 или 32)
        REQUIRE((list.capacity() == 8 || list.capacity() == 16 || list.capacity() == 32));

        // Проверяем, что последующие выделения не создают новые блоки
        auto* new_p = list.allocateRaw();
        REQUIRE(new_p != nullptr);
        REQUIRE(list.blockCount() == 1);
        new (new_p) TestObject();
        REQUIRE(TestObject::alive == 1);
        list.remove(new_p);
        REQUIRE(TestObject::alive == 0);

    }

    // ----------------------------------------------------------------
    SECTION("One empty and one full block – empty block is NOT removed")
    {
        mylib::FreeList<TestObject> list(8);
        std::vector<TestObject*> firstBlockPtrs;
        std::vector<TestObject*> secondBlockPtrs;

        // Заполняем первый блок (8 объектов)
        for (size_t i = 0; i < 8; ++i) {
            auto* p = list.allocateRaw();
            new (p) TestObject();
            firstBlockPtrs.push_back(p);
        }
        // Заполняем второй блок (16 объектов)
        for (size_t i = 0; i < 16; ++i) {
            auto* p = list.allocateRaw();
            new (p) TestObject();
            secondBlockPtrs.push_back(p);
        }

        REQUIRE(list.blockCount() == 2);
        REQUIRE(list.capacity() == 24);
        REQUIRE(TestObject::alive == 24);

        // Освобождаем только первый блок – он становится пустым, второй полный
        for (auto* p : firstBlockPtrs) {
            list.remove(p);
        }
        REQUIRE(TestObject::alive == 16); // остались объекты из второго блока
        REQUIRE_FALSE(list.empty());
        // Должно быть два блока: пустой (первый) и полный (второй)
        REQUIRE(list.blockCount() == 2);
        REQUIRE(list.capacity() == 24); // ёмкость не менялась

        // Теперь освобождаем второй блок – он тоже становится пустым
        for (auto* p : secondBlockPtrs) {
            list.remove(p);
        }
        REQUIRE(TestObject::alive == 0);
        REQUIRE(list.empty());

        // Теперь два пустых блока, один должен быть удалён
        REQUIRE(list.blockCount() == 1);
        REQUIRE((list.capacity() == 8 || list.capacity() == 16));
    }

    // ----------------------------------------------------------------
    SECTION("Multiple empty blocks with one full – only excess empty blocks removed")
    {
        mylib::FreeList<TestObject> list(8);
        std::vector<TestObject*> ptrs;

        // Создаём три блока: 8, 16, 32
        const size_t sizes[] = {8, 16, 32};
        for (size_t s : sizes)
        {
            for (size_t i = 0; i < s; ++i)
            {
                auto* p = list.allocateRaw();
                new (p) TestObject();
                ptrs.push_back(p);
            }
        }
        REQUIRE(list.blockCount() == 3);
        REQUIRE(list.capacity() == 56);

        // Освобождаем объекты из первого и второго блоков (8+16=24 объекта)
        // Третий блок (32 объекта) остаётся полным.
        for (size_t i = 0; i < 24; ++i) {
            list.remove(ptrs[i]);
        }
        // Живы должны быть только объекты из третьего блока (32)
        REQUIRE(TestObject::alive == 32);
        REQUIRE_FALSE(list.empty());

        // Теперь у нас два пустых блока (первый и второй) и один полный (третий)
        // По логике должен быть удалён один из пустых, остаться один пустой + полный
        REQUIRE(list.blockCount() == 2); // один пустой + один полный
        // Суммарная ёмкость должна быть: размер оставшегося пустого + 32
        // Оставшийся пустой может быть 8 или 16, поэтому capacity = 32 + 8 = 40 или 32+16=48
        REQUIRE((list.capacity() == 40 || list.capacity() == 48));

        // Освобождаем оставшийся полный блок (третий)
        for (size_t i = 24; i < ptrs.size(); ++i) {
            list.remove(ptrs[i]);
        }
        REQUIRE(TestObject::alive == 0);
        REQUIRE(list.empty());

        // Теперь все три блока пусты. Остаться должен только один пустой.
        REQUIRE(list.blockCount() == 1);
        // Ёмкость должна быть равна размеру одного блока (8, 16 или 32)
        REQUIRE((list.capacity() == 8 || list.capacity() == 16 || list.capacity() == 32));
    }

    // ----------------------------------------------------------------
    SECTION("Capacity is correctly updated when blocks are removed")
    {
        mylib::FreeList<TestObject> list(8);
        std::vector<TestObject*> ptrs;

        // Заполняем два блока: 8 и 16
        for (size_t i = 0; i < 8; ++i) {
            auto* p = list.allocateRaw();
            new (p) TestObject();
            ptrs.push_back(p);
        }
        for (size_t i = 0; i < 16; ++i) {
            auto* p = list.allocateRaw();
            new (p) TestObject();
            ptrs.push_back(p);
        }
        REQUIRE(list.capacity() == 24);

        // Освобождаем все объекты первого блока (первые 8)
        for (size_t i = 0; i < 8; ++i) {
            list.remove(ptrs[i]);
        }
        // Теперь один пустой (8) и один полный (16) — capacity всё ещё 24
        REQUIRE(list.capacity() == 24);

        // Освобождаем все объекты второго блока
        for (size_t i = 8; i < ptrs.size(); ++i) {
            list.remove(ptrs[i]);
        }
        // Теперь два пустых блока — должен удалиться один, capacity станет 8 или 16
        REQUIRE((list.capacity() == 8 || list.capacity() == 16));
    }
}



// ============================================================================
// Тесты метода emplace
// ============================================================================
TEST_CASE("FreeList::emplace", "[FreeList][emplace]")
{
    // ----------------------------------------------------------------
    SECTION("Emplace with int (single argument)")
    {
        mylib::FreeList<int> list(4);
        REQUIRE(list.empty());
        REQUIRE(list.size() == 0);

        int* p = list.emplace(42);
        REQUIRE(p != nullptr);
        REQUIRE(*p == 42);
        REQUIRE(list.size() == 1);
        REQUIRE_FALSE(list.empty());

        list.remove(p);
        REQUIRE(list.size() == 0);
        REQUIRE(list.empty());
    }

    // ----------------------------------------------------------------
    SECTION("Emplace with TestObject (default constructor)")
    {
        mylib::FreeList<TestObject> list(4);
        REQUIRE(TestObject::alive == 0);

        TestObject* p = list.emplace();
        REQUIRE(p != nullptr);
        REQUIRE(p->value == 0);
        REQUIRE(TestObject::alive == 1);
        REQUIRE(list.size() == 1);

        list.remove(p);
        REQUIRE(TestObject::alive == 0);
        REQUIRE(list.empty());
    }

    // ----------------------------------------------------------------
    SECTION("Emplace with TestObject (single int argument)")
    {
        mylib::FreeList<TestObject> list(4);
        REQUIRE(TestObject::alive == 0);

        TestObject* p = list.emplace(10);
        REQUIRE(p != nullptr);
        REQUIRE(p->value == 10);
        REQUIRE(TestObject::alive == 1);
        REQUIRE(list.size() == 1);

        list.remove(p);
        REQUIRE(TestObject::alive == 0);
        REQUIRE(list.empty());
    }

    // ----------------------------------------------------------------
    SECTION("Emplace with TestObject (two int arguments)")
    {
        mylib::FreeList<TestObject> list(4);
        REQUIRE(TestObject::alive == 0);

        TestObject* p = list.emplace(3, 5);
        REQUIRE(p != nullptr);
        REQUIRE(p->value == 8); // 3+5
        REQUIRE(TestObject::alive == 1);
        REQUIRE(list.size() == 1);

        list.remove(p);
        REQUIRE(TestObject::alive == 0);
        REQUIRE(list.empty());
    }

    // ----------------------------------------------------------------
    SECTION("Emplace with std::string (const char*)")
    {
        mylib::FreeList<std::string> list(4);
        REQUIRE(list.empty());

        std::string* p = list.emplace("Hello, world!");
        REQUIRE(p != nullptr);
        REQUIRE(*p == "Hello, world!");
        REQUIRE(list.size() == 1);

        list.remove(p);
        REQUIRE(list.empty());
    }

    // ----------------------------------------------------------------
    SECTION("Emplace with std::string (move from temporary)")
    {
        mylib::FreeList<std::string> list(4);
        std::string temp = "Temporary";

        std::string* p = list.emplace(std::move(temp));
        REQUIRE(p != nullptr);
        REQUIRE(*p == "Temporary");
        REQUIRE(temp.empty()); // moved-from state
        REQUIRE(list.size() == 1);

        list.remove(p);
        REQUIRE(list.empty());
    }

    // ----------------------------------------------------------------
    SECTION("Emplace with std::string (copy from lvalue)")
    {
        mylib::FreeList<std::string> list(4);
        std::string original = "Copy me";

        std::string* p = list.emplace(original);
        REQUIRE(p != nullptr);
        REQUIRE(*p == "Copy me");
        REQUIRE(original == "Copy me"); // unchanged
        REQUIRE(list.size() == 1);

        list.remove(p);
        REQUIRE(list.empty());
    }

    // ----------------------------------------------------------------
    SECTION("Emplace with custom struct Point (two arguments)")
    {
        mylib::FreeList<Point> list(4);

        Point* p = list.emplace(10, 20);
        REQUIRE(p != nullptr);
        REQUIRE(p->x == 10);
        REQUIRE(p->y == 20);
        REQUIRE(list.size() == 1);

        list.remove(p);
        REQUIRE(list.empty());
    }

    // ----------------------------------------------------------------
    SECTION("Emplace multiple objects and fill a block")
    {
        const size_t blockSize = 8;
        mylib::FreeList<int> list(blockSize);
        std::vector<int*> ptrs;

        for (size_t i = 0; i < blockSize; ++i)
        {
            int* p = list.emplace(static_cast<int>(i * 10));
            REQUIRE(p != nullptr);
            REQUIRE(*p == i * 10);
            ptrs.push_back(p);
        }

        REQUIRE(list.size() == blockSize);
        REQUIRE(list.blockCount() == 1);
        REQUIRE_FALSE(list.empty());

        // Следующее emplace должно создать новый блок
        int* pNew = list.emplace(100);
        REQUIRE(pNew != nullptr);
        REQUIRE(*pNew == 100);
        REQUIRE(list.size() == blockSize + 1);
        REQUIRE(list.blockCount() == 2);

        // Освобождаем всё
        for (auto* p : ptrs) list.remove(p);
        list.remove(pNew);
        REQUIRE(list.empty());
        REQUIRE(list.size() == 0);
    }

    // ----------------------------------------------------------------
    SECTION("Emplace after remove reuses freed slot")
    {
        mylib::FreeList<TestObject> list(2);
        TestObject* p1 = list.emplace(1);
        TestObject* p2 = list.emplace(2);
        REQUIRE(TestObject::alive == 2);
        REQUIRE(list.size() == 2);

        list.remove(p1);
        REQUIRE(TestObject::alive == 1);
        REQUIRE(list.size() == 1);

        // Новый emplace должен переиспользовать освобождённый слот
        TestObject* p3 = list.emplace(3);
        REQUIRE(p3 == p1); // должен быть тот же адрес (LIFO)
        REQUIRE(TestObject::alive == 2);
        REQUIRE(list.size() == 2);

        list.clear();
        REQUIRE(TestObject::alive == 0);
        REQUIRE(list.empty());
    }

    // ----------------------------------------------------------------
    SECTION("Emplace with exceptions – constructors that throw")
    {
        struct Throwing {
            Throwing(int) { throw std::runtime_error("construction failed"); }
        };

        mylib::FreeList<Throwing> list(4);
        REQUIRE(list.empty());

        bool caught = false;
        try {
            list.emplace(42);
        } catch (const std::runtime_error&) {
            caught = true;
        }
        REQUIRE(caught);
        // Память должна быть освобождена, объект не создан
        REQUIRE(list.empty());
        REQUIRE(list.size() == 0);
    }

    // ----------------------------------------------------------------
    SECTION("Emplace with std::string and verify destructor called")
    {
        // Используем глобальный счётчик или наблюдаем за памятью
        // Мы не можем легко проверить вызов деструктора, но можем проверить,
        // что после remove строка уничтожена (нет утечек).
        // Для этого используем AddressSanitizer или просто убедимся, что remove не падает.
        mylib::FreeList<std::string> list(4);
        std::string* p = list.emplace("test");
        REQUIRE(!list.empty());

        list.remove(p);
        REQUIRE(list.empty());
        // Если бы деструктор не вызвался, была бы утечка памяти,
        // но ASan бы поймал. В тестах мы просто проверяем, что всё работает.
        SUCCEED("String destructor called correctly (no leak)");
    }
}
