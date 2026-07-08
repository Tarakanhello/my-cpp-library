#ifndef LIST_H
#define LIST_H

#include <cassert>
#include <cstddef>
#include <iterator>
#include <memory>
#include <utility>


#include "mylib/memory.h"


namespace mylib
{
    /**
     * @brief Двусвязный список (двунаправленный).
     * @tparam T         Тип хранимых элементов.
     * @tparam ALLOCATOR Аллокатор для выделения памяти под узлы (по умолчанию MySimpleAllocator<T>).
     *
     * Реализует классический двусвязный список с фиктивным (sentinel) узлом,
     * что упрощает вставку и удаление в любом месте. Обеспечивает строгую гарантию
     * безопасности исключений для операций вставки.
     *
     * @note Использует внутреннюю структуру Node, наследующую BaseNode, для хранения
     *       данных и указателей prev/next.
     */
    template<typename T, typename ALLOCATOR = mylib::MySimpleAllocator<T>>
    class List final
    {
    private:

        /**
         * @brief Базовая структура узла (содержит только указатели).
         *
         * Используется как часть фиктивного узла и как основа для Node.
         */
        struct BaseNode;

        /**
         * @brief Узел, хранящий значение типа T.
         *
         * Наследует BaseNode, добавляя поле value.
         */
        struct Node;

        /** @brief Аллокатор для узлов (переопределён для Node). */
        using NODE_ALLOCATOR = typename std::allocator_traits<ALLOCATOR>::template rebind_alloc<Node>;

        /** @brief Вспомогательный трейт для работы с аллокатором узлов. */
        using ALLOC_TRAITS = std::allocator_traits<NODE_ALLOCATOR>;

        BaseNode m_sentinel;                ///< Фиктивный узел, служащий головой и хвостом (кольцевая структура).
        size_t m_size{};                    ///< Количество элементов в списке.
        NODE_ALLOCATOR m_nodeAllocator{};   ///< Аллокатор для узлов.

        /**
         * @brief Выделяет память под один узел без конструирования.
         * @return Указатель на выделенную память.
         * @throw std::bad_alloc при нехватке памяти.
         */
        Node* allocateNode();

        /**
         * @brief Конструирует элементы из диапазона и вставляет их в конец списка.
         * @tparam INPUT_IT Тип итератора ввода.
         * @param begin Начало диапазона.
         * @param end   Конец диапазона.
         * @throw Любое исключение при конструировании; при ошибке уже созданные узлы уничтожаются.
         */
        template<typename INPUT_IT>
        void constructElementsFromRange(INPUT_IT begin, INPUT_IT end);

        /**
         * @brief Конструирует объект T в уже выделенном узле.
         * @tparam ARGS Типы аргументов для конструктора T.
         * @param nodePtr Указатель на узел.
         * @param args    Аргументы, передаваемые конструктору T.
         */
        template<typename... ARGS>
        void constructNode(Node* nodePtr, ARGS&&... args);

        /**
         * @brief Создаёт узел с переданными аргументами (выделяет память и конструирует T).
         * @tparam ARGS Типы аргументов.
         * @param args Аргументы для конструктора T.
         * @return Указатель на созданный узел.
         * @throw std::bad_alloc или исключение конструктора T; при ошибке память освобождается.
         */
        template <typename... ARGS>
        Node* createNode(ARGS&&... args);

        /**
         * @brief Исключает узел из списка (перелинковка без освобождения памяти).
         * @param nodePtr Указатель на исключаемый узел.
         * @note Уменьшает m_size.
         */
        void cutNode(BaseNode* nodePtr) noexcept;

        /**
         * @brief Освобождает память узла без вызова деструктора.
         * @param nodePtr Указатель на узел.
         */
        void deallocateNode(Node* nodePtr) noexcept;

        /**
         * @brief Уничтожает все узлы и освобождает память.
         * @note noexcept – предполагается, что деструкторы не бросают исключения.
         */
        void destroyAll() noexcept;

        /**
         * @brief Уничтожает элементы в диапазоне узлов [beginPtr, endPtr) и освобождает память.
         * @param beginPtr Начало диапазона (включая).
         * @param endPtr   Конец диапазона (не включая).
         */
        void destroyElementsInRange(BaseNode* beginPtr, BaseNode* endPtr) noexcept;

        /**
         * @brief Уничтожает объект T внутри узла (вызывает деструктор).
         * @param nodePtr Указатель на узел.
         */
        void destroyNode(Node* nodePtr);

        void fixSentinelLinks();

        /**
         * @brief Вставляет узел newNodePtr после positionPtr.
         * @param positionPtr Узел, после которого вставляем.
         * @param newNodePtr  Вставляемый узел.
         */
        void insertAfter(BaseNode* positionPtr, BaseNode* newNodePtr) noexcept;

        /**
         * @brief Вставляет узел newNodePtr перед positionPtr.
         * @param positionPtr Узел, перед которым вставляем.
         * @param newNodePtr  Вставляемый узел.
         */
        void insertBefore(BaseNode* positionPtr, BaseNode* newNodePtr) noexcept;

        /**
         * @brief Освобождает указатели и обнуляет размер (без уничтожения элементов).
         * @note Используется в move-операциях.
         */
        void release() noexcept;

        /** @brief Возвращает указатель на первый узел (root). */
        BaseNode* root() noexcept { return m_sentinel.nextPtr; }
        const BaseNode* root() const noexcept{ return m_sentinel.nextPtr; }

        /** @brief Возвращает указатель на последний узел (tail). */
        BaseNode* tail() noexcept { return m_sentinel.prevPtr; }
        const BaseNode* tail() const noexcept { return m_sentinel.prevPtr; }

    public:

        /**
         * @brief Конструктор по умолчанию. Создаёт пустой список.
         * @param alloc Аллокатор (передаётся по значению).
         */
        List(const ALLOCATOR& alloc = ALLOCATOR());

        /**
         * @brief Конструктор, создающий список из count элементов, инициализированных копией value.
         * @param count Количество элементов.
         * @param value Значение для инициализации.
         * @param alloc Аллокатор.
         */
        List(size_t count, const T& value = T(), const ALLOCATOR& alloc = ALLOCATOR());

        /**
         * @brief Конструктор из std::initializer_list.
         * @param list Список инициализации.
         * @param alloc Аллокатор.
         */
        List(std::initializer_list<T> list, const ALLOCATOR& alloc = ALLOCATOR());

        /**
         * @brief Конструктор из произвольного диапазона итераторов.
         * @tparam INPUT_IT Тип итератора ввода.
         * @param first Начало диапазона.
         * @param last  Конец диапазона.
         * @param alloc Аллокатор.
         */
        template <typename INPUT_IT>
            requires std::input_iterator<INPUT_IT>
        List(INPUT_IT first, INPUT_IT last, const ALLOCATOR& alloc = ALLOCATOR());

        /**
         * @brief Конструктор копирования.
         * @param other Список для копирования.
         */
        List(const List& other);

        /**
         * @brief Конструктор перемещения.
         * @param other Список, из которого перемещаются данные.
         */
        List(List&& other) noexcept;

        /**
         * @brief Деструктор. Уничтожает все узлы и освобождает память.
         */
        ~List() noexcept;

        /**
         * @brief Оператор присваивания копированием (через copy-and-swap).
         * @param other Список для копирования.
         * @return Ссылка на *this.
         */
        List& operator=(const List& other);

        /**
         * @brief Оператор присваивания перемещением.
         * @param other Список, из которого перемещаются данные.
         * @return Ссылка на *this.
         */
        List& operator=(List&& other) noexcept;

        /**
         * @brief Возвращает ссылку на первый элемент.
         * @return Ссылка на первый элемент.
         * @pre Список не пуст (assert).
         */
        T& front() noexcept;
        const T& front() const noexcept;

        /**
         * @brief Возвращает ссылку на последний элемент.
         * @return Ссылка на последний элемент.
         * @pre Список не пуст (assert).
         */
        T& back() noexcept;
        const T& back() const noexcept;

        /**
         * @brief Вставляет элемент в начало списка (копированием).
         * @param value Добавляемый элемент.
         * @throw std::bad_alloc при нехватке памяти или исключение конструктора T.
         */
        void push_front(const T& value);
        void push_front(T&& value);

        /**
         * @brief Вставляет элемент в конец списка (копированием).
         * @param value Добавляемый элемент.
         */
        void push_back(const T& value);
        void push_back(T&& value);

        /**
         * @brief Удаляет первый элемент и возвращает его копию.
         * @return Копия удалённого элемента (перемещённая).
         * @pre Список не пуст (assert).
         * @note Обеспечивает строгую гарантию безопасности исключений (noexcept).
         */
        T pop_front() noexcept;

        /**
         * @brief Удаляет последний элемент и возвращает его копию.
         * @return Копия удалённого элемента (перемещённая).
         * @pre Список не пуст (assert).
         * @note noexcept.
         */
        T pop_back() noexcept;




        // ==================== ИТЕРАТОРЫ ====================

        class Iterator;
        class ConstIterator;
        using ReverseIterator = std::reverse_iterator<Iterator>;
        using ConstReverseIterator = std::reverse_iterator<ConstIterator>;

        /**
         * @brief Возвращает итератор на первый элемент.
         */
        Iterator begin() noexcept;

        /** @copydoc begin() const */
        ConstIterator begin() const noexcept;

        /** @copydoc begin() const (константный итератор) */
        ConstIterator cbegin() const noexcept;

        /**
         * @brief Возвращает итератор на элемент, следующий за последним (фиктивный узел).
         */
        Iterator end() noexcept;

         /** @copydoc end() const */
        ConstIterator end() const noexcept;

        /** @copydoc end() const (константный итератор) */
        ConstIterator cend() const noexcept;

        /**
         * @brief Возвращает обратный итератор на последний элемент.
         */
        ReverseIterator rbegin() noexcept;

        /** @copydoc rbegin() const */
        ConstReverseIterator rbegin() const noexcept;

        /** @copydoc rbegin() const (константный обратный) */
        ConstReverseIterator crbegin() const noexcept;

        /**
         * @brief Возвращает обратный итератор на элемент, предшествующий первому.
         */
        ReverseIterator rend() noexcept;

        /** @copydoc rend() const */
        ConstReverseIterator rend() const noexcept;

        /** @copydoc rend() const (константный обратный) */
        ConstReverseIterator crend() const noexcept;



        // ==================== ВСТАВКА И УДАЛЕНИЕ ПО ПОЗИЦИИ ====================

        /**
         * @brief Вставляет элемент перед позицией pos (копированием).
         * @param pos   Итератор, перед которым вставляется элемент (может быть end()).
         * @param value Вставляемое значение.
         * @return Итератор на новый элемент.
         * @throw std::bad_alloc или исключение конструктора T.
         */
        Iterator insert(ConstIterator pos, const T& value); // вставляет элемент перед позицией pos, возвращает итератор на новый элемент.
        Iterator insert(ConstIterator pos, T&& value);

        /**
         * @brief Удаляет элемент в позиции pos.
         * @param pos Итератор на удаляемый элемент (не должен быть end()).
         * @return Итератор на элемент, следующий за удалённым.
         * @pre pos != end().
         * @note Если pos == end(), возвращает end().
         */
        Iterator erase(ConstIterator pos); // удаляет элемент в позиции pos, возвращает итератор на следующий элемент.



        // ==================== РАЗМЕР И СВОЙСТВА ====================

        /**
         * @brief Возвращает количество элементов.
         */
        size_t size() const noexcept { return m_size; }

        /**
         * @brief Проверяет, пуст ли список.
         * @return true, если size() == 0.
         */
        bool empty() const noexcept { return m_size == 0; }

        /**
         * @brief Явное приведение к bool: true, если список не пуст.
         */
        explicit operator bool() const noexcept { return !empty(); };

        /**
         * @brief Очищает список (удаляет все элементы и освобождает память).
         */
        void clear() noexcept { destroyAll(); }

        /**
         * @brief Изменяет размер списка.
         * @param newSize Новый размер.
         * @param value   Значение для инициализации добавляемых элементов (если newSize > текущего).
         * @throw std::bad_alloc или исключение конструктора T при увеличении.
         * @note При уменьшении элементы удаляются, память освобождается.
         */
        void resize(size_t newSize, const T& value = T()); // изменяет размер; при увеличении вставляет копии value.

        /**
         * @brief Обменивает содержимое с другим списком за O(1).
         * @param other Другой список.
         */
        void swap(List& other) noexcept;
    }; // end class List



    template<typename T, typename ALLOCATOR>
    struct List<T, ALLOCATOR>::BaseNode
    {
        BaseNode* nextPtr{ nullptr };
        BaseNode* prevPtr{ nullptr };
    };



    template<typename T, typename ALLOCATOR>
    struct List<T, ALLOCATOR>::Node final : public List<T, ALLOCATOR>::BaseNode
    {
        T value{};

        /**
         * @brief Конструирует узел с переданными аргументами для value.
         * @tparam ARGS Типы аргументов.
         * @param args Аргументы для конструктора T.
         */
        template<typename... ARGS>
        Node(ARGS&&... args)
            : value { std::forward<ARGS>(args)... }
        {}
    };


    /**
     * @brief Итератор произвольного доступа (двунаправленный) для List.
     *
     * Позволяет обходить элементы списка в обоих направлениях.
     * Поддерживает инкремент, декремент, разыменование и сравнение.
     */
    template<typename T, typename ALLOCATOR>
    class List<T, ALLOCATOR>::Iterator final
    {
    private:
        BaseNode* m_currentNode; ///< Указатель на текущий узел.

    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = T*;
        using reference         = T&;

        friend class ConstIterator; ///< Для доступа к m_currentNode при конвертации.

        constexpr explicit Iterator(BaseNode* node = nullptr) noexcept;
        constexpr T&        operator*() const noexcept;
        constexpr T*        operator->() const noexcept;
        constexpr Iterator& operator++() noexcept;
        constexpr Iterator  operator++(int) noexcept;
        constexpr Iterator& operator--() noexcept;
        constexpr Iterator  operator--(int) noexcept;
        constexpr bool      operator!=(const Iterator& other) const noexcept;
        constexpr bool      operator==(const Iterator& other) const noexcept;
        constexpr const BaseNode* getNode() const noexcept { return m_currentNode; }
    }; // end class Iterator



    /**
     * @brief Константный итератор для List.
     *
     * Предоставляет доступ только для чтения к элементам.
     * Может быть неявно создан из Iterator.
     */
    template<typename T, typename ALLOCATOR>
    class List<T, ALLOCATOR>::ConstIterator final
    {
    private:
        const BaseNode* m_currentNode; ///< Указатель на текущий узел (константный).

    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const T*;
        using reference         = const T&;

        constexpr explicit ConstIterator(const BaseNode* node = nullptr) noexcept;
        constexpr ConstIterator(const Iterator& iterator) noexcept;
        constexpr const T&        operator*() const noexcept;
        constexpr const T*        operator->() const noexcept;
        constexpr ConstIterator& operator++() noexcept;
        constexpr ConstIterator  operator++(int) noexcept;
        constexpr ConstIterator& operator--() noexcept;
        constexpr ConstIterator  operator--(int) noexcept;
        constexpr bool      operator!=(const ConstIterator& other) const noexcept;
        constexpr bool      operator==(const ConstIterator& other) const noexcept;
        constexpr bool      operator==(const Iterator& other) const noexcept;
        constexpr const BaseNode* getNode() const noexcept { return m_currentNode; }
    }; // end class Iterator

} // end namespace



template<typename T, typename ALLOCATOR>
mylib::List<T, ALLOCATOR>::Node* mylib::List<T, ALLOCATOR>::
    allocateNode()
{
    return List<T, ALLOCATOR>::ALLOC_TRAITS::allocate(m_nodeAllocator, 1);
}



template<typename T, typename ALLOCATOR>
T& mylib::List<T, ALLOCATOR>::back() noexcept
{
    assert(!empty());

    return (static_cast<Node*>(tail()))->value;
}



template<typename T, typename ALLOCATOR>
const T& mylib::List<T, ALLOCATOR>::
    back() const noexcept
{
    assert(!empty());

    return (static_cast<const Node*>(tail()))->value;
}



template<typename T, typename ALLOCATOR>
mylib::List<T, ALLOCATOR>::Iterator mylib::List<T, ALLOCATOR>::
    begin() noexcept
{
    return Iterator{ root() };
}



template<typename T, typename ALLOCATOR>
mylib::List<T, ALLOCATOR>::ConstIterator mylib::List<T, ALLOCATOR>::
    begin() const noexcept
{
    return cbegin();
}



template<typename T, typename ALLOCATOR>
mylib::List<T, ALLOCATOR>::ConstIterator mylib::List<T, ALLOCATOR>::
    cbegin() const noexcept
{
    return ConstIterator{ root() };
}




template<typename T, typename ALLOCATOR>
mylib::List<T, ALLOCATOR>::ConstIterator mylib::List<T, ALLOCATOR>::
    cend() const noexcept
{
    return ConstIterator{ &m_sentinel };
}



template<typename T, typename ALLOCATOR>
constexpr mylib::List<T, ALLOCATOR>::ConstIterator::
    ConstIterator(const BaseNode* node ) noexcept
        : m_currentNode{ node }
{}



template<typename T, typename ALLOCATOR>
constexpr mylib::List<T, ALLOCATOR>::ConstIterator::
    ConstIterator(const Iterator&  iterator) noexcept
        : m_currentNode{ iterator.m_currentNode }
{}



template<typename T, typename ALLOCATOR>
constexpr const T& mylib::List<T, ALLOCATOR>::
    ConstIterator::operator*() const noexcept
{
    assert(m_currentNode);

    return static_cast<const Node*>(m_currentNode)->value;
}



template<typename T, typename ALLOCATOR>
constexpr const T* mylib::List<T, ALLOCATOR>::
    ConstIterator::operator->() const noexcept
{
    assert(m_currentNode);

    return &(static_cast<const Node*>(m_currentNode)->value);
}



template<typename T, typename ALLOCATOR>
constexpr mylib::List<T, ALLOCATOR>::ConstIterator& mylib::List<T, ALLOCATOR>::
    ConstIterator::operator++() noexcept
{
    assert(m_currentNode);

    m_currentNode = m_currentNode->nextPtr;

    return *this;
}



template<typename T, typename ALLOCATOR>
constexpr mylib::List<T, ALLOCATOR>::ConstIterator  mylib::List<T, ALLOCATOR>::
    ConstIterator::operator++(int) noexcept
{
    ConstIterator temp{ *this };

    ++(*this);

    return temp;
}



template<typename T, typename ALLOCATOR>
constexpr mylib::List<T, ALLOCATOR>::ConstIterator& mylib::List<T, ALLOCATOR>::
    ConstIterator::operator--() noexcept
{
    assert(m_currentNode);

    m_currentNode = m_currentNode->prevPtr;

    return *this;
}



template<typename T, typename ALLOCATOR>
constexpr mylib::List<T, ALLOCATOR>::ConstIterator  mylib::List<T, ALLOCATOR>::
    ConstIterator::operator--(int) noexcept
{
    ConstIterator temp{ *this };

    --(*this);

    return temp;
}



template<typename T, typename ALLOCATOR>
constexpr bool mylib::List<T, ALLOCATOR>::
    ConstIterator::operator!=(const ConstIterator& other) const noexcept
{
    return !(m_currentNode == other.m_currentNode);
}



template<typename T, typename ALLOCATOR>
constexpr bool mylib::List<T, ALLOCATOR>::
    ConstIterator::operator==(const ConstIterator& other) const noexcept
{
    return m_currentNode == other.m_currentNode;
}



template<typename T, typename ALLOCATOR>
constexpr bool mylib::List<T, ALLOCATOR>::
    ConstIterator::operator==(const Iterator& other) const noexcept
{
    return m_currentNode == other.m_currentNode;
}



template<typename T, typename ALLOCATOR>
template<typename INPUT_IT>
void mylib::List<T, ALLOCATOR>::
    constructElementsFromRange(INPUT_IT begin, INPUT_IT end)
{
    BaseNode* startNodePtr{ tail() };

    while(begin != end)
    {
        Node* newNodePtr{ nullptr };
        try
        {
            newNodePtr = createNode(*begin);
        }
        catch(...)
        {
            if(tail() != startNodePtr)
            {
                destroyElementsInRange(startNodePtr->nextPtr, &m_sentinel);
            }

            throw;
        }

        insertAfter(tail(), newNodePtr);

        ++begin;
    }
}



template<typename T, typename ALLOCATOR>
template<typename... ARGS>
void mylib::List<T, ALLOCATOR>::
    constructNode(Node* nodePtr, ARGS&&... args)
{
    List<T, ALLOCATOR>::ALLOC_TRAITS::construct(m_nodeAllocator, nodePtr, std::forward<ARGS>(args)...);
}



template<typename T, typename ALLOCATOR>
mylib::List<T, ALLOCATOR>::ConstReverseIterator mylib::List<T, ALLOCATOR>::
    crbegin() const noexcept
{
    return ConstReverseIterator{ cend() };
}



template<typename T, typename ALLOCATOR>
template<typename... ARGS>
mylib::List<T, ALLOCATOR>::Node* mylib::List<T, ALLOCATOR>::
    createNode(ARGS&&... args)
{
    Node* newNode{ allocateNode() };
    try
    {
        constructNode(newNode, std::forward<ARGS>(args)...);
    }
    catch(...)
    {

        deallocateNode(newNode);
        throw;
    }

    return newNode;
}



template<typename T, typename ALLOCATOR>
mylib::List<T, ALLOCATOR>::ConstReverseIterator mylib::List<T, ALLOCATOR>::
    crend() const noexcept
{
    return ConstReverseIterator{ cbegin() };
}



template<typename T, typename ALLOCATOR>
void mylib::List<T, ALLOCATOR>::
    cutNode(BaseNode* nodePtr) noexcept
{
    nodePtr->nextPtr->prevPtr = nodePtr->prevPtr;
    nodePtr->prevPtr->nextPtr = nodePtr->nextPtr;
    --m_size;
}



template<typename T, typename ALLOCATOR>
void mylib::List<T, ALLOCATOR>::
    deallocateNode(Node* nodePtr) noexcept
{
    List<T, ALLOCATOR>::ALLOC_TRAITS::deallocate(m_nodeAllocator, nodePtr, 1);
}



template<typename T, typename ALLOCATOR>
void mylib::List<T, ALLOCATOR>::
    destroyAll() noexcept
{
    destroyElementsInRange(root(), &m_sentinel);
}



template<typename T, typename ALLOCATOR>
void mylib::List<T, ALLOCATOR>::
    destroyElementsInRange(BaseNode* beginPtr, BaseNode* endPtr) noexcept
{
    while(beginPtr != endPtr)
    {
        BaseNode* nextPtr{ beginPtr->nextPtr };
        cutNode(beginPtr);
        destroyNode(static_cast<Node*>(beginPtr));
        deallocateNode(static_cast<Node*>(beginPtr));
        beginPtr = nextPtr;
    }
}



template<typename T, typename ALLOCATOR>
void mylib::List<T, ALLOCATOR>::
    destroyNode(Node* nodePtr)
{
    List<T, ALLOCATOR>::ALLOC_TRAITS::destroy(m_nodeAllocator, nodePtr);
}



template<typename T, typename ALLOCATOR>
mylib::List<T, ALLOCATOR>::Iterator mylib::List<T, ALLOCATOR>::
    end() noexcept
{
    return Iterator{ &m_sentinel };
}



template<typename T, typename ALLOCATOR>
mylib::List<T, ALLOCATOR>::ConstIterator mylib::List<T, ALLOCATOR>::
    end() const noexcept
{
    return cend();
}



template<typename T, typename ALLOCATOR>
mylib::List<T, ALLOCATOR>::Iterator mylib::List<T, ALLOCATOR>::erase(ConstIterator pos)
{
    if(pos == cend())
    {
        return end();
    }

    BaseNode* node{ const_cast<BaseNode*>(pos.getNode()) };
    cutNode(node);
    BaseNode* nextNode{ node->nextPtr };
    destroyNode(static_cast<Node*>(node));
    deallocateNode(static_cast<Node*>(node));

    return Iterator{ nextNode };
}


template<typename T, typename ALLOCATOR>
void mylib::List<T, ALLOCATOR>::fixSentinelLinks()
{
    if(empty())
    {
        m_sentinel.nextPtr = &m_sentinel;
        m_sentinel.prevPtr = &m_sentinel;
        return;
    }

    root()->prevPtr = &m_sentinel;
    tail()->nextPtr = &m_sentinel;
}



template<typename T, typename ALLOCATOR>
T& mylib::List<T, ALLOCATOR>::
    front() noexcept
{
    assert(!empty());

    return (static_cast<Node*>(root()))->value;
}



template<typename T, typename ALLOCATOR>
const T& mylib::List<T, ALLOCATOR>::
    front() const noexcept
{
    assert(!empty());

    return (static_cast<const Node*>(root()))->value;
}



template<typename T, typename ALLOCATOR>
mylib::List<T, ALLOCATOR>::Iterator mylib::List<T, ALLOCATOR>::
    insert(ConstIterator pos, const T& value)
{
    Node* newNode{ createNode(value) };
    insertBefore(const_cast<BaseNode*>(pos.getNode()), newNode);
    return Iterator{ newNode };
}



template<typename T, typename ALLOCATOR>
mylib::List<T, ALLOCATOR>::Iterator mylib::List<T, ALLOCATOR>::
    insert(ConstIterator pos, T&& value)
{
    Node* newNode{ createNode(std::move(value)) };
    insertBefore(const_cast<BaseNode*>(pos.getNode()), newNode);
    return Iterator{ newNode };
}


template<typename T, typename ALLOCATOR>
void mylib::List<T, ALLOCATOR>::
    insertAfter(BaseNode* positionPtr, BaseNode* newNodePtr) noexcept
{
    newNodePtr->nextPtr = positionPtr->nextPtr;
    newNodePtr->prevPtr = positionPtr;
    positionPtr->nextPtr->prevPtr = newNodePtr;
    positionPtr->nextPtr = newNodePtr;
    ++m_size;
}



template<typename T, typename ALLOCATOR>
void mylib::List<T, ALLOCATOR>::
    insertBefore(BaseNode* positionPtr, BaseNode* newNodePtr) noexcept
{
    newNodePtr->nextPtr = positionPtr;
    newNodePtr->prevPtr = positionPtr->prevPtr;
    positionPtr->prevPtr->nextPtr = newNodePtr;
    positionPtr->prevPtr = newNodePtr;
    ++m_size;
}



template<typename T, typename ALLOCATOR>
constexpr mylib::List<T, ALLOCATOR>::Iterator::
    Iterator(BaseNode* node) noexcept
    : m_currentNode{ node }
{}



template<typename T, typename ALLOCATOR>
constexpr T& mylib::List<T, ALLOCATOR>::
    Iterator::operator*() const noexcept
{
    assert(m_currentNode);

    return static_cast<Node*>(m_currentNode)->value;
}



template<typename T, typename ALLOCATOR>
constexpr T* mylib::List<T, ALLOCATOR>::
    Iterator::operator->() const noexcept
{
    assert(m_currentNode);

    return &(static_cast<Node*>(m_currentNode)->value);
}



template<typename T, typename ALLOCATOR>
constexpr mylib::List<T, ALLOCATOR>::Iterator& mylib::List<T, ALLOCATOR>::
    Iterator::operator++() noexcept
{
    assert(m_currentNode);

    m_currentNode = m_currentNode->nextPtr;

    return *this;
}



template<typename T, typename ALLOCATOR>
constexpr mylib::List<T, ALLOCATOR>::Iterator  mylib::List<T, ALLOCATOR>::
    Iterator::operator++(int) noexcept
{
    Iterator temp{ *this };

    ++(*this);

    return temp;
}



template<typename T, typename ALLOCATOR>
constexpr mylib::List<T, ALLOCATOR>::Iterator& mylib::List<T, ALLOCATOR>::
    Iterator::operator--() noexcept
{
    assert(m_currentNode);

    m_currentNode = m_currentNode->prevPtr;

    return *this;
}



template<typename T, typename ALLOCATOR>
constexpr mylib::List<T, ALLOCATOR>::Iterator  mylib::List<T, ALLOCATOR>::
    Iterator::operator--(int) noexcept
{
    Iterator temp{ *this };

    --(*this);

    return temp;
}



template<typename T, typename ALLOCATOR>
constexpr bool mylib::List<T, ALLOCATOR>::
    Iterator::operator!=(const Iterator& other) const noexcept
{
    return !(m_currentNode == other.m_currentNode);
}



template<typename T, typename ALLOCATOR>
constexpr bool mylib::List<T, ALLOCATOR>::
    Iterator::operator==(const Iterator& other) const noexcept
{
    return m_currentNode == other.m_currentNode;
}



template<typename T, typename ALLOCATOR>
mylib::List<T, ALLOCATOR>::
    List(const ALLOCATOR& alloc)
        : m_sentinel{ &m_sentinel, &m_sentinel }
        , m_size{ 0 }
        , m_nodeAllocator{ alloc }
{}



template<typename T, typename ALLOCATOR>
mylib::List<T, ALLOCATOR>::
    List(size_t count, const T& value, const ALLOCATOR& alloc)
        : m_sentinel{ &m_sentinel, &m_sentinel }
        , m_size { 0 }
        , m_nodeAllocator { alloc }
{
    resize(count, value);
}



template<typename T, typename ALLOCATOR>
mylib::List<T, ALLOCATOR>::
    List(std::initializer_list<T> list, const ALLOCATOR& alloc)
        : m_sentinel{ &m_sentinel, &m_sentinel }
        , m_size{ 0 }
        , m_nodeAllocator{ alloc }
{
    constructElementsFromRange(list.begin(), list.end());
}



template<typename T, typename ALLOCATOR>
template <typename INPUT_IT>
    requires std::input_iterator<INPUT_IT>
mylib::List<T, ALLOCATOR>::
    List(INPUT_IT first, INPUT_IT last, const ALLOCATOR& alloc)
        : m_sentinel{ &m_sentinel, &m_sentinel }
        , m_size{ 0 }
        , m_nodeAllocator{ alloc }
{
    constructElementsFromRange(first, last);
}



template<typename T, typename ALLOCATOR>
mylib::List<T, ALLOCATOR>::
    List(const List& other)
        : m_sentinel{ &m_sentinel, &m_sentinel }
        , m_size{ 0 }
        , m_nodeAllocator{ other.m_nodeAllocator }
{
    constructElementsFromRange(other.begin(), other.end());
}



template<typename T, typename ALLOCATOR>
mylib::List<T, ALLOCATOR>::
    List(List&& other) noexcept
        : m_sentinel{ &m_sentinel, &m_sentinel }
        , m_size{ 0 }
        , m_nodeAllocator{ std::move(other.m_nodeAllocator) }
{
    if (!other.empty())
    {
        BaseNode* first{ other.root() };
        BaseNode* last{ other.tail() };

        m_sentinel.nextPtr = first;
        m_sentinel.prevPtr = last;

        first->prevPtr = &m_sentinel;
        last->nextPtr  = &m_sentinel;
        m_size = other.m_size;

        other.release();
    }
}



template<typename T, typename ALLOCATOR>
mylib::List<T, ALLOCATOR>::
    ~List() noexcept
{
    destroyAll();
}



template<typename T, typename ALLOCATOR>
mylib::List<T, ALLOCATOR>& mylib::List<T, ALLOCATOR>::
    operator=(const List& other)
{
    if(this != &other)
    {
        List temp{ other };
        swap(temp);
    }

    return *this;
}



template<typename T, typename ALLOCATOR>
mylib::List<T, ALLOCATOR>& mylib::List<T, ALLOCATOR>::
    operator=(List&& other) noexcept
{
    if(this != &other)
    {
        swap(other);
        other.destroyAll();
    }

    return *this;
}



template<typename T, typename ALLOCATOR>
T mylib::List<T, ALLOCATOR>::pop_front() noexcept
{
    assert(!empty());

    Node* front{ static_cast<Node*>(root()) };
    cutNode(root());

    T value{ std::move(front->value) };

    destroyNode(front);
    deallocateNode(front);

    return value;
}



template<typename T, typename ALLOCATOR>
T mylib::List<T, ALLOCATOR>::pop_back() noexcept
{
    assert(!empty());

    Node* back{ static_cast<Node*>(tail()) };
    cutNode(tail());

    T value{ std::move(back->value) };

    destroyNode(back);
    deallocateNode(back);

    return value;
}



template<typename T, typename ALLOCATOR>
void mylib::List<T, ALLOCATOR>::
    push_back(const T& value)
{
    Node* newNode{ createNode(value) };
    insertAfter(tail(), newNode);
}



template<typename T, typename ALLOCATOR>
void mylib::List<T, ALLOCATOR>::push_back(T&& value)
{
    Node* newNode{ createNode(std::move(value)) };
    insertAfter(tail(), newNode);
}



template<typename T, typename ALLOCATOR>
void mylib::List<T, ALLOCATOR>::
    push_front(const T& value)
{
    Node* newNode{ createNode(value) };
    insertBefore(root(), newNode);
}



template<typename T, typename ALLOCATOR>
void mylib::List<T, ALLOCATOR>::
    push_front(T&& value)
{
    Node* newNode{ createNode(std::move(value)) };
    insertBefore(root(), newNode);
}



template<typename T, typename ALLOCATOR>
mylib::List<T, ALLOCATOR>::ReverseIterator mylib::List<T, ALLOCATOR>::
    rbegin() noexcept
{
    return ReverseIterator{ end() };
}



template<typename T, typename ALLOCATOR>
mylib::List<T, ALLOCATOR>::ConstReverseIterator mylib::List<T, ALLOCATOR>::
    rbegin() const noexcept
{
    return crbegin();
}



template<typename T, typename ALLOCATOR>
mylib::List<T, ALLOCATOR>::ReverseIterator mylib::List<T, ALLOCATOR>::
    rend() noexcept
{
    return ReverseIterator{ begin() };
}



template<typename T, typename ALLOCATOR>
mylib::List<T, ALLOCATOR>::ConstReverseIterator mylib::List<T, ALLOCATOR>::
    rend() const noexcept
{
    return crend();
}



template<typename T, typename ALLOCATOR>
void mylib::List<T, ALLOCATOR>::
    release() noexcept
{
    m_size = 0;
    m_sentinel.nextPtr = &m_sentinel;
    m_sentinel.prevPtr = &m_sentinel;
}



template<typename T, typename ALLOCATOR>
void mylib::List<T, ALLOCATOR>::resize(size_t newSize, const T& value)
{
    if(newSize == m_size)
    {
        return;
    }

    if(newSize < m_size)
    {
        while(m_size > newSize)
        {
            Node* back{ static_cast<Node*>(tail()) };
            cutNode(tail());
            destroyNode(back);
            deallocateNode(back);
        }
        return;
    }

    BaseNode* begin{ tail() };

    while(m_size < newSize)
    {
        Node* newNodePtr{ nullptr };
        try
        {
            newNodePtr = createNode(value);
        }
        catch (...)
        {
            destroyElementsInRange(begin->nextPtr, &m_sentinel);
            throw;
        }

        insertAfter(tail(), newNodePtr);
    }
}



template<typename T, typename ALLOCATOR>
void mylib::List<T, ALLOCATOR>::swap(List& other) noexcept
{
    if(this == &other)
    {
        return;
    }

    std::swap(m_size, other.m_size);
    std::swap(m_nodeAllocator, other.m_nodeAllocator);
    std::swap(m_sentinel, other.m_sentinel);

    fixSentinelLinks();
    other.fixSentinelLinks();
}


#endif // LIST_H
