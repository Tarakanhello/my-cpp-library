#include "mylib/containers/union_find.h"

#include <cmath>
#include <iostream>
#include <format>
#include <limits>
#include <stdexcept>
#include <utility>



mylib::UnionFind::UnionFind(size_t size)
    : m_components{ size }
{
    if(static_cast<size_t>(std::numeric_limits<int>::max()) < size)
    {
        throw std::length_error(std::format("mylib::UnionFind: size ({}) must be <= {}", size, std::numeric_limits<int>::max()));
    }

    m_parent = Vector(size, -1);
}



void mylib::UnionFind::add()
{
    if(m_parent.size() >= static_cast<size_t>(std::numeric_limits<int>::max()))
    {
        throw std::length_error(std::format("mylib::UnionFind::add(): size would exceed INT_MAX ({})", std::numeric_limits<int>::max()));
    }

    m_parent.push_back(-1);
    ++m_components;
}



size_t mylib::UnionFind::components() const noexcept
{
    return m_components;
}



size_t mylib::UnionFind::count() const noexcept
{
    return m_parent.size();
}



void mylib::UnionFind::debug() const noexcept
{
    if(m_parent.empty())
    {
        std::cout << "UnionFind is empty";
        return;
    }
    for(const auto& element : m_parent)
    {
        std::cout << std::format("[{}] ", element);
    }
}



size_t mylib::UnionFind::find(size_t x) noexcept
{
    size_t root{ rootOf(x) };

    while(x != root)
    {
        size_t next{ static_cast<size_t>(m_parent[x]) };
        m_parent[x] = static_cast<int>(root);
        x = next;
    }

    return root;
}



size_t mylib::UnionFind::peek(size_t x) const noexcept
{
    return rootOf(x);
}



size_t mylib::UnionFind::rootOf(size_t x) const noexcept
{
    size_t root{ x };
    while(m_parent[root] >= 0)
    {
        root = static_cast<size_t>(m_parent[root]);
    }

    return root;
}



std::vector<size_t> mylib::UnionFind::roots() const
{
    std::vector<size_t> answer;
    answer.reserve(components());
    for(size_t i{}; i < m_parent.size(); ++i)
    {
        if(m_parent[i] < 0)
        {
            answer.push_back(i);
        }
    }

    return answer;
}



bool mylib::UnionFind::same(size_t a, size_t b) noexcept
{
    return find(a) == find(b);
}



bool mylib::UnionFind::same(size_t a, size_t b) const noexcept
{
    return peek(a) == peek(b);
}



size_t mylib::UnionFind::size(size_t x) noexcept
{
    return static_cast<size_t>(-m_parent[find(x)]);
}



size_t mylib::UnionFind::size(size_t x) const noexcept
{
    return static_cast<size_t>(-m_parent[peek(x)]);
}



bool mylib::UnionFind::unite(size_t a, size_t b) noexcept
{
    size_t rootA{ find(a) };
    size_t rootB{ find(b) };

    if(rootA == rootB)
    {
        return false;
    }

    if(m_parent[rootA] > m_parent[rootB])
    {
        std::swap(rootA, rootB);
    }

    m_parent[rootA] += m_parent[rootB];
    m_parent[rootB] = static_cast<int>(rootA);

    --m_components;

    return true;
}
