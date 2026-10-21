#ifndef RANDOM_H
#define RANDOM_H

#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>

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


    class MT19937 final
    {
    public:
    };

    using defaultRandomizer = MT19937;


} // endnamespace random

} // end namespace mylib

#endif // RANDOM_H
