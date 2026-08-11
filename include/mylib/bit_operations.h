#ifndef BIT_OPERATIONS_H
#define BIT_OPERATIONS_H

#include <array>
#include <cassert>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace mylib
{

    namespace bit
    {

    // ============================================================================
    // Базовые функции – степени двойки и проверка
    // ============================================================================

        /**
         * @brief Вычисляет 2 в степени x (шаблонная версия, x известен на этапе компиляции).
         *
         * @tparam x Степень двойки, должна быть в диапазоне [0, 63].
         * @return constexpr uint64_t 2^x.
         */
        template<int x>
        [[nodiscard]] constexpr uint64_t twoPower() noexcept
        {
            static_assert(x >= 0 && x < 64, "mylib::bit::twoPower(): x must be in [0, 63]");
            return uint64_t{ 1 } << x;
        }

        /**
         * @brief Вычисляет 2 в степени x (рантайм-версия).
         *
         * @param x Степень двойки, должна быть в диапазоне [0, 63].
         * @return constexpr uint64_t 2^x.
         * @throws std::out_of_range если x вне допустимого диапазона.
         */
        [[nodiscard]] constexpr uint64_t twoPower(int x)
        {
            if(x < 0 || x >= 64)
            {
                throw std::out_of_range("mylib::bit::twoPower(int): x must be in [0, 63]");
            }

            return uint64_t{ 1 } << x;
        }

        /**
         * @brief Проверяет, является ли число степенью двойки.
         *
         * @param x Проверяемое число (беззнаковое 64-битное).
         * @return constexpr bool true, если x является степенью двойки (x > 0 и имеет ровно один установленный бит).
         */
        [[nodiscard]] constexpr bool isPowerOfTwo(uint64_t x) noexcept
        {
            return (x !=0 ) && ((x & (x - 1)) == 0);
        }

        // ============================================================================
        // Логарифмы по основанию 2 и следующая степень двойки
        // ============================================================================

        /**
         * @brief Вычисляет целую часть двоичного логарифма (floor(log2(x))) для константы времени компиляции.
         *
         * @tparam X Положительное число (X > 0).
         * @return constexpr int floor(log2(X)).
         * @note Для X == 0 срабатывает static_assert.
         */
        template<uint64_t X>
        [[nodiscard]] constexpr int log2floor() noexcept
        {
            static_assert(X != 0, "mylib::bit::log2floor(): X must be positive");
            uint64_t n{ X };
            int result{ 0 };
            while(n >>= 1)
            {
                ++result;
            }

            return result;
        }

        /**
         * @brief Вычисляет целую часть двоичного логарифма (floor(log2(x))) для рантайм-значения.
         *
         * Использует встроенные инструкции процессора (GCC/Clang: __builtin_clzll, MSVC: _BitScanReverse64)
         * или запасной цикл.
         *
         * @param x Положительное число (x > 0).
         * @return constexpr int floor(log2(x)).
         * @throws std::out_of_range если x == 0.
         */
        [[nodiscard]] constexpr int log2floor(uint64_t x)
        {
            if(0 == x)
            {
                throw std::out_of_range("mylib::bit:log2floor(uint64_t): x must be positive");
            }
#if defined(__GNUC__) || defined(__clang__)
            return 63 - __builtin_clzll(x);
#elif defined(_MSC_VER)
            unsigned long index;
            _BitScanReverse64(&index, x);
            return static_cast<int>(index);
#else
            int result{ 0 };
            while(x >>= 1)
            {
                ++result;
            }

            return result;
#endif
        }

        /**
         * @brief Вычисляет потолок двоичного логарифма (ceil(log2(X))) для константы времени компиляции.
         *
         * @tparam X Положительное число (X > 0).
         * @return constexpr int ceil(log2(X)).
         */
        template<uint64_t X>
        [[nodiscard]] constexpr int log2ceiling() noexcept
        {
            return log2floor<X>() + static_cast<int>(!isPowerOfTwo(X));
        }


        /**
         * @brief Вычисляет потолок двоичного логарифма (ceil(log2(x))) для рантайм-значения.
         *
         * @param x Положительное число (x > 0).
         * @return constexpr int ceil(log2(x)).
         * @throws std::out_of_range если x == 0 (пробрасывается из log2floor).
         */
        [[nodiscard]] constexpr int log2ceiling(uint64_t x)
        {
            return log2floor(x) + static_cast<int>(!isPowerOfTwo(x));
        }

        /**
         * @brief Возвращает наименьшую степень двойки, не меньшую x.
         *
         * @param x Исходное число.
         * @return constexpr uint64_t nextPowerOfTwo(x). Для x == 0 возвращает 0.
         */
        [[nodiscard]] constexpr uint64_t nextPowerOfTwo(uint64_t x) noexcept
        {
            if(x == 0)
            {
                return 0;
            }

            return isPowerOfTwo(x) ? x : twoPower(log2floor(x) + 1);
        }


        // ============================================================================
        // Константы
        // ============================================================================

        /// @brief Константа, представляющая нулевое значение (все биты сброшены)
        constexpr const uint64_t ZERO{ 0 };

        /// @brief Константа, представляющая полностью заполненное слово (все биты установлены).
        constexpr const uint64_t FULL{ ~ZERO };

        // ============================================================================
        // Блок 4: Операции с отдельными битами
        // ============================================================================

        /**
         * @brief Получает значение бита с индексом i в числе x.
         *
         * @param x Исходное число.
         * @param i Индекс бита (0 … 63).
         * @return constexpr bool true, если бит установлен, иначе false.
         * @throws std::out_of_range если i вне допустимого диапазона (пробрасывается из twoPower).
         */
        [[nodiscard]] constexpr bool get(uint64_t x, int i)
        {
            return (x & twoPower(i)) != 0;
        }

        /**
         * @brief Инвертирует бит с индексом i в числе x и возвращает новое число.
         *
         * @param x Исходное число.
         * @param i Индекс бита (0 … 63).
         * @return uint64_t Новое число с инвертированным битом.
         * @throws std::out_of_range если i вне допустимого диапазона (пробрасывается из twoPower).
         */
        [[nodiscard]] constexpr uint64_t flip(uint64_t x, int i)
        {
            return x ^ twoPower(i);
        }

        /**
         * @brief Устанавливает бит с индексом i в число x в заданное значение (0 или 1).
         *
         * @tparam T Тип числа, должен быть беззнаковым целым.
         * @param x Ссылка на изменяемое число.
         * @param i Индекс бита (0 … sizeof(T)*8 - 1).
         * @param value Устанавливаемое значение (true = 1, false = 0).
         * @throws std::out_of_range если i выходит за допустимые границы для типа T.
         */
        template<typename T>
            requires std::is_unsigned_v<T>
        constexpr void set(T& x, int i, bool value)
        {
            if(i < 0 || i >= static_cast<int>(sizeof(T) * 8))
            {
                throw std::out_of_range("mylib::bit::set(T, int, bool): index out of range");
            }

            T mask{ static_cast<T>(twoPower(i)) };

            x = (value ? (x | mask) : (x & (~mask)));
        }


        // ============================================================================
        // Битовые поля (маски и работа с группами бит)
        // ============================================================================

        /**
         * @brief Создаёт маску со старшими битами, начиная с позиции n.
         *
         * @param n Количество младших бит, которые будут равны 0 (0 … 64).
         * @return constexpr uint64_t Маска с единицами в битах [n, 64].
         * @throws std::out_of_range если n вне диапазона [0, 64].
         */
        [[nodiscard]] constexpr uint64_t upperMask(int n = 0)
        {
            if(n < 0 || n > 64)
            {
                throw std::out_of_range("mylib::bit::upperMask(int): n must be in [0, 64]");
            }
            if(n == 64)
            {
                return ZERO;
            }

            return FULL << n;
        }

        /**
         * @brief Создаёт маску с младшими n битами, установленными в 1.
         *
         * @param n Количество младших бит (0 … 64).
         * @return constexpr uint64_t Маска с единицами в битах [0, n-1].
         * @throws std::out_of_range если n вне диапазона [0, 64].
         */
        [[nodiscard]] constexpr uint64_t lowerMask(int n = 0)
        {
            if(n < 0 || n > 64)
            {
                throw std::out_of_range("mylib::bit::lowerMask(int): n must be in [0, 64]");
            }
            return ~upperMask(n);
        }

        /**
         * @brief Создаёт маску для поля длиной n бит, начиная с позиции i.
         *
         * @param i Начальная позиция (0 … 63).
         * @param n Длина поля в битах (0 … 63).
         * @return constexpr uint64_t Маска с единицами в битах [i, i+n-1].
         * @throws std::out_of_range если i или n вне [0, 63] или i + n > 64.
         */
        [[nodiscard]] constexpr uint64_t middleMask(int i, int n)
        {
            if((n < 0 || n >= 64) || (i < 0 || i >=64))
            {
                throw std::out_of_range("mylib::bit::middleMask(int, int): i and n must be in [0, 63]");
            }
            if (i + n > 64)
            {
                throw std::out_of_range("mylib::bit::middleMask(int, int): i + n exceeds 64");
            }
            return lowerMask(n) << i;
        }

        /**
         * @brief Извлекает битовое поле длиной n из числа x, начиная с позиции i.
         *
         * @param x Исходное число.
         * @param i Начальная позиция (0 … 63).
         * @param n Длина поля (0 … 63).
         * @return constexpr uint64_t Значение поля (в младших n битах результата).
         * @throws std::out_of_range если i или n вне [0, 63] или i + n > 64.
         */
        [[nodiscard]] constexpr uint64_t getValue(uint64_t x, int i, int n)
        {
            if((n < 0 || n >= 64) || (i < 0 || i >= 64))
            {
                throw std::out_of_range("mylib::bit::getValue(uint64_t, int, int): x and i must be in [0, 63]");
            }
            if (i + n > 64)
            {
                throw std::out_of_range("mylib::bit::getValue(uint64_t, int, int): i + n exceeds 64");
            }
            return (x >> i) & lowerMask(n);
        }

        /**
         * @brief Записывает младшие n бит значения value в поле числа x, начиная с позиции i.
         *
         * @tparam T Беззнаковый целый тип.
         * @param x Ссылка на изменяемое число.
         * @param value Значение для записи (используются только младшие n бит).
         * @param i Начальная позиция (0 … sizeof(T)*8 - 1).
         * @param n Длина поля (0 … sizeof(T)*8 - 1).
         * @throws std::out_of_range если i или n вне допустимого диапазона, или i + n > sizeof(T)*8.
         */
        template<typename T>
            requires std::is_unsigned_v<T>
        void setValue(T& x, T value, int i, int n)
        {
            const int bits{ static_cast<int>(sizeof(T)) * 8 };
            if((n < 0 || n >= bits) || (i < 0 || i >= bits))
            {
                throw std::out_of_range("mylib::bit::setValue(T, T, int, int): i and n must be in [0, bits-1]");
            }
            if (i + n > bits)
            {
                throw std::out_of_range("mylib::bit::setValue(T, T, int, int): i + n exceeds type width");
            }

            T mask{ static_cast<T>(middleMask(i, n)) };

            x &= ~mask;  // удаление
            x |= mask & (value << i); // установка
        }


        // ============================================================================
        // Классы для табличного подсчёта и инвертирования
        // ============================================================================

        /**
         * @brief Класс для быстрого подсчёта количества установленных бит (popcount) в 8‑битном числе.
         *
         * Использует предварительно вычисленную таблицу на 256 элементов, заполняемую на этапе компиляции.
         */
        class PopCount8 final
        {
        private:
            static inline constexpr std::array<uint8_t, 256> m_table = []() constexpr
            {
                std::array<uint8_t, 256> arr{};
                for(size_t i{}; i < arr.max_size(); ++i)
                {
                    uint8_t x{ static_cast<uint8_t>(i) };
                    uint8_t count{};

                    while(x)
                    {
                        count += (x & 1);
                        x >>= 1;
                    }

                    arr[i] = count;
                }

                return arr;
            }();

        public:
            /**
             * @brief Возвращает количество единичных бит в байте.
             * @param x 8‑битное беззнаковое число.
             * @return int число установленных бит (0 … 8).
             */
            constexpr int operator()(uint8_t x) const { return m_table[x]; }
        };


        /**
         * @brief Класс для быстрого инвертирования порядка бит (bit reversal) в 8‑битном числе.
         *
         * Таблица из 256 элементов заполняется на этапе компиляции.
         */
        class ReverseBits8 final
        {
        private:
            static inline constexpr std::array<uint8_t, 256> m_table = []() constexpr
            {
                std::array<uint8_t, 256> arr{};

                for(size_t i{}; i < arr.size(); ++i)
                {
                    uint8_t result{};
                    uint8_t x{ static_cast<uint8_t>(i) };

                    for(size_t j{}; j < 8; ++j)
                    {
                        result = (result << 1) + (x & 1);
                        x >>= 1;
                    }

                    arr[i] = result;
                }

                return arr;
            }();

        public:
            /**
             * @brief Возвращает байт с битами, записанными в обратном порядке.
             * @param x 8‑битное беззнаковое число.
             * @return uint8_t инвертированное значение.
             */
            constexpr uint8_t operator()(uint8_t x) const { return m_table[x]; }
        };

        // ============================================================================
        // Инвертирование бит (reverse)
        // ============================================================================

        /**
         * @brief Инвертирует порядок всех бит в слове T.
         *
         * @tparam T Беззнаковый целый тип.
         * @param x Исходное число.
         * @return constexpr T Число с битами, расположенными в обратном порядке.
         */
        template<typename T>
            requires std::is_unsigned_v<T>
        constexpr T reverseAllBits(T x)
        {
            ReverseBits8 r8;

            T result{};
            for(size_t i{}; i < sizeof(x); ++i, x >>= 8)
            {
                result = (result << 8) + r8(x);
            }

            return result;
        }

        /**
         * @brief Инвертирует порядок только младших n бит числа x.
         *
         * @tparam T Беззнаковый целый тип.
         * @param x Исходное число.
         * @param n Количество младших бит для инвертирования (по умолчанию все биты).
         * @return constexpr T Число, у которого младшие n бит инвертированы, старшие биты обнулены.
         * @note Если n == digits, функция эквивалентна reverseAllBits(x).
         * @warning Для корректного использования старшие биты результата обнуляются (сдвиг вправо).
         */
        template<typename T>
            requires std::is_unsigned_v<T>
        constexpr T reverseBits(T x, int n = std::numeric_limits<T>::digits)
        {
            assert(n >= 0 && n <= std::numeric_limits<T>::digits);

            if(n == 0)
            {
                return x;
            }


            const int shift{ std::numeric_limits<T>::digits - n };
            const T mask{ static_cast<T>(lowerMask(n)) };
            const T reversed{ static_cast<T>(reverseAllBits(static_cast<T>(x & mask)) >> shift) };

            return (x & ~mask) | reversed;
        }

        // ============================================================================
        // Подсчёт единичных бит (popcount)
        // ============================================================================

        /**
         * @brief Вычисляет количество установленных бит (popcount) в беззнаковом числе произвольной разрядности.
         *
         * Использует таблицу PopCount8 для каждого байта числа.
         *
         * @tparam T Беззнаковый целый тип.
         * @param x Исходное число.
         * @return constexpr int Количество единичных бит.
         */
        template<typename T>
            requires std::is_unsigned_v<T>
        constexpr int popcount(T x)
        {
            PopCount8 p8;
            int result{ 0 };
            for(; x; x>>= 8)
            {
                result += p8(static_cast<uint8_t>(x));
            }

            return result;
        }

        /**
         * @brief Вычисляет количество младших нулевых бит в 64‑битном числе.
         *
         * Использует выражение popcount(~x & (x-1)), которое даёт маску всех младших нулей.
         *
         * @param x 64‑битное беззнаковое число.
         * @return constexpr int Количество подряд идущих нулевых бит, начиная с младшего.
         * @note Для x == 0 возвращает 64, так как все биты нулевые.
         */
        constexpr int rightmostNullCount(uint64_t x)
        {
            return popcount(~x & (x - 1));
        }

    } // end bit namespace

} // end mylib namespace
#endif // BIT_OPERATIONS_H
