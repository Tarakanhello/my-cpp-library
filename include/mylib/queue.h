#ifndef QUEUE_H
#define QUEUE_H

#include <cstddef>
#include <initializer_list>
#include <stdexcept>
#include <utility>

#include "mylib/memory.h"
#include "mylib/structs.h"


namespace mylib
{

/**
 * @brief Шаблонный класс очереди с динамическим циклическим буфером.
 *
 * @tparam T         Тип хранимых элементов.
 * @tparam ALLOCATOR Тип аллокатора, совместимого с std::allocator_traits.
 *                   По умолчанию используется mylib::MySimpleAllocator<T>.
 *
 * @details Очередь реализована как циклический буфер, что обеспечивает
 *          амортизированное O(1) добавление и удаление элементов.
 *          При заполнении буфера ёмкость автоматически увеличивается
 *          (обычно в 2 раза). Минимальная ёмкость фиксирована и равна 8.
 *
 * @note Все операции с элементами используют предоставленный аллокатор.
 *       Класс является final и не предназначен для наследования.
 */
template<typename T, typename ALLOCATOR = mylib::MySimpleAllocator<T>>
    requires std::is_nothrow_move_constructible_v<T>
class Queue final
{
private:
    enum { MinCapacity = 8 }; ///< Минимальная ёмкость очереди.

    using AllocTraits = std::allocator_traits<ALLOCATOR>;

    T* m_data{ nullptr };   ///< Указатель на массив элементов.

    size_t m_front{};       ///< Индекс первого элемента в буфере.
    size_t m_size{};        ///< Текущее количество элементов.
    size_t m_capacity{};    ///< Текущая ёмкость буфера.

    ALLOCATOR m_alloc{};    ///< Экземпляр аллокатора.

    /**
     * @brief Выделяет память под заданное количество элементов.
     * @param size Количество элементов.
     * @return Указатель на выделенную память.
     * @throw Исключение, генерируемое аллокатором при неудаче.
     */
    T* allocate(size_t size)
    {
        return AllocTraits::allocate(m_alloc, size);
    }

    /**
     * @brief Конструирует элемент по переданным аргументам.
     * @tparam ARGS Типы аргументов конструктора.
     * @param elementPtr Указатель на место размещения.
     * @param args       Аргументы для конструктора T.
     * @post Увеличивает m_size на 1.
     */
    template<typename... ARGS>
    void constructElement(T* elementPtr, ARGS&&... args)
    {
        AllocTraits::construct(m_alloc, elementPtr, std::forward<ARGS>(args)...);
        ++m_size;
    }

    /**
     * @brief Освобождает выделенную память, если она была выделена.
     * @note Не уничтожает элементы, предполагается, что они уже уничтожены.
     */
    constexpr void deallocate() noexcept
    {
        if(m_data)
        {
            AllocTraits::deallocate(m_alloc, m_data, m_capacity);
        }
    }

    /**
     * @brief Уничтожает все элементы в очереди.
     * @post m_size становится равным 0.
     */
    constexpr void destroyAll() noexcept
    {
        for(size_t i{}; i < size(); ++i)
        {
            AllocTraits::destroy(m_alloc, m_data + offset(i));
        }
        m_size = 0;
    }

    /**
     * @brief Уничтожает один элемент по указателю.
     * @param elementPtr Указатель на элемент.
     * @post Уменьшает m_size на 1.
     */
    constexpr void destroyElement(T* elementPtr) noexcept
    {
        AllocTraits::destroy(m_alloc, elementPtr);
        --m_size;
    }

    /**
     * @brief Вычисляет максимально допустимый размер очереди.
     * @return Максимальное количество элементов, которое можно выделить.
     */
    static constexpr size_t maxSize() noexcept
    {
        return std::numeric_limits<size_t>::max() / sizeof(T);
    }

    /**
     * @brief Преобразует логический индекс в физический с учётом циклического буфера.
     * @param i Логический индекс (0 .. size()-1).
     * @return Физический индекс в массиве m_data.
     */
    constexpr size_t offset(size_t i) const noexcept
    {
        return (m_front + i) % m_capacity;
    }

    /**
     * @brief Сбрасывает состояние очереди, не освобождая память.
     * @post Все указатели обнуляются, размеры становятся равными 0.
     */
    constexpr void release() noexcept
    {
        m_data = nullptr;
        m_front = 0;
        m_size = 0;
        m_capacity = 0;
    }

public:
    /**
     * @brief Конструктор по умолчанию.
     * @param alloc Аллокатор, который будет использоваться очередью.
     * @post Создаётся пустая очередь без выделенной памяти.
     */
    explicit Queue(const ALLOCATOR& alloc = ALLOCATOR())
        : m_data{ nullptr }
        , m_front{ 0 }
        , m_size{ 0 }
        , m_capacity{ 0 }
        , m_alloc{ alloc }
    {}

    /**
     * @brief Конструктор, задающий начальную ёмкость.
     * @param capacity Желаемая начальная ёмкость. Если меньше MinCapacity,
     *                 будет использовано MinCapacity.
     * @param alloc    Аллокатор.
     * @throw std::bad_alloc или исключение аллокатора при выделении памяти.
     * @post Очередь пуста, но память под capacity элементов выделена.
     */
    explicit Queue(size_t capacity, const ALLOCATOR& alloc = ALLOCATOR())
        : m_data{ nullptr }
        , m_front{ 0 }
        , m_size{ 0 }
        , m_alloc{ alloc }
    {
        m_capacity = std::max(static_cast<size_t>(MinCapacity), capacity);
        m_data = allocate(m_capacity);
    }

    /**
     * @brief Конструктор копирования.
     * @param other Копируемая очередь.
     * @throw Исключения при выделении памяти, копировании элементов или
     *        при работе аллокатора.
     * @post Создаётся глубокая копия other.
     */
    Queue(const Queue& other)
        : m_front{}
        , m_size{}
        , m_capacity{ other.m_capacity }
        , m_alloc{ other.m_alloc }
    {
        try
        {
            m_data = allocate(m_capacity);
            BufferGuard guard{ m_data };
            for(size_t i{}; i < other.size(); ++i)
            {
                constructElement(m_data + i, *(other.m_data + other.offset(i)));
                guard.addConstructed();
            }
            guard.commit();
        }
        catch(...)
        {
            if(m_data)
            {
                deallocate();
            }

            release();
            throw;
        }
    }

    /**
     * @brief Конструктор, создающий очередь из списка инициализации.
     *
     * @param list Список элементов для инициализации.
     * @param alloc Аллокатор, который будет использоваться очередью.
     *
     * @throw Исключения при выделении памяти или копировании элементов.
     */
    Queue(const std::initializer_list<T>& list, const ALLOCATOR& alloc = ALLOCATOR())
        : m_front{ 0 }
        , m_size{ 0 }
        , m_capacity{ calculateNewCapacity(list.size(), maxSize(), MinCapacity, "Queue::ListConstructor") }
        , m_alloc{ alloc }
    {
        m_data = allocate(m_capacity);
        BufferGuard guard{ m_data };

        for (size_t i{}; i < list.size(); ++i)
        {
            constructElement(m_data + i, *(list.begin() + i));
            guard.addConstructed();
        }

        guard.commit();
    }

    /**
     * @brief Конструктор из произвольного диапазона итераторов.
     * @tparam INPUT_IT Тип итератора ввода.
     * @param first Начало диапазона.
     * @param last  Конец диапазона.
     * @param alloc Аллокатор.
     */
    template <typename INPUT_IT>
        requires std::forward_iterator<INPUT_IT>
    Queue(INPUT_IT first, INPUT_IT last, const ALLOCATOR& alloc = ALLOCATOR())
        : m_front{ 0 }
        , m_size{ 0 }
        , m_capacity{ calculateNewCapacity(std::distance(first, last), maxSize(), MinCapacity, "Queue::RangeConstructor") }
        , m_alloc{ alloc }
    {
        m_data = allocate(m_capacity);
        BufferGuard guard{ m_data };

        for(size_t i{}; first != last; ++i)
        {
            constructElement(m_data + i, *first);
            guard.addConstructed();
            ++first;
        }

        guard.commit();
    }

    /** @copydoc Queue(INPUT_IT, INPUT_IT, const ALLOCATOR&) */
    template <typename INPUT_IT>
        requires std::input_iterator<INPUT_IT> && (!std::forward_iterator<INPUT_IT>)
    Queue(INPUT_IT first, INPUT_IT last, const ALLOCATOR& alloc = ALLOCATOR())
        : Queue(alloc)  // делегируем конструктору по умолчанию
    {
        while (first != last)
        {
            push(*first);
            ++first;
        }
    }

    /**
     * @brief Конструктор перемещения.
     * @param other Перемещаемая очередь.
     * @post other становится пустой (перемещение владения).
     */
    Queue(Queue&& other) noexcept
        : m_data{ other.m_data }
        , m_front{ other.m_front }
        , m_size{ other.m_size }
        , m_capacity{ other.m_capacity }
        , m_alloc{ std::move(other.m_alloc) }
    {
        other.release();
    }

    /**
     * @brief Деструктор.
     * @note Уничтожает все элементы и освобождает выделенную память.
     */
    ~Queue() noexcept
    {
        destroyAll();
        deallocate();
        release();
    }

    /**
     * @brief Оператор присваивания копированием.
     * @param other Копируемая очередь.
     * @return Ссылка на текущий объект.
     * @throw Исключения при копировании (выделение памяти, конструирование элементов).
     * @note Используется идиома copy-and-swap, обеспечивающая строгую гарантию безопасности.
     */
    Queue& operator=(const Queue& other)
    {
        if(this != &other)
        {
            Queue temp{ other };
            swap(temp);
        }
        return *this;
    }

    /**
     * @brief Оператор присваивания перемещением.
     * @param other Перемещаемая очередь.
     * @return Ссылка на текущий объект.
     * @note noexcept.
     */
    Queue& operator=(Queue&& other) noexcept
    {
        if(this != &other)
        {
            swap(other);

            other.destroyAll();
            other.deallocate();
            other.release();
        }
        return *this;
    }

    /**
     * @brief Добавляет новый элемент в конец очереди.
     * @tparam ARGS Типы аргументов для конструирования элемента.
     * @param args Аргументы, передаваемые конструктору T.
     * @throw Исключения при выделении памяти или конструировании элемента.
     * @details Если очередь пуста, выделяется начальная память ёмкостью MinCapacity.
     *          При переполнении буфера ёмкость увеличивается (обычно вдвое),
     *          существующие элементы перемещаются в новый буфер.
     */
    template<typename... ARGS>
    void push(ARGS&&... args)
    {
        if(!m_data)
        {
            m_data = allocate(MinCapacity);
            m_capacity = MinCapacity;
        }

        if(size() == capacity())
        {
            size_t newCap{ calculateNewCapacity(m_capacity * 2, maxSize(), MinCapacity, "mylib::Queue") };
            Queue temp(newCap, m_alloc);
            for(size_t i{}; i < size(); ++i)
            {
                temp.constructElement(temp.m_data + i, std::move(*(m_data + offset(i))));
            }
            swap(temp);
            constructElement(m_data + offset(size()), std::forward<ARGS>(args)...);
        }
        else
        {
            constructElement(m_data + offset(size()), std::forward<ARGS>(args)...);
        }
    }

    /**
     * @brief Удаляет первый элемент из очереди.
     * @throw std::out_of_range, если очередь пуста.
     * @details Уничтожает элемент и сдвигает указатель front.
     */
    void pop()
    {
        if(empty())
        {
            throw std::out_of_range("Queue::pop() on empty queue");
        }
        destroyElement(m_data + m_front);
        m_front = offset(1);
    }

    /**
     * @brief Возвращает ссылку на первый элемент очереди.
     * @return Ссылка на первый элемент.
     * @throw std::out_of_range, если очередь пуста.
     */
    T& front()
    {
        if(empty())
        {
            throw std::out_of_range("Queue::front() on empty queue");
        }

        return *(m_data + m_front);
    }

    /** @copydoc front() */
    const T& front() const
    {
        if(empty())
        {
            throw std::out_of_range("Queue::front() on empty queue");
        }
        return *(m_data + m_front);
    }

    /**
     * @brief Возвращает ссылку на последний элемент очереди.
     * @return Ссылка на последний элемент.
     * @throw std::out_of_range, если очередь пуста.
     */
    T& back()
    {
        if(empty())
        {
            throw std::out_of_range("Queue::back() on empty queue");
        }

        return *(m_data + offset(size() - 1));
    }

    /** @copydoc back() */
    const T& back() const
    {
        if(empty())
        {
            throw std::out_of_range("Queue::back() on empty queue");
        }

        return *(m_data + offset(size() - 1));
    }

    /**
     * @brief Проверяет, пуста ли очередь.
     * @return true, если очередь не содержит элементов, иначе false.
     */
    bool empty() const noexcept { return size() == 0; }

    /**
     * @brief Преобразование к bool.
     * @return true, если очередь не пуста, иначе false.
     * @note Эквивалентно !empty().
     */
    explicit operator bool() const noexcept { return !empty(); }

    /**
     * @brief Возвращает количество элементов в очереди.
     * @return Текущий размер очереди.
     */
    size_t size() const noexcept { return m_size; }

    /**
     * @brief Возвращает текущую ёмкость очереди.
     *
     * @return size_t Количество элементов, под которые выделена память.
     */
    size_t capacity() const noexcept { return m_capacity; }

    /**
     * @brief Удаляет все элементы из очереди.
     * @note Память при этом не освобождается, ёмкость сохраняется.
     */
    void clear() noexcept
    {
        destroyAll();
    }

    /**
     * @brief Сравнение двух очередей на равенство.
     * @param other Очередь для сравнения.
     * @return true, если размеры и все соответствующие элементы равны.
     */
    bool operator==(const Queue& other) const
    {
        if(size() != other.size())
        {
            return false;
        }
        for(size_t i{}; i < size(); ++i)
        {
            if(*(m_data + offset(i)) != *(other.m_data + offset(i)))
            {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief Трёхстороннее (лексикографическое) сравнение очередей.
     * @param other Очередь для сравнения.
     * @return std::strong_ordering::less, greater или equal.
     * @details Сначала сравниваются размеры, затем поэлементно.
     */
    auto operator<=>(const Queue& other) const
    {
        if(size() != other.size())
        {
            return size() <=> other.size();
        }

        for(size_t i{}; i < size(); ++i)
        {
            if(*(m_data + offset(i)) < *(other.m_data + offset(i)))
            {
                return std::strong_ordering::less;
            }
            if(*(m_data + offset(i)) > *(other.m_data + offset(i)))
            {
                return std::strong_ordering::greater;
            }
        }
        return std::strong_ordering::equal;
    }

    /**
     * @brief Резервирует память для указанного количества элементов без их добавления.
     *
     * @param newCap Желаемая новая ёмкость. Если меньше текущей ёмкости, ничего не делает.
     *
     * @throw std::length_error Если newCap превышает максимально допустимый размер.
     * @throw Исключения при выделении памяти.
     */
    void reserve(size_t newCap)
    {
        if(newCap <= capacity())
        {
            return;
        }

        Queue temp(calculateNewCapacity(newCap, maxSize(), MinCapacity, "mylib::Queue::Reserve"), m_alloc);

        for(size_t i{}; i < size(); ++i)
        {
            temp.push(std::move(*(m_data + offset(i))));
        }

        swap(temp);
    }

    /**
     * @brief Уменьшает ёмкость очереди до её текущего размера.
     *
     * @throw Исключения при выделении памяти или перемещении элементов.
     * @note Если очередь пуста, освобождает всю память.
     */
    void shrink_to_fit()
    {
        if(empty())
        {
            deallocate();
            release();
            return;
        }

        Queue temp(size(), m_alloc);

        for(size_t i{}; i < size(); ++i)
        {
            temp.push(std::move(*(m_data + offset(i))));
        }

        swap(temp);
    }

    /**
     * @brief Обменивает содержимое двух очередей.
     * @param other Очередь, с которой производится обмен.
     */
    constexpr void swap(Queue& other) noexcept
    {
        std::swap(m_data, other.m_data);
        std::swap(m_front, other.m_front);
        std::swap(m_size, other.m_size);
        std::swap(m_capacity, other.m_capacity);
        std::swap(m_alloc, other.m_alloc);
    }

    /**
     * @brief Возвращает копию аллокатора, используемого очередью.
     *
     * @return ALLOCATOR Копия аллокатора.
     */
    ALLOCATOR get_allocator() const noexcept
    {
        return m_alloc;
    }
};



} // end namespace

#endif // QUEUE_H
