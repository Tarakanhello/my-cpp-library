#ifndef STATIC_FREE_LIST_H
#define STATIC_FREE_LIST_H


#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>

#include "mylib/memory.h"
#include "mylib/vector.h"

namespace mylib
{
    /**
     * @brief Класс пула фиксированного размера (static free list).
     *
     * Предназначен для эффективного управления объектами типа T без динамического
     * выделения памяти во время работы. Память под заданное количество объектов
     * (ёмкость) выделяется в конструкторе и не перераспределяется. Узлы (Node)
     * содержат сам объект T и указатель для организации списка свободных узлов.
     *
     * Основные операции:
     * - allocate()  — получить указатель на свободный узел (объект не конструируется).
     * - releaseNode() — вернуть узел в пул (деструктор не вызывается, память не освобождается).
     * - remove()    — уничтожить объект и вернуть узел в пул.
     *
     * @tparam T        Тип хранимых объектов.
     * @tparam ALLOCATOR Аллокатор, используемый для выделения памяти под массив узлов.
     *                   По умолчанию mylib::MySimpleAllocator<Node>.
     *
     * @note Класс некопируем, но перемещаем.
     */
    template<typename T, typename ALLOCATOR = mylib::MySimpleAllocator<T>>
    class StaticFreeList final
    {
    public:
        /**
         * @brief Внутренний узел, хранящий значение и указатель для списка.
         */
        struct Node;

    private:
        // Тип аллокатора для узлов (переопределяется через rebind).
        using NODE_ALLOCATOR = typename std::allocator_traits<ALLOCATOR>::template rebind_alloc<Node>;
        using ALLOC_TRAITS = std::allocator_traits<NODE_ALLOCATOR>;

        /**
         * @brief Выделяет память под массив из m_capacity узлов.
         * @return Указатель на начало массива.
         * @throw std::bad_alloc при неудаче.
         */
        Node* allocateNodes() { return ALLOC_TRAITS::allocate(m_nodeAllocator, m_capacity); }

        /**
         * @brief Освобождает память, выделенную под массив узлов.
         */
        void deallocateNodes() noexcept { ALLOC_TRAITS::deallocate(m_nodeAllocator, m_nodes, m_capacity); }

    private:
        // m_currentSize <= m_constructedSize <= m_capacity
        size_t m_currentSize{};     //!< Количество узлов, выделенных в данный момент (занятых).
        size_t m_constructedSize{}; //!< Общее количество узлов, в которых была вызвана конструкция (включая освобождённые).
        size_t m_capacity{};        //!< Максимальное количество узлов, которое может содержать пул.

        Node* m_returned{ nullptr }; //!< Голова списка свободных узлов (узлы, возвращённые через releaseNode).
        Node* m_nodes{ nullptr };    //!< Указатель на массив узлов (выделен в конструкторе).

        NODE_ALLOCATOR m_nodeAllocator{}; //!< Аллокатор для управления памятью узлов.

        /**
         * @brief Уничтожает все сконструированные элементы, которые в данный момент не находятся в списке свободных.
         * @note Метод вызывается из деструктора и оператора перемещающего присваивания.
         *       При возникновении исключений используется запасной алгоритм O(N²).
         */
        void destroyList() noexcept;

        /**
         * @brief Обнуляет все поля объекта (используется при перемещении).
         * @note Не уничтожает элементы и не освобождает память.
         */
        void release() noexcept;

    public:
        StaticFreeList() = default;

        /**
         * @brief Конструктор, создающий пул с заданной ёмкостью.
         * @param capacity Количество узлов, которое может вместить пул.
         * @param alloc    Аллокатор, который будет использоваться (передаётся по значению).
         * @throw std::bad_alloc при невозможности выделить память.
         */
        explicit StaticFreeList(size_t capacity, ALLOCATOR alloc = ALLOCATOR());

        StaticFreeList(const StaticFreeList&) = delete;
        StaticFreeList& operator=(const StaticFreeList&) = delete;

        /**
         * @brief Конструктор перемещения.
         * @param other Пул, из которого перемещаются данные.
         * @note После перемещения other становится пустым (release).
         */
        StaticFreeList(StaticFreeList&& other) noexcept;

        /**
         * @brief Оператор перемещающего присваивания.
         * @param other Пул, из которого перемещаются данные.
         * @return Ссылка на *this.
         */
        StaticFreeList& operator=(StaticFreeList&& other) noexcept;

        /**
         * @brief Деструктор.
         * @note Уничтожает все сконструированные элементы и освобождает память.
         */
        ~StaticFreeList() noexcept;

        size_t capacity() const noexcept { return m_capacity; }
        bool isFull() const noexcept { return m_currentSize == m_capacity; }
        bool empty() const noexcept { return m_currentSize == 0; }

        explicit operator bool() const noexcept { return !empty(); }

        /**
         * @brief Выделяет свободный узел из пула.
         * @return Указатель на узел, который можно использовать для размещения объекта.
         * @pre !isFull() – утверждение (assert) проверяет, что пул не заполнен.
         * @note Узел возвращается неинициализированным (конструктор T не вызывается).
         * @note Если есть узлы в списке свободных, берётся первый из них, иначе
         *       возвращается следующий несконструированный узел из массива.
         */
        Node* allocate() noexcept;

        /**
         * @brief Возвращает узел в пул (без вызова деструктора T).
         * @param node Указатель на узел, который нужно вернуть.
         * @pre node принадлежит этому пулу (проверяется assert).
         * @note Узел добавляется в начало списка свободных, его поле next перезаписывается.
         * @note Уменьшает m_currentSize.
         */
        void releaseNode(Node* node) noexcept;

        /**
         * @brief Уничтожает объект в узле и возвращает узел в пул.
         * @param node Указатель на узел.
         * @pre node принадлежит этому пулу.
         * @note Вызывает деструктор T, затем releaseNode(node).
         */
        void remove(Node* node) noexcept;
    }; // end StaticFreeList class



    /**
     * @brief Структура узла пула.
     * @tparam T Тип хранимого значения.
     *
     * Содержит:
     * - value – сам объект типа T.
     * - union { Node* next; void* userData; } – используется для организации
     *   списка свободных узлов (поле next) или может быть использован для пользовательских
     *   данных (userData), когда узел занят.
     */
    template<typename T, typename ALLOCATOR>
    struct StaticFreeList<T, ALLOCATOR>::
        Node final
    {
        T value; //!< Хранимый объект.

        union
        {
            Node* next;     //!< Указатель на следующий узел в списке свободных.
            void* userData; //!< Резерв для пользовательских данных (не используется внутри класса).
        };
    };

} // end namespace mylib



template<typename T, typename ALLOCATOR>
mylib::StaticFreeList<T, ALLOCATOR>::Node* mylib::StaticFreeList<T, ALLOCATOR>::
    allocate() noexcept
{
    assert(!isFull());

    Node* result{ m_returned };

    if(result)
    {
        m_returned = m_returned->next;
    }
    else
    {
        result = &(m_nodes[m_constructedSize++]);
    }
    ++m_currentSize;

    return result;
}



template<typename T, typename ALLOCATOR>
void mylib::StaticFreeList<T, ALLOCATOR>::
    destroyList() noexcept
{
    if(!empty())
    {
        try
        {
            mylib::Vector<bool> toDestruct(m_constructedSize, true);
            Node* returned{ m_returned };
            while(returned)
            {
                toDestruct[returned - m_nodes] = false;
                returned = returned->next;
            }

            for(size_t i{}; i < m_constructedSize; ++i)
            {
                if(toDestruct[i])
                {
                    m_nodes[i].value.~T();
                }
            }
        }
        catch(...)
        {
            // fallback O(N²) – надёжно, но медленно
            Node* curNode{ m_nodes };
            for(size_t i{}; i < m_constructedSize; ++i)
            {
                bool isReturned{ false };
                Node* returned{ m_returned };
                while(returned)
                {
                    if(returned == curNode)
                    {
                        isReturned = true;
                        break;
                    }
                    returned = returned->next;
                }
                if(!isReturned)
                {
                    curNode->value.~T();
                }

                ++curNode;
            }
        }
    }

    if(m_nodes)
        deallocateNodes();
}



template<typename T, typename ALLOCATOR>
void mylib::StaticFreeList<T, ALLOCATOR>::
    release() noexcept
{
    m_currentSize = 0;
    m_constructedSize = 0;
    m_capacity = 0;
    m_returned = nullptr;
    m_nodes = nullptr;
}



template<typename T, typename ALLOCATOR>
void mylib::StaticFreeList<T, ALLOCATOR>::
    releaseNode(Node* node) noexcept
{
    if(node)
    {
        assert(node - m_nodes >= 0 && node - m_nodes < m_constructedSize);
        node->next = m_returned;
        m_returned = node;
        --m_currentSize;
    }
}



template<typename T, typename ALLOCATOR>
void mylib::StaticFreeList<T, ALLOCATOR>::
    remove(Node* node) noexcept
{
    if(node)
    {
        assert(node - m_nodes >= 0 && node - m_nodes < m_constructedSize);
        node->value.~T();
        releaseNode(node);
    }
}



template<typename T, typename ALLOCATOR>
mylib::StaticFreeList<T, ALLOCATOR>::
    StaticFreeList(size_t capacity, ALLOCATOR alloc)
    : m_capacity{ capacity }
    , m_currentSize{}
    , m_constructedSize{}
    , m_returned{ nullptr }
    , m_nodeAllocator{ alloc }
    , m_nodes{ capacity > 0 ? allocateNodes() : nullptr }
{}



template<typename T, typename ALLOCATOR>
mylib::StaticFreeList<T, ALLOCATOR>::
    StaticFreeList(StaticFreeList&& other) noexcept
{
    if(this != &other)
    {
        m_capacity = other.m_capacity;
        m_currentSize = other.m_currentSize;
        m_constructedSize = other.m_constructedSize;
        m_returned = other.m_returned;
        m_nodeAllocator = std::move(other.m_nodeAllocator);
        m_nodes = other.m_nodes;

        other.release();
    }
}



template<typename T, typename ALLOCATOR>
mylib::StaticFreeList<T, ALLOCATOR>& mylib::StaticFreeList<T, ALLOCATOR>::
    operator=(StaticFreeList&& other) noexcept
{
    if(this != &other)
    {
        destroyList();

        m_capacity = other.m_capacity;
        m_currentSize = other.m_currentSize;
        m_constructedSize = other.m_constructedSize;
        m_returned = other.m_returned;
        m_nodeAllocator = std::move(other.m_nodeAllocator);
        m_nodes = other.m_nodes;

        other.release();
    }

    return *this;
}



template<typename T, typename ALLOCATOR>
mylib::StaticFreeList<T, ALLOCATOR>::
    ~StaticFreeList() noexcept
{
    destroyList();
}



#endif // STATIC_FREE_LIST_H
