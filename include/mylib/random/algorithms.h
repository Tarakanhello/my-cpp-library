#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include <concepts>
#include <mutex>
#include <random>
#include <ranges>
#include <shared_mutex>
#include <utility>

#include "mylib/containers/vector.h"
#include "mylib/random/distributions.h"

namespace mylib::algorithms
{



template< typename T>
using Vector = mylib::Vector<T>;



/**
 * @brief Generates a random unit vector in n-dimensional space.
 *
 * @tparam RNG The Uniform Random Bit Generator to use.
 * @param n The dimension of the vector.
 * @param rng The random number generator.
 * @return mylib::Vector<double> A vector of size n with Euclidean length 1.0.
 */
inline Vector<double> randomInitVector(size_t n)
{
    assert(n > 0);

    Vector<double> result(n);
    std::uniform_real_distribution<double> dist{ -1.0, 1.0 };

    double sumSq{};

    while(true)
    {
        sumSq = 0.0;

        for(size_t i{}; i < n; ++i)
        {
            result[i] = dist(mylib::random::GlobalRNG());
            sumSq += result[i] * result[i];
        }

        if(sumSq != 0.0)
        {
            break;
        }
    }

    double norm = std::sqrt(sumSq);

    for (std::size_t i = 0; i < n; ++i)
    {
        result[i] /= norm;
    }



    return result;

}



/**
 * @brief Shuffles the elements of a container using the provided random generator.
 *
 * Implements the Fisher-Yates (Knuth) shuffle algorithm. This algorithm guarantees
 * that every possible permutation of the container's elements is equally likely.
 *
 * @tparam T The type of the container. Must satisfy std::ranges::random_access_range
 *           (e.g., std::vector, mylib::Vector, C-style array).
 * @tparam GEN The type of the random number generator. Must satisfy
 *             std::uniform_random_bit_generator.
 *
 * @param c The container to shuffle in-place.
 * @param rng The random number generator to use. Advanced by this call.
 */
template<std::ranges::random_access_range T, std::uniform_random_bit_generator GEN>
inline void shuffle(T& c, GEN& rng)
{
    auto n{ std::ranges::size(c) };

    if(n < 2)
    {
        return;
    }

    for(size_t i{ 0 }; i < n - 1; ++i)
    {
        std::uniform_int_distribution<size_t> dist{ i, n - 1 };
        std::swap(c[i], c[dist(rng)]);
    }
}



/**
 * @brief Shuffles the elements of a container using a default thread-local generator.
 *
 * This is a convenience overload. It uses a `thread_local` instance of the default
 * `Randomizer`, ensuring thread safety without the overhead of creating a new
 * generator on every call.
 *
 * @tparam T The type of the container. Must satisfy std::ranges::random_access_range.
 *
 * @param c The container to shuffle in-place.
 *
 * @note Because the generator is `thread_local`, the same sequence of shuffles
 *       will not be reproducible across different runs or threads unless seeded explicitly.
 *       For deterministic testing, use the overload that accepts a specific generator.
 */
template<std::ranges::random_access_range T>
inline void shuffle(T& c)
{
    thread_local random::Randomizer rng{};

    shuffle(c, rng);
}



/**
 * @brief Selects k unique random elements from the range [0, n) and returns them in sorted order.
 *
 * @tparam GEN The Uniform Random Bit Generator.
 * @param k The number of elements to select. Must be > 0.
 * @param n The total number of elements to consider. Must be >= k.
 * @param rng The random number generator.
 * @return Vector<size_t> A sorted vector containing the selected indices.
 */
template<std::uniform_random_bit_generator GEN>
Vector<size_t> sampleIndices(size_t k, size_t n, GEN& rng)
{
    assert(k <= n && k > 0 && "k must be in range (0, n]");

    Vector<size_t> result;
    result.reserve(k);
    for(size_t considered{ 0 }, selected{ 0 }; selected < k; ++considered)
    {
        if(random::bernoulli(rng, static_cast<double>(k - selected) / static_cast<double>(n - considered)))
        {
            result.push_back(considered);
            ++selected;
        }
    }

    return result;
}



/**
 * @brief Generates a random combination of k unique elements from the range [0, n).
 *
 * The elements are selected without replacement and returned in a random order.
 * This is useful for simulating lotteries, drawing cards, or selecting random items
 * where the sequence of selection matters.
 *
 * @tparam GEN The Uniform Random Bit Generator.
 * @param k The number of elements to select. Must be > 0 and <= n.
 * @param n The total number of elements to choose from.
 * @param rng The random number generator.
 * @return Vector<std::size_t> A vector of k unique random indices from [0, n), in random order.
 *
 * @note Time complexity: O(n) due to sampleIndices. Space complexity: O(k).
 */
template<std::uniform_random_bit_generator GEN>
Vector<size_t> randomCombination(size_t k, size_t n, GEN& rng)
{
    Vector<size_t> result{ sampleIndices(k, n, rng) };
    shuffle(result, rng);

    return result;
}



/**
 * @brief A thread-safe reservoir sampler for selecting a random subset from a stream.
 *
 * Implements the Reservoir Sampling algorithm (Algorithm R), which allows selecting
 * `k` items from a stream of unknown or infinite size such that every item has an
 * equal probability of being in the final sample.
 *
 * @note Thread safety model: Safe for concurrent reads.
 *       The processItem() method uses an exclusive lock, so writes
 *       from multiple threads are serialized (executed one by one).
 *       This is required to maintain the mathematical correctness of the algorithm.
 *
 * @tparam T The type of elements to be sampled.
 * @tparam GEN The Uniform Random Bit Generator.
 */
template<typename T, std::uniform_random_bit_generator GEN>
class ReservoirSampler
{
private:
    size_t m_size{};            ///< The maximum number of items to sample.
    size_t m_processed{ 0 };    ///< The total number of items seen so far.
    Vector<T> m_reservoir;      ///< The actual reservoir storing the selected items.
    GEN& m_gen{};               ///< Reference to the random number generator.

    /// @brief Mutex to protect concurrent access. `mutable` allows locking in const methods.
    mutable std::shared_mutex m_mutex;

public:
    ReservoirSampler() = delete;

    // Копирование и перемещение запрещены, так как класс содержит ссылку (m_gen)
    // и мьютекс, который нельзя просто так скопировать.
    ReservoirSampler(const ReservoirSampler&) = delete;
    ReservoirSampler& operator=(const ReservoirSampler&) = delete;
    ReservoirSampler(ReservoirSampler&&) = delete;
    ReservoirSampler& operator=(ReservoirSampler&&) = delete;

    /**
     * @brief Constructs a ReservoirSampler.
     *
     * @param size The number of elements to select (k).
     * @param gen The random number generator. Must outlive the sampler.
     */
    ReservoirSampler(size_t size, GEN& gen)
        : m_size{ size }
        , m_gen{ gen }
    {
        m_reservoir.reserve(size);
    }

    /**
     * @brief Returns the target sample size (k).
     */
    size_t size() const noexcept { return m_size; }

    /**
     * @brief Returns the total number of items processed so far.
     *
     * @note Thread-safe. Uses a shared lock, allowing concurrent reads.
     */
    size_t processed() const
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_processed;
    }

    /**
     * @brief Returns a copy of the currently selected items.
     *
     * @return Vector<T> A copy of the reservoir.
     * @note Thread-safe. Uses a shared lock, allowing concurrent reads.
     */
    Vector<T> getSelectedItems() const
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_reservoir;
    }

    /**
     * @brief Processes a new item from the stream.
     *
     * If the reservoir is not yet full, the item is added. Otherwise, it replaces
     * a random existing item with probability `m_size / m_processed`.
     *
     * @tparam U The type of the incoming item (must match T).
     * @param item The item to process. Uses perfect forwarding to avoid unnecessary copies.
     *
     * @note Thread-safe. Uses an exclusive lock (unique_lock), blocking all other operations.
     */
    template<typename U>
        requires std::same_as<T, std::remove_cvref_t<U>>
    void processItem(U&& item)
    {
        std::unique_lock<std::shared_mutex> lock{ m_mutex };
        ++m_processed;
        if(m_reservoir.size() < m_size)
        {
            m_reservoir.push_back(std::forward<U>(item));
        }
        else
        {
            // Выбираем случайный индекс от 0 до m_processed - 1.
            // Если он попадает в диапазон резервуара [0, m_size - 1], заменяем элемент.
            std::uniform_int_distribution<size_t> index{ 0, m_processed - 1 };
            size_t kickedOut{ index(m_gen) };

            if(kickedOut < m_size)
            {
                m_reservoir[kickedOut] = std::forward<U>(item);
            }
        }
    }
};

/**
 * @brief Convenience function to perform reservoir sampling on an existing range.
 *
 * Iterates over the given range and returns a random sample of the specified size.
 *
 * @tparam Range The type of the input range (must satisfy std::ranges::input_range).
 * @tparam GEN The Uniform Random Bit Generator.
 * @param range The input stream of elements.
 * @param size The number of elements to select.
 * @param rng The random number generator to use.
 * @return Vector<std::ranges::range_value_t<Range>> A vector containing the sampled items.
 *
 * @note This is a sequential (single-threaded) operation. For parallel processing,
 *       instantiate and use ReservoirSampler directly.
 */
template<std::ranges::input_range Range, std::uniform_random_bit_generator GEN>
Vector<std::ranges::range_value_t<Range>> reservoirSample(const Range& range, std::size_t size, GEN& rng)
{
    using T = std::ranges::range_value_t<Range>;

    // Убираем возможную ссылку из типа GEN для корректной подстановки в шаблон
    ReservoirSampler<T, std::remove_cvref_t<GEN>> sampler{ size, rng };

    for(const auto& item : range)
    {
        sampler.processItem(item);
    }

    return sampler.getSelectedItems();
}

} // end mylib::random namespace




#endif // ALGORITHMS_H
