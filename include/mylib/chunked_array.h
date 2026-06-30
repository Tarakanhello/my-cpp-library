#ifndef CHUNKED_ARRAY_H
#define CHUNKED_ARRAY_H

#include <cassert>
#include <compare>
#include <format>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <utility>

#include "mylib/memory.h"
#include "mylib/vector.h"



namespace mylib
{

    /**
     * @brief Блочный массив (chunked array) – динамический массив, разбитый на блоки фиксированного размера.
     * @tparam T Тип хранимых элементов.
     * @tparam CHUNK_SIZE Размер одного блока (количество элементов). По умолчанию 32.
     * @tparam ALLOCATOR Аллокатор для выделения памяти под элементы (не для указателей).
     *
     * Позволяет эффективно вставлять/удалять элементы в конце без перераспределения всей памяти.
     * В отличие от Vector, не требует копирования элементов при росте – добавляются только новые блоки.
     * Поддерживает строгую гарантию безопасности исключений для основных операций.
     */
    template<typename T,
             size_t CHUNK_SIZE = 32,
             typename ALLOCATOR = mylib::MySimpleAllocator<T>>
    class ChunkedArray final
    {
    private:
        /** @brief Тип аллокатора для массива указателей на блоки. */
        using ALLOCATOR_ptr = typename std::allocator_traits<ALLOCATOR>::
                                                    template rebind_alloc<T*>;

        size_t m_size{};                                    ///< Количество элементов.
        constexpr static size_t m_chunkSize{ CHUNK_SIZE };  ///< Размер блока (константа времени компиляции).
        ALLOCATOR m_chunkAllocator{};                       ///< Аллокатор для выделения блоков.
        mylib::Vector<T*, ALLOCATOR_ptr> m_arrayOfChunks{}; ///< Вектор указателей на блоки.

        /**
         * @brief Выделяет массив указателей на блоки и инициализирует каждый блок.
         * @throw std::bad_alloc при нехватке памяти; при ошибке уже выделенные блоки освобождаются.
         */
        void allocateArrayOfChunks();

        /**
         * @brief Выделяет один блок памяти для элементов.
         * @return Указатель на выделенный блок.
         * @throw std::bad_alloc при нехватке памяти.
         */
        T* allocateChunk();

        /**
         * @brief Вставляет элемент в конец (внутренняя реализация для lvalue и rvalue).
         * @tparam ARGS Типы аргументов для конструирования элемента.
         * @param args Аргументы, передаваемые конструктору T.
         * @throw Любое исключение при конструировании или выделении памяти.
         */
        template<typename... ARGS>
        void append(ARGS&&... args);

        /**
         * @brief Конструирует элементы в диапазоне [from, to) со значением value.
         * @param from Начальный индекс.
         * @param to   Конечный индекс (не включается).
         * @param value Значение для копирования.
         * @throw Любое исключение, брошенное конструктором T. При ошибке уже созданные элементы уничтожаются.
         */
        void constructElements(size_t from, size_t to, const T& value = T());

        /**
         * @brief Конструирует элементы из диапазона [first, last) начиная с индекса from.
         * @tparam ITERATOR Тип итератора (должен поддерживать operator* и инкремент).
         * @param from Индекс, с которого начинается вставка.
         * @param first Начало диапазона.
         * @param last Конец диапазона.
         * @throw Любое исключение при конструировании; при ошибке созданные элементы уничтожаются.
         */
        template<typename ITERATOR>
        void constructElementsFromRange(size_t from,
                                        ITERATOR first,
                                        ITERATOR last);

        /**
         * @brief Вычисляет индекс блока для заданного индекса элемента.
         * @param i Индекс элемента.
         * @return Номер блока.
         */
        constexpr size_t chunkIndex(size_t i) const noexcept;

        /**
         * @brief Освобождает все блоки и массив указателей (без уничтожения элементов).
         * @note Предполагается, что элементы уже уничтожены.
         */
        void deallocateArrayOfChunks() noexcept;

        /**
         * @brief Освобождает один блок памяти.
         * @param chunk Указатель на блок.
         */
        void deallocateChunk(T* chunk) noexcept;

        /**
         * @brief Уничтожает все элементы во всех блоках и освобождает память блоков.
         * @note noexcept – деструкторы и deallocate не должны бросать исключения.
         */
        void destroyAllChunks() noexcept;

        /**
         * @brief Уничтожает элементы в диапазоне индексов [from, to).
         * @param from Начальный индекс.
         * @param to   Конечный индекс (не включается).
         * @note noexcept – вызов деструкторов не должен бросать исключения.
         */
        void destroyElements(size_t from, size_t to) noexcept;

        /**
         * @brief Возвращает указатель на блок по его индексу.
         * @param chunkIndex Индекс блока.
         * @return Указатель на блок.
         */
        constexpr T* getChunk(size_t chunkIndex) noexcept;
        constexpr const T* getChunk(size_t chunkIndex) const noexcept;

        /**
         * @brief Вычисляет смещение внутри блока для заданного индекса элемента.
         * @param i Индекс элемента.
         * @return Смещение внутри блока.
         */
        constexpr size_t offsetInChunk(size_t i) const noexcept;

        /**
         * @brief Освобождает указатели на блоки и обнуляет размер (без уничтожения элементов).
         * @note Используется в move-операциях.
         */
        void release() noexcept;

        /**
         * @brief Удаляет последние count блоков из массива указателей и освобождает их память.
         * @param count Количество удаляемых блоков.
         * @note Предполагается, что элементы в этих блоках уже уничтожены.
         */
        void removeLastBlocks(size_t count) noexcept;

        /**
         * @brief Обменивает содержимое с другим объектом.
         * @param other Другой ChunkedArray.
         */
        void swap(ChunkedArray& other) noexcept;

    public:
        // ==================== КОНСТРУКТОРЫ И ДЕСТРУКТОР ====================

        /**
         * @brief Конструктор по умолчанию. Создаёт пустой массив.
         */
        ChunkedArray();

        /**
         * @brief Конструктор, создающий массив с size элементами, инициализированными значением value.
         * @param size Количество элементов.
         * @param value Значение для инициализации (по умолчанию T()).
         * @param alloc Аллокатор.
         */
        ChunkedArray(size_t size, T value = T(), ALLOCATOR alloc = ALLOCATOR());

        /**
         * @brief Конструктор из std::initializer_list.
         * @param list Список инициализации.
         * @param alloc Аллокатор.
         */
        ChunkedArray(std::initializer_list<T> list, ALLOCATOR alloc = ALLOCATOR());

        /**
         * @brief Конструктор копирования.
         * @param other Массив для копирования.
         */
        ChunkedArray(const ChunkedArray& other);

        /**
     * @brief Конструктор из произвольного контейнера, поддерживающего итераторы.
     * @tparam CONTAINER Тип контейнера (должен иметь методы begin() и end()).
     * @param container Контейнер, элементы которого копируются в новый ChunkedArray.
     * @param alloc Аллокатор для выделения блоков.
     *
     * Создаёт массив, содержащий копии всех элементов из переданного контейнера.
     * Количество элементов определяется как std::distance(container.begin(), container.end()).
     *
     * @note Конструктор участвует в разрешении перегрузки только если:
     *       - CONTAINER имеет begin() и end() (возвращают итераторы),
     *       - CONTAINER не является самим ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>
     *         (чтобы не конфликтовать с конструктором копирования).
     *
     * @throw std::bad_alloc при нехватке памяти для выделения блоков.
     * @throw Любое исключение, брошенное конструктором T при копировании элементов.
     *       В случае ошибки все выделенные блоки освобождаются.
     */
        template<typename CONTAINER>
        requires requires(CONTAINER c)
        {
            c.begin();
            c.end();
        } && (!std::same_as<CONTAINER, ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>>)
        ChunkedArray(const CONTAINER& container, ALLOCATOR alloc = ALLOCATOR());

        /**
         * @brief Оператор присваивания копированием (через copy-and-swap).
         * @param other Массив для копирования.
         * @return Ссылка на *this.
         */
        ChunkedArray& operator=(const ChunkedArray& other);

        /**
         * @brief Конструктор перемещения.
         * @param other Массив, из которого перемещаются данные.
         */
        ChunkedArray(ChunkedArray&& other) noexcept;

        /**
         * @brief Оператор присваивания перемещением.
         * @param other Массив, из которого перемещаются данные.
         * @return Ссылка на *this.
         */
        ChunkedArray& operator=(ChunkedArray&& other) noexcept;

        /**
         * @brief Деструктор. Уничтожает все элементы и освобождает память блоков.
         */
        ~ChunkedArray();

         // ==================== ДОСТУП К ЭЛЕМЕНТАМ ====================

        /**
         * @brief Доступ к элементу с проверкой границ.
         * @param i Индекс элемента.
         * @return Ссылка на элемент.
         * @throw std::out_of_range если i >= size().
         */
        constexpr T& at(size_t i);
        constexpr const T& at(size_t i) const;

        /**
         * @brief Доступ к элементу без проверки границ.
         * @param i Индекс элемента.
         * @return Ссылка на элемент.
         * @pre i < size() (проверяется через assert в отладочной сборке).
         */
        constexpr T& operator[](size_t i) noexcept;
        constexpr const T& operator[](size_t i) const noexcept;


        /**
         * @brief Возвращает ссылку на первый элемент.
         * @return Ссылка на первый элемент.
         * @pre Массив не пуст (assert).
         */
        constexpr T& front() noexcept;
        constexpr const T& front() const noexcept;

        /**
         * @brief Возвращает ссылку на последний элемент.
         * @return Ссылка на последний элемент.
         * @pre Массив не пуст (assert).
         */
        constexpr T& back() noexcept;
        constexpr const T& back() const noexcept;

        // ==================== ВСТАВКА ====================

        /**
         * @brief Добавляет элемент в конец (копирование).
         * @param value Добавляемый элемент.
         */
        void push_back(const T& value);

        /**
         * @brief Добавляет элемент в конец (перемещение).
         * @param value Добавляемый элемент.
         */
        void push_back(T&& value);

        /**
         * @brief Конструирует элемент в конце из переданных аргументов.
         * @tparam ARGS Типы аргументов конструктора T.
         * @param args Аргументы для конструирования.
         */
        template<typename... ARGS>
        void emplace_back(ARGS&&... args);

        // ==================== УДАЛЕНИЕ ====================

        /**
         * @brief Удаляет последний элемент.
         * @return Удалённый элемент (копия).
         * @pre Массив не пуст (assert).
         * @note Если блок становится пустым, он освобождается.
         */
        T pop_back();

        /**
         * @brief Очищает массив (удаляет все элементы и освобождает память блоков).
         */
        void clear();

         // ==================== ИЗМЕНЕНИЕ РАЗМЕРА И ЁМКОСТИ ====================

        /**
         * @brief Изменяет размер массива.
         * @param newSize Новый размер.
         * @param value Значение для инициализации новых элементов (если newSize > текущего).
         * @note При увеличении добавляются новые блоки; при уменьшении лишние блоки освобождаются.
         * @throw Любое исключение при конструировании или выделении памяти.
         */
        void resize(size_t newSize, const T& value = T());

        /**
         * @brief Резервирует память для как минимум newCapacity элементов (резервирует блоки).
         * @param newCapacity Желаемая ёмкость.
         * @note Если newCapacity <= size(), ничего не делает.
         * @throw std::bad_alloc при нехватке памяти.
         */
        void reserve(size_t newCapacity);

        /**
         * @brief Уменьшает количество блоков до минимально необходимого для текущего размера.
         * @throw std::bad_alloc при нехватке памяти (если требуется перераспределение).
         */
        void shrink_to_fit();

        // ==================== СВОЙСТВА ====================

        /**
         * @brief Возвращает текущий размер (количество элементов).
         */
        size_t size() const noexcept { return m_size; }

        /**
         * @brief Проверяет, пуст ли массив.
         * @return true, если size() == 0.
         */
        constexpr bool empty() const noexcept { return size() == 0; }

        /**
         * @brief Возвращает количество выделенных блоков.
         */
        size_t blockCount() const noexcept { return m_arrayOfChunks.size(); }

        /**
         * @brief Явное приведение к bool: true, если массив не пуст.
         */
        explicit operator bool() const noexcept { return !empty(); }

        // ==================== ИТЕРАТОРЫ ====================

        class Iterator;
        class ConstIterator;
        using ReverseIterator = std::reverse_iterator<Iterator>;
        using constReverseIterator = std::reverse_iterator<ConstIterator>;

        /**
         * @brief Возвращает итератор на первый элемент.
         */
        constexpr Iterator begin() noexcept;

        /**
         * @brief Возвращает константный итератор на первый элемент.
         */
        constexpr ConstIterator begin() const noexcept;

        /**
         * @brief Возвращает константный итератор на первый элемент.
         */
        constexpr ConstIterator cbegin() const noexcept;

        /**
         * @brief Возвращает итератор на элемент, следующий за последним.
         */
        constexpr Iterator end() noexcept;

        /**
         * @brief Возвращает константный итератор на элемент, следующий за последним.
         */
        constexpr ConstIterator end() const noexcept;

        /**
         * @brief Возвращает константный итератор на элемент, следующий за последним.
         */
        constexpr ConstIterator cend() const noexcept;

        /**
         * @brief Возвращает обратный итератор на элемент, следующий за последним.
         */
        constexpr ReverseIterator rbegin() noexcept;

        /**
         * @brief Возвращает константный обратный итератор на элемент, следующий за последним.
         */
        constexpr constReverseIterator rbegin() const noexcept;

        /**
         * @brief Возвращает константный обратный итератор на элемент, следующий за последним.
         */
        constexpr constReverseIterator crbegin() const noexcept;

        /**
         * @brief Возвращает обратный итератор на первый элемент.
         */
        constexpr ReverseIterator rend() noexcept;
        /**
         * @brief Возвращает константный обратный итератор на первый элемент.
         */
        constexpr constReverseIterator rend() const noexcept;

        /**
         * @brief Возвращает константный обратный итератор на первый элемент.
         */
        constexpr constReverseIterator crend() const noexcept;

    }; // end class ChunkedArray


    /**
     * @brief Итератор произвольного доступа для ChunkedArray.
     *
     * Позволяет обходить элементы, хранящиеся в блоках. При достижении конца текущего блока
     * автоматически переключается на следующий блок. Поддерживает все операции
     * RandomAccessIterator: инкремент, декремент, арифметику, сравнение и доступ по индексу.
     */
    template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
    class ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::Iterator
    {
    private:
        T** m_chunkBegin;       ///< Указатель на первый блок в массиве блоков.
        T** m_chunkPtr;         ///< Указатель на текущий блок.
        T** m_chunksEnd;        ///< Указатель на конец массива блоков (за последним блоком).
        T* m_currentElementPtr; ///< Указатель на текущий элемент внутри блока.
        size_t m_offset;        ///< Смещение текущего элемента внутри блока (0..CHUNK_SIZE-1).

    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = T*;
        using reference         = T&;

        friend class ConstIterator;

        /**
         * @brief Конструирует итератор, указывающий на элемент с индексом startIndex.
         * @param beginChunkPtr Указатель на первый блок в массиве блоков.
         * @param chunksEnd    Указатель на конец массива блоков.
         * @param startIndex   Индекс элемента, на который должен указывать итератор (по умолчанию 0).
         * @note Если startIndex выходит за пределы допустимого диапазона, поведение не определено.
         */
        constexpr Iterator(T** beginChunkPtr,
                           T** chunksEnd,
                           size_t startIndex = 0) noexcept;
        constexpr T&        operator*() const noexcept;
        constexpr T*        operator->() const noexcept;
        constexpr Iterator& operator++() noexcept;
        constexpr Iterator  operator++(int) noexcept;
        constexpr Iterator& operator--() noexcept;
        constexpr Iterator  operator--(int) noexcept;
        constexpr Iterator& operator+=(std::ptrdiff_t n) noexcept;
        constexpr Iterator& operator-=(std::ptrdiff_t n) noexcept;
        constexpr Iterator  operator+(std::ptrdiff_t n) noexcept;
        constexpr Iterator  operator-(std::ptrdiff_t n) noexcept;
        friend constexpr  Iterator  operator+(std::ptrdiff_t n,
                                              const Iterator& it) noexcept { return it + n; }
        constexpr std::ptrdiff_t    operator-(const Iterator& other) const noexcept;
        constexpr auto      operator<=>(const Iterator& other) const noexcept;
        constexpr bool      operator==(const Iterator& other) const noexcept;
        constexpr T&        operator[](std::ptrdiff_t n) const noexcept;
    }; // end class Iterator



    /**
     * @brief Константный итератор произвольного доступа для ChunkedArray.
     *
     * Предоставляет доступ только для чтения к элементам, хранящимся в блоках.
     * Автоматически переключается между блоками при достижении их границ.
     * Поддерживает все операции RandomAccessIterator: инкремент, декремент,
     * арифметику, сравнение и доступ по индексу.
     *
     * @note Может быть неявно создан из неконстантного Iterator.
     */
    template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
    class ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::ConstIterator
    {
    private:
        const T* const* m_chunkBegin;         ///< Указатель на первый блок в массиве блоков.
        const T* const* m_chunkPtr;           ///< Указатель на текущий блок.
        const T* const* m_chunksEnd;          ///< Указатель на конец массива блоков (за последним блоком).
        const T* m_currentElementPtr;   ///< Указатель на текущий элемент внутри блока.
        size_t m_offset;                ///< Смещение текущего элемента внутри блока (0..CHUNK_SIZE-1).

    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const T*;
        using reference         = const T&;

        /**
         * @brief Конструирует итератор, указывающий на элемент с индексом startIndex.
         * @param beginChunkPtr Указатель на первый блок в массиве блоков.
         * @param chunksEnd    Указатель на конец массива блоков (за последним блоком).
         * @param startIndex   Индекс элемента, на который должен указывать итератор (по умолчанию 0).
         * @note Если startIndex выходит за пределы допустимого диапазона, поведение не определено.
         */
        constexpr ConstIterator(const T* const* beginChunkPtr,
                                const T* const* chunksEnd,
                                size_t startIndex = 0) noexcept;

        /**
         * @brief Конвертирующий конструктор из неконстантного итератора.
         * @param other Неконстантный итератор.
         */
        constexpr ConstIterator(const Iterator& other) noexcept;
        constexpr const T& operator*() const noexcept;
        constexpr const T* operator->() const noexcept;
        constexpr ConstIterator& operator++() noexcept;
        constexpr ConstIterator operator++(int) noexcept;
        constexpr ConstIterator& operator--() noexcept;
        constexpr ConstIterator operator--(int) noexcept;
        constexpr ConstIterator& operator+=(std::ptrdiff_t n) noexcept;
        constexpr ConstIterator& operator-=(std::ptrdiff_t n) noexcept;
        constexpr ConstIterator operator+(std::ptrdiff_t n) noexcept;
        constexpr ConstIterator operator-(std::ptrdiff_t n) noexcept;
        friend constexpr  ConstIterator operator+(std::ptrdiff_t n,
                                                 const ConstIterator& it) noexcept {return it + n; }
        constexpr std::ptrdiff_t operator-(const ConstIterator& other) const noexcept;
        constexpr auto operator<=>(const ConstIterator& other) const noexcept;
        constexpr bool operator==(const ConstIterator& other) const noexcept;
        constexpr const T& operator[](std::ptrdiff_t n) const noexcept;
    }; // end class ConstIterator
} // end namespace mylib



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    ChunkedArray()
    : m_size{ 0 }
    , m_chunkAllocator{}
    , m_arrayOfChunks{}
{}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    ChunkedArray(size_t size, T value, ALLOCATOR alloc)
    : m_size{ size }
    , m_chunkAllocator{ alloc }
{
    allocateArrayOfChunks();

    try
    {
        constructElements(0, size, value);
    }
    catch(...)
    {
        deallocateArrayOfChunks();
        throw;
    }
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    ChunkedArray(std::initializer_list<T> list, ALLOCATOR alloc)
    : m_size{ list.size() }
    , m_chunkAllocator{ alloc }
{
    allocateArrayOfChunks();

    try
    {
        constructElementsFromRange(0, list.begin(), list.end());
    }
    catch(...)
    {
        deallocateArrayOfChunks();
        throw;
    }
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    ChunkedArray(const ChunkedArray& other)
    : m_size{ other.size() }
    , m_chunkAllocator{ other.m_chunkAllocator }
{
    allocateArrayOfChunks();

    try
    {
        constructElementsFromRange(0, other.begin(), other.end());
    }
    catch(...)
    {
        deallocateArrayOfChunks();
        throw;
    }
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
template<typename CONTAINER>
requires requires(CONTAINER c)
{
    c.begin();
    c.end();
} && (!std::same_as<CONTAINER, mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>>)
mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    ChunkedArray(const CONTAINER& container, ALLOCATOR alloc)
    : m_chunkAllocator{ alloc }
{
    m_size = std::distance(container.begin(), container.end());

    allocateArrayOfChunks();

    try
    {
        constructElementsFromRange(0, container.begin(), container.end());
    }
    catch(...)
    {
        deallocateArrayOfChunks();
        throw;
    }
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>& mylib::ChunkedArray<T, CHUNK_SIZE,
                                                                   ALLOCATOR>::
    operator=(const ChunkedArray& other)
{
    if(this != &other)
    {
        ChunkedArray temp{ other };

        swap(temp);
    }

    return *this;
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    ChunkedArray(ChunkedArray&& other) noexcept
    : m_size{ other.m_size }
    , m_chunkAllocator{ std::move(other.m_chunkAllocator) }
{
    m_arrayOfChunks = std::move(other.m_arrayOfChunks);
    other.release();
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>& mylib::ChunkedArray<T, CHUNK_SIZE,
                                                                   ALLOCATOR>::
    operator=(ChunkedArray&& other) noexcept
{
    if(this != &other)
    {
        swap(other);
        other.destroyAllChunks();
        other.release();
    }

    return *this;
}


template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    ~ChunkedArray()
{
    destroyAllChunks();
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
T* mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    allocateChunk()
{
    return m_chunkAllocator.allocate(m_chunkSize);
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
void mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    allocateArrayOfChunks()
{
    if(size())
    {
        m_arrayOfChunks = mylib::Vector<T*,
                        ALLOCATOR_ptr>((size() - 1) / m_chunkSize + 1, nullptr);
        size_t count{};
        try
        {
            for(auto& chunk : m_arrayOfChunks)
            {
                chunk = allocateChunk();
                ++count;
            }
        }
        catch(...)
        {
            for(size_t i{}; i < count; ++i)
            {
                deallocateChunk(m_arrayOfChunks[i]);
            }

            m_arrayOfChunks.clear();
            throw;
        }
    }
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
template<typename... ARGS>
void mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    append(ARGS&&... args)
{
    if( size() == 0 || offsetInChunk(size() - 1) == m_chunkSize - 1)
    {
        // Нужен новый блок
        T* newChunk{ allocateChunk() };
        try
        {
             // Конструируем элемент прямо в новом блоке
            new (newChunk) T(std::forward<ARGS>(args)...);
        }
        catch(...)
        {
            deallocateChunk(newChunk);
            throw;
        }

        try
        {
            m_arrayOfChunks.push_back(newChunk);
        }
        catch(...)
        {
            newChunk->~T();
            deallocateChunk(newChunk);
            throw;
        }
    }
    else
    {
        new (&m_arrayOfChunks[chunkIndex(size())][offsetInChunk(size())])
                T( std::forward<ARGS>(args)...);
    }

    ++m_size;
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr T& mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    at(size_t i)
{
    if(i >= size())
    {
        throw std::out_of_range(std::format("An index must be less then the size. "
                                            "You have index: {}, size: {}", i, size()));
    }

    return (*this)[i];
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr const T& mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    at(size_t i) const
{
    if(i >= size())
    {
        throw std::out_of_range(std::format("An index must be less then the size. "
                                            "You have index: {}, size: {}", i, size()));
    }

    return (*this)[i];
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr T& mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    back() noexcept
{
    assert(!empty());
    return (*this)[size() - 1];
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr const T& mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    back() const noexcept
{
    assert(!empty());
    return (*this)[size() - 1];
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::Iterator
mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    begin() noexcept
{
    return Iterator{ m_arrayOfChunks.data(), m_arrayOfChunks.data() + m_arrayOfChunks.size() };
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::ConstIterator
mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    begin() const noexcept
{
    return cbegin();
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::ConstIterator
mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    cbegin() const noexcept
{
    return ConstIterator{ m_arrayOfChunks.data(), m_arrayOfChunks.data() + m_arrayOfChunks.size() };
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::ConstIterator mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    cend() const noexcept
{
    return ConstIterator{ m_arrayOfChunks.data(), m_arrayOfChunks.data() + m_arrayOfChunks.size(), size() };

}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr size_t mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    chunkIndex(size_t i) const noexcept
{
    return i / m_chunkSize;
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    ConstIterator::ConstIterator(const T* const* beginChunkPtr,
                                 const T* const* chunksEnd,
                                 size_t startIndex) noexcept
    : m_chunkBegin{ beginChunkPtr }
    , m_chunkPtr{ beginChunkPtr }
    , m_chunksEnd{ chunksEnd }
    , m_offset{ startIndex % m_chunkSize }
    , m_currentElementPtr{ nullptr }
{
    if(m_chunkPtr && m_chunksEnd && m_chunkPtr < m_chunksEnd)
    {
        size_t blockIndex{ startIndex / m_chunkSize };
        m_chunkPtr = m_chunkPtr + blockIndex;
        m_currentElementPtr = *m_chunkPtr + m_offset;
    }
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    ConstIterator::ConstIterator(const Iterator& other) noexcept
    : m_chunkBegin { other.m_chunkBegin }
    , m_chunkPtr{ other.m_chunkPtr }
    , m_chunksEnd{ other.m_chunksEnd }
    , m_offset{ other.m_offset }
    , m_currentElementPtr{ other.m_currentElementPtr }
{}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr const T& mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    ConstIterator::operator*() const noexcept
{
    assert(m_currentElementPtr);
    return *m_currentElementPtr;
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr const T* mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    ConstIterator::operator->() const noexcept
{
    assert(m_currentElementPtr);
    return m_currentElementPtr;
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    ConstIterator& mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    ConstIterator::operator++() noexcept
{
    assert(m_chunkPtr && m_currentElementPtr && m_chunkPtr < m_chunksEnd);

    ++m_offset;
    if(m_offset == m_chunkSize)
    {
        ++m_chunkPtr;
        if(m_chunkPtr < m_chunksEnd)
        {
            m_currentElementPtr = *m_chunkPtr;
        }
        else
        {
            m_currentElementPtr = nullptr;
        }
        m_offset = 0;
    }
    else
    {
        m_currentElementPtr = *m_chunkPtr + m_offset;
    }

    return *this;
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    ConstIterator mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    ConstIterator::operator++(int) noexcept
{
    ConstIterator tmp(*this);

    ++(*this);

    return tmp;
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    ConstIterator& mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    ConstIterator::operator--() noexcept
{
    assert(m_chunkPtr && m_currentElementPtr && m_chunkPtr < m_chunksEnd);

    if (m_chunkPtr == m_chunkBegin && m_offset == 0)
    {
        m_currentElementPtr = nullptr;
        return *this;
    }

    if(0 == m_offset)
    {
        m_offset = m_chunkSize;
        --m_chunkPtr;
    }

    --m_offset;

    if(m_chunkPtr >= m_chunkBegin && m_chunkPtr < m_chunksEnd)
    {
        m_currentElementPtr = *m_chunkPtr + m_offset;
    }
    else
    {
        m_currentElementPtr = nullptr;
    }

    return *this;
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    ConstIterator mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    ConstIterator::operator--(int) noexcept
{
    ConstIterator temp{ *this };

    --(*this);

    return temp;
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    ConstIterator& mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    ConstIterator::operator+=(std::ptrdiff_t n) noexcept
{
    if(0 == n)
    {
        return *this;
    }

    assert(m_chunkPtr && m_currentElementPtr && m_chunkPtr < m_chunksEnd);

    if(n < 0)
    {
        return (*this -= -n);
    }

    if(n >= m_chunkSize)
    {
        size_t multiplier{ n / m_chunkSize };
        m_chunkPtr += multiplier;
        n -= (multiplier * m_chunkSize);
        if(m_chunkPtr >= m_chunksEnd)
        {
            m_currentElementPtr = nullptr;
            m_offset = 0;
            return *this;
        }
    }

    if(m_offset + n >= m_chunkSize)
    {
        ++m_chunkPtr;
        m_offset = m_offset + n - m_chunkSize;
    }
    else
    {
        m_offset += n;
    }

    if(m_chunkPtr < m_chunksEnd)
    {
        m_currentElementPtr = *m_chunkPtr + m_offset;
    }
    else
    {
        m_currentElementPtr = nullptr;
    }

    return *this;
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    ConstIterator& mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    ConstIterator::operator-=(std::ptrdiff_t n) noexcept
{
    if(0 == n)
    {
        return *this;
    }

    assert(m_chunkPtr && m_currentElementPtr && m_chunkPtr < m_chunksEnd);

    if(n < 0)
    {
        return (*this += -n);
    }

    if(n >= m_chunkSize)
    {
        size_t multiplier{ n / m_chunkSize };

        m_chunkPtr -= multiplier;
        n -= (multiplier * m_chunkSize);

        if(m_chunkPtr < m_chunkBegin)
        {
            m_currentElementPtr = nullptr;
            m_offset = 0;
            return *this;
        }
    }

    if(static_cast<std::ptrdiff_t>(m_offset) - n < 0)
    {
        --m_chunkPtr;
        m_offset = m_offset - n + m_chunkSize;
    }
    else
    {
        m_offset -= n;
    }

    if(m_chunkPtr >= m_chunkBegin && m_chunkPtr < m_chunksEnd)
    {
        m_currentElementPtr = *m_chunkPtr + m_offset;
    }
    else
    {
        m_currentElementPtr = nullptr;
    }

    return *this;
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    ConstIterator mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    ConstIterator::operator+(std::ptrdiff_t n) noexcept
{
    ConstIterator temp{ *this };
    temp += n;
    return temp;
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    ConstIterator mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    ConstIterator::operator-(std::ptrdiff_t n) noexcept
{

    ConstIterator temp{ *this };
    temp -= n;
    return temp;

}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr std::ptrdiff_t mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    ConstIterator::operator-(const ConstIterator& other) const noexcept
{
    std::ptrdiff_t timesMultiply{ m_chunkPtr - other.m_chunkPtr };

    std::ptrdiff_t offset{ static_cast<std::ptrdiff_t>(m_offset) - static_cast<std::ptrdiff_t>(other.m_offset) };

    return timesMultiply * m_chunkSize + offset;
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr auto mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    ConstIterator::operator<=>(const ConstIterator& other) const noexcept
{
    if(m_chunkPtr < other.m_chunkPtr)
    {
        return std::strong_ordering::less;
    }
    else if(m_chunkPtr > other.m_chunkPtr)
    {
        return std::strong_ordering::greater;
    }
    else
    {
        if(m_currentElementPtr == other.m_currentElementPtr)
        {
            return std::strong_ordering::equal;
        }
        else if(m_currentElementPtr == nullptr)
        {
            return std::strong_ordering::greater;
        }
        else if(other.m_currentElementPtr == nullptr)
        {
            return std::strong_ordering::less;
        }
    }

    return m_offset <=> other.m_offset;
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr bool mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    ConstIterator::operator==(const ConstIterator& other) const noexcept
{
    return m_chunkPtr == other.m_chunkPtr && m_currentElementPtr == other.m_currentElementPtr;
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr const T& mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    ConstIterator::operator[](std::ptrdiff_t n) const noexcept
{
    return *(*this + n);
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
void mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    constructElements(size_t from, size_t to, const T& value)
{
    size_t i{ from };
    try
    {
        for(; i < to; ++i)
        {
            new (&m_arrayOfChunks[chunkIndex(i)][offsetInChunk(i)]) T(value);
        }
    }
    catch(...)
    {
        destroyElements(from, i);
        throw;
    }
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
template<typename ITERATOR>
void mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    constructElementsFromRange(size_t from, ITERATOR first, ITERATOR last)
{
    size_t i{ from };
    try
    {
        while(first != last)
        {
            new (&m_arrayOfChunks[chunkIndex(i)][offsetInChunk(i)]) T (*first);
            ++first;
            ++i;
        }
    }
    catch(...)
    {
        destroyElements(from, i);
        throw;
    }
}


template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
void mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::clear()
{
    destroyAllChunks();
    m_size = 0;
    m_arrayOfChunks.clear();
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::constReverseIterator
    mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    crbegin() const noexcept
{
    return cend();

}




template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::constReverseIterator
    mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    crend() const noexcept
{
    return cbegin();
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
void mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    deallocateArrayOfChunks() noexcept
{
    if(m_arrayOfChunks)
    {
        for(auto& chunk : m_arrayOfChunks)
        {
            deallocateChunk(chunk);
        }
        m_arrayOfChunks.clear();
    }
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
void mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    deallocateChunk(T* chunk) noexcept
{
    m_chunkAllocator.deallocate(chunk, m_chunkSize);
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
void mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    destroyAllChunks() noexcept
{
    if(m_arrayOfChunks)
    {
        for(size_t i{}; i < size(); ++i)
        {
            m_arrayOfChunks[chunkIndex(i)][offsetInChunk(i)].~T();
        }
        for(size_t i{}; i < m_arrayOfChunks.size(); ++i)
        {
            deallocateChunk(m_arrayOfChunks[i]);
            m_arrayOfChunks[i] = nullptr;
        }
    }
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
void mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    destroyElements(size_t from, size_t to) noexcept
{
    for(size_t i{ from }; i < to; ++i)
    {
        m_arrayOfChunks[chunkIndex(i)][offsetInChunk(i)].~T();
    }
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::Iterator
    mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    end() noexcept
{
    return Iterator{ m_arrayOfChunks.data(), m_arrayOfChunks.data() + m_arrayOfChunks.size(), size() };
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::ConstIterator
mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    end() const noexcept
{
    return cend();
}


template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
template<typename... ARGS>
void mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    emplace_back(ARGS&&... args)
{
    append(std::forward<ARGS>(args)...);
}


template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr T& mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    front() noexcept
{
    assert(!empty());
    return (*this)[0];
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr const T& mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    front() const noexcept
{
    assert(!empty());
    return (*this)[0];
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr T* mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    getChunk(size_t chunkIndex) noexcept
{
    return m_arrayOfChunks[chunkIndex];
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr const T* mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    getChunk(size_t chunkIndex) const noexcept
{
    return m_arrayOfChunks[chunkIndex];
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    Iterator::Iterator(T** beginChunkPtr, T** chunksEnd, size_t startIndex) noexcept
        : m_chunkBegin{ beginChunkPtr }
        , m_chunkPtr{ beginChunkPtr }
        , m_chunksEnd{ chunksEnd }
        , m_offset{ startIndex % m_chunkSize }
        , m_currentElementPtr{ nullptr }
{
    if(m_chunkPtr && m_chunksEnd && m_chunkPtr < m_chunksEnd)
    {
        size_t blockIndex{ startIndex / m_chunkSize };
        m_chunkPtr = m_chunkPtr + blockIndex;
        m_currentElementPtr = *m_chunkPtr + m_offset;
    }
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr T& mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    Iterator::operator*() const noexcept
{
    assert(m_currentElementPtr);
    return *m_currentElementPtr;
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr T* mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    Iterator::operator->() const noexcept
{
    assert(m_currentElementPtr);
    return m_currentElementPtr;
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::Iterator&
    mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    Iterator::operator++() noexcept
{
    assert(m_chunkPtr && m_currentElementPtr && m_chunkPtr < m_chunksEnd);

    ++m_offset;
    if(m_offset == m_chunkSize)
    {
        ++m_chunkPtr;
        if(m_chunkPtr < m_chunksEnd)
        {
            m_currentElementPtr = *m_chunkPtr;
        }
        else
        {
            m_currentElementPtr = nullptr;
        }
        m_offset = 0;
    }
    else
    {
        m_currentElementPtr = *m_chunkPtr + m_offset;
    }

    return *this;
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::Iterator
    mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    Iterator::operator++(int) noexcept
{
    Iterator tmp(*this);

    ++(*this);

    return tmp;
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::Iterator&
    mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    Iterator::operator--() noexcept
{
    assert(m_chunkPtr && m_currentElementPtr && m_chunkPtr < m_chunksEnd);

    if (m_chunkPtr == m_chunkBegin && m_offset == 0)
    {
        // уже на первом элементе первого блока – декремент невозможен
        m_currentElementPtr = nullptr;
        return *this;
    }

    if(0 == m_offset)
    {
        m_offset = m_chunkSize;
        --m_chunkPtr;
    }

    --m_offset;

    if(m_chunkPtr >= m_chunkBegin && m_chunkPtr < m_chunksEnd)
    {
        m_currentElementPtr = *m_chunkPtr + m_offset;
    }
    else
    {
        m_currentElementPtr = nullptr;
    }

    return *this;
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::Iterator
    mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    Iterator::operator--(int) noexcept
{
    Iterator temp{ *this };

    --(*this);

    return temp;
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::Iterator&
    mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    Iterator::operator+=(std::ptrdiff_t n) noexcept
{
    if(0 == n)
    {
        return *this;
    }

    assert(m_chunkPtr && m_currentElementPtr && m_chunkPtr < m_chunksEnd);

    if(n < 0)
    {
        return (*this -= -n);
    }

    if(n >= m_chunkSize)
    {
        size_t multiplier{ n / m_chunkSize };
        m_chunkPtr += multiplier;
        n -= (multiplier * m_chunkSize);
        if(m_chunkPtr >= m_chunksEnd)
        {
            m_currentElementPtr = nullptr;
            m_offset = 0;
            return *this;
        }
    }

    if(m_offset + n >= m_chunkSize)
    {
        ++m_chunkPtr;
        m_offset = m_offset + n - m_chunkSize;
    }
    else
    {
        m_offset += n;
    }

    if(m_chunkPtr < m_chunksEnd)
    {
        m_currentElementPtr = *m_chunkPtr + m_offset;
    }
    else
    {
        m_currentElementPtr = nullptr;
    }

    return *this;
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::Iterator&
    mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    Iterator::operator-=(std::ptrdiff_t n) noexcept
{
    if(0 == n)
    {
        return *this;
    }

    assert(m_chunkPtr && m_currentElementPtr && m_chunkPtr < m_chunksEnd);

    if(n < 0)
    {
        return (*this += -n);
    }

    if(n >= m_chunkSize)
    {
        size_t multiplier{ n / m_chunkSize };

        m_chunkPtr -= multiplier;
        n -= (multiplier * m_chunkSize);

        if(m_chunkPtr < m_chunkBegin)
        {
            m_currentElementPtr = nullptr;
            m_offset = 0;
            return *this;
        }
    }

    if(static_cast<std::ptrdiff_t>(m_offset) - n < 0)
    {
        --m_chunkPtr;
        m_offset = m_offset - n + m_chunkSize;
    }
    else
    {
        m_offset -= n;
    }

    if(m_chunkPtr >= m_chunkBegin && m_chunkPtr < m_chunksEnd)
    {
        m_currentElementPtr = *m_chunkPtr + m_offset;
    }
    else
    {
        m_currentElementPtr = nullptr;
    }

    return *this;
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::Iterator
    mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    Iterator::operator+(std::ptrdiff_t n) noexcept
{
    Iterator temp{ *this };
    temp += n;
    return temp;
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::Iterator
    mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    Iterator::operator-(std::ptrdiff_t n) noexcept
{

    Iterator temp{ *this };
    temp -= n;
    return temp;

}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr std::ptrdiff_t mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    Iterator::operator-(const mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::Iterator& other) const noexcept
{
    std::ptrdiff_t timesMultiply{ m_chunkPtr - other.m_chunkPtr };

    std::ptrdiff_t offset{ static_cast<std::ptrdiff_t>(m_offset) - static_cast<std::ptrdiff_t>(other.m_offset) };

    return timesMultiply * m_chunkSize + offset;
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr auto mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    Iterator::operator<=>(const mylib::ChunkedArray<T,
                                                    CHUNK_SIZE,
                                                    ALLOCATOR>::Iterator& other) const noexcept
{
    if(m_chunkPtr < other.m_chunkPtr)
    {
        return std::strong_ordering::less;
    }
    else if(m_chunkPtr > other.m_chunkPtr)
    {
        return std::strong_ordering::greater;
    }
    else
    {
        if(m_currentElementPtr == other.m_currentElementPtr)
        {
            return std::strong_ordering::equal;
        }
        else if(m_currentElementPtr == nullptr)
        {
            return std::strong_ordering::greater;
        }
        else if(other.m_currentElementPtr == nullptr)
        {
            return std::strong_ordering::less;
        }
    }

    return m_offset <=> other.m_offset;
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr bool mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    Iterator::operator==(const mylib::ChunkedArray<T,
                                                   CHUNK_SIZE,
                                                   ALLOCATOR>::Iterator& other) const noexcept
{
    return m_chunkPtr == other.m_chunkPtr && m_currentElementPtr == other.m_currentElementPtr;
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr T& mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    Iterator::operator[](std::ptrdiff_t n) const noexcept
{
    return *(*this + n);
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr size_t mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    offsetInChunk(size_t i) const noexcept
{
    return i % m_chunkSize;
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr T& mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    operator[](size_t i) noexcept
{
    assert(i < size());
    return m_arrayOfChunks[chunkIndex(i)][offsetInChunk(i)];
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr const T& mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    operator[](size_t i) const noexcept
{
    assert(i < size());
    return m_arrayOfChunks[chunkIndex(i)][offsetInChunk(i)];
}


template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
T mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    pop_back()
{
    assert(!empty());

    T temp{ back() };

    m_arrayOfChunks[chunkIndex(size() - 1)][offsetInChunk(size() - 1)].~T();
    --m_size;

    if(size() > 0 && (chunkIndex(size() - 1) < m_arrayOfChunks.size() - 1))
    {
        deallocateChunk(m_arrayOfChunks.pop_back());
    }
    else if(size() == 0)
    {
        clear();
    }

    return temp;
}


template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
void mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    push_back(const T& value)
{
    append(value);
}


template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
void mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    push_back(T&& value)
{
    append(std::move(value));
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::ReverseIterator
    mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    rbegin() noexcept
{
    return end();

}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::constReverseIterator
    mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    rbegin() const noexcept
{
    return cend();

}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
void mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    release() noexcept
{
    m_size = 0;
    m_arrayOfChunks.clear();
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::ReverseIterator
    mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    rend() noexcept
{
    return begin();
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::constReverseIterator
    mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    rend() const noexcept
{
    return cbegin();
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
void mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    resize(size_t newSize, const T& value)
{
    if(newSize == size())
    {
        return;
    }

    if(newSize == 0)
    {
        clear();
        return;
    }

    if(newSize < size())
    {
        destroyElements(newSize, size());

        size_t neededBlocks{ chunkIndex(newSize - 1) + 1 };

        if (m_arrayOfChunks.size() > neededBlocks)
        {
            removeLastBlocks(m_arrayOfChunks.size() - neededBlocks);
        }

        m_size = newSize;

        return;
    }
    else
    {
        size_t oldSize{ size() };
        size_t currentBlocks{ m_arrayOfChunks.size() };
        size_t newBlocksNeeded{ chunkIndex(newSize - 1) + 1 };

        m_arrayOfChunks.reserve(newBlocksNeeded);

        size_t blocksAdded{ 0 };

        for(size_t i{ currentBlocks }; i < newBlocksNeeded; ++i)
        {
            T* newChunk{ nullptr };
            try
            {
                newChunk = allocateChunk();
                m_arrayOfChunks.push_back(newChunk); // может выбросить исключение
                ++blocksAdded;
            }
            catch(...)
            {
                if(newChunk)
                {
                    deallocateChunk(newChunk);
                }
                removeLastBlocks(blocksAdded);
                throw;
            }
        }



        size_t constructed{ 0 };
        try
        {
            for(size_t i{ oldSize }; i < newSize; ++i)
            {
                new (&m_arrayOfChunks[chunkIndex(i)][offsetInChunk(i)]) T(value);
                ++constructed;
            }
        }
        catch (...)
        {
            // Уничтожаем уже сконструированные элементы
            destroyElements(oldSize, oldSize + constructed);

            // Освобождаем все новые блоки и удаляем их из вектора
            removeLastBlocks(blocksAdded);
            throw;
        }
    }

    m_size = newSize;
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
void mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    removeLastBlocks(size_t count) noexcept
{
    for(size_t i{}; i < count; ++i)
    {
        deallocateChunk(m_arrayOfChunks.pop_back());
    }
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
void mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    reserve(size_t newCapacity)
{
    if(newCapacity <= size())
    {
        return;
    }

    size_t newSize{ chunkIndex(newCapacity - 1) + 1 };
    m_arrayOfChunks.reserve(newSize);
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
void mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    shrink_to_fit()
{
    if(empty())
    {
        clear();
        return;
    }

    size_t needed{ chunkIndex(m_size - 1) + 1 };   // сколько чанков нужно для m_size элементов
    size_t current{ m_arrayOfChunks.size() };

    if(needed < current)
    {
        removeLastBlocks(current - needed);

        m_arrayOfChunks.shrink_to_fit();
    }
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
void mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    swap(ChunkedArray& other) noexcept
{
    std::swap(m_size, other.m_size);
    std::swap(m_chunkAllocator, other.m_chunkAllocator);
    std::swap(m_arrayOfChunks, other.m_arrayOfChunks);
}

#endif // CHUNKED_ARRAY_H

