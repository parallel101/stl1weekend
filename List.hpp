#pragma once

#include <cstddef>
#include <iterator>
#include <memory>
#include <limits>
#include <algorithm>
#include <utility>
#include <initializer_list>
#include "_Common.hpp"

template <class T>
struct ListBaseNode {
    ListBaseNode *m_next;
    ListBaseNode *m_prev;

    inline T &value();
    inline T const &value() const;
};

template <class T>
struct ListValueNode : ListBaseNode<T> {
    union {
        T m_value;
    };
};

template <class T>
inline T &ListBaseNode<T>::value() {
    return static_cast<ListValueNode<T> &>(*this).m_value;
}

template <class T>
inline T const &ListBaseNode<T>::value() const {
    return static_cast<ListValueNode<T> const &>(*this).m_value;
}

template <class T, class Alloc = std::allocator<T>>
struct List {
    using value_type = T;
    using allocator_type = Alloc;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using pointer = T *;
    using const_pointer = T const *;
    using reference = T &;
    using const_reference = T const &;

private:
    using ListNode = ListBaseNode<T>;
    using AllocNode = typename std::allocator_traits<Alloc>::template rebind_alloc<ListValueNode<T>>;

    ListNode m_dummy;
    std::size_t m_size;
    [[no_unique_address]] Alloc m_alloc;

    ListNode *newNode() {
        AllocNode allocNode{m_alloc};
        return std::allocator_traits<AllocNode>::allocate(allocNode, 1);
    }

    void deleteNode(ListNode *node) noexcept {
        AllocNode allocNode{m_alloc};
        std::allocator_traits<AllocNode>::deallocate(allocNode, static_cast<ListValueNode<T> *>(node));
    }

public:
    List() noexcept {
        m_size = 0;
        m_dummy.m_prev = m_dummy.m_next = &m_dummy;
    }

    explicit List(Alloc const &alloc) noexcept : m_alloc(alloc) {
        m_size = 0;
        m_dummy.m_prev = m_dummy.m_next = &m_dummy;
    }

    List(List &&that) noexcept : m_alloc(std::move(that.m_alloc)) {
        _uninit_move_assign(std::move(that));
    }

    List(List &&that, Alloc const &alloc) noexcept : m_alloc(alloc) {
        _uninit_move_assign(std::move(that));
    }

    List &operator=(List &&that) {
        m_alloc = std::move(that.m_alloc);
        clear();
        _uninit_move_assign(std::move(that));
    }

private:
    void _uninit_move_assign(List &&that) {
        auto prev = that.m_dummy.m_prev;
        auto next = that.m_dummy.m_next;
        prev->m_next = &m_dummy;
        next->m_prev = &m_dummy;
        m_dummy = that.m_dummy;
        that.m_dummy.m_prev = that.m_dummy.m_next = &that.m_dummy;
        m_size = that.m_size;
        that.m_size = 0;
    }

public:
    List(List const &that) : m_alloc(that.m_alloc) {
        _uninit_assign(that.cbegin(), that.cend());
    }

    List(List const &that, Alloc const &alloc) : m_alloc(alloc) {
        _uninit_assign(that.cbegin(), that.cend());
    }

    List &operator=(List const &that) {
        assign(that.cbegin(), that.cend());
    }

    bool empty() noexcept {
        return m_dummy.m_prev == m_dummy.m_next;
    }

    T &front() noexcept {
        return m_dummy.m_next->value();
    }

    T &back() noexcept {
        return m_dummy.m_prev->value();
    }

    T const &front() const noexcept {
        return m_dummy.m_next->value();
    }

    T const &back() const noexcept {
        return m_dummy.m_prev->value();
    }

    explicit List(size_t n, Alloc const &alloc = Alloc()) : m_alloc(alloc) {
        _uninit_assign(n);
    }

    explicit List(size_t n, T const &val, Alloc const &alloc = Alloc()) : m_alloc(alloc) {
        _uninit_assign(n, val);
    }

    // input_iterator = *it it++ ++it it!=it it==it
    // output_iterator = *it=val it++ ++it it!=it it==it
    // forward_iterator = *it *it=val it++ ++it it!=it it==it
    // bidirectional_iterator = *it *it=val it++ ++it it-- --it it!=it it==it
    // random_access_iterator = *it *it=val it[n] it[n]=val it++ ++it it-- --it it+=n it-=n it+n it-n it!=it it==it

    template <std::input_iterator InputIt>
    List(InputIt first, InputIt last, Alloc const &alloc = Alloc()) : m_alloc(alloc) {
        _uninit_assign(first, last);
    }

    List(std::initializer_list<T> ilist, Alloc const &alloc = Alloc())
    : List(ilist.begin(), ilist.end(), alloc) {}

    List &operator=(std::initializer_list<T> ilist) {
        assign(ilist);
    }

private:
    template <std::input_iterator InputIt>
    void _uninit_assign(InputIt first, InputIt last) {
        m_size = 0;
        ListNode *prev = &m_dummy;
        while (first != last) {
            ListNode *node = newNode();
            prev->m_next = node;
            node->m_prev = prev;
            std::construct_at(&node->value(), *first);
            prev = node;
            ++first;
            ++m_size;
        }
        m_dummy.m_prev = prev;
        prev->m_next = &m_dummy;
    }

    void _uninit_assign(size_t n, T const &val) {
        ListNode *prev = &m_dummy;
        while (n) {
            ListNode *node = newNode();
            prev->m_next = node;
            node->m_prev = prev;
            std::construct_at(&node->value(), val);
            prev = node;
            --n;
        }
        m_dummy.m_prev = prev;
        prev->m_next = &m_dummy;
        m_size = n;
    }

    void _uninit_assign(size_t n) {
        ListNode *prev = &m_dummy;
        while (n) {
            ListNode *node = newNode();
            prev->m_next = node;
            node->m_prev = prev;
            std::construct_at(&node->value());
            prev = node;
            --n;
        }
        m_dummy.m_prev = prev;
        prev->m_next = &m_dummy;
        m_size = n;
    }

public:
    std::size_t size() const noexcept {
        return m_size;
    }

    static constexpr std::size_t max_size() noexcept {
        return std::numeric_limits<std::size_t>::max();
    }

    template <std::input_iterator InputIt>
    void assign(InputIt first, InputIt last) {
        clear();
        _uninit_assign(first, last);
    }

    void assign(std::initializer_list<T> ilist) {
        clear();
        _uninit_assign(ilist.begin(), ilist.end());
    }

    void assign(size_t n, T const &val) {
        clear();
        _uninit_assign(n, val);
    }

    void push_back(T const &val) {
        emplace_back(val);
    }

    void push_back(T &&val) {
        emplace_back(std::move(val));
    }

    void push_front(T const &val) {
        emplace_front(val);
    }

    void push_front(T &&val) { // don't repeat yourself (DRY)
        emplace_front(std::move(val));
    }

    template <class ...Args>
    T &emplace_back(Args &&...args) {
        ListNode *node = newNode();
        ListNode *prev = m_dummy.m_prev;
        prev->m_next = node;
        node->m_prev = prev;
        node->m_next = &m_dummy;
        std::construct_at(&node->value(), std::forward<Args>(args)...);
        m_dummy.m_prev = node;
        ++m_size;
        return node->value();
    }

    template <class ...Args>
    T &emplace_front(Args &&...args) {
        ListNode *node = newNode();
        ListNode *next = m_dummy.m_next;
        next->m_prev = node;
        node->m_next = next;
        node->m_prev = &m_dummy;
        std::construct_at(&node->value(), std::forward<Args>(args)...);
        m_dummy.m_next = node;
        ++m_size;
        return node->value();
    }

    /* template <std::invocable<T &> Visitor> */
    /* void foreach(Visitor visit) { */
    /*     ListNode *curr = m_dummy.m_next; */
    /*     while (curr != &m_dummy) { */
    /*         visit(curr->value()); */
    /*         curr = curr->m_next; */
    /*     } */
    /* } */

    /* template <std::invocable<T const &> Visitor> */
    /* void foreach(Visitor visit) const { */
    /*     ListNode *curr = m_dummy.m_next; */
    /*     while (curr != &m_dummy) { */
    /*         visit(curr->value()); */
    /*         curr = curr->m_next; */
    /*     } */
    /* } */

    ~List() noexcept {
        clear();
    }

    void clear() noexcept {
        ListNode *curr = m_dummy.m_next;
        while (curr != &m_dummy) {
            std::destroy_at(&curr->value());
            auto next = curr->m_next;
            deleteNode(curr);
            curr = next;
        }
        m_dummy.m_prev = m_dummy.m_next = &m_dummy;
        m_size = 0;
    }

    struct iterator {
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = T;
        using difference_type = ptrdiff_t;
        using pointer = T *;
        using reference = T &;

    private:
        ListNode *m_curr;

        friend List;

        explicit iterator(ListNode *curr) noexcept
        : m_curr(curr) {}

    public:
        iterator() = default;

        iterator &operator++() noexcept { // ++iterator
            m_curr = m_curr->m_next;
            return *this;
        }

        iterator operator++(int) noexcept { // iterator++
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        iterator &operator--() noexcept { // --iterator
            m_curr = m_curr->m_prev;
            return *this;
        }

        iterator operator--(int) noexcept { // iterator--
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        T &operator*() const noexcept {
            return m_curr->value();
        }

        bool operator!=(iterator const &that) const noexcept {
            return m_curr != that.m_curr;
        }

        bool operator==(iterator const &that) const noexcept {
            return !(*this != that);
        }
    };

    struct const_iterator {
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = T;
        using difference_type = ptrdiff_t;
        using pointer = T const *;
        using reference = T const &;

    private:
        ListNode const *m_curr;

        friend List;

        explicit const_iterator(ListNode const *curr) noexcept
        : m_curr(curr) {}

    public:
        const_iterator() = default;

        const_iterator(iterator that) noexcept : m_curr(that.m_curr) {
        }

        explicit operator iterator() noexcept {
            return iterator{const_cast<ListNode *>(m_curr)};
        }

        const_iterator &operator++() noexcept { // ++iterator
            m_curr = m_curr->m_next;
            return *this;
        }

        const_iterator operator++(int) noexcept { // iterator++
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        const_iterator &operator--() noexcept { // --iterator
            m_curr = m_curr->m_prev;
            return *this;
        }

        const_iterator operator--(int) noexcept { // iterator--
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        T const &operator*() const noexcept {
            return m_curr->value();
        }

        bool operator!=(const_iterator const &that) const noexcept {
            return m_curr != that.m_curr;
        }

        bool operator==(const_iterator const &that) const noexcept {
            return !(*this != that);
        }
    };

    iterator begin() noexcept {
        return iterator{m_dummy.m_next};
    }

    iterator end() noexcept {
        return iterator{&m_dummy};
    }

    const_iterator cbegin() const noexcept {
        return const_iterator{m_dummy.m_next};
    }

    const_iterator cend() const noexcept {
        return const_iterator{&m_dummy};
    }

    const_iterator begin() const noexcept {
        return cbegin();
    }

    const_iterator end() const noexcept {
        return cend();
    }

    using reverse_iterator = std::reverse_iterator<iterator>;
    using reverse_const_iterator = std::reverse_iterator<const_iterator>;

    reverse_iterator rbegin() noexcept {
        return std::make_reverse_iterator(end());
    }

    reverse_iterator rend() noexcept {
        return std::make_reverse_iterator(begin());
    }

    reverse_const_iterator crbegin() const noexcept {
        return std::make_reverse_iterator(cend());
    }

    reverse_const_iterator crend() const noexcept {
        return std::make_reverse_iterator(cbegin());
    }

    reverse_const_iterator rbegin() const noexcept {
        return crbegin();
    }

    reverse_const_iterator rend() const noexcept {
        return crend();
    }

    iterator erase(const_iterator pos) noexcept {
        ListNode *node = const_cast<ListNode *>(pos.m_curr);
        auto next = node->m_next;
        auto prev = node->m_prev;
        prev->m_next = next;
        next->m_prev = prev;
        std::destroy_at(&node->value());
        deleteNode(node);
        --m_size;
        return iterator{next};
    }

    iterator erase(const_iterator first, const_iterator last) noexcept {
        while (first != last) {
            first = erase(first);
        }
        return iterator(first);
    }

    void pop_front() noexcept {
        erase(begin());
    }

    void pop_back() noexcept {
        erase(std::prev(end()));
    }

    std::size_t remove(T const &val) noexcept {
        auto first = begin();
        auto last = begin();
        std::size_t count = 0;
        while (first != last) {
            if (*first == val) {
                first = erase(first);
                ++count;
            } else {
                ++first;
            }
        }
        return count;
    }

    template <class Pred>
    std::size_t remove_if(Pred &&pred) noexcept {
        auto first = begin();
        auto last = begin();
        std::size_t count = 0;
        while (first != last) {
            if (pred(*first)) {
                first = erase(first);
                ++count;
            } else {
                ++first;
            }
        }
        return count;
    }

    template <class ...Args>
    iterator emplace(const_iterator pos, Args &&...args) {
        ListNode *curr = newNode();
        ListNode *next = const_cast<ListNode *>(pos.m_curr);
        ListNode *prev = next->m_prev;
        curr->m_prev = prev;
        prev->m_next = curr;
        curr->m_next = next;
        next->m_prev = curr;
        std::construct_at(&curr->value(), std::forward<Args>(args)...);
        ++m_size;
        return iterator{curr};
    }

    iterator insert(const_iterator pos, const T &val) {
        return emplace(pos, val);
    }

    iterator insert(const_iterator pos, T &&val) {
        return emplace(pos, std::move(val));
    }

    iterator insert(const_iterator pos, std::size_t n, T const &val) {
        auto orig = pos;
        bool had_orig = false;
        while (n) {
            pos = emplace(pos, val);
            if (!had_orig) {
                had_orig = true;
                orig = pos;
            }
            ++pos;
            --n;
        }
        return iterator(orig);
    }

    template <std::input_iterator InputIt>
    iterator insert(const_iterator pos, InputIt first, InputIt last) {
        auto orig = pos;
        bool had_orig = false;
        while (first != last) {
            pos = emplace(pos, *first);
            if (!had_orig) {
                had_orig = true;
                orig = pos;
            }
            ++pos;
            ++first;
        }
        return iterator(orig);
    }

    iterator insert(const_iterator pos, std::initializer_list<T> ilist) {
        return insert(pos, ilist.begin(), ilist.end());
    }

    void splice(const_iterator pos, List &&that) {
        insert(pos, std::make_move_iterator(that.begin()), std::make_move_iterator(that.end()));
    }

    Alloc get_allocator() const noexcept {
        return m_alloc;
    }

    _LIBPENGCXX_DEFINE_COMPARISON(List);
};
