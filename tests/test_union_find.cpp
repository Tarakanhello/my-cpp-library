#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <climits>
#include <cstdint>
#include <iostream>
#include <random>
#include <set>
#include <sstream>
#include <vector>

#include "mylib/mylib.h"

namespace
{
using DSU = mylib::UnionFind;

// Полная проверка инвариантов через публичный API.
// Вызывать после каждой мутации.
void requireDsuInvariants(const DSU& dsu, const char* ctx = "")
{
    INFO("DSU context: " << ctx);
    INFO("count() = " << dsu.count()
                      << ", components() = " << dsu.components());

    const size_t n{ dsu.count() };

    // components <= count
    REQUIRE(dsu.components() <= n);

    // roots() отсортирован по возрастанию
    const auto rs = dsu.roots();
    REQUIRE(std::is_sorted(rs.begin(), rs.end()));

    // roots().size() == components()
    REQUIRE(rs.size() == dsu.components());

    // Все элементы в [0, n)
    for (size_t r : rs) REQUIRE(r < n);

    // Сумма размеров всех множеств == count()
    size_t total{ 0 };
    for (size_t r : rs)
    {
        const size_t s = dsu.size(r);
        REQUIRE(s >= 1);
        total += s;
    }
    REQUIRE(total == n);

    // Каждый корень — «свой собственный» корень
    for (size_t r : rs)
    {
        REQUIRE(dsu.peek(r) == r);
    }

    // Все элементы указывают на один из корней
    for (size_t i = 0; i < n; ++i)
    {
        const size_t root = dsu.peek(i);
        REQUIRE(std::find(rs.begin(), rs.end(), root) != rs.end());
    }
}
} // namespace


// ============================================================================
//  UnionFind: конструктор и базовые свойства
// ============================================================================
TEST_CASE("UnionFind construction and basic properties",
          "[union_find][construction]")
{
    SECTION("ctor is explicit and requires size")
    {
        STATIC_REQUIRE(std::is_constructible_v<DSU, size_t>);
        STATIC_REQUIRE_FALSE(std::is_default_constructible_v<DSU>);
        STATIC_REQUIRE_FALSE(std::is_convertible_v<size_t, DSU>);
    }

    SECTION("empty DSU")
    {
        DSU dsu{ 0 };
        REQUIRE(dsu.count() == 0);
        REQUIRE(dsu.components() == 0);
        REQUIRE(dsu.roots().empty());
        requireDsuInvariants(dsu, "empty");
    }

    SECTION("size 1")
    {
        DSU dsu{ 1 };
        REQUIRE(dsu.count() == 1);
        REQUIRE(dsu.components() == 1);
        REQUIRE(dsu.roots() == std::vector<size_t>{ 0 });
        REQUIRE(dsu.find(0) == 0);
        REQUIRE(dsu.peek(0) == 0);
        REQUIRE(dsu.size(0) == 1);
        REQUIRE(dsu.same(0, 0));
        requireDsuInvariants(dsu, "size 1");
    }

    SECTION("size 10 — all singletons")
    {
        constexpr size_t n{ 10 };
        DSU dsu{ n };
        REQUIRE(dsu.count() == n);
        REQUIRE(dsu.components() == n);

        for (size_t i = 0; i < n; ++i)
        {
            REQUIRE(dsu.find(i) == i);
            REQUIRE(dsu.peek(i) == i);
            REQUIRE(dsu.size(i) == 1);
            for (size_t j = 0; j < n; ++j)
                REQUIRE(dsu.same(i, j) == (i == j));
        }

        auto rs = dsu.roots();
        REQUIRE(rs.size() == n);
        for (size_t i = 0; i < n; ++i) REQUIRE(rs[i] == i);
        requireDsuInvariants(dsu, "size 10");
    }

    SECTION("size 1000")
    {
        DSU dsu{ 1000 };
        REQUIRE(dsu.count() == 1000);
        REQUIRE(dsu.components() == 1000);
        requireDsuInvariants(dsu, "size 1000");
    }

    SECTION("size > INT_MAX throws std::length_error (check is before alloc)")
    {
        const size_t tooBig{ static_cast<size_t>(INT_MAX) + 1 };
        REQUIRE_THROWS_AS(DSU{ tooBig }, std::length_error);
        REQUIRE_THROWS_AS(DSU{ tooBig + 1 }, std::length_error);
    }
}


// ============================================================================
//  UnionFind: find и peek
// ============================================================================
TEST_CASE("UnionFind find and peek", "[union_find][find][peek]")
{
    SECTION("find(x) == x and peek(x) == x initially")
    {
        DSU dsu{ 20 };
        for (size_t i = 0; i < 20; ++i)
        {
            REQUIRE(dsu.find(i) == i);
            REQUIRE(dsu.peek(i) == i);
        }
    }

    SECTION("find == peek after unites")
    {
        DSU dsu{ 10 };
        dsu.unite(0, 1);
        dsu.unite(2, 3);
        dsu.unite(0, 3);

        for (size_t i = 0; i < 10; ++i)
            REQUIRE(dsu.find(i) == dsu.peek(i));
    }

    SECTION("find is idempotent")
    {
        DSU dsu{ 10 };
        dsu.unite(0, 1);
        dsu.unite(1, 2);
        dsu.unite(2, 3);

        const size_t r1 = dsu.find(3);
        const size_t r2 = dsu.find(3);
        const size_t r3 = dsu.find(3);
        REQUIRE(r1 == r2);
        REQUIRE(r2 == r3);
    }

    SECTION("find(root) == root")
    {
        DSU dsu{ 10 };
        dsu.unite(0, 5);
        const size_t root = dsu.find(0);
        REQUIRE(dsu.find(root) == root);
        REQUIRE(dsu.peek(root) == root);
    }

    SECTION("peek is const-callable, find is not")
    {
        DSU dsu{ 5 };
        const DSU& c = dsu;
        REQUIRE(c.peek(0) == 0);
        // c.find(0); — не компилируется, что и ожидается
    }
}


// ============================================================================
//  UnionFind: unite
// ============================================================================
TEST_CASE("UnionFind unite", "[union_find][unite]")
{
    SECTION("unite returns true and decrements components")
    {
        DSU dsu{ 5 };
        REQUIRE(dsu.components() == 5);

        REQUIRE(dsu.unite(0, 1) == true);
        REQUIRE(dsu.components() == 4);
        REQUIRE(dsu.same(0, 1));

        REQUIRE(dsu.unite(2, 3) == true);
        REQUIRE(dsu.components() == 3);

        REQUIRE(dsu.unite(0, 3) == true);
        REQUIRE(dsu.components() == 2);
        requireDsuInvariants(dsu, "basic unite");
    }

    SECTION("unite already-connected returns false")
    {
        DSU dsu{ 5 };
        REQUIRE(dsu.unite(0, 1) == true);
        REQUIRE(dsu.unite(0, 1) == false);
        REQUIRE(dsu.unite(1, 0) == false);
        REQUIRE(dsu.components() == 4);
    }

    SECTION("self unite returns false")
    {
        DSU dsu{ 5 };
        for (size_t i = 0; i < 5; ++i)
            REQUIRE(dsu.unite(i, i) == false);
        REQUIRE(dsu.components() == 5);
    }

    SECTION("union by size — big absorbs small")
    {
        DSU dsu{ 10 };
        for (size_t i = 1; i < 5; ++i) dsu.unite(0, i);
        REQUIRE(dsu.size(0) == 5);

        dsu.unite(5, 6);
        REQUIRE(dsu.size(5) == 2);

        dsu.unite(0, 5);
        REQUIRE(dsu.size(0) == 7);
        REQUIRE(dsu.size(5) == 7);
        REQUIRE(dsu.find(0) == 0);           // большая группа сохранила корень
        REQUIRE(dsu.find(5) == dsu.find(0));
    }

    SECTION("unite all -> single component")
    {
        DSU dsu{ 100 };
        for (size_t i = 1; i < 100; ++i) dsu.unite(0, i);
        REQUIRE(dsu.components() == 1);
        REQUIRE(dsu.size(0) == 100);
        for (size_t i = 0; i < 100; ++i)
        {
            REQUIRE(dsu.same(0, i));
            REQUIRE(dsu.size(i) == 100);
        }
        requireDsuInvariants(dsu, "unite all");
    }

    SECTION("chain unite")
    {
        DSU dsu{ 10 };
        for (size_t i = 0; i + 1 < 10; ++i) dsu.unite(i, i + 1);
        REQUIRE(dsu.components() == 1);
        REQUIRE(dsu.size(0) == 10);
        for (size_t i = 0; i < 10; ++i) REQUIRE(dsu.same(i, 0));
    }

    SECTION("order of unites does not affect partition")
    {
        DSU a{ 5 }, b{ 5 };
        a.unite(0, 1); a.unite(2, 3); a.unite(0, 2);
        b.unite(2, 3); b.unite(0, 1); b.unite(2, 0);

        for (size_t i = 0; i < 5; ++i)
            for (size_t j = 0; j < 5; ++j)
                REQUIRE(a.same(i, j) == b.same(i, j));
    }
}


// ============================================================================
//  UnionFind: same
// ============================================================================
TEST_CASE("UnionFind same", "[union_find][same]")
{
    SECTION("reflexive")
    {
        DSU dsu{ 10 };
        const DSU& c = dsu;
        for (size_t i = 0; i < 10; ++i)
        {
            REQUIRE(dsu.same(i, i));
            REQUIRE(c.same(i, i));
        }
    }

    SECTION("symmetric")
    {
        DSU dsu{ 10 };
        dsu.unite(0, 1);
        dsu.unite(2, 3);

        REQUIRE(dsu.same(0, 1) == dsu.same(1, 0));
        REQUIRE(dsu.same(2, 3) == dsu.same(3, 2));
        REQUIRE(dsu.same(0, 2) == dsu.same(2, 0));
        REQUIRE_FALSE(dsu.same(0, 2));
    }

    SECTION("const and non-const overloads agree")
    {
        DSU dsu{ 10 };
        dsu.unite(0, 1);
        dsu.unite(1, 2);
        dsu.unite(5, 6);

        const DSU& c = dsu;
        for (size_t i = 0; i < 10; ++i)
            for (size_t j = 0; j < 10; ++j)
                REQUIRE(dsu.same(i, j) == c.same(i, j));
    }

    SECTION("transitive")
    {
        DSU dsu{ 10 };
        dsu.unite(0, 1);
        dsu.unite(1, 2);
        dsu.unite(2, 3);
        REQUIRE(dsu.same(0, 3));
        REQUIRE(dsu.same(1, 3));
        REQUIRE(dsu.same(0, 2));
    }
}


// ============================================================================
//  UnionFind: size
// ============================================================================
TEST_CASE("UnionFind size", "[union_find][size]")
{
    SECTION("singleton size is 1")
    {
        DSU dsu{ 10 };
        for (size_t i = 0; i < 10; ++i) REQUIRE(dsu.size(i) == 1);
    }

    SECTION("size grows with unite")
    {
        DSU dsu{ 10 };
        REQUIRE(dsu.size(0) == 1);
        dsu.unite(0, 1);
        REQUIRE(dsu.size(0) == 2);
        REQUIRE(dsu.size(1) == 2);
        dsu.unite(0, 2);
        REQUIRE(dsu.size(0) == 3);
        dsu.unite(3, 0);
        REQUIRE(dsu.size(0) == 4);
        REQUIRE(dsu.size(3) == 4);
    }

    SECTION("size(x) == size(find(x))")
    {
        DSU dsu{ 20 };
        dsu.unite(0, 1); dsu.unite(2, 3); dsu.unite(0, 3);
        dsu.unite(5, 6); dsu.unite(6, 7);

        for (size_t i = 0; i < 20; ++i)
            REQUIRE(dsu.size(i) == dsu.size(dsu.find(i)));
    }

    SECTION("const and non-const overloads agree")
    {
        DSU dsu{ 10 };
        dsu.unite(0, 1); dsu.unite(1, 2); dsu.unite(5, 6);

        const DSU& c = dsu;
        for (size_t i = 0; i < 10; ++i)
            REQUIRE(dsu.size(i) == c.size(i));
    }

    SECTION("sum of sizes == count")
    {
        DSU dsu{ 30 };
        dsu.unite(0, 1); dsu.unite(2, 3); dsu.unite(4, 5);
        dsu.unite(0, 5); dsu.unite(10, 11); dsu.unite(12, 13);

        size_t total = 0;
        for (size_t r : dsu.roots()) total += dsu.size(r);
        REQUIRE(total == dsu.count());
    }
}


// ============================================================================
//  UnionFind: add
// ============================================================================
TEST_CASE("UnionFind add", "[union_find][add]")
{
    SECTION("add to empty")
    {
        DSU dsu{ 0 };
        REQUIRE(dsu.count() == 0);
        REQUIRE(dsu.components() == 0);

        dsu.add();
        REQUIRE(dsu.count() == 1);
        REQUIRE(dsu.components() == 1);
        REQUIRE(dsu.find(0) == 0);
        REQUIRE(dsu.size(0) == 1);
        requireDsuInvariants(dsu, "add to empty");
    }

    SECTION("add multiple times")
    {
        DSU dsu{ 0 };
        for (size_t i = 0; i < 10; ++i)
        {
            dsu.add();
            REQUIRE(dsu.count() == i + 1);
            REQUIRE(dsu.components() == i + 1);
        }
        for (size_t i = 0; i < 10; ++i)
        {
            REQUIRE(dsu.find(i) == i);
            REQUIRE_FALSE(dsu.same(i, (i + 1) % 10));
        }
    }

    SECTION("add after unites")
    {
        DSU dsu{ 5 };
        dsu.unite(0, 1);
        dsu.unite(1, 2);
        REQUIRE(dsu.components() == 3);

        dsu.add();
        REQUIRE(dsu.count() == 6);
        REQUIRE(dsu.components() == 4);
        REQUIRE(dsu.find(5) == 5);
        REQUIRE(dsu.size(5) == 1);
        REQUIRE_FALSE(dsu.same(0, 5));

        dsu.unite(0, 5);
        REQUIRE(dsu.same(0, 5));
        REQUIRE(dsu.size(0) == 4);
        requireDsuInvariants(dsu, "add after unite");
    }

    SECTION("add preserves existing structure")
    {
        DSU dsu{ 3 };
        dsu.unite(0, 1);
        const size_t r = dsu.find(0);

        dsu.add();
        dsu.add();
        REQUIRE(dsu.find(0) == r);
        REQUIRE(dsu.same(0, 1));
    }
}


// ============================================================================
//  UnionFind: roots
// ============================================================================
TEST_CASE("UnionFind roots", "[union_find][roots]")
{
    SECTION("empty -> empty")
    {
        DSU dsu{ 0 };
        REQUIRE(dsu.roots().empty());
    }

    SECTION("all singletons -> all indices, ascending")
    {
        constexpr size_t n{ 20 };
        DSU dsu{ n };
        auto rs = dsu.roots();
        REQUIRE(rs.size() == n);
        for (size_t i = 0; i < n; ++i) REQUIRE(rs[i] == i);
    }

    SECTION("ascending order after arbitrary unites")
    {
        DSU dsu{ 10 };
        dsu.unite(9, 0);
        dsu.unite(8, 7);
        dsu.unite(3, 4);

        auto rs = dsu.roots();
        REQUIRE(std::is_sorted(rs.begin(), rs.end()));
        REQUIRE(rs.size() == dsu.components());
    }

    SECTION("every element points to a listed root")
    {
        DSU dsu{ 20 };
        dsu.unite(0, 1); dsu.unite(1, 2);
        dsu.unite(5, 6); dsu.unite(10, 11);

        auto rs = dsu.roots();
        std::set<size_t> rootSet(rs.begin(), rs.end());

        for (size_t i = 0; i < 20; ++i)
            REQUIRE(rootSet.count(dsu.find(i)) == 1);
    }
}


// ============================================================================
//  UnionFind: debug
// ============================================================================
TEST_CASE("UnionFind debug", "[union_find][debug]")
{
    SECTION("debug doesn't throw, prints something")
    {
        DSU empty{ 0 };
        std::ostringstream oss;
        auto* oldBuf = std::cout.rdbuf(oss.rdbuf());
        empty.debug();
        std::cout.rdbuf(oldBuf);
        REQUIRE_FALSE(oss.str().empty());

        DSU dsu{ 5 };
        dsu.unite(0, 1);
        std::ostringstream oss2;
        oldBuf = std::cout.rdbuf(oss2.rdbuf());
        dsu.debug();
        std::cout.rdbuf(oldBuf);
        REQUIRE_FALSE(oss2.str().empty());
    }
}


// ============================================================================
//  UnionFind: noexept / traits
// ============================================================================
TEST_CASE("UnionFind noexcept contract", "[union_find][traits]")
{
    STATIC_REQUIRE(noexcept(std::declval<DSU&>().find(0)));
    STATIC_REQUIRE(noexcept(std::declval<const DSU&>().peek(0)));
    STATIC_REQUIRE(noexcept(std::declval<DSU&>().unite(0, 0)));
    STATIC_REQUIRE(noexcept(std::declval<DSU&>().same(0, 0)));
    STATIC_REQUIRE(noexcept(std::declval<const DSU&>().same(0, 0)));
    STATIC_REQUIRE(noexcept(std::declval<DSU&>().size(0)));
    STATIC_REQUIRE(noexcept(std::declval<const DSU&>().size(0)));
    STATIC_REQUIRE(noexcept(std::declval<const DSU&>().count()));
    STATIC_REQUIRE(noexcept(std::declval<const DSU&>().components()));
    STATIC_REQUIRE(noexcept(std::declval<const DSU&>().debug()));
}


// ============================================================================
//  UnionFind: property-based / stress
// ============================================================================
TEST_CASE("UnionFind randomized stress", "[union_find][stress][property]")
{
    std::mt19937_64 rng{ 0xC0FFEEULL };

    SECTION("random unites preserve invariants and components tracking")
    {
        constexpr size_t n{ 100 };
        DSU dsu{ n };
        std::uniform_int_distribution<size_t> idx{ 0, n - 1 };

        for (size_t iter = 0; iter < 500; ++iter)
        {
            const size_t a = idx(rng);
            const size_t b = idx(rng);
            const size_t compBefore = dsu.components();
            const bool merged = dsu.unite(a, b);
            const size_t compAfter = dsu.components();

            INFO("iter = " << iter << ", a = " << a << ", b = " << b);
            if (merged) REQUIRE(compAfter == compBefore - 1);
            else        REQUIRE(compAfter == compBefore);

            requireDsuInvariants(dsu, "random unites");
        }
        REQUIRE(dsu.components() >= 1);
    }

    SECTION("matches reference implementation on small n")
    {
        constexpr size_t n{ 30 };
        DSU dsu{ n };
        std::uniform_int_distribution<size_t> idx{ 0, n - 1 };

        std::vector<std::set<size_t>> ref;
        for (size_t i = 0; i < n; ++i) ref.push_back({ i });

        auto findSetRef = [&](size_t x) -> size_t
        {
            for (size_t i = 0; i < ref.size(); ++i)
                if (ref[i].count(x)) return i;
            return SIZE_MAX;
        };

        for (size_t iter = 0; iter < 200; ++iter)
        {
            const size_t a = idx(rng);
            const size_t b = idx(rng);
            const bool merged = dsu.unite(a, b);

            size_t sa = findSetRef(a), sb = findSetRef(b);
            const bool refMerged = (sa != sb);
            REQUIRE(merged == refMerged);

            if (refMerged)
            {
                if (ref[sa].size() < ref[sb].size()) std::swap(sa, sb);
                for (size_t x : ref[sb]) ref[sa].insert(x);
                ref.erase(ref.begin() + static_cast<std::ptrdiff_t>(sb));
            }

            for (size_t i = 0; i < n; ++i)
                for (size_t j = 0; j < n; ++j)
                    REQUIRE(dsu.same(i, j) == (findSetRef(i) == findSetRef(j)));

            requireDsuInvariants(dsu, "vs reference");
        }
    }

    SECTION("interleaved add and unite")
    {
        DSU dsu{ 0 };
        std::uniform_int_distribution<int> op{ 0, 1 };

        for (size_t iter = 0; iter < 200; ++iter)
        {
            if (dsu.count() == 0 || op(rng) == 0)
            {
                dsu.add();
            }
            else
            {
                std::uniform_int_distribution<size_t> idx{ 0, dsu.count() - 1 };
                dsu.unite(idx(rng), idx(rng));
            }
            requireDsuInvariants(dsu, "interleaved");
        }
    }

    SECTION("deep chain + path compression does not corrupt state")
    {
        // Последовательная цепочка 0-1-2-...-N, затем запросы
        constexpr size_t n{ 2000 };
        DSU dsu{ n };
        for (size_t i = 0; i + 1 < n; ++i) dsu.unite(i, i + 1);
        REQUIRE(dsu.components() == 1);

        // Много find-запросов — должны остаться консистентны
        for (size_t i = 0; i < n; ++i)
            REQUIRE(dsu.find(i) == dsu.find(0));

        requireDsuInvariants(dsu, "deep chain");
    }
}


// ============================================================================
//  UnionFind: Kruskal-like сценарии
// ============================================================================
TEST_CASE("UnionFind Kruskal-like usage", "[union_find][real-world]")
{
    SECTION("classic Kruskal MST weight")
    {
        struct Edge { size_t u, v; int w; };
        std::vector<Edge> edges{
            { 0, 1, 1 }, { 1, 2, 2 }, { 0, 2, 3 }, { 2, 3, 4 }, { 3, 4, 5 }
        };
        std::sort(edges.begin(), edges.end(),
                  [](const Edge& a, const Edge& b){ return a.w < b.w; });

        DSU dsu{ 5 };
        int totalWeight = 0;
        size_t usedEdges = 0;

        for (const auto& e : edges)
        {
            if (dsu.unite(e.u, e.v))
            {
                totalWeight += e.w;
                ++usedEdges;
            }
        }

        REQUIRE(usedEdges == 4);        // spanning tree из 5 вершин
        REQUIRE(dsu.components() == 1);
        REQUIRE(totalWeight == 12);     // 1 + 2 + 4 + 5
        REQUIRE(dsu.size(0) == 5);
    }

    SECTION("cycle detection")
    {
        DSU dsu{ 4 };
        REQUIRE(dsu.unite(0, 1));
        REQUIRE(dsu.unite(1, 2));
        REQUIRE(dsu.unite(2, 3));
        REQUIRE_FALSE(dsu.unite(3, 0)); // цикл
        REQUIRE(dsu.components() == 1);
    }

    SECTION("connected components count")
    {
        // Граф: 0-1, 2-3-4, 5 изолирован
        DSU dsu{ 6 };
        dsu.unite(0, 1);
        dsu.unite(2, 3);
        dsu.unite(3, 4);
        // 5 сам по себе
        REQUIRE(dsu.components() == 3);

        REQUIRE(dsu.same(0, 1));
        REQUIRE(dsu.same(2, 4));
        REQUIRE_FALSE(dsu.same(0, 2));
        REQUIRE_FALSE(dsu.same(5, 0));
        REQUIRE(dsu.size(0) == 2);
        REQUIRE(dsu.size(2) == 3);
        REQUIRE(dsu.size(5) == 1);
    }
}
