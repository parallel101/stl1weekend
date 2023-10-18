#pragma once

#include <cstddef>
#include <iterator>
#include <memory>
#include <limits>
#include <stdexcept>
#include <utility>
#include <compare>
#include <initializer_list>

template <class T, class Alloc = std::allocator<T>>
struct Vector {
    using value_type = T;
    using allocator_type = Alloc;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using pointer = T *;
    using const_pointer = T const *;
    using reference = T &;
    using const_reference = T const &;
    using iterator = T *;
    using const_iterator = T const *;
    using reverse_iterator = std::reverse_iterator<T *>;
    using const_reverse_iterator = std::reverse_iterator<T const *>;

    T *m_data;
    size_t m_size;
    size_t m_cap;
    [[no_unique_address]] Alloc m_alloc;

    Vector() noexcept {
        m_data = nullptr;
        m_size = 0;
        m_cap = 0;
    }

    Vector(std::initializer_list<T> ilist, Alloc const &alloc = Alloc())
    : Vector(ilist.begin(), ilist.end(), alloc) {}

    explicit Vector(size_t n, Alloc const &alloc = Alloc()) : m_alloc(alloc) {
        m_data = m_alloc.allocate(n);
        m_cap = m_size = n;
        for (size_t i = 0; i != n; i++) {
            std::construct_at(&m_data[i]); // m_data[i] = 0
        }
    }

    Vector(size_t n, T const &val, Alloc const &alloc = Alloc()) : m_alloc(alloc) {
        m_data = m_alloc.allocate(n);
        m_cap = m_size = n;
        for (size_t i = 0; i != n; i++) {
            std::construct_at(&m_data[i], val); // m_data[i] = val
        }
    }

    template <std::random_access_iterator InputIt>
    Vector(InputIt first, InputIt last, Alloc const &alloc = Alloc()) : m_alloc(alloc) {
        size_t n = last - first;
        m_data = m_alloc.allocate(n);
        m_cap = m_size = n;
        for (size_t i = 0; i != n; i++) {
            std::construct_at(&m_data[i], *first);
            ++first;
        }
    }

    void clear() noexcept {
        for (size_t i = 0; i != m_size; i++) {
            std::destroy_at(&m_data[i]);
        }
        m_size = 0;
    }

    void resize(size_t n) {
        if (n < m_size) {
            for (size_t i = n; i != m_size; i++) {
                std::destroy_at(&m_data[i]);
            }
            m_size = n;
        } else if (n > m_size) {
            reserve(n);
            for (size_t i = m_size; i != n; i++) {
                std::construct_at(&m_data[i]); // m_data[i] = 0
            }
        }
        m_size = n;
    }

    void resize(size_t n, T const &val) {
        if (n < m_size) {
            for (size_t i = n; i != m_size; i++) {
                std::destroy_at(&m_data[i]);
            }
            m_size = n;
        } else if (n > m_size) {
            reserve(n);
            for (size_t i = m_size; i != n; i++) {
                std::construct_at(&m_data[i], val); // m_data[i] = val
            }
        }
        m_size = n;
    }

    void shrink_to_fit() noexcept {
        auto old_data = m_data;
        auto old_cap = m_cap;
        m_cap = m_size;
        if (m_size == 0) {
            m_data = nullptr;
        } else {
            m_data = m_alloc.allocate(m_size);
        }
        if (old_cap != 0) [[likely]] {
            for (size_t i = 0; i != m_size; i++) {
                std::construct_at(&m_data[i], std::move_if_noexcept(old_data[i])); // m_data[i] = std::move(old_data[i])
                std::destroy_at(&old_data[i]);
            }
            m_alloc.deallocate(old_data, old_cap);
        }
    }

    void reserve(size_t n) {
        if (n <= m_cap) return;
        n = std::max(n, m_cap * 2);
        /* printf("grow from %zd to %zd\n", m_cap, n); */
        auto old_data = m_data;
        auto old_cap = m_cap;
        if (n == 0) {
            m_data = nullptr;
            m_cap = 0;
        } else {
            m_data = m_alloc.allocate(n);
            m_cap = n;
        }
        if (old_cap != 0) {
            for (size_t i = 0; i != m_size; i++) {
                std::construct_at(&m_data[i], std::move_if_noexcept(old_data[i]));
            }
            for (size_t i = 0; i != m_size; i++) {
                std::destroy_at(&old_data[i]);
            }
            m_alloc.deallocate(old_data, old_cap);
        }
    }

    size_t capacity() const noexcept {
        return m_cap;
    }

    size_t size() const noexcept {
        return m_size;
    }

    bool empty() const noexcept {
        return m_size == 0;
    }

    static constexpr size_t max_size() noexcept {
        return std::numeric_limits<size_t>::max() / sizeof(T);
    }

    T const &operator[](size_t i) const noexcept {
        return m_data[i];
    }

    T &operator[](size_t i) noexcept {
        return m_data[i];
    }

    T const &at(size_t i) const {
        if (i >= m_size) [[unlikely]] throw std::out_of_range("vector::at");
        return m_data[i];
    }

    T &at(size_t i) {
        if (i >= m_size) [[unlikely]] throw std::out_of_range("vector::at");
        return m_data[i];
    }

    Vector(Vector &&that) noexcept : m_alloc(std::move(that.m_alloc)) {
        m_data = that.m_data;
        m_size = that.m_size;
        m_cap = that.m_cap;
        that.m_data = nullptr;
        that.m_size = 0;
        that.m_cap = 0;
    }

    Vector(Vector &&that, Alloc const &alloc) noexcept : m_alloc(alloc) {
        m_data = that.m_data;
        m_size = that.m_size;
        m_cap = that.m_cap;
        that.m_data = nullptr;
        that.m_size = 0;
        that.m_cap = 0;
    }

    Vector &operator=(Vector &&that) noexcept {
        if (&that == this) [[unlikely]] return *this;
        for (size_t i = 0; i != m_size; i++) {
            std::destroy_at(&m_data[i]);
        }
        if (m_cap != 0) {
            m_alloc.deallocate(m_data, m_cap);
        }
        m_data = that.m_data;
        m_size = that.m_size;
        m_cap = that.m_cap;
        that.m_data = nullptr;
        that.m_size = 0;
        that.m_cap = 0;
        return *this;
    }

    void swap(Vector &that) noexcept {
        std::swap(m_data, that.m_data);
        std::swap(m_size, that.m_size);
        std::swap(m_cap, that.m_cap);
        std::swap(m_alloc, that.m_alloc);
    }

    Vector(Vector const &that) : m_alloc(that.m_alloc) {
        m_cap = m_size = that.m_size;
        if (m_size != 0) {
            m_data = m_alloc.allocate(m_size);
            for (size_t i = 0; i != m_size; i++) {
                std::construct_at(&m_data[i], std::as_const(that.m_data[i]));
            }
        } else {
            m_data = nullptr;
        }
    }

    Vector(Vector const &that, Alloc const &alloc) : m_alloc(alloc) {
        m_cap = m_size = that.m_size;
        if (m_size != 0) {
            m_data = m_alloc.allocate(m_size);
            for (size_t i = 0; i != m_size; i++) {
                std::construct_at(&m_data[i], std::as_const(that.m_data[i]));
            }
        } else {
            m_data = nullptr;
        }
    }

    Vector &operator=(Vector const &that) {
        if (&that == this) [[unlikely]] return *this;
        reserve(that.m_size);
        m_size = that.m_size;
        for (size_t i = 0; i != m_size; i++) {
            std::construct_at(&m_data[i], std::as_const(that.m_data[i]));
        }
        return *this;
    }

    T const &front() const noexcept {
        return *m_data;
    }

    T &front() noexcept {
        return *m_data;
    }

    T const &back() const noexcept {
        return m_data[m_size - 1];
    }

    T &back() noexcept {
        return m_data[m_size - 1];
    }

    void push_back(T const &val) {
        if (m_size + 1 >= m_cap) [[unlikely]] reserve(m_size + 1);
        std::construct_at(&m_data[m_size], val);
        m_size = m_size + 1;
    }

    void push_back(T &&val) {
        if (m_size + 1 >= m_cap) [[unlikely]] reserve(m_size + 1);
        std::construct_at(&m_data[m_size], std::move(val));
        m_size = m_size + 1;
    }

    template <class ...Args>
    T &emplace_back(Args &&...args) {
        if (m_size + 1 >= m_cap) [[unlikely]] reserve(m_size + 1);
        T *p = &m_data[m_size];
        std::construct_at(p, std::forward<Args>(args)...);
        m_size = m_size + 1;
        return *p;
    }

    T *data() noexcept {
        return m_data;
    }

    T const *data() const noexcept {
        return m_data;
    }

    T const *cdata() const noexcept {
        return m_data;
    }

    T *begin() noexcept {
        return m_data;
    }

    T *end() noexcept {
        return m_data + m_size;
    }

    T const *begin() const noexcept {
        return m_data;
    }

    T const *end() const noexcept {
        return m_data + m_size;
    }

    T const *cbegin() const noexcept {
        return m_data;
    }

    T const *cend() const noexcept {
        return m_data + m_size;
    }

    std::reverse_iterator<T *> rbegin() noexcept {
        return std::make_reverse_iterator(m_data + m_size);
    }

    std::reverse_iterator<T *> rend() noexcept {
        return std::make_reverse_iterator(m_data);
    }

    std::reverse_iterator<T const *> rbegin() const noexcept {
        return std::make_reverse_iterator(m_data + m_size);
    }

    std::reverse_iterator<T const *> rend() const noexcept {
        return std::make_reverse_iterator(m_data);
    }

    std::reverse_iterator<T const *> crbegin() const noexcept {
        return std::make_reverse_iterator(m_data + m_size);
    }

    std::reverse_iterator<T const *> crend() const noexcept {
        return std::make_reverse_iterator(m_data);
    }

    void pop_back() noexcept {
        m_size -= 1;
        std::destroy_at(&m_data[m_size]);
    }

    T *erase(T const *it) noexcept(std::is_nothrow_move_assignable_v<T>) {
        size_t i = it - m_data;
        for (size_t j = i + 1; j != m_size; j++) {
            m_data[j - 1] = std::move(m_data[j]);
        }
        m_size -= 1;
        std::destroy_at(&m_data[m_size]);
        return const_cast<T *>(it);
    }

    T *erase(T const *first, T const *last) noexcept(std::is_nothrow_move_assignable_v<T>) {
        size_t diff = last - first;
        for (size_t j = last - m_data; j != m_size; j++) {
            m_data[j - diff] = std::move(m_data[j]);
        }
        m_size -= diff;
        for (size_t j = m_size; j != m_size + diff; j++) {
            std::destroy_at(&m_data[j]);
        }
        return const_cast<T *>(first);
    }

    void assign(size_t n, T const &val) {
        clear();
        reserve(n);
        m_size = n;
        for (size_t i = 0; i != n; i++) {
            std::construct_at(&m_data[i], val);
        }
    }

    template <std::random_access_iterator InputIt>
    void assign(InputIt first, InputIt last) {
        clear();
        size_t n = last - first;
        reserve(n);
        m_size = n;
        for (size_t i = 0; i != n; i++) {
            std::construct_at(m_data[i], *first);
            ++first;
        }
    }

    void assign(std::initializer_list<T> ilist) {
        assign(ilist.begin(), ilist.end());
    }

    Vector &operator=(std::initializer_list<T> ilist) {
        assign(ilist.begin(), ilist.end());
    }

    template <class ...Args>
    T *emplace(T const *it, Args &&...args) {
        size_t j = it - m_data;
        reserve(m_size + 1);
        // j ~ m_size => j + 1 ~ m_size + 1
        for (size_t i = m_size; i != j; i--) {
            std::construct_at(&m_data[i], std::move(m_data[i - 1]));
            std::destroy_at(&m_data[i - 1]);
        }
        m_size += 1;
        std::construct_at(&m_data[j], std::forward<Args>(args)...);
        return m_data + j;
    }

    T *insert(T const *it, T &&val) {
        size_t j = it - m_data;
        reserve(m_size + 1);
        // j ~ m_size => j + 1 ~ m_size + 1
        for (size_t i = m_size; i != j; i--) {
            std::construct_at(&m_data[i], std::move(m_data[i - 1]));
            std::destroy_at(&m_data[i - 1]);
        }
        m_size += 1;
        std::construct_at(&m_data[j], std::move(val));
        return m_data + j;
    }

    T *insert(T const *it, T const &val) {
        size_t j = it - m_data;
        reserve(m_size + 1);
        // j ~ m_size => j + 1 ~ m_size + 1
        for (size_t i = m_size; i != j; i--) {
            std::construct_at(&m_data[i], std::move(m_data[i - 1]));
            std::destroy_at(&m_data[i - 1]);
        }
        m_size += 1;
        std::construct_at(&m_data[j], val);
        return m_data + j;
    }

    T *insert(T const *it, size_t n, T const &val) {
        size_t j = it - m_data;
        if (n == 0) [[unlikely]] return const_cast<T *>(it);
        reserve(m_size + n);
        // j ~ m_size => j + n ~ m_size + n
        for (size_t i = m_size; i != j; i--) {
            std::construct_at(&m_data[i + n - 1], std::move(m_data[i - 1]));
            std::destroy_at(&m_data[i - 1]);
        }
        m_size += n;
        for (size_t i = j; i != j + n; i++) {
            std::construct_at(&m_data[i], val);
        }
        return m_data + j;
    }

    template <std::random_access_iterator InputIt>
    T *insert(T const *it, InputIt first, InputIt last) {
        size_t j = it - m_data;
        size_t n = last - first;
        if (n == 0) [[unlikely]] return const_cast<T *>(it);
        reserve(m_size + n);
        // j ~ m_size => j + n ~ m_size + n
        for (size_t i = m_size; i != j; i--) {
            std::construct_at(&m_data[i + n - 1], std::move(m_data[i - 1]));
            std::destroy_at(&m_data[i - 1]);
        }
        m_size += n;
        for (size_t i = j; i != j + n; i++) {
            std::construct_at(&m_data[i], *first);
            ++first;
        }
        return m_data + j;
    }

    T *insert(T const *it, std::initializer_list<T> ilist) {
        return insert(it, ilist.begin(), ilist.end());
    }

    ~Vector() noexcept {
        for (size_t i = 0; i != m_size; i++) {
            std::destroy_at(&m_data[i]);
        }
        if (m_cap != 0) {
            m_alloc.deallocate(m_data, m_cap);
        }
    }

    Alloc get_allocator() const noexcept {
        return m_alloc;
    }

    bool operator==(Vector const &that) noexcept {
        return std::equal(begin(), end(), that.begin(), that.end());
    }

    auto operator<=>(Vector const &that) noexcept {
        return std::lexicographical_compare_three_way(begin(), end(), that.begin(), that.end());
    }
};
