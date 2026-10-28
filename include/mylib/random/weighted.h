#ifndef WEIGHTED_H
#define WEIGHTED_H

#include <concepts>
#include <ranges>

#include "mylib/random/distributions.h"
#include "mylib/random/engines.h"

#include "mylib/core/overflow.h"
#include "mylib//containers/vector.h"
#include "mylib/containers/stack.h"

namespace mylib::random
{

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

} // end mylib::random namespace


#endif // WEIGHTED_H
