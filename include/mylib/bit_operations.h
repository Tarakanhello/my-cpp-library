#ifndef BIT_OPERATIONS_H
#define BIT_OPERATIONS_H

#include <array>
#include <bit>
#include <cstdint>
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
            return x & twoPower(i);
        }

        /**
         * @brief Инвертирует бит с индексом i в числе x и возвращает новое число.
         *
         * @param x Исходное число.
         * @param i Индекс бита (0 … 63).
         * @return uint64_t Новое число с инвертированным битом.
         * @throws std::out_of_range если i вне допустимого диапазона (пробрасывается из twoPower).
         */
        [[nodiscard]] uint64_t flip(uint64_t x, int i)
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
         * @param n Количество младших бит, которые будут равны 0 (0 … 63).
         * @return constexpr uint64_t Маска с единицами в битах [n, 63].
         * @throws std::out_of_range если n вне диапазона [0, 63].
         */
        [[nodiscard]] constexpr uint64_t upperMask(int n = 0)
        {
            if(n < 0 || n >= 64)
            {
                throw std::out_of_range("mylib::bit::upperMask(int): n must be in [0, 63]");
            }

            return FULL << n;
        }

        /**
         * @brief Создаёт маску с младшими n битами, установленными в 1.
         *
         * @param n Количество младших бит (0 … 63).
         * @return constexpr uint64_t Маска с единицами в битах [0, n-1].
         * @throws std::out_of_range если n вне диапазона [0, 63].
         */
        [[nodiscard]] constexpr uint64_t lowerMask(int n = 0)
        {
            if(n < 0 || n >= 64)
            {
                throw std::out_of_range("mylib::bit::lowerMask(int): n must be in [0, 63]");
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



        class PopCount8 final
        {
        private:
            static inline constexpr std::array<uint8_t, 256> table = []() constexpr
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
            constexpr int operator()(uint8_t x) const { return table[x]; }
        };

        constexpr int rightmostNullCount(uint64_t x)
        {
            return std::popcount(~x & (x - 1));
        }

    } // end bit namespace

} // end mylib namespace
#endif // BIT_OPERATIONS_H
