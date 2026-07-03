#ifndef LIST_H
#define LIST_H

#include <cassert>
#include <cstddef>
#include <memory>
#include <utility>


#include "mylib/memory.h"

namespace mylib
{

    template<typename T, typename ALLOCATOR = mylib::MySimpleAllocator<T>>
    class List final
    {
    private:
        struct BaseNode;
        struct Node;

        using NODE_ALLOCATOR = typename std::allocator_traits<ALLOCATOR>::template rebind_alloc<Node>;
        using ALLOC_TRAITS = std::allocator_traits<NODE_ALLOCATOR>;

        BaseNode m_sentinel;
        size_t m_size{};
        NODE_ALLOCATOR m_nodeAllocator{};

        Node* allocateNode() const;
        template<typename INPUT_IT>
        void constructElementsFromRange(INPUT_IT begin, INPUT_IT end);
        void constructNode(Node* nodePtr, const T& value);
        void constructNode(Node* nodePtr, T&& value);
        void cutNode(BaseNode* nodePtr) noexcept;
        void deallocateNode(Node* nodePtr) noexcept;
        void destroyAll() noexcept;
        void destroyElementsInRange(BaseNode* beginPtr, BaseNode* endPtr) noexcept;
        void destroyNode(Node* nodePtr);
        void insertAfter(BaseNode* positionPtr, BaseNode* newNodePtr) noexcept;
        void insertBefore(BaseNode* positionPtr, BaseNode* newNodePtr) noexcept;
        void release() noexcept;
        BaseNode* root() noexcept { return m_sentinel.nextPtr; }
        const BaseNode* root() const noexcept{ return m_sentinel.nextPtr; }
        BaseNode* tail() noexcept { return m_sentinel.prevPtr; }
        const BaseNode* tail() const noexcept { return m_sentinel.prevPtr; }

    public:
        List(const ALLOCATOR& alloc = ALLOCATOR());
        List(size_t count, const T& value = T(), const ALLOCATOR& alloc = ALLOCATOR());
        List(std::initializer_list<T> list, const ALLOCATOR& alloc = ALLOCATOR());
        template <typename INPUT_IT>
        List(INPUT_IT first, INPUT_IT last, const ALLOCATOR& alloc = ALLOCATOR());
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

        size_t size() const noexcept { return m_size; }
        bool empty() const noexcept;
        explicit operator bool() const noexcept;
        void clear();

        void resize(size_t newSize, const T& value = T()); // изменяет размер; при увеличении вставляет копии value.
        void swap(List& other) noexcept;
    }; // end class List



    template<typename T, typename ALLOCATOR>
    struct mylib::List<T, ALLOCATOR>::BaseNode
    {
        BaseNode* nextPtr{ nullptr };
        BaseNode* prevPtr{ nullptr };
    };



    template<typename T, typename ALLOCATOR>
    struct mylib::List<T, ALLOCATOR>::Node final : public mylib::List<T, ALLOCATOR>::BaseNode
    {
        T value{};

        template<typename... ARGS>
        Node(ARGS&&... args)
            : value { std::forward<ARGS>(args)... }
        {}
    };

} // end namespace



template<typename T, typename ALLOCATOR>
mylib::List<T, ALLOCATOR>::Node* mylib::List<T, ALLOCATOR>::
    allocateNode() const
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
const T& mylib::List<T, ALLOCATOR>::back() const noexcept
{
    assert(!empty());

    return (static_cast<const Node*>(tail()))->value;
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
            newNodePtr = allocateNode();
            constructNode(newNodePtr, *begin);
        }
        catch(...)
        {
            if(tail() != startNodePtr)
            {
                destroyElementsInRange(startNodePtr->nextPtr, &m_sentinel);
            }
            if(newNodePtr)
            {
                deallocateNode(newNodePtr);
            }

            throw;
        }

        insertAfter(tail(), newNodePtr);

        ++begin;
        ++m_size;
    }
}



template<typename T, typename ALLOCATOR>
void mylib::List<T, ALLOCATOR>::
    constructNode(Node* nodePtr, const T& value)
{
    List<T, ALLOCATOR>::ALLOC_TRAITS::construct(m_nodeAllocator, nodePtr, value);
}



template<typename T, typename ALLOCATOR>
void mylib::List<T, ALLOCATOR>::
    constructNode(Node* nodePtr, T&& value)
{
    List<T, ALLOCATOR>::ALLOC_TRAITS::construct(m_nodeAllocator, nodePtr, std::move(value));
}



template<typename T, typename ALLOCATOR>
void mylib::List<T, ALLOCATOR>::
    cutNode(BaseNode* nodePtr) noexcept
{
    nodePtr->nextPtr->prevPtr = nodePtr->prevPtr;
    nodePtr->prevPtr->nextPtr = nodePtr->nextPtr;
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
        --m_size;
    }
}



template<typename T, typename ALLOCATOR>
void mylib::List<T, ALLOCATOR>::
    destroyNode(Node* nodePtr)
{
    List<T, ALLOCATOR>::ALLOC_TRAITS::destroy(m_nodeAllocator, nodePtr);
}



template<typename T, typename ALLOCATOR>
T& mylib::List<T, ALLOCATOR>::front() noexcept
{
    assert(!empty());

    return (static_cast<Node*>(root()))->value;
};



template<typename T, typename ALLOCATOR>
const T& mylib::List<T, ALLOCATOR>::front() const noexcept
{
    assert(!empty());

    return (static_cast<const Node*>(root()))->value;
}



template<typename T, typename ALLOCATOR>
void mylib::List<T, ALLOCATOR>::
    insertAfter(BaseNode* positionPtr, BaseNode* newNodePtr) noexcept
{
    newNodePtr->nextPtr = positionPtr->nextPtr;
    newNodePtr->prevPtr = positionPtr;
    positionPtr->nextPtr->prevPtr = newNodePtr;
    positionPtr->nextPtr = newNodePtr;
}



template<typename T, typename ALLOCATOR>
void mylib::List<T, ALLOCATOR>::
    insertBefore(BaseNode* positionPtr, BaseNode* newNodePtr) noexcept
{
    newNodePtr->nextPtr = positionPtr;
    newNodePtr->prevPtr = positionPtr->prevPtr;
    positionPtr->prevPtr->nextPtr = newNodePtr;
    positionPtr->prevPtr = newNodePtr;
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
    while(m_size < count)
    {
        Node* newNodePtr{ nullptr };
        try
        {
            newNodePtr = allocateNode();
            constructNode(newNodePtr, value);
        }
        catch (...)
        {
            destroyAll();
            if(newNodePtr)
            {
                deallocateNode(newNodePtr);
            }
            throw;
        }

        insertAfter(tail(), newNodePtr);
        ++m_size;
    }
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
        : m_sentinel{ std::move(other.m_sentinel) }
        , m_size{ other.m_size }
        , m_nodeAllocator{ std::move(other.m_nodeAllocator) }
{
    other.release();
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
void mylib::List<T, ALLOCATOR>::
    release() noexcept
{
    m_size = 0;
    m_sentinel.nextPtr = &m_sentinel;
    m_sentinel.prevPtr = &m_sentinel;
}



template<typename T, typename ALLOCATOR>
void mylib::List<T, ALLOCATOR>::swap(List& other) noexcept
{
    std::swap(m_size, other.m_size);
    std::swap(m_sentinel, other.m_sentinel);
    std::swap(m_nodeAllocator, other.m_nodeAllocator);
}


#endif // LIST_H
