#ifndef CHUNKED_ARRAY_H
#define CHUNKED_ARRAY_H

#include <cassert>
#include <compare>
#include <initializer_list>
#include <iterator>
#include <memory>

#include "mylib/memory.h"
#include "mylib/structs.h"
#include "mylib/vector.h"



namespace mylib
{

    template<typename T,
             size_t CHUNK_SIZE = 32,
             typename ALLOCATOR = mylib::MySimpleAllocator<T>>
    class ChunkedArray final : public mylib::ArithmeticType<T>
    {
    private:
        using ALLOCATOR_ptr = typename std::allocator_traits<ALLOCATOR>::
                                                    template rebind_alloc<T*>;

        size_t m_size{};
        constexpr static size_t m_chunkSize{ CHUNK_SIZE };
        ALLOCATOR m_chunkAllocator{};
        mylib::Vector<T*, ALLOCATOR_ptr> m_arrayOfChunks{};

        void allocateArrayOfChunks();
        T* allocateChunk();
        constexpr void constructElements(size_t from, size_t to, const T& value = T());

        template<typename ITERATOR>
        constexpr void constructElementsFromRange(size_t from,
                                                  ITERATOR first,
                                                  ITERATOR last);
        constexpr size_t chunkIndex(size_t i) const noexcept;
        void deallocateChunk(T* chunk) noexcept;
        void destroyAllChunks() noexcept;
        void destroyElements(size_t from, size_t to) noexcept;
        constexpr T* getChunk(size_t chunkIndex) noexcept;
        constexpr const T* getChunk(size_t chunkIndex) const noexcept;
        constexpr size_t offsetInChunk(size_t i) const noexcept;
        void release() noexcept;
        void swap(ChunkedArray& other) noexcept;

    public:
        // Конструкторы:
        ChunkedArray();
        ChunkedArray(size_t, T = T(), ALLOCATOR = ALLOCATOR());
        ChunkedArray(std::initializer_list<T>, ALLOCATOR = ALLOCATOR());
        ChunkedArray(const ChunkedArray&);
        ChunkedArray& operator=(const ChunkedArray&);
        ChunkedArray(ChunkedArray&&) noexcept;
        ChunkedArray& operator=(ChunkedArray&&) noexcept;

        ~ChunkedArray();

        // Доступ:
        constexpr T& at(size_t);
        constexpr const T& at(size_t) const;

        constexpr T& operator[](size_t) noexcept;
        constexpr const T& operator[](size_t) const noexcept;

        constexpr T& front() noexcept;
        constexpr const T& front() const noexcept;

        constexpr T& back() noexcept;
        constexpr const T& back() const noexcept;

        // Вставка:
        void push_back(const T&);
        void push_back(T&&);
        void emplace_back(/*будет написано позже*/);

        // Удаление:
        T pop_back(); // удалить последний элемент; если блок стал пустым – освободить блок и удалить указатель.
        void clear();


        void resize(size_t newSize, const T& value = T());

        //Когда m_size == CHUNK_SIZE * m_arrayOfChunks.size() – нужно добавить новый блок.
        // При добавлении нового блока нужно выделить память для блока через аллокатор.
        // Если выделение блока выбрасывает исключение, старые данные не повреждаются.
        // При освобождении блока нужно вызвать деструкторы элементов (если они есть) и освободить память.

        void reserve(size_t newCapacity);

        void shrink_to_fit();

        size_t size() const noexcept { return m_size; }
        bool empty() const noexcept { return m_size == 0; }
        size_t blockCount() const noexcept { return m_arrayOfChunks.size(); }

        explicit operator bool() noexcept { return !empty(); }

        // ITERATORS

        class Iterator;
        class ConstIterator;
        using reverse_iterator = std::reverse_iterator<Iterator>;
        using const_reverse_iterator = std::reverse_iterator<ConstIterator>;

        /**
         * @brief Возвращает итератор на первый элемент.
         */
        constexpr Iterator begin() noexcept
        {
            return Iterator{ m_arrayOfChunks.data(), m_arrayOfChunks.data() + m_arrayOfChunks.size() };
        }

        /**
         * @brief Возвращает константный итератор на первый элемент.
         */
        constexpr ConstIterator begin() const noexcept
        {
            return cbegin();
        }

        /**
         * @brief Возвращает константный итератор на первый элемент.
         */
        constexpr ConstIterator cbegin() const noexcept
        {
            return ConstIterator{ m_arrayOfChunks.data(), m_arrayOfChunks.data() + m_arrayOfChunks.size() };
        }

        /**
         * @brief Возвращает итератор на элемент, следующий за последним.
         */
        constexpr Iterator end() noexcept
        {
            return Iterator{ m_arrayOfChunks.data(), m_arrayOfChunks.data() + m_arrayOfChunks.size(), size() };
        }

        /**
         * @brief Возвращает константный итератор на элемент, следующий за последним.
         */
        constexpr ConstIterator end() const noexcept
        {
            return cend();
        }

        /**
         * @brief Возвращает константный итератор на элемент, следующий за последним.
         */
        constexpr ConstIterator cend() const noexcept
        {
            return ConstIterator{ m_arrayOfChunks.data(), m_arrayOfChunks.data() + m_arrayOfChunks.size(), size() };

        }

        /**
         * @brief Возвращает обратный итератор на элемент, следующий за последним.
         */
        constexpr reverse_iterator rbegin() noexcept
        {
            return end();

        }

        /**
         * @brief Возвращает константный обратный итератор на элемент, следующий за последним.
         */
        constexpr const_reverse_iterator rbegin() const noexcept
        {
            return cend();

        }

        /**
         * @brief Возвращает константный обратный итератор на элемент, следующий за последним.
         */
        constexpr const_reverse_iterator crbegin() const noexcept
        {
            return cend();

        }

        /**
         * @brief Возвращает обратный итератор на первый элемент.
         */
        constexpr reverse_iterator rend() noexcept
        {
            return begin();
        }

        /**
         * @brief Возвращает константный обратный итератор на первый элемент.
         */
        constexpr const_reverse_iterator rend() const noexcept
        {
            return cbegin();
        }

        /**
         * @brief Возвращает константный обратный итератор на первый элемент.
         */
        constexpr const_reverse_iterator crend() const noexcept
        {
            return cbegin();
        }

    }; // end class ChunkedArray



    template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
    class ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::Iterator
    {
    private:
        T** m_chunkPtr;
        T** m_chunksEnd;
        T* m_currentElementPtr;
        size_t m_offset;

    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = T*;
        using reference         = T&;

        friend class ConstIterator;

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


    template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
    class ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::ConstIterator
    {
    private:
        const T** m_chunkPtr;
        const T** m_chunksEnd;
        const T* m_currentElementPtr;
        size_t m_offset;

    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const T*;
        using reference         = const T&;

        constexpr ConstIterator(const T** beginChunkPtr,
                                const T** chunksEnd,
                                size_t startIndex = 0) noexcept;
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

    constructElements(0, size, value);
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    ChunkedArray(std::initializer_list<T> list, ALLOCATOR alloc)
    : m_size{ list.size() }
    , m_chunkAllocator{ alloc }
{
    allocateArrayOfChunks();

    constructElementsFromRange(0, list.begin(), list.end());
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    ChunkedArray(const ChunkedArray& other)
    : m_size{ other.size() }
    , m_chunkAllocator{ other.m_chunkAllocator }
{
    allocateArrayOfChunks();

    constructElementsFromRange(0, other.begin(), other.end());
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
    if(m_size)
    {
        m_arrayOfChunks = mylib::Vector<T*,
                        ALLOCATOR_ptr>((m_size - 1) / m_chunkSize + 1, nullptr);
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
constexpr size_t mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    chunkIndex(size_t i) const noexcept
{
    return i / m_chunkSize;
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
constexpr mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    ConstIterator::ConstIterator(const T** beginChunkPtr,
                                 const T** chunksEnd,
                                 size_t startIndex) noexcept
    : m_chunkPtr{ beginChunkPtr }
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
    : m_chunkPtr{ other.m_chunkPtr }
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

    if(0 == m_offset)
    {
        m_offset = m_chunkSize;
        --m_chunkPtr;
    }

    --m_offset;

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

    while(n >= m_chunkSize)
    {
        ++m_chunkPtr;
        n -= m_chunkSize;
        if(m_chunkPtr == m_chunksEnd)
        {
            m_currentElementPtr = nullptr;
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

    while(n >= m_chunkSize)
    {
        --m_chunkPtr;
        n -= m_chunkSize;
        if(m_chunkPtr == m_chunksEnd)
        {
            m_currentElementPtr = nullptr;
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
constexpr void mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
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
constexpr void mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
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
        for(size_t i{}; i < m_size; ++i)
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
void mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::destroyElements(size_t from, size_t to) noexcept
{
    for(size_t i{ from }; i < to; ++i)
    {
        m_arrayOfChunks[chunkIndex(i)][offsetInChunk(i)].~T();
    }
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
        : m_chunkPtr{ beginChunkPtr }
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

    if(0 == m_offset)
    {
        m_offset = m_chunkSize;
        --m_chunkPtr;
    }

    --m_offset;

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

    while(n >= m_chunkSize)
    {
        ++m_chunkPtr;
        n -= m_chunkSize;
        if(m_chunkPtr == m_chunksEnd)
        {
            m_currentElementPtr = nullptr;
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

    while(n >= m_chunkSize)
    {
        --m_chunkPtr;
        n -= m_chunkSize;
        if(m_chunkPtr == m_chunksEnd)
        {
            m_currentElementPtr = nullptr;
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
    Iterator::operator<=>(const mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::Iterator& other) const noexcept
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
    Iterator::operator==(const mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::Iterator& other) const noexcept
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
void mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::
    release() noexcept
{
    m_size = 0;
    m_arrayOfChunks.clear();
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

