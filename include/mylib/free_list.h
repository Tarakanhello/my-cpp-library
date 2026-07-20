#ifndef FREE_LIST_H
#define FREE_LIST_H

#include <algorithm>
#include <cstddef>

#include "mylib/list.h"
#include "mylib/memory.h"
#include "mylib/static_free_list.h"



namespace mylib
{

    /**
     * @brief Динамический пул фиксированных блоков с автоматическим ростом.
     *
     * Класс управляет набором блоков (StaticFreeList), каждый из которых содержит
     * массив узлов для объектов типа T. При исчерпании свободного места в текущем
     * блоке создаётся новый блок с размером, увеличивающимся в два раза (до
     * максимального заданного размера). Это позволяет эффективно распределять память
     * для большого числа объектов без частых выделений.
     *
     * @tparam T             Тип хранимых объектов.
     * @tparam minBlockSize  Минимальный размер блока (количество узлов).
     * @tparam maxBlockSize  Максимальный размер блока.
     * @tparam ALLOCATOR     Аллокатор, используемый для выделения памяти под блоки.
     *
     * Особенности:
     * - Операции выделения/освобождения не вызывают конструкторы/деструкторы T
     *   (кроме emplace/remove).
     * - Поддерживает перемещение, но не копирование.
     * - При освобождении последнего узла в блоке блок остаётся в пуле до тех пор,
     *   пока не появится второй пустой блок – тогда один из них удаляется.
     */
    template<typename T, size_t minBlockSize = 8, size_t maxBlockSize = 8192, typename ALLOCATOR = mylib::MySimpleAllocator<T>>
    class FreeList final
    {
    private:
        static constexpr const size_t DEFAULT_SIZE      { 32 };

    public:
        /** @brief Тип блока – StaticFreeList, содержащий узлы с T. */
        using Block = StaticFreeList<T, ALLOCATOR>;
        /** @brief Аллокатор для блоков (переопределён для Block). */
        using BlockAllocator = typename std::allocator_traits<ALLOCATOR>::template rebind_alloc<Block>;
        /** @brief Список блоков (двусвязный), используемый для хранения всех блоков. */
        using BlockList = mylib::List<Block, BlockAllocator>;

    private:
        BlockList m_blocks{};           //!< Контейнер всех блоков.
        size_t m_currentBlockSize{};    //!< Размер следующего создаваемого блока.
        size_t m_size{};                //!< Общее количество занятых узлов (выделенных объектов).
        size_t m_capacity{};            //!< Общая ёмкость всех блоков.

        const size_t m_minBlockSize{  minBlockSize };   //!< Минимальный размер блока.
        const size_t m_maxBlockSize{ maxBlockSize };    //!< Максимальный размер блока.

        size_t m_numberOfEmptyBlock{};  //!< Количество полностью пустых блоков.

        /**
         * @brief Создаёт новый блок и добавляет его в начало списка.
         * @post Новый блок имеет размер m_currentBlockSize, который затем удваивается
         *       (ограничен m_maxBlockSize). Ёмкость пула увеличивается, счётчик пустых
         *       блоков инкрементируется.
         */
        void createNewBlock();

        /**
         * @brief Сбрасывает состояние пула в исходное (без очистки памяти).
         * @note Используется в move-операциях.
         */
        void release() noexcept;

        // ========================================================================
        // Жизненный цикл: конструкторы, деструктор, перемещение
        // ========================================================================
    public:
        /**
         * @brief Конструктор, создающий пул с начальной ёмкостью.
         * @param initialSize Желаемое начальное количество элементов.
         *                    Если меньше minBlockSize, используется minBlockSize.
         *                    Если больше maxBlockSize, создаётся несколько блоков
         *                    максимального размера, чтобы покрыть initialSize.
         */
        explicit FreeList(size_t initialSize = DEFAULT_SIZE);

        FreeList(const FreeList&) = delete;
        FreeList& operator=(const FreeList&) = delete;

        /**
         * @brief Конструктор перемещения.
         * @param other Пул, из которого перемещаются данные.
         */
        FreeList(FreeList&& other) noexcept;

        /**
         * @brief Оператор перемещающего присваивания.
         * @param other Пул, из которого перемещаются данные.
         * @return Ссылка на *this.
         */
        FreeList& operator=(FreeList&& other) noexcept;

        /** @brief Деструктор (ничего не делает, все блоки удаляются автоматически). */
        ~FreeList() noexcept {}

        // ========================================================================
        // Основные операции: выделение и освобождение памяти
        // ========================================================================

        /**
         * @brief Выделяет сырую память под объект T без вызова конструктора.
         * @return Указатель на неинициализированную память.
         */
        T* allocateRaw();

        /**
         * @brief Освобождает сырую память без вызова деструктора.
         * @param ptr Указатель, полученный от allocateRaw().
         */
        void deallocateRaw(T* ptr) noexcept;

        /**
         * @brief Создаёт объект T в пуле с переданными аргументами.
         * @tparam ARGS Типы аргументов для конструктора T.
         * @param args Аргументы конструктора.
         * @return Указатель на сконструированный объект.
         * @throws Любое исключение, брошенное конструктором T или выделением памяти.
         *         В случае исключения выделенная память автоматически возвращается в пул.
         */
        template<typename... ARGS>
        T* emplace(ARGS&&... args);

        /**
         * @brief Уничтожает объект и освобождает память.
         * @param ptr Указатель на объект, полученный от emplace или allocateRaw.
         * @param onlyDeallocate Если true, деструктор не вызывается (используется только для отката).
         */
        void remove(T* ptr, bool onlyDeallocate = false);

        // ========================================================================
        // Управление состоянием пула
        // ========================================================================

        /**
         * @brief Очищает все блоки и возвращает пул в состояние, аналогичное только что созданному.
         * @post empty() == true, size() == 0, blockCount() == 1, capacity() == m_minBlockSize.
         */
        void clear();


        // ========================================================================
        // Свойства и индикаторы состояния
        // ========================================================================
        bool empty() const noexcept { return m_size == 0; }
        explicit operator bool() const noexcept { return !empty(); }
        size_t size() const noexcept { return m_size; }
        size_t capacity() const noexcept{ return m_capacity; }
        size_t blockCount() const noexcept { return m_blocks.size(); }
    };


} // end namespace mylib;



template<typename T, size_t minBlockSize, size_t maxBlockSize, typename ALLOCATOR>
T* mylib::FreeList<T, minBlockSize,maxBlockSize, ALLOCATOR>::allocateRaw()
{
    if(m_size == m_capacity)
    {
        createNewBlock();
    }

    bool blockWasEmpty{ m_blocks.front().empty() };

    typename Block::Node* node{ m_blocks.front().allocate() };

    if(blockWasEmpty)
    {
        --m_numberOfEmptyBlock;
    }

    node->userData = static_cast<void*>(m_blocks.begin().getNode());

    if(m_blocks.front().isFull())
    {
        m_blocks.moveToEnd(m_blocks.begin());
    }

    ++m_size;
    return &(node->value);
}



template<typename T, size_t minBlockSize, size_t maxBlockSize, typename ALLOCATOR>
void mylib::FreeList<T, minBlockSize,maxBlockSize, ALLOCATOR>::deallocateRaw(T* ptr) noexcept
{
    remove(ptr, true);
}



template<typename T, size_t minBlockSize, size_t maxBlockSize, typename ALLOCATOR>
template<typename... ARGS>
T* mylib::FreeList<T, minBlockSize,maxBlockSize, ALLOCATOR>::emplace(ARGS&&... args)
{
    T* place{ allocateRaw() };

    try
    {
        new (place) T(std::forward<ARGS>(args)...);
    }
    catch(...)
    {
        deallocateRaw(place);
        throw;
    }

    return place;
}



template<typename T, size_t minBlockSize, size_t maxBlockSize, typename ALLOCATOR>
void mylib::FreeList<T, minBlockSize,maxBlockSize, ALLOCATOR>::
    clear()
{
    m_blocks.clear();

    release();

    createNewBlock();
}




template<typename T, size_t minBlockSize, size_t maxBlockSize, typename ALLOCATOR>
void mylib::FreeList<T, minBlockSize,maxBlockSize, ALLOCATOR>::
    createNewBlock()
{
    m_blocks.push_front(StaticFreeList<T, ALLOCATOR>{ m_currentBlockSize });
    m_capacity += m_currentBlockSize;
    m_currentBlockSize = std::min(m_maxBlockSize, 2 * m_currentBlockSize);

    ++m_numberOfEmptyBlock;
}



template<typename T, size_t minBlockSize, size_t maxBlockSize, typename ALLOCATOR>
mylib::FreeList<T, minBlockSize,maxBlockSize, ALLOCATOR>::
    FreeList(size_t initialSize)
    : m_currentBlockSize{ m_minBlockSize }
    , m_size{ 0 }
    , m_capacity{ 0 }
    , m_numberOfEmptyBlock{ 0 }
{
    size_t neededCapacity{ std::max(initialSize, m_minBlockSize) };
    if(neededCapacity >= m_maxBlockSize)
    {
        m_currentBlockSize = m_maxBlockSize;

        // Количество блоков m_maxBlockSize, чтобы покрыть neededCapacity с избытком
        size_t numBlocks{ (neededCapacity + m_maxBlockSize - 1) / m_maxBlockSize };
        for (size_t i{}; i < numBlocks; ++i)
        {
            createNewBlock();
        }
    }
    else
    {
        m_currentBlockSize = neededCapacity;
        createNewBlock();
    }
}



template<typename T, size_t minBlockSize, size_t maxBlockSize, typename ALLOCATOR>
mylib::FreeList<T, minBlockSize,maxBlockSize, ALLOCATOR>::
    FreeList(FreeList&& other) noexcept
    : m_blocks{ std::move(other.m_blocks) }
    , m_currentBlockSize{ other.m_currentBlockSize }
    , m_size{ other.m_size }
    , m_capacity{ other.m_capacity }
    , m_numberOfEmptyBlock{ other.m_numberOfEmptyBlock }
{
    other.release();
}



template<typename T, size_t minBlockSize, size_t maxBlockSize, typename ALLOCATOR>
mylib::FreeList<T, minBlockSize,maxBlockSize, ALLOCATOR>&
    mylib::FreeList<T, minBlockSize,maxBlockSize, ALLOCATOR>::
        operator=(FreeList&& other) noexcept
{
    if(this != &other)
    {
        m_blocks = std::move(other.m_blocks);
        m_currentBlockSize = other.m_currentBlockSize;
        m_size = other.m_size;
        m_capacity = other.m_capacity;
        m_numberOfEmptyBlock = other.m_numberOfEmptyBlock;

        other.release();
    }

    return *this;
}



template<typename T, size_t minBlockSize, size_t maxBlockSize, typename ALLOCATOR>
void mylib::FreeList<T, minBlockSize,maxBlockSize, ALLOCATOR>::
    release() noexcept
{
    m_currentBlockSize = m_minBlockSize;
    m_size = 0;
    m_capacity = 0;
    m_numberOfEmptyBlock = 0;
}



template<typename T, size_t minBlockSize, size_t maxBlockSize, typename ALLOCATOR>
void mylib::FreeList<T, minBlockSize,maxBlockSize, ALLOCATOR>::
    remove(T* ptr, bool onlyDeallocate)
{
    if(ptr == nullptr )
    {
        return;
    }

    typename Block::Node* blockNode{ reinterpret_cast<Block::Node*>(ptr) };

    assert(blockNode->userData);

    typename BlockList::Iterator it{ reinterpret_cast<BlockList::BaseNode*>(blockNode->userData) };

    Block* block{ it.operator->() };
    onlyDeallocate ? block->releaseNode(blockNode) : block->remove(blockNode);
    --m_size;
    m_blocks.moveToBegin(it);

    if(block->empty())
    {
        ++m_numberOfEmptyBlock;

        if(m_numberOfEmptyBlock > 1)
        {
            m_capacity -= block->capacity();
            --m_numberOfEmptyBlock;
            m_blocks.erase(m_blocks.begin());
        }
    }
}


#endif // FREE_LIST_H
