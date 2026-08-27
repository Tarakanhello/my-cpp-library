#ifndef BIT_OPERATIONS_H
#define BIT_OPERATIONS_H

#include <array>
#include <cassert>
#include <concepts>
#include <cstdint>
#include <format>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace mylib
{

    namespace bit
    {

    // ============================================================================
        // Базовые функции – степени двойки и проверка (обобщённые)
        // ============================================================================

        /**
         * @brief Вычисляет 2 в степени x (шаблонная версия, x известен на этапе компиляции).
         *
         * @tparam x Степень двойки, должна быть в диапазоне [0, digits-1].
         * @tparam T Беззнаковый целый тип (выводится автоматически).
         * @return constexpr T 2^x.
         */
        template<int x, typename T = uint64_t>
            requires std::unsigned_integral<T>
        [[nodiscard]] constexpr T twoPower() noexcept
        {
            static_assert(x >= 0 && x < std::numeric_limits<T>::digits,
                          "mylib::bit::twoPower(): x must be in [0, digits-1]");
            return T{1} << x;
        }

        /**
         * @brief Вычисляет 2 в степени x (рантайм-версия).
         *
         * @param x Степень двойки, должна быть в диапазоне [0, digits-1].
         * @tparam T Беззнаковый целый тип.
         * @return constexpr T 2^x.
         * @throws std::out_of_range если x вне допустимого диапазона.
         */
        template<typename T = uint64_t>
            requires std::unsigned_integral<T>
        [[nodiscard]] constexpr T twoPower(int x)
        {
            constexpr int digits = std::numeric_limits<T>::digits;
            if (x < 0 || x >= digits)
            {
                throw std::out_of_range(
                    std::format("mylib::bit::twoPower<T>(int): x({}) must be in [0, {}]", x, digits - 1));
            }
            return T{1} << x;
        }

        /**
         * @brief Проверяет, является ли число степенью двойки.
         *
         * @tparam T Беззнаковый целый тип.
         * @param x Проверяемое число.
         * @return constexpr bool true, если x > 0 и имеет ровно один установленный бит.
         */
        template<typename T = uint64_t>
            requires std::unsigned_integral<T>
        [[nodiscard]] constexpr bool isPowerOfTwo(T x) noexcept
        {
            return (x != 0) && ((x & (x - 1)) == 0);
        }

        template<typename T>
            requires std::signed_integral<T>
        [[nodiscard]] constexpr bool isPowerOfTwo(T x) noexcept
        {
            if(x <= 0)
            {
                return false;
            }

            using U = std::make_unsigned_t<T>;
            return isPowerOfTwo(static_cast<U>(x));
        }


        // ============================================================================
        // Логарифмы по основанию 2 и следующая степень двойки (обобщённые)
        // ============================================================================

        /**
         * @brief Вычисляет целую часть двоичного логарифма (floor(log2(X))) для константы времени компиляции.
         *
         * @tparam X Значение (X > 0).
         * @return constexpr int floor(log2(X)).
         */
        template<auto X>
            requires std::integral<decltype(X)> && (X > 0)
        [[nodiscard]] constexpr int log2floor() noexcept
        {
            using T = std::make_unsigned_t<decltype(X)>;
            T n{ static_cast<T>(X) };
            int result{};
            while (n >>= 1)
                ++result;
            return result;
        }

        /**
         * @brief Вычисляет целую часть двоичного логарифма (floor(log2(x))) для рантайм-значения.
         *
         * Использует встроенные инструкции для 64‑битных типов, иначе — цикл.
         *
         * @tparam T Беззнаковый целый тип.
         * @param x Положительное число (x > 0).
         * @return constexpr int floor(log2(x)).
         * @throws std::out_of_range если x == 0.
         */
        template<typename T = uint64_t>
            requires std::unsigned_integral<T>
        [[nodiscard]] constexpr int log2floor(T x)
        {
            if (x == 0)
                throw std::out_of_range("mylib::bit::log2floor<T>(T): x must be positive");

            constexpr int digits = std::numeric_limits<T>::digits;

#if defined(__GNUC__) || defined(__clang__)
            if constexpr (digits == 64)
                return 63 - __builtin_clzll(static_cast<unsigned long long>(x));
            else if constexpr (digits == 32)
                return 31 - __builtin_clz(static_cast<unsigned int>(x));
            else
#endif
            {
                int result = 0;
                while (x >>= 1)
                    ++result;
                return result;
            }
        }

        template<typename T>
            requires std::signed_integral<T>
        [[nodiscard]] constexpr int log2floor(T x)
        {
            if(x <= 0)
            {
                throw std::out_of_range("mylib::bit::log2floor: x must be positive");
            }

            using U = std::make_unsigned_t<T>;
            return log2floor(static_cast<U>(x));
        }

        /**
         * @brief Вычисляет потолок двоичного логарифма (ceil(log2(X))) для константы.
         *
         * @tparam X Значение (X > 0).
         * @return constexpr int ceil(log2(X)).
         */
        template<auto X>
            requires std::integral<decltype(X)> && (X > 0)
        [[nodiscard]] constexpr int log2ceiling() noexcept
        {
            using T = std::make_unsigned_t<decltype(X)>;
            T n{ static_cast<T>(X) };
            return log2floor<X>() + static_cast<int>(!isPowerOfTwo(n));
        }

        /**
         * @brief Вычисляет потолок двоичного логарифма (ceil(log2(x))) для рантайм-значения.
         *
         * @tparam T Беззнаковый целый тип.
         * @param x Положительное число (x > 0).
         * @return constexpr int ceil(log2(x)).
         * @throws std::out_of_range если x == 0 (пробрасывается из log2floor).
         */
        template<typename T = uint64_t>
            requires std::unsigned_integral<T>
        [[nodiscard]] constexpr int log2ceiling(T x)
        {
            return log2floor(x) + static_cast<int>(!isPowerOfTwo(x));
        }


        template<typename T>
            requires std::signed_integral<T>
        [[nodiscard]] constexpr int log2ceiling(T x)
        {
            if(x <= 0)
            {
                throw std::out_of_range("mylib::bit::log2ceiling: x must be positive");
            }

            using U = std::make_unsigned_t<T>;
            return log2ceiling(static_cast<U>(x));
        }

        /**
         * @brief Возвращает наименьшую степень двойки, не меньшую x.
         *
         * @tparam T Беззнаковый целый тип.
         * @param x Исходное число.
         * @return constexpr T nextPowerOfTwo(x). Для x == 0 возвращает 0.
         */
        template<typename T = uint64_t>
            requires std::unsigned_integral<T>
        [[nodiscard]] constexpr T nextPowerOfTwo(T x) noexcept
        {
            if (x == 0)
                return 0;
            if (isPowerOfTwo(x))
                return x;
            return twoPower<T>(log2floor(x) + 1);
        }


        template<typename T>
            requires std::signed_integral<T>
        [[nodiscard]] constexpr T nextPowerOfTwo(T x)
        {
            if(x < 0)
            {
                throw std::out_of_range("mylib::bit::nextPowerOfTwo: x must be non-negative");
            }
            if(x == 0)
            {
                return 0;
            }

            using U = std::make_unsigned_t<T>;
            return nextPowerOfTwo(static_cast<U>(x));
        }


        // ============================================================================
        // Константы
        // ============================================================================

        /// @brief Константа, представляющая нулевое значение (все биты сброшены)
        constexpr const uint64_t ZERO{ 0 };

        /// @brief Константа, представляющая полностью заполненное слово (все биты установлены).
        constexpr const uint64_t FULL{ ~ZERO };

        /// @brief Возвращает нулевое значение типа T (все биты сброшены).
        template<typename T = uint64_t>
            requires std::unsigned_integral<T>
        [[nodiscard]] constexpr T zero() noexcept
        {
            return T{0};
        }

        /// @brief Возвращает полностью заполненное слово типа T (все биты установлены).
        template<typename T = uint64_t>
            requires std::unsigned_integral<T>
        [[nodiscard]] constexpr T full() noexcept
        {
            return ~T{0};
        }

        // ============================================================================
        // Битовые маски (обобщённые)
        // ============================================================================

        /**
         * @brief Создаёт маску со старшими битами, начиная с позиции n.
         *
         * @tparam T Беззнаковый целый тип.
         * @param n Количество младших бит, которые будут равны 0 (0 … digits).
         * @return constexpr T Маска с единицами в битах [n, digits-1].
         * @throws std::out_of_range если n вне [0, digits].
         */
        template<typename T = uint64_t>
            requires std::unsigned_integral<T>
        [[nodiscard]] constexpr T upperMask(int n)
        {
            constexpr int digits = std::numeric_limits<T>::digits;
            if (n < 0 || n > digits)
                throw std::out_of_range(
                    std::format("mylib::bit::upperMask<T>(int): n({}) must be in [0, {}]", n, digits));

            if (n == digits)
                return T{0};
            if (n == 0)
                return full<T>();
            // Сдвиг на n (n < digits) безопасен
            return full<T>() << n;
        }

        /**
         * @brief Создаёт маску с младшими n битами, установленными в 1.
         *
         * @tparam T Беззнаковый целый тип.
         * @param n Количество младших бит (0 … digits).
         * @return constexpr T Маска с единицами в битах [0, n-1].
         * @throws std::out_of_range если n вне [0, digits].
         */
        template<typename T = uint64_t>
            requires std::unsigned_integral<T>
        [[nodiscard]] constexpr T lowerMask(int n)
        {
            constexpr int digits = std::numeric_limits<T>::digits;
            if (n < 0 || n > digits)
                throw std::out_of_range(
                    std::format("mylib::bit::lowerMask<T>(int): n({}) must be in [0, {}]", n, digits));

            if (n == digits)
                return full<T>();
            if (n == 0)
                return T{0};
            // n < digits, сдвиг безопасен
            return (T{1} << n) - 1;
        }

        /**
         * @brief Создаёт маску для поля длиной n бит, начиная с позиции i.
         *
         * @tparam T Беззнаковый целый тип.
         * @param i Начальная позиция (0 … digits-1).
         * @param n Длина поля в битах (0 … digits).
         * @return constexpr T Маска с единицами в битах [i, i+n-1].
         * @throws std::out_of_range если i или n вне допустимых диапазонов или i + n > digits.
         */
        template<typename T = uint64_t>
            requires std::unsigned_integral<T>
        [[nodiscard]] constexpr T middleMask(int i, int n)
        {
            constexpr int digits = std::numeric_limits<T>::digits;
            if (i < 0 || i >= digits)
                throw std::out_of_range(
                    std::format("mylib::bit::middleMask<T>(int,int): i({}) must be in [0, {}]", i, digits - 1));
            if (n < 0 || n > digits)
                throw std::out_of_range(
                    std::format("mylib::bit::middleMask<T>(int,int): n({}) must be in [0, {}]", n, digits));
            if (i + n > digits)
                throw std::out_of_range(
                    std::format("mylib::bit::middleMask<T>(int,int): i + n ({}) exceeds digits ({})", i + n, digits));

            if (n == 0)
                return T{0};
            // i + n <= digits, значит i < digits и n > 0, сдвиг безопасен
            return lowerMask<T>(n) << i;
        }

        // ============================================================================
        // Операции с отдельными битами (обобщённые)
        // ============================================================================

        /**
         * @brief Получает значение бита с индексом i в числе x.
         *
         * @tparam T Беззнаковый целый тип.
         * @param x Исходное число.
         * @param i Индекс бита (0 … digits-1).
         * @return constexpr bool true, если бит установлен.
         * @throws std::out_of_range если i вне диапазона.
         */
        template<typename T = uint64_t>
            requires std::unsigned_integral<T>
        [[nodiscard]] constexpr bool get(T x, int i)
        {
            constexpr int digits = std::numeric_limits<T>::digits;
            if (i < 0 || i >= digits)
                throw std::out_of_range(
                    std::format("mylib::bit::get<T>(T,int): i({}) must be in [0, {}]", i, digits - 1));
            return (x & twoPower<T>(i)) != 0;
        }

        /**
         * @brief Инвертирует бит с индексом i в числе x и возвращает новое число.
         *
         * @tparam T Беззнаковый целый тип.
         * @param x Исходное число.
         * @param i Индекс бита (0 … digits-1).
         * @return constexpr T Новое число с инвертированным битом.
         * @throws std::out_of_range если i вне диапазона.
         */
        template<typename T = uint64_t>
            requires std::unsigned_integral<T>
        [[nodiscard]] constexpr T flip(T x, int i)
        {
            constexpr int digits = std::numeric_limits<T>::digits;
            if (i < 0 || i >= digits)
                throw std::out_of_range(
                    std::format("mylib::bit::flip<T>(T,int): i({}) must be in [0, {}]", i, digits - 1));
            return x ^ twoPower<T>(i);
        }

        /**
         * @brief Устанавливает бит с индексом i в число x в заданное значение (0 или 1).
         *
         * @tparam T Беззнаковый целый тип.
         * @param x Ссылка на изменяемое число.
         * @param i Индекс бита (0 … digits-1).
         * @param value Устанавливаемое значение (true = 1, false = 0).
         * @throws std::out_of_range если i вне диапазона.
         */
        template<typename T = uint64_t>
            requires std::unsigned_integral<T>
        constexpr void set(T& x, int i, bool value)
        {
            constexpr int digits = std::numeric_limits<T>::digits;
            if (i < 0 || i >= digits)
                throw std::out_of_range(
                    std::format("mylib::bit::set<T>(T&,int,bool): i({}) must be in [0, {}]", i, digits - 1));

            T mask = twoPower<T>(i);
            x = value ? (x | mask) : (x & ~mask);
        }

        // ============================================================================
        // Битовые поля (извлечение и запись) – обобщённые
        // ============================================================================


        /**
         * @brief Извлекает битовое поле длиной n из числа x, начиная с позиции i.
         *
         * @tparam T Беззнаковый целый тип.
         * @param x Исходное число.
         * @param i Начальная позиция (0 … digits-1).
         * @param n Длина поля (1 … digits).
         * @return constexpr T Значение поля (в младших n битах результата).
         * @throws std::out_of_range если i или n вне допустимых диапазонов или i + n > digits.
         */
        template<typename T = uint64_t>
            requires std::unsigned_integral<T>
        [[nodiscard]] constexpr T getValue(T x, int i, int n)
        {
            constexpr int digits = std::numeric_limits<T>::digits;
            if (i < 0 || i >= digits)
                throw std::out_of_range(
                    std::format("mylib::bit::getValue<T>(T,int,int): i({}) must be in [0, {}]", i, digits - 1));
            if (n < 1 || n > digits)
                throw std::out_of_range(
                    std::format("mylib::bit::getValue<T>(T,int,int): n({}) must be in [1, {}]", n, digits));
            if (i + n > digits)
                throw std::out_of_range(
                    std::format("mylib::bit::getValue<T>(T,int,int): i + n ({}) exceeds digits ({})", i + n, digits));

            return (x >> i) & lowerMask<T>(n);
        }

        /**
         * @brief Записывает младшие n бит значения value в поле числа x, начиная с позиции i.
         *
         * @tparam T Тип изменяемого числа (беззнаковый).
         * @tparam Z Тип значения (беззнаковый, может отличаться от T).
         * @param x Ссылка на изменяемое число.
         * @param value Значение для записи (используются только младшие n бит).
         * @param i Начальная позиция (0 … digits-1).
         * @param n Длина поля (0 … digits). По умолчанию – все биты Z.
         * @throws std::out_of_range если i или n вне допустимых диапазонов, или i + n > digits.
         */
        template<typename T = uint64_t, typename Z = uint64_t>
            requires std::unsigned_integral<T> && std::unsigned_integral<Z>
        constexpr void setValue(T& x, Z value, int i = 0, int n = std::numeric_limits<Z>::digits)
        {
            constexpr int digits = std::numeric_limits<T>::digits;
            if (i < 0 || i >= digits)
                throw std::out_of_range(
                    std::format("mylib::bit::setValue<T,Z>(T&,Z,int,int): i({}) must be in [0, {}]", i, digits - 1));
            if (n < 0 || n > digits)
                throw std::out_of_range(
                    std::format("mylib::bit::setValue<T,Z>(T&,Z,int,int): n({}) must be in [0, {}]", n, digits));
            if (i + n > digits)
                throw std::out_of_range(
                    std::format("mylib::bit::setValue<T,Z>(T&,Z,int,int): i + n ({}) exceeds digits ({})", i + n, digits));

            if (n == 0)
                return; // ничего не делаем

            T mask = middleMask<T>(i, n);
            x = (x & ~mask) | ( (static_cast<T>(value) << i) & mask );
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
        // Инвертирование бит (reverse) – обобщённое
        // ============================================================================

        /**
         * @brief Инвертирует порядок всех бит в слове T.
         *
         * @tparam T Беззнаковый целый тип.
         * @param x Исходное число.
         * @return constexpr T Число с битами в обратном порядке.
         */
        template<typename T = uint64_t>
            requires std::unsigned_integral<T>
        constexpr T reverseAllBits(T x)
        {
            ReverseBits8 r8;
            T result = 0;
            for (size_t i = 0; i < sizeof(T); ++i, x >>= 8)
            {
                result = (result << 8) | r8(static_cast<uint8_t>(x));
            }
            return result;
        }

        /**
         * @brief Инвертирует порядок только младших n бит числа x.
         *
         * @tparam T Беззнаковый целый тип.
         * @param x Исходное число.
         * @param n Количество младших бит для инвертирования (0 … digits).
         * @return constexpr T Число, у которого младшие n бит инвертированы, старшие биты обнулены.
         */
        template<typename T = uint64_t>
            requires std::unsigned_integral<T>
        constexpr T reverseBits(T x, int n = std::numeric_limits<T>::digits)
        {
            constexpr int digits = std::numeric_limits<T>::digits;
            assert(n >= 0 && n <= digits);

            if (n == 0)
                return x;

            const int shift = digits - n;
            T mask = lowerMask<T>(n);
            T reversed = reverseAllBits(static_cast<T>(x & mask)) >> shift;
            return (x & ~mask) | reversed;
        }


        // ============================================================================
        // Подсчёт единичных бит (popcount)
        // ============================================================================

        /**
         * @brief Вычисляет количество установленных бит (popcount) в беззнаковом числе произвольной разрядности.
         *
         * @tparam T Беззнаковый целый тип.
         * @param x Исходное число.
         * @return constexpr int Количество единичных бит.
         */
        template<typename T = uint64_t>
            requires std::unsigned_integral<T>
        constexpr int popcount(T x)
        {
            PopCount8 p8;
            int result = 0;
            while (x)
            {
                result += p8(static_cast<uint8_t>(x));
                x >>= 8;
            }
            return result;
        }

        /**
         * @brief Вычисляет количество младших нулевых бит в числе T.
         *
         * Использует выражение popcount(~x & (x-1)), которое даёт маску всех младших нулей.
         *
         * @tparam T Беззнаковый целый тип.
         * @param x Исходное число.
         * @return constexpr int Количество подряд идущих нулевых бит, начиная с младшего.
         * @note Для x == 0 возвращает digits (все биты нулевые).
         */
        template<typename T = uint64_t>
            requires std::unsigned_integral<T>
        [[nodiscard]] constexpr int rightmostNullCount(T x)
        {
            constexpr int digits = std::numeric_limits<T>::digits;
            if (x == 0)
                return digits;
            return popcount(~x & (x - 1));
        }

        template<typename T>
            requires std::signed_integral<T>
        [[nodiscard]] constexpr int rightmostNullCount(T x)
        {
            if(x < 0)
            {
                throw std::out_of_range("mylib::bit::rightmostNullCount: x must be non-negative");
            }
            using U = std::make_unsigned_t<T>;
            return rightmostNullCount(static_cast<U>(x));
        }
    } // end bit namespace

} // end mylib namespace
#endif // BIT_OPERATIONS_H
