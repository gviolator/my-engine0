#pragma once
#include <iterator>

#include "my/diag/check.h"

namespace my
{

    template <typename T>
    class IntrusiveList;

    template <typename T>
    class IntrusiveListNode;

    /// <summary>
    /// Represents intrusive list: where each element stored outside of the list, but is an IntrusiveListNode.
    /// </summary>
    template <typename T>
    class IntrusiveList
    {
    public:
        using Node = IntrusiveListNode<T>;
        using value_type = T;
        using reference = T&;
        using const_reference = const T&;
        using pointer = T*;
        using const_pointer = const T*;

        struct iterator
        {
            using iterator_category = std::bidirectional_iterator_tag;
            using value_type = T;
            using difference_type = ptrdiff_t;
            using pointer = T*;
            using reference = T&;

            Node* node;

            iterator(Node* node_ = nullptr) :
                node(node_)
            {
            }

            iterator& operator++()
            {
                MY_DEBUG_CHECK(node != nullptr, "can not be increment iterator");

                node = node->m_next;

                return *this;
            }

            iterator operator++(int)
            {
                MY_DEBUG_CHECK(node != nullptr, "can not be increment iterator");

                iterator retIter(node);

                node = node->m_next;

                return retIter;
            }

            iterator& operator--()
            {
                MY_DEBUG_CHECK(node != nullptr, "can not be decrement iterator");

                MY_DEBUG_CHECK(node->m_prev != nullptr, "can not be decrement iterator");

                node = node->m_prev;

                return *this;
            }

            iterator operator--(int)
            {
                MY_DEBUG_CHECK(node != nullptr, "can not be decrement iterator");

                MY_DEBUG_CHECK(node->m_prev != nullptr, "can not be decrement iterator");

                iterator retIter(node);

                node = node->m_prev;

                return retIter;
            }

            const_reference operator*() const
            {
                MY_DEBUG_CHECK(node != nullptr, "intrusive list iterator can not be dereferenced");

                return *static_cast<T*>(node);
            }

            reference operator*()
            {
                MY_DEBUG_CHECK(node != nullptr, "intrusive list iterator can not be dereferenced");

                return *static_cast<T*>(node);
            }

            pointer operator->()
            {
                MY_DEBUG_CHECK(node != nullptr, "intrusive list iterator can not be dereferenced");

                return static_cast<T*>(node);
            }

            const_pointer operator->() const
            {
                MY_DEBUG_CHECK(node != nullptr, "intrusive list iterator can not be dereferenced");

                return static_cast<T*>(node);
            }

            friend bool operator==(const iterator& iter1, const iterator& iter2)
            {
                return iter1.node == iter2.node;
            }

            friend bool operator!=(const iterator& iter1, const iterator& iter2)
            {
                return iter1.node != iter2.node;
            }
        };

        using const_iterator = iterator;

        IntrusiveList() noexcept;

        ~IntrusiveList() noexcept;

        IntrusiveList(const IntrusiveList&) = delete;

        IntrusiveList(IntrusiveList&&);

        IntrusiveList& operator=(const IntrusiveList&) = delete;

        IntrusiveList& operator=(IntrusiveList&&);

        /**
            @brief Clear intrusive list.
        */
        void clear() noexcept;

        /// <summary>
        /// Compute size of the linked list.
        /// </summary>
        /// <returns>How many elements contains within list.</returns>
        size_t size() const noexcept;

        /// <summary>
        /// MY_DEBUG_CHECK that list is empty.
        /// </summary>
        /// <returns>true if list contains no element, false otherwise</returns>
        bool empty() const noexcept;

        /// <summary>
        ///
        /// </summary>
        /// <returns>Reference to the last list element</returns>
        reference back();

        /// <summary>
        ///
        /// </summary>
        /// <returns>Constant reference to the last list element</returns>
        const_reference back() const;

        /// <summary>
        ///
        /// </summary>
        /// <returns>Constant reference to the first list element</returns>
        reference front();

        /// <summary>
        ///
        /// </summary>
        /// <returns>Constant reference to the first list element</returns>
        const_reference front() const;

        /// <summary>
        /// Place element at the end of the list.
        /// </summary>
        /// <param name="element">Inserted element</param>
        void push_back(T& element);

        /// <summary>
        /// Place element at the front of the list.
        /// </summary>
        /// <param name="element">Inserted element</param>
        void push_front(T& element);

        /// <summary>
        /// Insert value before pos.
        /// </summary>
        /// <param name="pos">Where to insert</param>
        /// <param name="node">Inserted element</param>
        /// <returns>Iterator pointing to the inserted value.</returns>
        iterator insert(iterator pos, T& element);

        /// <summary>
        ///
        /// </summary>
        /// <param name="first"></param>
        /// <param name="last"></param>
        /// <returns></returns>
        iterator erase(iterator first, iterator last);

        /// <summary>
        /// Removes specified element (referenced by iterator) from the list.
        /// </summary>
        /// <param name="pos">Removed element (iterator).</param>
        /// <returns>Iterator following the last removed element. If the iterator pos refers to the last element, the end() iterator is returned. </returns>
        iterator erase(iterator pos);

        /// <summary>
        ///
        /// </summary>
        /// <param name="element"></param>
        void removeElement(T& element);

        bool contains(const T& element) const;

        iterator begin();

        iterator end();

        const_iterator begin() const;

        const_iterator end() const;

        iterator toIterator(T& element);

        const_iterator toConstIterator(const T& element) const;

    private:
        static Node& asFreeNode(T& element);

        void moveFrom(IntrusiveList&& list);

        Node* m_head = nullptr;
        Node* m_tail = nullptr;
    };

    /// <summary>
    /// In order for an instance of a class to be placed in an intrusive list, it must inherit from IntrusiveListNode.
    /// </summary>
    template <typename T>
    class IntrusiveListNode
    {
    protected:
        ~IntrusiveListNode()
        {
            static_assert(std::is_base_of_v<IntrusiveListNode<T>, T>, "Type is not IntrusiveListNode");

            if (m_list)
            {
                m_list->removeElement(static_cast<T&>(*this));
            }
        }

    private:
        void clearListElementState()
        {
            m_list = nullptr;
            m_prev = nullptr;
            m_next = nullptr;
        }

        IntrusiveList<T>* m_list = nullptr;
        IntrusiveListNode* m_prev = nullptr;
        IntrusiveListNode* m_next = nullptr;

        template <typename /*, typename*/>
        friend class IntrusiveList;
    };

    //-----------------------------------------------------------------------------
    template <typename T>
    IntrusiveList<T>::IntrusiveList() noexcept
    {
        static_assert(std::is_base_of_v<IntrusiveListNode<T>, T>, "Type is not IntrusiveListNode");
    }

    template <typename T>
    void IntrusiveList<T>::moveFrom(IntrusiveList&& list)
    {
        if (&list == this)
        {
            return;
        }

        clear();

        MY_DEBUG_CHECK(m_head == nullptr && m_tail == nullptr);

        std::swap(m_head, list.m_head);
        std::swap(m_tail, list.m_tail);

        auto node = m_head;

        while (node)
        {
            node->m_list = this;
            node = node->m_next;
        }
    }

    template <typename T>
    IntrusiveList<T>::IntrusiveList(IntrusiveList&& list)
    {
        moveFrom(std::move(list));
    }

    template <typename T>
    IntrusiveList<T>& IntrusiveList<T>::operator=(IntrusiveList&& list)
    {
        moveFrom(std::move(list));

        return *this;
    }

    template <typename T>
    IntrusiveList<T>::~IntrusiveList() noexcept
    {
        clear();
    }

    template <typename T>
    void IntrusiveList<T>::clear() noexcept
    {
        auto node = m_tail;

        while (node != nullptr)
        {
            auto prev = node->m_prev;
            node->clearListElementState();
            node = prev;
        }

        m_head = nullptr;
        m_tail = nullptr;
    }

    template <typename T>
    size_t IntrusiveList<T>::size() const noexcept
    {
        size_t counter = 0;
        auto node = m_head;

        while (node != nullptr)
        {
            ++counter;
            node = node->m_next;
        }

        return counter;
    }

    template <typename T>
    bool IntrusiveList<T>::empty() const noexcept
    {
        return m_head == nullptr;
    }

    template <typename T>
    typename IntrusiveList<T>::reference IntrusiveList<T>::back()
    {
        MY_DEBUG_CHECK(m_tail != nullptr, "list is empty (or invalid state)");

        return *static_cast<T*>(m_tail);
    }

    template <typename T>
    typename IntrusiveList<T>::const_reference IntrusiveList<T>::back() const
    {
        MY_DEBUG_CHECK(m_tail != nullptr, "list is empty (or invalid state)");

        return *static_cast<const T*>(m_tail);
    }

    template <typename T>
    typename IntrusiveList<T>::reference IntrusiveList<T>::front()
    {
        MY_DEBUG_CHECK(m_head != nullptr, "list is empty (or invalid state)");

        return *static_cast<T*>(m_head);
    }

    template <typename T>
    typename IntrusiveList<T>::const_reference IntrusiveList<T>::front() const
    {
        MY_DEBUG_CHECK(m_head != nullptr, "list is empty (or invalid state)");

        return *static_cast<const T*>(m_head);
    }

    template <typename T>
    void IntrusiveList<T>::push_back(T& element)
    {
        Node& node = asFreeNode(element);

        node.m_list = this;

        if (m_head == nullptr)
        {
            MY_DEBUG_CHECK(m_tail == nullptr);

            m_head = &node;
            m_tail = m_head;
        }
        else
        {
            MY_DEBUG_CHECK(m_tail != nullptr);

            m_tail->m_next = &node;

            node.m_prev = m_tail;
            m_tail = &node;
        }
    }

    template <typename T>
    void IntrusiveList<T>::push_front(T& element)
    {
        [[maybe_unused]] auto _ = insert(this->begin(), element);
    }

    template <typename T>
    typename IntrusiveList<T>::iterator IntrusiveList<T>::insert(iterator pos, T& element)
    {
        MY_DEBUG_CHECK(pos.node == nullptr || pos.node->m_list == this, "Invalid iterator");

        if (pos.node == nullptr)
        {  // end
            push_back(element);

            return iterator{m_tail};
        }

        Node& node = asFreeNode(element);

        node.m_list = this;

        node.m_next = pos.node;

        node.m_prev = pos.node->m_prev;

        if (node.m_prev != nullptr)
        {
            node.m_prev->m_next = &node;
        }

        pos.node->m_prev = &node;

        if (pos.node == m_head)
        {
            m_head = &node;
        }

        return iterator{&node};
    }

    template <typename T>
    typename IntrusiveList<T>::iterator IntrusiveList<T>::erase(iterator first, iterator last)
    {
        MY_DEBUG_CHECK(first.node != nullptr, "Iterator can not be dereferenced");

        MY_DEBUG_CHECK(first.node->m_list == this, "Invalid list reference. Possible element pointed by iterator already removed from list.");

        if (last == end())
        {
            // remove all form first to end
            iterator nextAfterRemoved = first;

            do
            {
                nextAfterRemoved = erase(nextAfterRemoved);
            } while (nextAfterRemoved != end());

            return end();
        }
        else
        {
            // MY_DEBUG_CHECK than the last is belong to this list
            MY_DEBUG_CHECK(last.node != nullptr, "Iterator can not be dereferenced");

            MY_DEBUG_CHECK(last.node->m_list == this, "Invalid list reference. Possible element pointed by iterator already removed from list.");

            // MY_DEBUG_CHECK that the last stays after the first
            bool lastStaysAfterFirst = false;

            iterator firstPP = first;
            while (++firstPP != end())
            {
                if (firstPP == last)
                {
                    lastStaysAfterFirst = true;
                    break;
                }
            }
            MY_DEBUG_CHECK(lastStaysAfterFirst, "The last iterator doesn't stay before the first one");

            // remove all elements from the first to the last (the last must stay in the list)
            iterator nextAfterRemoved = first;

            do
            {
                nextAfterRemoved = erase(nextAfterRemoved);
            } while (nextAfterRemoved != last);

            return last;
        }
    }

    template <typename T>
    typename IntrusiveList<T>::iterator IntrusiveList<T>::erase(iterator pos)
    {
        MY_DEBUG_CHECK(pos.node != nullptr, "Iterator can not be dereferenced");
        MY_DEBUG_CHECK(pos.node->m_list == this, "Invalid list reference. Possible element pointed by iterator already removed from list.");

        auto prev = pos.node->m_prev;
        auto next = pos.node->m_next;

        if (prev != nullptr)
        {
            MY_DEBUG_CHECK(prev->m_next == pos.node);

            prev->m_next = next;
        }
        else
        {  // head is erased
            m_head = next;
        }

        if (next != nullptr)
        {
            MY_DEBUG_CHECK(next->m_prev == pos.node);

            next->m_prev = prev;
        }
        else
        {  // tail is erased
            m_tail = prev;
        }

        pos.node->clearListElementState();

        pos.node = nullptr;

        return iterator{next};
    }

    template <typename T>
    void IntrusiveList<T>::removeElement(T& element)
    {
        auto& node = static_cast<Node&>(element);

        [[maybe_unused]] auto _ = erase(iterator{&node});
    }

    template <typename T>
    typename IntrusiveList<T>::iterator IntrusiveList<T>::begin()
    {
        return iterator{m_head};
    }

    template <typename T>
    typename IntrusiveList<T>::iterator IntrusiveList<T>::end()
    {
        return iterator{};
    }

    template <typename T>
    typename IntrusiveList<T>::const_iterator IntrusiveList<T>::begin() const
    {
        return const_iterator{m_head};
    }

    template <typename T>
    typename IntrusiveList<T>::const_iterator IntrusiveList<T>::end() const
    {
        return const_iterator{};
    }

    template <typename T>
    typename IntrusiveList<T>::Node& IntrusiveList<T>::asFreeNode(T& element)
    {
        auto& node = static_cast<Node&>(element);

        MY_DEBUG_CHECK(node.m_next == nullptr && node.m_prev == nullptr && node.m_list == nullptr, "Node already within list");

        return node;
    }

    template <typename T>
    bool IntrusiveList<T>::contains(const T& element) const
    {
        return static_cast<const IntrusiveListNode<T>&>(element).m_list == this;
    }

}  // namespace my

namespace std
{

    template <typename T>
    decltype(auto) begin(my::IntrusiveList<T>& container)
    {
        return container.begin();
    }

    template <typename T>
    decltype(auto) begin(const my::IntrusiveList<T>& container)
    {
        return container.begin();
    }

    template <typename T>
    decltype(auto) end(my::IntrusiveList<T>& container)
    {
        return container.end();
    }

    template <typename T>
    decltype(auto) end(const my::IntrusiveList<T>& container)
    {
        return container.end();
    }

}  // namespace std
