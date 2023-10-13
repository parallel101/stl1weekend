#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <string>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

template <class T, class Alloc = std::allocator<T>>
struct Vector {
    using value_type = T;
    using size_type = size_t;
    using pointer = T *;
    using const_pointer = T const *;
    using reference = T &;
    using const_reference = T const &;
    using iterator = T *;
    using const_iterator = T const *;
    using reverse_iterator = std::reverse_iterator<T *>;
    using const_reverse_iterator = std::reverse_iterator<T const *>;
    using allocator = Alloc;

    T *m_data;
    size_t m_size;
    size_t m_cap;

    Vector() noexcept {
        m_data = nullptr;
        m_size = 0;
        m_cap = 0;
    }

    Vector(std::initializer_list<T> ilist) : Vector(ilist.begin(), ilist.end()) {}

    explicit Vector(size_t n) {
        m_data = allocator{}.allocate(n);
        m_cap = m_size = n;
        for (size_t i = 0; i != n; i++) {
            std::construct_at(&m_data[i]);
        }
    }

    explicit Vector(size_t n, T const &val) {
        m_data = allocator{}.allocate(n);
        m_cap = m_size = n;
        for (size_t i = 0; i != n; i++) {
            std::construct_at(&m_data[i], val);
        }
    }

    template <std::random_access_iterator InputIt>
    Vector(InputIt first, InputIt last) {
        size_t n = last - first;
        m_data = allocator{}.allocate(n);
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
            m_data = allocator{}.allocate(m_size);
        }
        if (old_cap != 0) [[likely]] {
            for (size_t i = 0; i != m_size; i++) {
                std::construct_at(&m_data[i], std::move_if_noexcept(old_data[i])); // m_data[i] = std::move(old_data[i])
                std::destroy_at(&old_data[i]);
            }
            allocator{}.deallocate(old_data, old_cap);
        }
    }

    void reserve(size_t n) {
        if (n <= m_cap) return;
        n = std::max(n, m_cap * 2);
        printf("grow from %zd to %zd\n", m_cap, n);
        auto old_data = m_data;
        auto old_cap = m_cap;
        if (n == 0) {
            m_data = nullptr;
            m_cap = 0;
        } else {
            m_data = allocator{}.allocate(n);
            m_cap = n;
        }
        if (old_cap != 0) {
            for (size_t i = 0; i != m_size; i++) {
                std::construct_at(&m_data[i], std::move_if_noexcept(old_data[i]));
            }
            for (size_t i = 0; i != m_size; i++) {
                std::destroy_at(&old_data[i]);
            }
            allocator{}.deallocate(old_data, old_cap);
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

    Vector(Vector &&that) noexcept {
        m_data = that.m_data;
        m_size = that.m_size;
        m_cap = that.m_cap;
        that.m_data = nullptr;
        that.m_size = 0;
        that.m_cap = 0;
    }

    Vector &operator=(Vector &&that) noexcept {
        if (m_cap != 0) {
            allocator{}.deallocate(m_data, m_cap);
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
    }

    Vector(Vector const &that) {
        m_cap = m_size = that.m_size;
        if (m_size != 0) {
            m_data = allocator{}.allocate(m_size);
            for (size_t i = 0; i != m_size; i++) {
                std::construct_at(&m_data[i], std::as_const(that.m_data[i]));
            }
        } else {
            m_data = nullptr;
        }
    }

    Vector &operator=(Vector const &that) {
        reserve(that.m_size);
        m_size = that.m_size;
        if (m_size != 0) {
            for (size_t i = 0; i != m_size; i++) {
                std::construct_at(&m_data[i], std::as_const(that.m_data[i]));
            }
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
        if (m_size < m_cap) [[unlikely]] reserve(m_size + 1);
        std::construct_at(&m_data[m_size], val);
        m_size = m_size + 1;
    }

    void push_back(T &&val) {
        if (m_size < m_cap) [[unlikely]] reserve(m_size + 1);
        std::construct_at(&m_data[m_size], std::move(val));
        m_size = m_size + 1;
    }

    template <class ...Args>
    T &emplace_back(Args &&...args) {
        if (m_size < m_cap) [[unlikely]] reserve(m_size + 1);
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
        return std::make_reverse_iterator(m_data);
    }

    std::reverse_iterator<T *> rend() noexcept {
        return std::make_reverse_iterator(m_data + m_size);
    }

    std::reverse_iterator<T const *> rbegin() const noexcept {
        return std::make_reverse_iterator(m_data);
    }

    std::reverse_iterator<T const *> rend() const noexcept {
        return std::make_reverse_iterator(m_data + m_size);
    }

    std::reverse_iterator<T const *> crbegin() const noexcept {
        return std::make_reverse_iterator(m_data);
    }

    std::reverse_iterator<T const *> crend() const noexcept {
        return std::make_reverse_iterator(m_data + m_size);
    }

    T *erase(T const *it) noexcept(std::is_nothrow_move_assignable_v<T>) {
        size_t i = it - m_data;
        for (size_t j = i + 1; j != m_size; j++) {
            m_data[j - 1] = std::move_if_noexcept(m_data[j]);
        }
        m_size -= 1;
        std::destroy_at(&m_data[m_size]);
        return const_cast<T *>(it + 1);
    }

    T *erase(T const *first, T const *last) noexcept(std::is_nothrow_move_assignable_v<T>) {
        size_t diff = last - first;
        for (size_t j = last - m_data; j != m_size; j++) {
            m_data[j - diff] = std::move_if_noexcept(m_data[j]);
        }
        m_size -= diff;
        for (size_t j = m_size; j != m_size + diff; j++) {
            std::destroy_at(&m_data[j]);
        }
        return const_cast<T *>(last);
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
            m_data[i] = *first;
            ++first;
        }
    }

    void assign(std::initializer_list<T> ilist) {
        assign(ilist.begin(), ilist.end());
    }

    T *insert(T const *it, size_t n, T const &val) {
        size_t j = it - m_data;
        if (n == 0) [[unlikely]] return;
        reserve(m_size + n);
        m_size += n;
        // j ~ m_size => j + n ~ m_size + n
        for (size_t i = n; i != 0; i++) {
            std::construct_at(&m_data[j + n + i - 1], std::move(m_data[j + i - 1]));
        }
        for (size_t i = j; i != j + n; i++) {
            std::construct_at(&m_data[i], val);
        }
    }

    template <std::random_access_iterator InputIt>
    void insert(T const *it, InputIt first, InputIt last) {
        size_t j = it - m_data;
        size_t n = last - first;
        if (n == 0) [[unlikely]] return;
        reserve(m_size + n);
        m_size += n;
        // j ~ m_size => j + n ~ m_size + n
        for (size_t i = n; i != 0; i++) {
            std::construct_at(&m_data[j + n + i - 1], std::move(m_data[j + i - 1]));
        }
        for (size_t i = j; i != j + n; i++) {
            std::construct_at(&m_data[i], *first);
            ++first;
        }
    }

    void insert(T const *it, std::initializer_list<T> ilist) {
        insert(it, ilist.begin(), ilist.end());
    }

    ~Vector() noexcept {
        if (m_cap != 0) {
            allocator{}.deallocate(m_data, m_cap);
        }
    }
};

int main() {
    Vector<int> arr; // data size cap
    // size=0 cap=0
    // size=1 cap=1 *
    // size=2 cap=2 *
    // size=3 cap=4 *
    // size=4 cap=4
    // size=5 cap=8 *
    // size=6 cap=8
    // size=7 cap=8
    // size=8 cap=8
    // size=9 cap=16 *
    // size=10 cap=16
    for (int i = 0; i < 16; i++) { // O(n)
        printf("arr.push_back(%d)\n", i);
        arr.push_back(i); // O(1)+
    }
    for (size_t i = 0; i < arr.size(); i++) {
        printf("arr[%zd] = %d\n", i, arr[i]);
    }

    Vector<int> bar(3);
    printf("arr.size() = %zd\n", arr.size());
    printf("bar.size() = %zd\n", bar.size());
    bar = std::move(arr);
    printf("arr.size() = %zd\n", arr.size());
    printf("bar.size() = %zd\n", bar.size());
}
