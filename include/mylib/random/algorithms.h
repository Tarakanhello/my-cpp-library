#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include <mutex>
#include <random>
#include <ranges>

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




template<typename T, std::uniform_random_bit_generator GEN>
class ReservoirSampler
{
private:
    size_t m_size{};
    size_t m_processed{};
    Vector<T> m_reservoir;
    GEN& m_gen{};

    mutable std::mutex m_mutex;

public:
    ReservoirSampler() = delete;
    ReservoirSampler(const ReservoirSampler&) = delete;
    ReservoirSampler& operator=(const ReservoirSampler&) = delete;

    ReservoirSampler(size_t size, GEN& gen)
        : m_size{ size }
        , m_processed{ 0 }
        , m_gen{ gen }
    {
        m_reservoir.reserve(size);
    }

    size_t size() const noexcept { return m_size; }
    size_t processed() const noexcept
    {
        std::lock_guard<std::mutex> lock{ m_mutex };
        return m_processed;
    }

    Vector<T> getSelectedItems() const
    {
        std::lock_guard<std::mutex> lock{ m_mutex };
        return m_reservoir;
    }

    void processItem(T item)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        ++m_processed;
        if(m_reservoir.size() < m_size)
        {
            m_reservoir.push_back(std::move(item));
        }
        else
        {
            std::uniform_int_distribution<size_t> index{ 0, m_processed - 1 };
            size_t kickedOut{ index(m_gen) };
            if(kickedOut < m_size)
            {
                m_reservoir[kickedOut] = std::move(item);
            }
        }

    }
};

template<std::ranges::input_range Range, std::uniform_random_bit_generator GEN>
Vector<std::ranges::range_value_t<Range>> reservourSample(const Range& range, std::size_t size, GEN& rng)
{
    using T = std::ranges::range_value_t<Range>;

    ReservoirSampler<T, std::remove_reference_t<GEN>> sampler{ size, rng };

    for(const auto& item : range)
    {
        sampler.processItem(item);
    }

    return sampler.getSelectedItems();
}

} // end mylib::random namespace




#endif // ALGORITHMS_H
