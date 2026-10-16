#ifndef UNION_FIND_H
#define UNION_FIND_H

#include <vector>

#include "vector.h"

namespace mylib
{

/**
 * @brief Disjoint Set Union (Union-Find) structure.
 *
 * Maintains a partition of elements into disjoint sets, supporting:
 *   - finding the representative (root) of an element's set;
 *   - merging two sets;
 *   - checking whether two elements belong to the same set;
 *   - querying the size of an element's set.
 *
 * Internally stores a single array `m_parent` of `int`:
 *   - `m_parent[i] < 0` — i is a root, and `-m_parent[i]` is the size of its set;
 *   - `m_parent[i] >= 0` — i is a non-root, and `m_parent[i]` is the index of
 *     its parent.
 *
 * Two elements belong to the same set iff their roots are equal.
 *
 * @note Invariants:
 *       - `count() <= INT_MAX` (checked at construction).
 *       - `m_components` always equals the number of negative entries in
 *         `m_parent` (i.e. the number of disjoint sets).
 *       - For every non-root i, the chain `i -> m_parent[i] -> ...` terminates
 *         at a root whose entry is negative.
 *
 * @note This class is NOT thread-safe. Concurrent calls to `find`, `unite`,
 *       or any non-const method cause a data race. Even const methods must
 *       not run concurrently with any non-const method.
 */
class UnionFind final
{
private:
    using Vector = mylib::Vector<int>;

    Vector m_parent{};          ///< Parent array (see class description).
    size_t m_components{};      ///< Number of disjoint sets currently.

    // ================================================================
    //  Internal helpers
    // ================================================================

    /**
     * @brief Walks up the parent chain to the root of x.
     *
     * Does not perform path compression and does not modify the structure.
     *
     * @param x Element index.
     * @return Index of the root of x.
     * @pre 0 <= x < count().
     */
    size_t rootOf(size_t x) const noexcept;

public:
    // ================================================================
    //  Construction
    // ================================================================

    /**
     * @brief Constructs a DSU of n singleton elements.
     *
     * Each element is its own set; the number of components equals n.
     *
     * @param size Number of elements. Must not exceed INT_MAX.
     *
     * @throw std::length_error if size > INT_MAX.
     *
     * @note After the call, `count() == size` and `components() == size`.
     */
    explicit UnionFind(size_t size);

    // ================================================================
    //  Find and lookups
    // ================================================================

    /**
     * @brief Finds the root of the set containing x, with path compression.
     *
     * Flattens the path from x to the root so that subsequent lookups are faster.
     * This method modifies the internal structure; it is not const.
     *
     * @param x Element index.
     * @return Index of the root of x's set.
     * @pre 0 <= x < count().
     *
     * @note This is the "fast" version: it changes the tree, but makes later
     *       calls cheaper. For a pure read-only lookup use peek().
     *
     * @see peek
     * @see same
     */
    size_t find(size_t x) noexcept;

    /**
     * @brief Finds the root of the set containing x, without path compression.
     *
     * A read-only lookup. Does not modify the structure, so it can be called
     * on a const object and is safe to use alongside const overloads.
     *
     * @param x Element index.
     * @return Index of the root of x's set.
     * @pre 0 <= x < count().
     *
     * @note Slower than find() if called repeatedly on the same element,
     *       because it does not flatten the path.
     *
     * @see find
     * @see same
     */
    size_t peek(size_t x) const noexcept;

    // ================================================================
    //  Union
    // ================================================================

    /**
     * @brief Merges the sets containing a and b.
     *
     * Uses union-by-size: the smaller set is attached under the larger one,
     * so the resulting tree stays shallow. Decrements components() on success.
     *
     * @param a First element.
     * @param b Second element.
     * @return true  if a and b were in different sets and the sets were merged;
     *         false if a and b were already in the same set (no change).
     * @pre 0 <= a, b < count().
     *
     * @note This is the primary operation used in Kruskal-like algorithms:
     *       `if (dsu.unite(u, v)) { ... }` — the branch is taken only when
     *       the edge actually joins two different components.
     *
     * @see same
     * @see components
     */
    bool unite(size_t a, size_t b) noexcept;

    // ================================================================
    //  Queries
    // ================================================================

    /**
     * @brief Checks whether a and b belong to the same set.
     *
     * Uses find(), so it performs path compression.
     *
     * @param a First element.
     * @param b Second element.
     * @return true if a and b are in the same set.
     * @pre 0 <= a, b < count().
     *
     * @see same(size_t, size_t) const
     * @see find
     */
    bool same(size_t a, size_t b) noexcept;

    /**
     * @brief Checks whether a and b belong to the same set (read-only).
     *
     * Uses peek(), so it does not modify the structure and can be called on
     * a const object.
     *
     * @param a First element.
     * @param b Second element.
     * @return true if a and b are in the same set.
     * @pre 0 <= a, b < count().
     *
     * @see same(size_t, size_t)
     * @see peek
     */
    bool same(size_t a, size_t b) const noexcept;

    /**
     * @brief Returns the size of the set containing x.
     *
     * Uses find(), so it performs path compression.
     *
     * @param x Element index.
     * @return Number of elements in x's set.
     * @pre 0 <= x < count().
     *
     * @see size(size_t) const
     */
    size_t size(size_t x) noexcept;

    /**
     * @brief Returns the size of the set containing x (read-only).
     *
     * Uses peek(), so it does not modify the structure.
     *
     * @param x Element index.
     * @return Number of elements in x's set.
     * @pre 0 <= x < count().
     *
     * @see size(size_t)
     */
    size_t size(size_t x) const noexcept;

    /**
     * @brief Returns the total number of elements.
     */
    size_t count() const noexcept;

    /**
     * @brief Returns the current number of disjoint sets.
     *
     * Maintained incrementally by add() and unite(); O(1).
     */
    size_t components() const noexcept;

    // ================================================================
    //  Insertion
    // ================================================================

    /**
     * @brief Adds a new singleton element to the DSU.
     *
     * The new element gets index count() (old value) and forms its own set.
     * Increments components().
     *
     */
    void add();

    // ================================================================
    //  Enumeration
    // ================================================================

    /**
     * @brief Returns the indices of all current roots (one per set).
     *
     * The order of the returned indices is ascending.
     *
     * @return std::vector<size_t> containing one representative per set.
     *         Its size equals components().
     *
     * @note This is a diagnostic helper; the same information can be obtained
     *       by scanning m_parent for negative entries.
     */
    std::vector<size_t> roots() const;

    // ================================================================
    //  Diagnostics
    // ================================================================

    /**
     * @brief Prints the internal parent array to std::cout.
     *
     * Format: each element in brackets, e.g. `[-3] [0] [0] [2] `.
     * Root entries are printed as their negative size; non-root entries as
     * their parent index.
     *
     * @note For debugging only.
     */
    void debug() const noexcept;
};

} // end namespace

#endif // UNION_FIND_H
