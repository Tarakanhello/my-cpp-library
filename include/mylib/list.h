#ifndef LIST_H
#define LIST_H

#include <cassert>
#include <cstddef>
#include <limits>



namespace mylib
{

template<typename T>
class List final
{
private:
    struct Node
    {
        T item;
        Node* next;
        Node* prev;

        template<typename ARGUMENT>
        Node(const ARGUMENT& theItem)
            : item{ theItem }
            , next{ nullptr }
            , prev{ nullptr }
        {}
    };

    Node* m_root{ nullptr };
    Node* m_last{ nullptr };

    template<typename ARGUMENT>
    void append(const ARGUMENT& a)
    {
        Node* n{ new Node{ a } };
        n->prev = m_last;
        if(m_last)
        {
            m_last->next = n;
        }

        m_last = n;

        if(!m_root)
        {
            m_root = n;
        }
    }

    void cut(Node* n) // отвязка узла из списка, соединение предыдущего и следующего
    {
        assert(n);

        (n == m_last ? m_last : n->next->prev) = n->prev;
        (n == m_root ? m_root : n->prev->next) = n->next;

    }

public:
    constexpr List() : m_root{ nullptr }, m_last{ nullptr } {}

    List(const List& other)
    {
        for(Node* n{ other.m_root }; n; n = n->next)
        {
            append(n->item);
        }
    }

    List& operator=(const List& other)
    {
        if(this != &other)
        {
            while(m_root)
            {
                Node* toBeDeleted{ m_root };
                m_root = m_root->next;

                delete toBeDeleted;
            }

            for(Node* n{ other.m_root }; n; n = n->next)
            {
                append(n->item);
            }
        }

        return *this;
    }

    ~List() noexcept
    {
        while(m_root)
        {
            Node* toBeDeleted{ m_root };
            m_root = m_root->next;

            delete toBeDeleted;
        }
    }

    class Iterator
    {
    private:
        Node* m_current;

    public:
        Iterator(Node* n) : m_current{ n } {}

        Node* getNode() { return m_current; }

        Iterator& operator++()
        {
            assert(m_current);

            m_current = m_current->next;

            return *this;
        }

        Iterator& operator--()
        {
            assert(m_current);

            m_current = m_current->prev;

            return *this;
        }

        T& operator*() const
        {
            assert(m_current);
            return m_current->item;
        }

        T* operator->() const
        {
            assert(m_current);
            return &m_current->item;
        }

        bool operator ==(const Iterator& other) const
        {
            return m_current == other.m_current;
        }
    }; // end class Iterator

    Iterator begin(){ return Iterator{ m_root }; }
    Iterator rBegin(){ return Iterator{ m_last }; }
    Iterator end(){ return Iterator{ nullptr }; }
    Iterator rEnd(){ return Iterator{ nullptr }; }


    void moveBefore(Iterator what, Iterator where)
    {
        assert(what != end());

        if(what != where)
        {
            Node* n{ what.getNode() };
            Node* w{ where.getNode() };

            cut(n);
            n->next = w;

            if(w)
            {
                n->prev = w->prev;
                w->prev = n;
            }
            else
            {
                n->prev = m_last;
                m_last = n;
            }

            if(n->prev)
            {
                n->prev->next = n;
            }
            if(w == m_root)
            {
                m_root = n;
            }
        }
    }

    template<typename ARGUMENT>
    void prepend(const ARGUMENT& a)
    {
        append(a);
        moveBefore(rBegin(), begin());
    }

    void remove(Iterator what)
    {
        assert(what != end());

        cut(what.getNode());
        delete what.getNode();
    }

};

} // end namespace
#endif // LIST_H
