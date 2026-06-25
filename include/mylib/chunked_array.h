#ifndef CHUNKED_ARRAY_H
#define CHUNKED_ARRAY_H

#include <initializer_list>
#include <memory>

#include "mylib/memory.h"
#include "mylib/structs.h"
#include "mylib/vector.h"



template<typename T,
         size_t CHUNK_SIZE = 32,
         typename ALLOCATOR = mylib::MySimpleAllocator<T>>
class ChunkedArray final : public mylib::ArithmeticType<T>
{
private:
    using ALLOCATOR_ptr = typename std::allocator_traits<ALLOCATOR>::template rebind_alloc<T*>;

    mylib::Vector<T*, ALLOCATOR_ptr> m_blocks{};
    size_t m_size{};
    size_t m_chunkSize{};
    ALLOCATOR m_chunkAllocator{};
    ALLOCATOR_ptr m_blockAllocator{};

    size_t blockIndex(size_t i) const { return i / m_chunkSize; }
    size_t offset(size_t i) const { return i % m_chunkSize; }

    T* getBlock(size_t blockIndex) { return m_blocks[blockIndex]; }
    const T* getBlock(size_t blockIndex) const { return m_blocks[blockIndex]; }

    void allocateBlock(); // выделяет новый блок через m_allocator.allocate(CHUNK_SIZE)
    void destroyBlock(T* block, size_t count); // уничтожает count элементов в блоке.
    void deallocateBlock(T* block); // m_allocator.deallocate(block, CHUNK_SIZE)

public:
    // Конструкторы:
    ChunkedArray();
    ChunkedArray(size_t, T = T());
    ChunkedArray(std::initializer_list<T>);


    // Доступ:
    ChunkedArray& at(size_t);
    const ChunkedArray& at(size_t) const;

    ChunkedArray& operator[](size_t);
    const ChunkedArray& operator[](size_t) const;

    ChunkedArray& front();
    const ChunkedArray& front() const;

    ChunkedArray& back();
    const ChunkedArray& back() const;


    // Вставка:
    void push_back(const T&);
    void push_back(const T&&);
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
};

#endif // CHUNKED_ARRAY_H
