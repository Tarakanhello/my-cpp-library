#ifndef FREE_LIST_H
#define FREE_LIST_H

#include "mylib/list.h"
#include "mylib/memory.h"

template<typename T, typename ALLOCATOR = mylib::MySimpleAllocator<T>>
class FreeList final
{
private:
    static constexpr size_t MIN_BLOCK_SIZE{ 8 };
    static constexpr size_t MAX_BLOCK_SIZE{ 8192 };
    static constexpr size_t DEFAULT_SIZE{ 32 };


    struct Block
    {
        size_t capacity;    // общее количество ячеек в блоке
        size_t size;        // текущее количество занятых ячеек
        size_t maxSize;     // максимальный индекс выделенных ячеек (для новых)

        struct Item
        {
            T item;
            union
            {
                T* next; // для свободных ячеек
                void* userData; // для занятых (указатель на Block)
            };
        };

        Item* nodes;    // массив Item
        Item* userData; // массив Item

        Block(size_t cap);
        ~Block();
        // Создаём массив флагов размером maxSize, все ставим в true
        // (значит, подлежат уничтожению).
        // Проходим по списку возвращённых ячеек и помечаем их как false
        // (они уже уничтожены, их деструкторы вызывать не надо).
        // Затем проходим по всем ячейкам от 0 до maxSize-1 и, если флаг true,
        // вызываем деструктор объекта.
        // После этого освобождаем всю память массива.

        bool isFull() const { return size == capacity; }
        bool isEmpty() const { return size == 0; }
        T* allocate();
        // Сначала проверяем, есть ли ячейка в списке возвращённых.
        // возвращенная ячейка - ранее использовавшаяся, но сейчас пустая (удаленная)
        // Если есть — берём первую и удаляем её из списка.
        // Если список пуст — значит, все ранее выделенные ячейки заняты,
        // и мы берём новую из ещё не использованной части массива
        // (с индексом maxSize), увеличивая maxSize.
        // Если массив полностью заполнен (size == capacity),
        // выделение невозможно — нужно создать новый блок
        // (это уже динамический вариант).

        void remove(Item* node);
        // Вызываем деструктор объекта.
        // Добавляем ячейку в начало списка возвращённых
        // (просто перенаправляем указатель next).
        // Уменьшаем size (количество занятых ячеек).

        void collectGarbage(); // вызывается в деструкторе блока
    };

    using BlockList = mylib::List<Block, ALLOCATOR>;

    BlockList blocks;
    size_t blockSise{}; // размер для нового блока
    void createNewBlock();

public:
    explicit FreeList(size_t initialSize = DEFAULT_SIZE, ALLOCATOR alloc = ALLOCATOR());
    ~FreeList();


    FreeList(const FreeList&) = delete;
    FreeList& operator=(const FreeList&) = delete;

    T* allocate();
    // Берём первый блок в списке. Если он полон или список пуст,
    // создаём новый блок (с текущим размером, который увеличивается вдвое)
    // и добавляем его в начало.
    // Выделяем ячейку из этого блока.


    void remove(T* item);
    // По указателю на объект определяем, из какого блока он родом
    // (для этого в объекте хранится ссылка на блок).
    // Возвращаем ячейку в этот блок.
    // Если блок стал пустым, удаляем его (память освобождается).
    // Если блок не пуст, перемещаем его в начало списка
    // (чтобы неполные блоки были впереди).
    // Если блок стал неполным, но был полным, тоже перемещаем в начало.

};

#endif // FREE_LIST_H
