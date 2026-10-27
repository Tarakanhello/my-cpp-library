#ifndef RANDOM_H
#define RANDOM_H

#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <random>

#include "concepts.h"
#include "mylib/bit_operations.h"
#include "mylib/overflow.h"
#include "mylib/vector.h"
#include "mylib/stack.h"

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

        /**
         * @brief Инициализация через SeedSequence.
         *
         * Генерирует два 32-битных числа и склеивает их в одно 64-битное.
         * Гарантирует, что итоговое состояние не будет равно нулю.
         */
        template<SeedSequence SeedSeq>
        void initBySeedSeq(SeedSeq& seq)
        {
            std::array<uint32_t, 2> seedData;
            seq.generate(seedData.begin(), seedData.end());

            m_state = (static_cast<result_type>(seedData[0]) << 32) | seedData[1];

            if(m_state == 0)
            {
                m_state = PASSWORD;
            }
        }


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
         * @brief Конструктор с использованием std::seed_seq (или любого SeedSequence).
         */
        template<SeedSequence SeedSeq>
        explicit QualityXorshift64(SeedSeq& seq)
        {
            initBySeedSeq(seq);
        }

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




    /**
     * @brief A high-level convenience wrapper around a Uniform Random Bit Generator (URBG).
     *
     * This class provides an easy-to-use interface for common random generation tasks,
     * such as rolling dice, flipping coins, or generating floating-point numbers.
     * It eliminates the need to manually instantiate and manage `<random>` distributions
     * for everyday use cases.
     *
     * By default, it uses the `defaultRandomizer` (which is `MersenneTwister<uint64_t>`),
     * but it can wrap any type that satisfies the `std::uniform_random_bit_generator` concept.
     *
     * @tparam GENERATOR The underlying random number engine.
     */
    template<std::uniform_random_bit_generator GENERATOR = defaultRandomizer<>>
    class Randomizer final
    {
    public:
        /// @brief The type of values produced by the underlying generator.
        using result_type = typename GENERATOR::result_type;

        /// @brief Returns the minimum possible value of the underlying generator.
        static constexpr result_type min() { return GENERATOR::min(); }

        /// @brief Returns the maximum possible value of the underlying generator.
        static constexpr result_type max() { return GENERATOR::max(); }

    private:
        GENERATOR m_g;

        double m_cachedNormal{ 0.0 };
        bool m_hasCachedNormal{ false };

        /**
         * @brief Creates a default-initialized generator using high-entropy sources.
         *
         * Combines `std::random_device` (hardware entropy) with the current system time
         * to create a `std::seed_seq`. This ensures that two objects created at the same
         * time will not produce identical sequences.
         *
         * @return A fully initialized GENERATOR object.
         */
        static GENERATOR createDefault()
        {
            if constexpr(std::is_constructible_v<GENERATOR, std::seed_seq&>)
            {
                std::random_device rd{};
                std::seed_seq seq{ rd(),
                                   rd(),
                                   rd(),
                                   rd(),
                                   rd(),
                                   rd(),
                                   rd(),
                                   static_cast<std::uint32_t>(std::chrono::system_clock::now().time_since_epoch().count()) };
                return GENERATOR{ seq };
            }
            else if constexpr(std::is_default_constructible_v<GENERATOR>)
            {
                return GENERATOR{};
            }
            else
            {
                throw std::invalid_argument("GENERATOR must be constructible from std::seed_seq or default-constructible");
            }
        }

    public:
        /**
         * @brief Default constructor. Initializes the generator with high entropy.
         */
        Randomizer()
            : m_g{ createDefault() }
        {}

        /**
         * @brief Constructs the generator with a specific seed.
         * @param seed The starting value. Useful for reproducible tests.
         */
        explicit Randomizer(result_type seed)
            : m_g{ seed }
        {}

        /**
         * @brief Constructs the generator using a SeedSequence (e.g., std::seed_seq).
         * @param seq The sequence object to use for initialization.
         */
        template<SeedSequence SeedSeq>
        explicit Randomizer(SeedSeq& seq)
            : m_g{ seq }
        {}

        /**
         * @brief Advances the generator and returns the next raw random value.
         * Required to satisfy the UniformRandomBitGenerator concept.
         */
        result_type operator()() noexcept
        {
            return m_g();
        }

        /**
         * @brief Alias for operator(). Returns the next raw random value.
         */
        result_type next() noexcept
        {
            return (*this)();
        }

        /**
         * @brief Generates a uniformly distributed integer in the range [0, n - 1].
         *
         * @note This function does NOT use the modulo operator (`%`). Using `%` introduces
         *       a slight statistical bias (modulo bias). Instead, it uses
         *       `std::uniform_int_distribution`, which employs rejection sampling to
         *       guarantee mathematically perfect uniformity.
         *
         * @param n The exclusive upper bound (must be > 0).
         * @return A random integer between 0 and n - 1.
         */
        result_type mod(result_type n)
        {
            assert(n > 0 && "mylib::random::Randomizer::mod(result_type n): n must be positive");

            std::uniform_int_distribution<result_type> numbers{ 0, static_cast<result_type>(n - 1) };

            return numbers(m_g);
        }

        /**
         * @brief Generates either 1 or -1 with a 50/50 probability.
         * Useful for random walks or physics simulations.
         */
        int sign() { return mod(2) ? 1 : -1; }

        /**
         * @brief Generates a uniformly distributed integer in the inclusive range [a, b].
         *
         * @tparam IntType The integer type (e.g., int, long long). Defaults to int.
         * @param a The lower bound (inclusive).
         * @param b The upper bound (inclusive).
         * @return A random integer between a and b.
         *
         * @note We use `IntType` for the distribution, NOT `result_type`. If `result_type`
         *       is unsigned (uint64_t) and you pass negative values for `a` or `b`, using
         *       `result_type` would cause severe underflow bugs.
         * @note `noexcept` is omitted because `std::uniform_int_distribution::operator()`
         *       is not guaranteed to be `noexcept` by the C++ standard.
         */
        template<std::integral IntType = int>
        IntType inRange(IntType a, IntType b)
        {
            assert(b >= a && "mylib::random::Randomizer::inRange(IntType a, IntType b): b must be b >= a");

            std::uniform_int_distribution<IntType> numbers{ a, b };

            return numbers(m_g);
        }

        /**
         * @brief Generates a uniformly distributed floating-point number in the half-open interval [0, 1).
         *
         * @tparam RealType The floating-point type to generate (e.g., float, double, long double).
         * @return A random number in [0, 1).
         *
         * @note Not marked `noexcept` because `std::uniform_real_distribution::operator()`
         *       is not guaranteed to be `noexcept` by the C++ standard.
         */
        template<std::floating_point RealType = double>
        RealType uniform01()
        {
            std::uniform_real_distribution<RealType> dist{ RealType{ 0.0 }, RealType{ 1.0 } };

            return dist(m_g);
        }

        /**
         * @brief Generates a random number following an exponential distribution.
         *
         * @param a The rate parameter (alpha). Must be > 0.
         * @return Time until the next event.
         *
         * @note Uses inverse transform sampling: x = -ln(u) / a.
         */
        double exponential(double a = 1.0)
        {
            assert(a > 0 && "mylib::random::Randomizer::exponential(double alpha): alpha must be positive");

            double u{ uniform01() };

            if(u == 0.0)
            {
                u = std::nextafter(0.0, 1.0);
            }

            return -std::log(u) / a;
        }

        /**
         * @brief Generates a random number following a Cauchy distribution.
         *
         * @param m The location parameter (median). Default is 0.
         * @param q The scale parameter (width). Default is 1.
         * @return A random number from the Cauchy distribution.
         */
        double cauchy(double m = 0.0, double q = 1.0)
        {
            assert(q > 0.0 && "Scale parameter 'q' must be positive!");

            double u{ uniform01() };

            if(u == 0.0)
            {
                u = std::nextafter(0.0, 1.0);
            }

            return m + q * std::tan(std::numbers::pi * (u - 0.5));
        }

        /**
         * @brief Generates a standard normal random number (mean=0, stddev=1)
         *        using the Marsaglia polar method.
         *
         * @return A random number from the standard normal distribution.
         */
        double normal01()
        {
            if(m_hasCachedNormal)
            {
                m_hasCachedNormal = false;
                return m_cachedNormal;
            }
            double a{};
            double b{};
            double c{};

            do
            {
                a = 2.0 * uniform01() - 1.0;
                b = 2.0 * uniform01() - 1.0;

                c = a * a + b * b;
            } while( c >= 1.0 || c == 0.0);


            double temp{ std::sqrt(-2.0 * std::log(c) / c) };

            m_cachedNormal = b * temp;
            m_hasCachedNormal = true;

            return a * temp;
        }

        /**
         * @brief Generates a random number following a Normal (Gaussian) distribution.
         *
         * Scales and shifts a standard normal value (mean=0, stddev=1) generated by
         * normal01() to have the specified mean and standard deviation.
         *
         * @param m The mean (mu) of the distribution. Shifts the center of the bell curve.
         * @param q The standard deviation (sigma) of the distribution. Controls the width
         *          of the bell curve. Must be strictly greater than 0.
         * @return A random number from the specified normal distribution.
         *
         * @note For a standard normal distribution, use normal01() instead.
         */
        double normal(double m, double q)
        {
            assert(q > 0.0 && "Standard deviation must be positive!");
            return m + q * normal01();
        }

        /**
         * @brief Generates a random number following a Gamma distribution.
         *
         * Uses the Marsaglia-Tsang method for shape >= 1, and a mathematical
         * trick (boost) for shape < 1.
         *
         * @param shape The shape parameter (often denoted as k or alpha). Must be > 0.
         * @param scale The scale parameter (often denoted as theta or beta). Must be > 0. Default is 1.0.
         * @return A random number from the Gamma distribution.
         */
        double gamma(double shape = 1.0, double scale = 1.0)
        {
            assert(shape > 0.0 && scale > 0.0);

            // Если форма меньше 1, используем трюк:
            // Гамма(k) = Гамма(k+1) * U^(1/k)
            // где U - равномерное распределение от 0 до 1
            if (shape < 1.0)
            {
                double u = uniform01();
                return gamma(shape + 1.0, 1.0) * std::pow(u, 1.0 / shape) * scale;
            }

            // Алгоритм Марсальи-Цанга для shape >= 1
            double d = shape - 1.0 / 3.0;
            double c = 1.0 / std::sqrt(9.0 * d);

            for (;;)
            {
                double x, v;
                do
                {
                    x = normal01(); // Используем наше нормальное распределение
                    v = 1.0 + c * x;
                } while (v <= 0.0);

                v = v * v * v;
                double u = uniform01();

                // Быстрая проверка (отбраковка)
                if (u < 1.0 - 0.0331 * (x * x) * (x * x))
                {
                    return d * v * scale;
                }

                // Точная проверка (логарифмическая)
                if (std::log(u) < 0.5 * x * x + d * (1.0 - v + std::log(v)))
                {
                    return d * v * scale;
                }
            }
        }

        /**
         * @brief Generates a random number following a Chi-Squared distribution.
         *
         * The Chi-Squared distribution with k degrees of freedom is a special case
         * of the Gamma distribution: Gamma(shape = k/2, scale = 2).
         *
         * @param k Degrees of freedom. Must be > 0.
         * @return A random number from the Chi-Squared distribution.
         */
        double chiSquared(double k)
        {
            assert(k > 0.0);
            // Хи-квадрат — это просто Гамма с формой k/2 и масштабом 2
            return gamma(k / 2.0, 2.0);
        }

        /**
         * @brief Generates a random number following a Student's t-distribution.
         *
         * Formula: T = Z / sqrt(V / v)
         * where Z is standard normal, and V is Chi-Squared with v degrees of freedom.
         *
         * @param v Degrees of freedom. Must be > 0.
         * @return A random number from the Student's t-distribution.
         */
        double studentT(double v)
        {
            assert(v > 0.0);
            double z = normal01();
            double chisq = chiSquared(v);

            return z / std::sqrt(chisq / v);
        }

        /**
         * @brief Generates a random number following a Beta distribution.
         *
         * Formula: B = X / (X + Y)
         * where X is Gamma(alpha, 1) and Y is Gamma(beta_param, 1).
         *
         * @param alpha First shape parameter. Must be > 0.
         * @param beta_param Second shape parameter. Must be > 0.
         * @return A random number from the Beta distribution (always in range [0, 1]).
         */
        double beta(double alpha, double beta_param)
        {
            assert(alpha > 0.0 && beta_param > 0.0);

            double x = gamma(alpha, 1.0);
            double y = gamma(beta_param, 1.0);

            return x / (x + y);
        }

        /**
         * @brief Generates a random number following a Fisher-Snedecor (F) distribution.
         *
         * Formula: F = (U1 / m) / (U2 / n)
         * where U1 is Chi-Squared with m degrees of freedom, and U2 is Chi-Squared with n.
         *
         * @param m Numerator degrees of freedom. Must be > 0.
         * @param n Denominator degrees of freedom. Must be > 0.
         * @return A random number from the F-distribution.
         */
        double fisherF(double m, double n)
        {
            assert(m > 0.0 && n > 0.0);

            double u1 = chiSquared(m);
            double u2 = chiSquared(n);

            return (u1 / m) / (u2 / n);
        }

        /**
         * @brief Triangular distribution. Returns a value between 0 and 1 with a peak at `middle`.
         */
        double triangular(double middle = 0.5)
        {
            assert(middle > 0.0 && middle < 1.0);
            double u = uniform01();
            return std::sqrt(u <= middle ? middle * u : (1.0 - middle) * (1.0 - u));
        }

        /**
         * @brief Scaled Triangular distribution [a, b].
         */
        double triangular(double a, double b, double middle)
        {
            return a + (b - a) * triangular((middle - a) / (b - a));
        }

        /**
         * @brief Lévy distribution (heavy tails).
         */
        double levy(double scale = 0.455)
        {
            // Note: normal(0, 1/sqrt(scale)) means standard deviation is 1/sqrt(scale)
            double temp = normal(0.0, 1.0 / std::sqrt(scale));
            return 1.0 / (temp * temp);
        }

        /**
         * @brief Symmetrized Lévy distribution.
         */
        double symmetrizedLevy(double scale = 0.455)
        {
            return sign() * levy(scale);
        }

        /**
         * @brief Bernoulli distribution (true with probability p, false with 1-p).
         */
        bool bernoulli(double p)
        {
            assert(p >= 0.0 && p <= 1.0);
            return uniform01() < p;
        }

        /**
         * @brief Binomial distribution (number of successes in n trials with probability p).
         */
        int binomial(double p, int n)
        {
            assert(p >= 0.0 && p <= 1.0 && n >= 0);
            int result = 0;
            for (int i = 0; i < n; ++i)
            {
                result += bernoulli(p);
            }
            return result;
        }

        /**
         * @brief Geometric distribution (number of trials until first success).
         */
        int geometric(double p)
        {
            assert(p > 0.0 && p <= 1.0);
            int result = 1;
            while (!bernoulli(p))
            {
                ++result;
            }
            return result;
        }

        /**
         * @brief Optimized geometric distribution for p=0.5 using bitwise operations.
         * Counts trailing zeros in a random 64-bit number.
         */
        int geometric05()
        {
            // В C++20 можно использовать std::countr_zero(next())
            // Или функцию bit::rightmostNullCount из вашей библиотеки
            return bit::rightmostNullCount(next()) + 1;
        }

        /**
         * @brief Generates a random number following a Poisson distribution.
         *
         * @param l The expected number of events (lambda).
         *          It equals the rate of events multiplied by the time interval.
         *          Must be > 0.
         * @return The number of events (integer).
         */
        int poisson(double l)
        {
            assert(l > 0.0);

            int result = -1;

            // Оптимизация: вместо медленного сложения логарифмов,
            // мы умножаем равномерные числа и сравниваем с exp(-l)
            for(double p = 1.0; p > std::exp(-l); p *= uniform01())
            {
                ++result;
            }

            return result;
        }
    };

    inline Randomizer<>& GlobalRNG()
    {
        static Randomizer<> r;
        return r;
    }

    /**
     * @brief A weighted random sampler using the Walker–Vose alias method.
     *
     * Preprocesses a list of non-negative weights into two parallel tables
     * (`m_prob` and `m_alias`) of size `n`, after which every call to
     * `sample()` runs in strict O(1) time, regardless of `n`. This makes the
     * alias method the standard choice when the same weighted distribution
     * must be sampled many times.
     *
     * The weights do not need to sum to 1 — they are normalized internally.
     * They do, however, need to be finite, non-negative, and not all zero.
     *
     * @tparam RealType The floating-point type used for weights and for the
     *                  internal probability table. Must satisfy
     *                  `std::floating_point`.
     * @tparam GENERATOR The underlying URBG used by the no-argument
     *                   `sample()`. Must satisfy
     *                   `std::uniform_random_bit_generator`.
     *
     * @note Construction is O(n) in both time and memory; sampling is O(1).
     * @note Objects of this class are immutable after construction and may be
     *       shared across threads for concurrent reads. The no-argument
     *       `sample()` uses a `thread_local` RNG, so it is safe to call from
     *       multiple threads without external synchronization.
     */
    template<std::floating_point RealType,
             std::uniform_random_bit_generator GENERATOR = defaultRandomizer<size_t>>
    class AliasMethod final
    {
    private:
        /// @brief Storage for the probability table. Owns its data.
        using Vector = mylib::Vector<RealType>;
        /// @brief Storage for the "small" / "large" work lists during construction.
        using Stack = mylib::Stack<size_t>;
        // @brief Storage for the alias table. Owns its data.
        using Alias = mylib::Vector<size_t>;

    private:
        /// @brief Probability of keeping the sampled index `i` (range [0, 1]).
        Vector m_prob{};
        /// @brief Index to fall back to when the coin flip against `m_prob[i]` fails.
        Alias  m_alias{};

        /**
         * @brief Validates the input weights and builds the alias table.
         *
         * Performs the classic Walker–Vose preprocessing:
         *   1. Validate every weight (finite, non-negative) and the total sum.
         *   2. Scale each weight by `n / sum`, so the average scaled weight is 1.
         *   3. Partition indices into `small` (scaled < 1) and `large` (scaled >= 1).
         *   4. Repeatedly pair a `small` with a `large`, "borrowing" probability
         *      mass from the latter, until one of the two lists is empty.
         *   5. Fill the remaining entries with `prob = 1.0` (their alias is never
         *      consulted, because `u < 1.0` always holds for `u ∈ [0, 1)`).
         *
         * @param begin Iterator to the first weight.
         * @param end   Iterator one past the last weight.
         * @param size  Number of elements in `[begin, end)`.
         *
         * @throws std::invalid_argument if the range is empty, contains a
         *         non-finite or negative weight, or if the sum is zero or
         *         non-finite (overflow).
         *
         * @note `size` is passed explicitly to avoid recomputing
         *       `std::distance(begin, end)` for forward iterators.
         * @note The scaling `(*it / sum) * size` is done in that order to
         *       avoid overflow of `*it * size` when weights are large.
         * @note Due to floating-point rounding, some entries may end up with
         *       `m_prob[i]` slightly outside [0, 1] (e.g. 1 + 1e-16). This
         *       does not break correctness — it only shifts probability mass
         *       by one ULP.
         */
        void checkAndConstruct(auto begin, auto end, size_t size)
        {
            if(size == 0)
            {
                throw std::invalid_argument("Empty vector");
            }

            RealType sum{ 0 };

            for(auto it{ begin }; it != end; ++it)
            {
                if(!std::isfinite(*it))
                {
                    throw std::invalid_argument("NaN or inf value in vector");
                }
                if(*it < 0)
                {
                    throw std::invalid_argument("Negative weight");
                }
                sum += *it;
            }

            if(sum == 0)
            {
                throw std::invalid_argument("Zero weight");
            }
            if(!std::isfinite(sum))
            {
                throw std::invalid_argument("Sum overflow");
            }

            Vector scaled{};
            scaled.reserve(size);

            Stack small{};
            Stack large{};

            for(auto it{ begin }; it != end; ++it)
            {
                RealType s{ (*it / sum) * static_cast<RealType>(size) };
                scaled.emplace_back(s);

                s < RealType{ 1.0 } ? small.push(scaled.size() - 1)
                                    : large.push(scaled.size() - 1);
            }

            m_prob.resize(size);
            m_alias.resize(size);

            while(!small.empty() && !large.empty())
            {
                auto s{ small.top() };
                small.pop();

                auto l{ large.top() };
                large.pop();

                m_prob[s] = scaled[s];
                m_alias[s] = l;

                scaled[l] -= (RealType{ 1.0 } - scaled[s]);
                if(scaled[l] < 1.0)
                {
                    small.push(l);
                }
                else
                {
                    large.push(l);
                }
            }

            while(!large.empty())
            {
                m_prob[large.top()] = RealType{ 1.0 };
                large.pop();
            }

            while(!small.empty())
            {
                m_prob[small.top()] = RealType{ 1.0 };
                small.pop();
            }
        }


    public:
        /**
         * @brief Deleted: an alias table requires at least one weight.
         */
        AliasMethod() = delete;

        /**
         * @brief Constructs an alias table from an iterator range of weights.
         *
         * @tparam ITERATOR A forward iterator whose value type is exactly
         *                  `RealType` (not merely convertible to it).
         * @param begin Iterator to the first weight.
         * @param end   Iterator one past the last weight.
         *
         * @throws std::invalid_argument (see `checkAndConstruct`).
         *
         * @note The `same_as` constraint (rather than `convertible_to`)
         *       prevents accidental construction from `std::vector<int>`
         *       or `std::vector<float>` when `RealType` is `double`, which
         *       would silently lose precision or trigger implicit
         *       conversions the caller did not intend.
         */
        template<std::forward_iterator ITERATOR>
            requires std::same_as<std::iter_value_t<ITERATOR>, RealType>
        AliasMethod(ITERATOR begin, ITERATOR end)
        {
            checkAndConstruct(begin, end, std::distance(begin, end));
        }

        /**
         * @brief Constructs an alias table from an initializer list.
         *
         * Allows the convenient syntax:
         * @code
         * AliasMethod<double> dist{ 1.0, 2.0, 3.0 };
         * @endcode
         *
         * @param list The weights to normalize.
         * @throws std::invalid_argument (see `checkAndConstruct`).
         */
        AliasMethod(std::initializer_list<RealType> list)
        {
            checkAndConstruct(list.begin(), list.end(), list.size());
        }

        /**
         * @brief Constructs an alias table from an arbitrary sized range.
         *
         * @tparam CONTAINER A sized range whose value type is exactly
         *                   `RealType`.
         * @param c The container of weights.
         *
         * @throws std::invalid_argument (see `checkAndConstruct`).
         *
         * @note Marked `explicit` to prevent accidental implicit conversion
         *       from a `std::vector<RealType>`, which would otherwise let
         *       a container silently bind to a function expecting an
         *       `AliasMethod`.
         * @note The `sized_range` constraint is required because
         *       `checkAndConstruct` needs `n` up front for the scaling step.
         */
        template<typename CONTAINER>
            requires std::ranges::sized_range<CONTAINER> &&
                     std::same_as<std::ranges::range_value_t<CONTAINER>, RealType>
        explicit AliasMethod(const CONTAINER& c)
        {
            checkAndConstruct(c.begin(), c.end(), c.size());
        }

        /**
         * @brief Returns the number of outcomes (the size of the probability table).
         */
        size_t size() const noexcept
        {
            return m_prob.size();
        }

        /**
         * @brief Draws a single sample using a per-thread default RNG.
         *
         * The index `i` is drawn uniformly from [0, n), and the result is
         * `i` with probability `m_prob[i]`, or `m_alias[i]` otherwise.
         * This is the classic O(1) alias-method decision.
         *
         * @return An index in [0, n), distributed according to the
         *         normalized input weights.
         *
         * @note The `Randomizer` is `thread_local`, so this overload is safe
         *       to call concurrently from multiple threads without
         *       synchronization. The first call in each thread pays the cost
         *       of `Randomizer`'s default construction, which seeds from
         *       `std::random_device` — potentially noticeable.
         * @note Not marked `noexcept`: `std::uniform_int_distribution` and
         *       `std::uniform_real_distribution` do not guarantee `noexcept`
         *       `operator()`.
         * @note The two distributions are constructed locally on each call
         *       rather than stored as `static`. `static` would turn this
         *       into a data race across threads (function-local statics are
         *       shared, not per-thread), which is exactly the bug this
         *       overload exists to avoid.
         */
        size_t sample() const
        {
            assert(size() > 0 && "mylib::random::AliasMethod::sample(): empty size");
            thread_local Randomizer<GENERATOR> rng{};

            std::uniform_int_distribution<size_t> ints{ 0, size() - 1 };
            std::uniform_real_distribution<RealType> real{ RealType{0.0}, RealType{1.0} };

            const size_t i{ ints(rng) };
            const RealType u{ real(rng) };

            return (u < m_prob[i]) ? i : m_alias[i];
        }

        /**
         * @brief Draws a single sample using a caller-supplied URBG.
         *
         * Use this overload when reproducibility matters (tests, simulations
         * with a fixed seed) or when you want to avoid the `thread_local`
         * RNG and its default-construction cost.
         *
         * @tparam GEN A URBG; see `std::uniform_random_bit_generator`.
         * @param gen The generator to draw from. Advanced by this call.
         * @return An index in [0, n), distributed according to the
         *         normalized input weights.
         *
         * @note `gen` is taken by non-const reference because every call
         *       mutates its internal state.
         * @note Not marked `noexcept` (same reason as the no-argument overload).
         */
        template<std::uniform_random_bit_generator GEN>
        size_t sample(GEN& gen) const
        {
            assert(size() > 0 && "mylib::random::AliasMethod::sample(GEN&): empty size");
            std::uniform_int_distribution<size_t> ints{0, size() - 1};
            std::uniform_real_distribution<RealType> real{ 0, 1 };

            size_t i{ ints(gen) };
            RealType u{ real(gen) };

            return (u < m_prob[i]) ? i : m_alias[i];
        }
    };

    /**
     * @brief Deduction guide: deduce `RealType` from a sized range of weights.
     *
     * Enables `AliasMethod dist{ std::vector<double>{...} };`.
     */
    template<std::ranges::sized_range CONTAINER>
        requires std::floating_point<std::ranges::range_value_t<CONTAINER>>
    AliasMethod(const CONTAINER& c) -> AliasMethod<std::ranges::range_value_t<CONTAINER>>;

    /**
     * @brief Deduction guide: deduce `RealType` from an iterator pair.
     *
     * Enables `AliasMethod dist{ v.begin(), v.end() };`.
     */
    template<std::forward_iterator ITERATOR>
        requires std::floating_point<std::iter_value_t<ITERATOR>>
    AliasMethod(ITERATOR, ITERATOR) -> AliasMethod<std::iter_value_t<ITERATOR>>;



    /**
     * @brief A weighted random sampler using a segment-tree-like structure (Sum Heap).
     *
     * Preprocesses a list of weights into a binary tree stored in an array (vector),
     * where each parent node stores the sum of its own weight and its children's weights.
     * This enables two key operations:
     *   - O(log n) weighted sampling (`next`).
     *   - O(log n) weight updates (`add`, `subtract`, `remove`).
     *
     * Unlike the `AliasMethod` (which is O(1) for sampling but O(n) for updates),
     * `SumHeap` is the standard choice when weights change frequently (e.g., loot
     * tables with dynamic drop rates, load balancing).
     *
     * @tparam T The unsigned integer type used for weights and sums. Must satisfy
     *           `std::unsigned_integral`. Floating-point types are intentionally
     *           rejected to prevent rounding errors during weight updates.
     *
     * @note Construction from a range is O(n log n) time and O(n) space.
     * @note `next()` and `find()` are `const` and safe to call concurrently
     *       on the same instance, provided the underlying `GlobalRNG()` is thread-safe.
     */
    template<std::unsigned_integral T>
    class SumHeap final
    {
    private:
        /// @brief Storage for the heap. `m_heap[0]` is the total sum of all weights.
        using Vector = mylib::Vector<T>;

    private:
        /// @brief The underlying array representing the binary tree.
        Vector m_heap{};

        // =========================================================================
        // Private Helpers: Tree Navigation
        // =========================================================================

        /**
         * @brief Calculates the index of the left child in the array.
         * @param i The index of the parent.
         * @return The index `2 * i + 1`.
         * @throws std::std::length_error via `checkMulOverflow` if `2 * i` overflows.
         */
        size_t leftChild(size_t i) const
        {
            checkMulOverflow(size_t{ 2 }, i, "mylib::random::SumHeap::leftChild(size_t)");
            return 2 * i + 1;
        }

        /**
         * @brief Calculates the index of the parent in the array.
         * @param i The index of the child.
         * @return The index `(i - 1) / 2`. Returns 0 if `i == 0`.
         */
        size_t parent(size_t i) const noexcept
        {
            if(i == 0)
            {
                return i;
            }

            return (i - 1) / 2;
        }

        /**
         * @brief Calculates the index of the right child in the array.
         * @param i The index of the parent.
         * @return The index `2 * i + 2`.
         * @throws std::std::length_error via `checkMulOverflow` if `2 * i` overflows.
         */
        size_t rightChild(size_t i) const
        {
            checkMulOverflow(size_t{ 2 }, i, "mylib::random::SumHeap::rightChild(size_t)");
            return 2 * i + 2;
        }

        // =========================================================================
        // Private Helpers: Internal Logic
        // =========================================================================

        /**
         * @brief Validates a range of weights, checks for overflow, and builds the tree.
         *
         * Calculates the total sum of the range to ensure no overflow occurs,
         * then appends each weight one by one using `add()`.
         *
         * @param begin Iterator to the first weight.
         * @param end   Iterator one past the last weight.
         * @param size  Number of elements in the range.
         *
         * @throws std::invalid_argument if the range is empty.
         * @throws std::std::length_error if the sum of all weights exceeds `T`'s limit.
         *
         * @note Construction is O(n log n) because `add()` performs an O(log n)
         *       traversal up the tree for each element.
         */
        void createFromRange(auto begin, auto end, size_t size)
        {
            if(size == 0)
            {
                throw std::invalid_argument("Empty vector");
            }

            T sum{ 0 };

            for(auto it{ begin }; it != end; ++it)
            {
                checkAddOverflow(sum, *it, "mylib::random::SumHeap::createFromRange");
                sum += *it;
            }

            m_heap.reserve(size);

            for(auto it{ begin }; it != end; ++it)
            {
                add(*it);
            }
        }

        /**
         * @brief Traverses the tree to find the index corresponding to the given value.
         *
         * This uses the Post-order layout: Left Child -> Right Child -> Parent.
         * At each node, it subtracts the left child's sum from `value`. If `value`
         * drops below the left child's sum, it goes left. Otherwise, it checks the
         * right child. If neither matches, the value must belong to the parent.
         *
         * @param value The random value in the range `[0, total())`.
         * @return The index of the selected weight.
         */
        size_t find(T value) const
        {
            assert(value < total() && "value must be in [0, total())");

            size_t currentIndex{ 0 };

            while(true)
            {
                size_t leftIndex{ leftChild(currentIndex) };

                // 1. Проверка на выход за границы
                if(leftIndex >= m_heap.size())
                {
                    break; // Детей нет, возвращаем текущий узел
                }

                // 2. Проверяем левого ребенка
                if(value < m_heap[leftIndex])
                {
                    currentIndex = leftIndex;
                    continue; // Уходим в левую ветку
                }

                // Если не попали в левого, "отрезаем" его вес от value
                value -= m_heap[leftIndex];

                // 3. Проверяем правого ребенка
                size_t rightIndex{ rightChild(currentIndex) };

                if(rightIndex < m_heap.size() &&
                    value < m_heap[rightIndex])
                {
                    currentIndex = rightIndex;
                    continue; // Уходим в правую ветку
                }

                // 4. Если не попали ни в левого, ни в правого — это ТОЧНО Родитель!
                break;
            }

            return currentIndex;
        }

        /**
         * @brief Subtracts a value from a node and all its ancestors up to the root.
         * @param value The amount to subtract.
         * @param i The starting node index.
         */
        void subtractFromAncestors(T value, size_t i) noexcept
        {
            for(;; i = parent(i))
            {
                m_heap[i] -= value;
                if(i < 1)
                {
                    break;
                }
            }
        }

    public:
        // =========================================================================
        // Constructors & Assignment
        // =========================================================================

        /**
         * @brief Default constructor. Creates an empty heap.
         */
        SumHeap() = default;

        /**
         * @brief Constructs the heap from an arbitrary sized range.
         * @tparam CONTAINER A sized range whose value type is exactly `T`.
         * @param c The container of weights.
         * @throws std::invalid_argument if the range is empty.
         * @throws std::std::length_error if the total sum overflows `T`.
         * @note Marked `explicit` to prevent accidental implicit conversions.
         */
        template<typename CONTAINER>
            requires std::ranges::sized_range<CONTAINER> &&
                     std::same_as<std::ranges::range_value_t<CONTAINER>, T>
        SumHeap(const CONTAINER& c)
        {
            createFromRange(c.begin(), c.end(), c.size());
        }

        /**
         * @brief Constructs the heap from an iterator range.
         * @tparam IT A forward iterator whose value type is exactly `T`.
         * @param begin Iterator to the first weight.
         * @param end   Iterator one past the last weight.
         * @throws std::invalid_argument if the range is empty.
         * @throws std::std::length_error if the total sum overflows `T`.
         */
        template<std::forward_iterator IT>
            requires std::same_as<std::iter_value_t<IT>, T>
        SumHeap(IT begin, IT end)
        {
            createFromRange(begin, end, std::distance(begin, end));
        }

        /**
         * @brief Constructs the heap from an initializer list.
         * @param list The weights to initialize with.
         * @throws std::invalid_argument if the list is empty.
         * @throws std::std::length_error if the total sum overflows `T`.
         */
        SumHeap(std::initializer_list<T> list)
        {
            createFromRange(list.begin(), list.end(), list.size());
        }

        // =========================================================================
        // Public Readers (Queries)
        // =========================================================================

        /**
         * @brief Calculates the cumulative weight up to and including node `i`.
         *
         * Returns the end boundary of node `i` on the numerical line `[0, total())`.
         *
         * @param i The node index.
         * @return The cumulative sum.
         */
        T cumulative(size_t i) const noexcept
        {
            if (i >= m_heap.size())
            {
                return T{ 0 };
            }

            T sum{ m_heap[i] };
            while(i > 0)
            {
                size_t last{ i };
                i = parent(i);
                size_t l{ leftChild(i) };
                if(l != last)
                {
                    sum += m_heap[l];
                }
            }

            return sum;
        }

        /**
         * @brief Retrieves the original (clean) weight of a specific node.
         *
         * Because the tree stores subtree sums, this calculates the node's own
         * weight by subtracting the sums of its left and right children.
         *
         * @param i The node index.
         * @return The original weight of the node.
         */
        T get(size_t i) const noexcept
        {
            if (i >= m_heap.size())
            {
                return T{ 0 };
            }

            T result{ m_heap[i] };
            size_t c{ leftChild(i) };
            if(c < m_heap.size())
            {
                result -= m_heap[c];
                c = rightChild(i);
                if(c < m_heap.size())
                {
                    result -= m_heap[c];
                }
            }

            return result;
        }

        /**
         * @brief Returns the total sum of all weights in the heap.
         * @return The total sum, or `0` if the heap is empty.
         */
        T total() const noexcept { return m_heap.empty() ? T{ 0 } : m_heap[0]; }

        // =========================================================================
        // Public Modifiers (Updates)
        // =========================================================================

        /**
         * @brief Appends a new node with the given weight to the end of the heap.
         * @param value The weight of the new node.
         */
        void add(T value)
        {
            m_heap.push_back(0);
            add(value, m_heap.size() - 1);
        }

        /**
         * @brief Increases the weight of an existing node by `value`.
         *
         * Propagates the addition up to the root to maintain subtree sums.
         *
         * @param value The amount to add (must be positive or zero).
         * @param i The node index.
         */
        void add(T value, size_t i) noexcept
        {
            assert(i < m_heap.size() && "mylib::random::SumHeap::add(T, size_T): Index out of bounds in add!");

            for(;; i = parent(i))
            {
                m_heap[i] += value;
                if(i < 1)
                {
                    break;
                }
            }
        }

        /**
         * @brief Completely removes a node by setting its weight to zero.
         *
         * This is equivalent to calling `subtract(get(i), i)`.
         *
         * @param i The node index.
         */
        void remove(size_t i) noexcept
        {
            assert(i < m_heap.size() && "mylib::random::SumHeap::remove(size_t i): Index out of bounds in remove!");

            T value{ get(i) };
            if(value > 0)
            {
                subtractFromAncestors(value, i);
            }
        }

        /**
         * @brief Decreases the weight of an existing node by `value`.
         *
         * Propagates the subtraction up to the root.
         *
         * @param value The amount to subtract.
         * @param i The node index.
         * @note In debug builds, asserts that `value` does not exceed the node's
         *       current weight.
         */
        void subtract(T value, size_t i) noexcept
        {
            assert(i < m_heap.size() && "mylib::random::SumHeap::subtract(T, size_t): Index out of bounds in subtract!");
            assert(value <= get(i) && "mylib::random::SumHeap::subtract(T, size_t): Cannot subtract more than current weight!");
            subtractFromAncestors(value, i);
        }

        // =========================================================================
        // Public Generation (Sampling)
        // =========================================================================

        /**
         * @brief Draws a random index using the global thread-local RNG.
         *
         * Generates a random integer in `[0, total() - 1]` and finds the
         * corresponding node.
         *
         * @return The index of the selected node, or `0` if the heap is empty.
         */
        size_t next() const
        {
            if(total() == 0)
            {
                return 0;
            }

            return find(static_cast<T>(GlobalRNG().inRange(uint64_t{ 0 }, static_cast<uint64_t>(total()) - 1)));
        }

        /**
         * @brief Draws a random index using a caller-supplied URBG.
         *
         * Use this overload for reproducibility (e.g., in tests).
         *
         * @tparam GEN A URBG; see `std::uniform_random_bit_generator`.
         * @param rng The generator to draw from.
         * @return The index of the selected node, or `0` if the heap is empty.
         */
        template<std::uniform_random_bit_generator GEN>
        size_t next(GEN& rng) const
        {
            if(total() == 0)
            {
                return 0;
            }

            std::uniform_int_distribution<std::uint64_t> real{ 0, static_cast<uint64_t>(total()) - 1 };
            return find(static_cast<T>(real(rng)));
        }
    };

    // =============================================================================
    // Deduction Guides
    // =============================================================================

    /**
     * @brief Deduction guide: deduce `T` from a sized range of weights.
     */
    template<std::ranges::sized_range CONTAINER>
        requires std::unsigned_integral<std::ranges::range_value_t<CONTAINER>>
    SumHeap(const CONTAINER& c) -> SumHeap<std::ranges::range_value_t<CONTAINER>>;

    /**
     * @brief Deduction guide: deduce `T` from an iterator pair.
     */
    template<std::forward_iterator IT>
        requires std::unsigned_integral<std::iter_value_t<IT>>
    SumHeap(IT begin, IT end) -> SumHeap<std::iter_value_t<IT>>;
} // endnamespace random

} // end namespace mylib

#endif // RANDOM_H
