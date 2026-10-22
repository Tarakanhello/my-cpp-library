#ifndef RANDOM_H
#define RANDOM_H

#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <random>

#include "concepts.h"
#include "mylib/bit_operations.h"

namespace mylib
{

namespace random
{

    /**
     * @brief A high-quality 64-bit pseudo-random number generator (xorshift64*).
     *
     * The generator maintains a single 64-bit state word and advances it with the
     * classic "xorshift64*" transformation: three xor-shift steps followed by a
     * multiplication by an odd 64-bit constant. This produces a sequence with the
     * following properties:
     *
     *   - Full period 2^64 - 1 (zero is the only state that would loop forever,
     *     and it is excluded by construction — see the constructor).
     *   - Passes standard statistical test batteries (BigCrush, PractRand up to
     *     reasonable sample sizes) for non-cryptographic use.
     *   - Very fast: about one multiply and three shifts per output.
     *   - Deterministic: the same seed always produces the same sequence on every
     *     platform (the arithmetic is fixed-width and unsigned).
     *
     * @note NOT cryptographically secure. Do not use for key material,
     *       authentication tokens, or anything else where predictability matters.
     *       Use `std::random_device` / a CSPRNG for those purposes.
     *
     * @note Not thread-safe: concurrent calls to operator() / uniform01() race on
     *       m_state. Give each thread its own instance.
     *
     * The class satisfies the C++ *UniformRandomBitGenerator* requirements
     * (`result_type`, `min()`, `max()`, `operator()`), so it can be plugged
     * directly into `<random>` distributions:
     *
     * @code
     * mylib::QualityXorshift64 rng{ 42 };
     * std::uniform_int_distribution<int> dist{ 1, 6 };
     * int die{ dist(rng) };   // works as-is
     * @endcode
     *
     * @see operator()
     * @see uniform01
     */
    class QualityXorshift64
    {
    public:
        /**
             * @brief Type of the values produced by the generator.
             *
             * Required by the UniformRandomBitGenerator concept. Every call to
             * operator() returns a value of this type.
             */
        using result_type = uint64_t;

    private:
        /**
             * @brief Internal generator state.
             *
             * Invariant: `m_state != 0`. A zero state is absorbing — the xorshift
             * steps and the multiply would keep it at zero forever, so the generator
             * would only ever return 0. The constructor guarantees this by
             * substituting PASSWORD for a zero seed.
             */
        result_type m_state;

        /**
             * @brief Constant mixed into the default seed and used to avoid the zero state.
             *
             * Doubles as:
             *   - a "salt" for the default (clock-based) seed, so that two processes
             *     started in the same millisecond do not produce identical sequences;
             *   - a fallback value if the caller explicitly passes seed == 0, since a
             *     zero state would make the generator degenerate (see m_state).
             *
             * @note Any non-zero value would work; the specific number is arbitrary.
             */
        static constexpr result_type PASSWORD{ 19870804ull };

        /**
             * @brief The core xorshift64* step: mixes a 64-bit value.
             *
             * Applies three xor-shift operations (left 21, right 35, left 4) followed
             * by a multiplication by the odd constant 2685821657736338717.
             *
             * @param x Input value (usually the current state).
             * @return The mixed value.
             *
             * @note The whole mapping is a bijection on `uint64_t`:
             *       - each `x ^= x << k` (k > 0) is invertible;
             *       - each `x ^= x >> k` (k > 0) is invertible;
             *       - multiplication by an odd constant is invertible modulo 2^64.
             *       Bijectivity is what guarantees the full 2^64 - 1 period on the
             *       non-zero state subspace.
             * @note `static constexpr` so the mixing function can also be used
             *       internally in constant expressions.
             * @note `noexcept`: pure unsigned arithmetic.
             */
        static constexpr result_type transform(result_type x) noexcept
        {
            x ^= x << 21;
            x ^= x >> 35;
            x ^= x << 4;

            return x * 2685821657736338717ull;
        }

        /**
             * @brief Advances the generator and returns the next pseudo-random value.
             *
             * Equivalent to `m_state = transform(m_state); return m_state;`.
             *
             * @return The next 64-bit output (uniformly distributed over the full
             *         `uint64_t` range).
             *
             * @note The returned value is the *new* state, i.e. it is fully mixed and
             *       safe to use directly as an output (this is what the `*` in
             *       xorshift64* denotes — the scrambled state is the output).
             * @note Each call mutates the generator; two consecutive calls return
             *       different values (for a non-degenerate state).
             * @note `noexcept`: pure unsigned arithmetic.
             *
             * @see operator()
             * @see uniform01
             */
        result_type next() noexcept { return m_state = transform(m_state); }

    public:
        /**
             * @brief Constructs a generator from a seed.
             *
             * @param seed Initial state. Defaults to the current wall-clock time in
             *             milliseconds, XOR-ed with PASSWORD. If `seed == 0`, PASSWORD
             *             is used instead so that the state is never zero (see
             *             m_state for the invariant).
             *
             * @note The default seed is intentionally non-deterministic (clock-based).
             *       For reproducible sequences, pass an explicit seed.
             * @note The default argument is evaluated at the call site, so each
             *       default-constructed instance gets its own timestamp-based seed.
             */
        explicit QualityXorshift64(result_type seed = std::chrono::duration_cast<std::chrono::milliseconds>(
                                                          std::chrono::system_clock::now().time_since_epoch()).count() ^ PASSWORD)
            : m_state{ seed ? seed : PASSWORD }
        {}

        /**
             * @brief Lower bound of the generated range (inclusive).
             *
             * Required by the UniformRandomBitGenerator concept. Outputs span the
             * entire `uint64_t` range, so this is 0.
             *
             * @return 0.
             * @note `static constexpr` and `noexcept`.
             */
        static constexpr result_type min() noexcept { return 0; }

        /**
             * @brief Upper bound of the generated range (inclusive).
             *
             * Required by the UniformRandomBitGenerator concept. Since outputs are
             * full-range `uint64_t` values, this is `UINT64_MAX`.
             *
             * @return `std::numeric_limits<result_type>::max()`.
             * @note `static constexpr` and `noexcept`.
             */
        static constexpr result_type max() noexcept { return std::numeric_limits<result_type>::max(); }

        /**
             * @brief Function-call operator, advancing the generator.
             *
             * Provided so the class satisfies the *UniformRandomBitGenerator*
             * concept and can be passed directly to `<random>` distributions
             * (e.g. `std::uniform_int_distribution`, `std::normal_distribution`).
             *
             * @return The next pseudo-random 64-bit value.
             *
             * @note Equivalent to next().
             * @note `noexcept`.
             *
             * @see next
             */
        result_type operator()() noexcept
        {
            return next();
        }

        /**
             * @brief Generates a uniformly distributed `double` in the half-open
             *        interval [0, 1).
             *
             * Converts the next 64-bit output to a double and divides by 2^64
             * (computed as `std::ldexp(1.0, 64)` so that no intermediate overflow
             * occurs and the exact power of two is used).
             *
             * @return A `double` in [0, 1). The value 1.0 is never produced because
             *         the numerator is at most 2^64 - 1.
             *
             * @note Uniform in the sense of next() being uniform over `uint64_t`;
             *       `double` has 53 mantissa bits, so the low bits are truncated —
             *       the result is uniform over the representable grid, not over all
             *       real numbers.
             * @note Advances the generator exactly once.
             * @note `noexcept`: no allocation, no exceptions.
             *
             * @see next
             */
        double uniform01() noexcept { return static_cast<double>(next()) / std::ldexp(1.0, 64); }

    }; // end QualityXorshift64


    /**
     * @brief A template-based Mersenne Twister pseudo-random number generator.
     *
     * This class implements the Mersenne Twister algorithm, which is the de facto
     * standard for non-cryptographic PRNGs. It is templated to support both 32-bit
     * (equivalent to std::mt19937) and 64-bit (equivalent to std::mt19937_64) variants.
     *
     * Key properties:
     *   - Extremely long period (2^19937 - 1 for 64-bit, 2^19937 - 1 for 32-bit).
     *   - Passes stringent statistical randomness tests (e.g., BigCrush).
     *   - Requires a large internal state (312 or 624 words).
     *
     * @tparam UIntType The underlying unsigned integer type (uint32_t or uint64_t).
     *
     * @note NOT cryptographically secure.
     * @note Not thread-safe: concurrent calls to operator() race on m_index and
     *       m_state. Give each thread its own instance.
     */
    template<UInt_32_or_64 UIntType = uint64_t>
    class MersenneTwister final
    {
    public:
        /**
         * @brief Type of the values produced by the generator.
         * Required by the UniformRandomBitGenerator concept.
         */
        using result_type = UIntType;

        /**
         * @brief Lower bound of the generated range (inclusive).
         * @return 0.
         */
        static constexpr result_type min() noexcept { return 0; }

        /**
         * @brief Upper bound of the generated range (inclusive).
         * @return std::numeric_limits<result_type>::max().
         */
        static constexpr result_type max() noexcept { return std::numeric_limits<result_type>::max(); }

    private:
        // === 1. Параметры структуры ===
        /// @brief Размер машинного слова в битах (32 или 64).
        static constexpr size_t bitWidth            { sizeof(result_type) * 8 };

        /// @brief Количество слов в массиве состояния (624 для 32-бит, 312 для 64-бит).
        static constexpr size_t stateSize           { bitWidth == 64 ? 312 : 624};

        /// @brief Смещение для операции "закручивания" (Twist).
        static constexpr size_t middleWordOffset    { bitWidth == 64  ? 156 : 397 };

        /// @brief Точка разделения для склеивания старших и младших битов.
        static constexpr size_t separationPoint     { 31 };

        // === 2. Параметры закручивания (Twisting) ===
        /// @brief Магическая константа (матрица A) для XOR-операции при закручивании.
        static constexpr result_type matrixA        { bitWidth == 64 ? 0xB5026F5AA96619E9ULL : 0x9908B0DFULL };

        // === 3. Параметры темперирования (Tempering) ===
        /// @brief Параметры сдвигов и масок для финального перемешивания (темперирования).
        static constexpr result_type temperShiftU   { bitWidth == 64 ? 29 : 11 };
        static constexpr result_type temperMaskD    { bitWidth == 64 ? 0x5555555555555555ULL : 0xFFFFFFFFULL };
        static constexpr result_type temperShiftS   { bitWidth == 64 ? 17 : 7 };
        static constexpr result_type temperMaskB    { bitWidth == 64 ? 0x71D67FFFEDA60000ULL : 0x9D2C5680ULL };
        static constexpr result_type temperShiftT   { bitWidth == 64 ? 37 : 15 };
        static constexpr result_type temperMaskC    { bitWidth == 64 ? 0xFFF7EEE000000000ULL : 0xEFC60000ULL };
        static constexpr result_type temperShiftL   { bitWidth == 64 ? 43 : 18 };

        // === 4. Параметр инициализации ===
        /// @brief Множитель для генерации начального состояния из одного числа (seed).
        static constexpr result_type initMultiplier { bitWidth == 64 ? 6364136223846793005ULL : 1812433253ULL };

        /// @brief Внутреннее состояние генератора (массив слов).
        std::array<result_type, stateSize> m_state;

        /// @brief Текущий индекс в массиве m_state. Когда достигает stateSize, вызывается twist().
        std::size_t m_index;

        /**
         * @brief Инициализирует внутреннее состояние с использованием одного числа (seed).
         *
         * Использует линейную конгруэнтную рекуррентную формулу (LCG) для заполнения
         * всего массива m_state на основе одного начального значения.
         *
         * @param seed Начальное число.
         */
        void initBySeed(result_type seed) noexcept
        {
            m_state[0] = seed;
            for(size_t i{ 1 }; i < stateSize; ++i)
            {
                m_state[i] = initMultiplier * (m_state[i - 1] ^ (m_state[i - 1] >> (bitWidth - 2))) + i;
            }

            m_index = stateSize; // Заставляем генератор сделать twist() при первом вызове
        }

        /**
         * @brief Инициализирует внутреннее состояние с использованием объекта SeedSequence.
         *
         * Генерирует массив 32-битных чисел с помощью seq.generate().
         * Если целевой тип 64-битный, числа попарно склеиваются в 64-битные слова
         * (старшие 32 бита и младшие 32 бита).
         *
         * @tparam SeedSeq Тип, удовлетворяющий концепту SeedSequence.
         * @param seq Объект, генерирующий начальные данные.
         */
        template<SeedSequence SeedSeq>
        void initBySeedSeq(SeedSeq& seq)
        {
            static constexpr size_t k{ bitWidth == 32 ? 1 : 2 }; // 1 для 32, 2 для 64
            static constexpr size_t genSize{ stateSize * k };

            std::array<result_type, genSize> seedData;
            seq.generate(seedData.begin(), seedData.end());

            for(size_t i{ 0 }; i < stateSize; ++i)
            {
                if constexpr(bitWidth == 64)
                {
                    m_state[i] =  seedData[2 * i]
                               + (seedData[2 * i + 1] << 32);
                }
                else
                {
                    m_state[i] = seedData[i];
                }
            }

            m_index = stateSize; // Заставляем генератор сделать twist() при первом вызове
        }

        /**
         * @brief Генерирует следующий блок из stateSize псевдослучайных чисел.
         *
         * Это ядро алгоритма Mersenne Twister. Оно перемешивает биты массива m_state,
         * используя технику "закручивания" (twisting) со сдвигом и применением matrixA.
         * После завершения цикла индекс сбрасывается в 0.
         */
        void twist() noexcept
        {
            constexpr result_type upperMask{ bit::upperMask<result_type>(separationPoint) };
            constexpr result_type lowerMask{ ~upperMask };

            for(std::size_t i{ 0 }; i < stateSize; ++i)
            {
                // Склеиваем старшие биты текущего элемента и младшие биты следующего
                result_type y{ (m_state[i] & upperMask) | (m_state[(i + 1) % stateSize] & lowerMask) };

                 // Закручиваем: применяем XOR с элементом со смещением middleWordOffset
                m_state[i] = m_state[(i + middleWordOffset) % stateSize] ^ (y >> 1);

                // Если младший бит y был 1, применяем матрицу A
                if(y & 1)
                {
                    m_state[i] ^= matrixA;
                }
            }

            m_index = 0;
        }

    public:
        /**
         * @brief Конструктор по умолчанию.
         *
         * Инициализирует генератор с использованием std::random_device и текущего времени.
         * Это гарантирует, что каждый экземпляр получит уникальную последовательность,
         * даже если объекты создаются в одну и ту же миллисекунду.
         */
        MersenneTwister()
        {
            std::random_device rd;
            auto time { std::chrono::system_clock::now().time_since_epoch().count() };
            std::seed_seq seq{ rd(), rd(), rd(), rd(), static_cast<std::uint32_t>(time) };

            initBySeedSeq(seq);
        }

        /**
         * @brief Конструктор с явным указанием начального числа (seed).
         *
         * Полезен для детерминированных тестов, когда нужно получить одинаковую
         * последовательность чисел при каждом запуске программы.
         *
         * @param seed Начальное значение (зерно).
         */
        explicit MersenneTwister(UIntType seed) noexcept
        {
            initBySeed(seed);
        }

        /**
         * @brief Конструктор с использованием внешнего SeedSequence.
         *
         * Позволяет передать любой объект, удовлетворяющий концепту SeedSequence
         * (например, std::seed_seq или ваш собственный), для инициализации.
         *
         * @tparam SeedSeq Тип, удовлетворяющий концепту SeedSequence.
         * @param seq Объект для генерации начального состояния.
         */
        template<SeedSequence SeedSeq>
        explicit MersenneTwister(SeedSeq& seq)
        {
            initBySeedSeq(seq);
        }

        /**
         * @brief Генерирует следующее псевдослучайное число.
         *
         * Если текущий индекс превысил размер массива, сначала происходит
         * "закручивание" (twist). Затем берется число из массива и применяется
         * финальное "темперирование" (tempering) для улучшения статистических свойств.
         *
         * @return Следующее псевдослучайное число типа result_type.
         */
        result_type operator()() noexcept
        {
            if(m_index >= stateSize)
            {
                twist();
            }

            result_type x{ m_state[m_index++] };

            // Темперирование (Tempering): перемешиваем биты для устранения линейности
            x ^= (x >> temperShiftU) & temperMaskD;
            x ^= (x << temperShiftS) & temperMaskB;
            x ^= (x << temperShiftT) & temperMaskC;
            x ^= (x >> temperShiftL);

            return x;
        }

    };


    /**
     * @brief Псевдоним для MersenneTwister, используемый по умолчанию.
     * По умолчанию использует 64-битный тип.
     */
    template<UInt_32_or_64 UIntType = uint64_t>
    using defaultRandomizer = MersenneTwister<UIntType>;


} // endnamespace random

} // end namespace mylib

#endif // RANDOM_H
