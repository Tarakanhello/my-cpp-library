#ifndef CHUNKED_ARRAY_H
#define CHUNKED_ARRAY_H

#include <initializer_list>
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
        using ALLOCATOR_ptr = typename std::allocator_traits<ALLOCATOR>::template rebind_alloc<T*>;

        size_t m_size{};
        constexpr static size_t m_chunkSize{ CHUNK_SIZE };
        ALLOCATOR m_blockAllocator{};
        mylib::Vector<T*, ALLOCATOR_ptr> m_blocks{};


        size_t blockIndex(size_t i) const { return i / m_chunkSize; }
        size_t offset(size_t i) const { return i % m_chunkSize; }

        T* getBlock(size_t blockIndex) { return m_blocks[blockIndex]; }
        const T* getBlock(size_t blockIndex) const { return m_blocks[blockIndex]; }

        void createBlocks();
        T* allocateBlock(); // выделяет новый блок через m_allocator.allocate(CHUNK_SIZE)
        void destroyBlock(T* block, size_t count); // уничтожает count элементов в блоке.
        void deallocateBlock(T* block); // m_allocator.deallocate(block, CHUNK_SIZE)

    public:
        // Конструкторы:
        ChunkedArray();
        ChunkedArray(size_t, T = T(), ALLOCATOR = ALLOCATOR());
        ChunkedArray(std::initializer_list<T>, ALLOCATOR = ALLOCATOR());

        ~ChunkedArray();


        // Доступ:
        T& at(size_t);
        const T& at(size_t) const;

        T& operator[](size_t);
        const T& operator[](size_t) const;

        T& front();
        const T& front() const;

        T& back();
        const T& back() const;


        // Вставка:
        void push_back(const T&);
        void push_back(T&&);
        void emplace_back(...);

        // Удаление:
        T pop_back(); // удалить последний элемент; если блок стал пустым – освободить блок и удалить указатель.

        //Когда m_size == CHUNK_SIZE * m_blocks.size() – нужно добавить новый блок.
        // При добавлении нового блока нужно выделить память для блока через аллокатор.
        // Если выделение блока выбрасывает исключение, старые данные не повреждаются.
        // При освобождении блока нужно вызвать деструкторы элементов (если они есть) и освободить память.

        void reserve(size_t size);

        void shrink_to_fit();

        // ITERATORS
        /*
         * Итератор должен хранить:
         *   - указатель на текущий блок,
         *   - смещение внутри блока,
         *   - указатель на конец текущего блока,
         *   - общий указатель на текущий элемент.
         */

    }; // end class ChunkedArray

} // end namespace mylib

template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::ChunkedArray()
    : m_size{ 0 }
    , m_blockAllocator{}
    , m_blocks{}
{}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::ChunkedArray(size_t size,
                                                            T value,
                                                            ALLOCATOR alloc)
    : m_size{ size }
    , m_blockAllocator{ alloc }
{

    createBlocks();

    for(size_t i{}, j{}; auto& block : m_blocks)
    {
        T* newBlock{ allocateBlock() };
        for(; i == blockIndex(j) && j < m_size; ++j)
        {
            new (&newBlock[offset(j)]) T(value);
        }
        block = newBlock;
        ++i;
    }
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::ChunkedArray(std::initializer_list<T> list, ALLOCATOR alloc)
    : m_size{ list.size() }
    , m_blockAllocator{ alloc }
{
    createBlocks();

    for(size_t i{}, j{}; auto& block : m_blocks)
    {
        T* newBlock{ allocateBlock() };
        for(; i == blockIndex(j) && j < m_size; ++j)
        {
            new (&newBlock[offset(j)]) T(list.begin()[j]);
        }
        block = newBlock;
        ++i;
    }
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
T* mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::allocateBlock()
{
    return m_blockAllocator.allocate(m_chunkSize);
}



template<typename T, size_t CHUNK_SIZE, typename ALLOCATOR>
void mylib::ChunkedArray<T, CHUNK_SIZE, ALLOCATOR>::createBlocks()
{
    if(!m_blocks.empty())
    {
        for(size_t i{}; i < m_size; ++i)
        {
            m_blocks[blockIndex(i)][offset(i)].~T();
        }
        for(size_t i{}; i < m_blocks.size(); ++i)
        {
            deallocateBlock(m_blocks[i]);
            m_blocks[i] = nullptr;
        }

    }

    if(m_size)
        m_blocks = mylib::Vector<T*, ALLOCATOR_ptr>((m_size - 1) / m_chunkSize + 1, nullptr);
    else
    {
        m_blocks.clear();
    }
}

#endif // CHUNKED_ARRAY_H

