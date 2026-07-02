#ifndef LIST_H
#define LIST_H

#include <cassert>
#include <memory>


#include "mylib/memory.h"

namespace mylib
{

template<typename T, typename ALLOCATOR = mylib::MySimpleAllocator<T>>
class List final
{
private:
    struct BaseNode
    {
        BaseNode* next{ nullptr };
        BaseNode* prev{ nullptr };
    };

    struct Node final : public BaseNode
    {
        T value{};

        template<typename... ARGS>
        Node(ARGS&&... args)
            : value { std::forward<ARGS>(args)... }
        {}
    };

    using NODE_ALLOCATOR = typename std::allocator_traits<ALLOCATOR>::template rebind_alloc<Node>;
    using ALLOC_TRAITS = std::allocator_traits<NODE_ALLOCATOR>;

    BaseNode m_sentinel;

    size_t m_size{};

    NODE_ALLOCATOR m_nodeAllocator{};

    void cutNode(Node* node) noexcept
    {
        node->next->prev = node->prev;
        node->prev->next = node->next;
    }

    void insertBefore(Node* position, Node* newNode) noexcept
    {
        newNode->next = position;
        newNode->prev = position->prev;
        position->prev->next = newNode;
        position->prev = newNode;
    }

    void insertAfter(Node* position, Node* newNode) noexcept
    {
        newNode->next = position->next;
        newNode->prev = position;
        position->next->prev = newNode;
        position->next = newNode;
    }

    void destroyAll() noexcept
    {
        Node* current{ m_sentinel.next };
        while(current != &m_sentinel)
        {
            cutNode(current);
            current->~Node();
            deallocateNode(current);
        }
    }

    Node* allocateNode() const { return m_nodeAllocator.allocate(1); }
    void constructNode(Node* node, T value) { m_nodeAllocator.construct(node, value); }

public:
    List(const ALLOCATOR& alloc = ALLOCATOR())
        : m_sentinel{ &m_sentinel, &m_sentinel }
        , m_size{ 0 }
        , m_nodeAllocator{ alloc }
    {}

    List(size_t count, const T& value = T(), const ALLOCATOR& alloc = ALLOCATOR())
        : m_sentinel{ &m_sentinel, &m_sentinel }
        , m_size { 0 }
        , m_nodeAllocator { alloc }
    {
        while(m_size < count)
        {
            Node* newNode{ m_nodeAllocator.allocate(1) };
            try
            {
                constructNode(newNode, value);
                insertAfter(&m_sentinel, newNode);
                ++m_size;
            }
            catch (...)
            {
                m_nodeAllocator.deallocate(newNode, 1);
                destroyAll();
            }
        }
    }

    List(std::initializer_list<T> list, const ALLOCATOR& alloc = ALLOCATOR());

    template <typename InputIt>
    List(InputIt first, InputIt last, const ALLOCATOR& alloc = ALLOCATOR());
    List(const List& other);
    List(List&& other) noexcept;
    ~List() noexcept;

    List& operator=(const List& other);
    List& operator=(List&& other) noexcept;

    T& front() noexcept;
    const T& front() const noexcept;
    T& back() noexcept;
    const T& back() const noexcept;

    void push_front(const T& value);
    void push_front(T&& value);
    void push_back(const T& value);
    void push_back(T&& value);
    T pop_front();
    T pop_back();

    /*
     * Требования к итераторам:
     *      - Категория – двунаправленный итератор (std::bidirectional_iterator_tag).
     *      - Поддержка операторов: ++it, it++, --it, it--, *it, it->, ==, !=.
     *      - Константные итераторы должны быть конвертируемы из неконстантных.
     *      - Итератор должен хранить указатель на текущий узел и (для end) – указатель на фиктивный хвостовой узел или nullptr.
     */
    class Iterator;
    class ConstIterator;
    using ReverseIterator = std::reverse_iterator<Iterator>;
    using ConstReverseIterator = std::reverse_iterator<ConstIterator>;

    Iterator begin() noexcept;
    ConstIterator begin() const noexcept;
    ConstIterator cbegin() const noexcept;
    Iterator end() noexcept;
    ConstIterator end() const noexcept;
    ConstIterator cend() const noexcept;
    ReverseIterator rbegin() noexcept;
    ConstReverseIterator rbegin() const noexcept;
    ConstReverseIterator crbegin() const noexcept;
    ReverseIterator rend() noexcept;
    ConstReverseIterator rend() const noexcept;
    ConstReverseIterator crend() const noexcept;

    Iterator insert(ConstIterator pos, const T& value); // вставляет элемент перед позицией pos, возвращает итератор на новый элемент.
    Iterator insert(ConstIterator pos, T&& value);
    Iterator erase(ConstIterator pos); // даляет элемент в позиции pos, возвращает итератор на следующий элемент.

    size_t size() const noexcept;
    bool empty() const noexcept;
    explicit operator bool() const noexcept;
    void clear();

    void resize(size_t newSize, const T& value = T()); // изменяет размер; при увеличении вставляет копии value.
    void swap(List& other) noexcept;
};

} // end namespace
#endif // LIST_H
