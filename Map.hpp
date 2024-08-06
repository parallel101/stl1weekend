#pragma once

/*
红黑树是如何保证最深叶子和最浅叶子的深度差不超过 2 倍的呢？
他设定了这样 5 条规则：

1. 节点可以是红色或黑色的。
2. 根节点总是黑色的。
3. 所有叶子节点都是黑色（叶子节点就是 NULL）。
4. 红色节点的两个子节点必须都是黑色的。
5. 从任一节点到其所有叶子节点的路径都包含相同数量的黑色节点。

看起来好像很复杂，但实际上大多是废话，有用的只是 4 和 5 这两条。
规则 4 翻译一下就是：不得出现相邻的红色节点（相邻指两个节点是父子关系）。这条规则还有一个隐含的信息：黑色节点可以相邻！
规则 5 翻译一下就是：从根节点到所有底层叶子的距离（以黑色节点数量计），必须相等。
因为规则 4 的存在，红色节点不可能相邻，也就是说最深的枝干只能是：红-黑-红-黑-红-黑-红-黑。
结合规则 5 来看，也就是说每条枝干上的黑色节点数量必须相同，因为最深的枝干是 4 个黑节点了，所以最浅的枝干至少也得有 4 个节点全是黑色的：黑-黑-黑-黑。
可以看到，规则 4 和规则 5 联合起来实际上就保证了：最深枝干的深度不会超过最浅枝干的 2 倍。
*/

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <iterator>
#include <utility>

#if __cpp_concepts && __cpp_lib_concepts
#define _LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(category, T) category T
#else
#define _LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(category, T) class T, \
    std::enable_if_t<std::is_convertible_v< \
        typename std::iterator_traits<T>::iterator_category, \
        category##_tag>>
#endif
#define _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare) \
    class Compare##Tp = Compare, \
    class = typename Compare##Tp::is_transparent

enum TreeColor {
    BLACK,
    RED,
};

enum TreeChildDir {
    LEFT,
    RIGHT,
};

struct TreeNode {
    TreeNode *left;     // 左子节点指针
    TreeNode *right;    // 右子节点指针
    TreeNode *parent;   // 父节点指针
    TreeNode **pparent; // 父节点中指向本节点指针的指针
    TreeColor color;    // 红或黑
};

template <class T>
struct TreeNodeImpl : TreeNode {
    union {
        T value;
    }; // union 可以阻止里面成员的自动初始化，方便不支持 T() 默认构造的类型

    TreeNodeImpl() noexcept {}
    ~TreeNodeImpl() noexcept {}
};

template <bool kReverse>
struct TreeIteratorBase;

template <>
struct TreeIteratorBase<false> {
    union {
        TreeNode *node;
        TreeNode **proot;
    };
    bool offBy1;

    TreeIteratorBase(TreeNode *node) noexcept : node(node), offBy1(false) {
    }

    TreeIteratorBase(TreeNode **proot) noexcept : proot(proot), offBy1(true) {}

    bool operator==(TreeIteratorBase const &that) const noexcept {
        return (!offBy1 && !that.offBy1 && node == that.node) || (offBy1 && that.offBy1);
    }

    bool operator!=(TreeIteratorBase const &that) const noexcept {
        return !(*this == that);
    }

    void operator++() noexcept {   // ++it
        if (offBy1) {
            offBy1 = false;
            node = *proot;
            assert(node);
            while (node->left != nullptr) {
                node = node->left;
            }
            return;
        }
        assert(node);
        if (node->right != nullptr) {
            node = node->right;
            while (node->left != nullptr) {
                node = node->left;
            }
        } else {
            while (node->parent != nullptr && node->pparent == &node->parent->right) {
                node = node->parent;
            }
            if (node->parent == nullptr) {
                offBy1 = true;
                return;
            }
            node = node->parent;
        }
    }

    void operator--() noexcept {   // --it
        // 为了支持 --end()
        if (offBy1) {
            offBy1 = false;
            node = *proot;
            assert(node);
            while (node->right != nullptr) {
                node = node->right;
            }
            return;
        }
        assert(node);
        if (node->left != nullptr) {
            node = node->left;
            while (node->right != nullptr) {
                node = node->right;
            }
        } else {
            while (node->parent != nullptr && node->pparent == &node->parent->left) {
                node = node->parent;
            }
            if (node->parent == nullptr) {
                offBy1 = true;
                return;
            }
            node = node->parent;
        }
    }

    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
};

template <>
struct TreeIteratorBase<true> : TreeIteratorBase<false> {
    using TreeIteratorBase<false>::TreeIteratorBase;

    void operator++() noexcept {   // ++it
        TreeIteratorBase<false>::operator--();
    }

    void operator--() noexcept {   // --it
        TreeIteratorBase<false>::operator++();
    }
};

template <class T, bool kReverse>
struct TreeIterator : TreeIteratorBase<kReverse> {
    using TreeIteratorBase<kReverse>::TreeIteratorBase;

    template <class T0 = T>
    explicit operator std::enable_if_t<std::is_const_v<T0>,
        TreeIterator<std::remove_const_t<T0>, kReverse>>() const noexcept {
        return this->node;
    }

    template <class T0 = T>
    operator std::enable_if_t<!std::is_const_v<T0>,
        TreeIterator<std::add_const_t<T0>, kReverse>>() const noexcept {
        return this->node;
    }

    TreeIterator &operator++() noexcept {   // ++it
        TreeIteratorBase<kReverse>::operator++();
        return *this;
    }

    TreeIterator &operator--() noexcept {   // --it
        TreeIteratorBase<kReverse>::operator--();
        return *this;
    }

    TreeIterator operator++(int) noexcept { // it++
        TreeIterator tmp = *this;
        ++*this;
        return tmp;
    }

    TreeIterator operator--(int) noexcept { // it--
        TreeIterator tmp = *this;
        --*this;
        return tmp;
    }

    T *operator->() const noexcept {
        return std::addressof(static_cast<TreeNodeImpl<T> *>(this->node)->value);
    }

    T &operator*() const noexcept {
        return static_cast<TreeNodeImpl<T> *>(this->node)->value;
    }

    using value_type = std::remove_const_t<T>;
    using reference = T &;
    using pointer = T *;
};

struct TreeRoot {
    TreeNode *root;

    TreeRoot() noexcept : root(nullptr) {
    }
};

struct TreeBase {
    TreeRoot *block;

    explicit TreeBase(TreeRoot *block) : block(block) {}

    static void _M_rotate_left(TreeNode *node) noexcept {
        TreeNode *right = node->right;
        node->right = right->left;
        if (right->left != nullptr) {
            right->left->parent = node;
            right->left->pparent = &node->right;
        }
        right->parent = node->parent;
        right->pparent = node->pparent;
        *node->pparent = right;
        right->left = node;
        node->parent = right;
        node->pparent = &right->left;
    }

    static void _M_rotate_right(TreeNode *node) noexcept {
        TreeNode *left = node->left;
        node->left = left->right;
        if (left->right != nullptr) {
            left->right->parent = node;
            left->right->pparent = &node->left;
        }
        left->parent = node->parent;
        left->pparent = node->pparent;
        *node->pparent = left;
        left->right = node;
        node->parent = left;
        node->pparent = &left->right;
    }

    static void _M_fix_violation(TreeNode *node) noexcept {
        while (true) {
            TreeNode *parent = node->parent;
            if (parent == nullptr) { // 等价于：node == root，因为 root->parent == nullptr
                // 情况 0: node == root
                node->color = BLACK;
                return;
            }
            if (node->color == BLACK || parent->color == BLACK)
                return;
            TreeNode *uncle;
            TreeNode *grandpa = parent->parent;
            assert(grandpa);
            TreeChildDir parent_dir = parent->pparent == &grandpa->left ? LEFT : RIGHT;
            if (parent_dir == LEFT) {
                uncle = grandpa->right;
            } else {
                assert(parent->pparent == &grandpa->right);
                uncle = grandpa->left;
            }
            TreeChildDir node_dir = node->pparent == &parent->left ? LEFT : RIGHT;
            if (uncle != nullptr && uncle->color == RED) {
                // 情况 1: 叔叔是红色人士
                parent->color = BLACK;
                uncle->color = BLACK;
                grandpa->color = RED;
                node = grandpa;
            } else if (node_dir == parent_dir) {
                if (node_dir == RIGHT) {
                    assert(node->pparent == &parent->right);
                    // 情况 2: 叔叔是黑色人士（RR）
                    TreeBase::_M_rotate_left(grandpa);
                } else {
                    // 情况 3: 叔叔是黑色人士（LL）
                    TreeBase::_M_rotate_right(grandpa);
                }
                std::swap(parent->color, grandpa->color);
                node = grandpa;
            } else {
                if (node_dir == RIGHT) {
                    assert(node->pparent == &parent->right);
                    // 情况 4: 叔叔是黑色人士（LR）
                    TreeBase::_M_rotate_left(parent);
                } else {
                    // 情况 5: 叔叔是黑色人士（RL）
                    TreeBase::_M_rotate_right(parent);
                }
                node = parent;
            }
        }
    }

    TreeNode *_M_min_node() const noexcept {
        TreeNode *current = block->root;
        if (current != nullptr) {
            while (current->left != nullptr) {
                current = current->left;
            }
        }
        return current;
    }

    TreeNode *_M_max_node() const noexcept {
        TreeNode *current = block->root;
        if (current != nullptr) {
            while (current->right != nullptr) {
                current = current->right;
            }
        }
        return current;
    }

    template <class T, class Tv, class Compare>
    TreeNode *_M_find_node(Tv &&value, Compare comp) const noexcept {
        TreeNode *current = block->root;
        while (current != nullptr) {
            if (comp(value, static_cast<TreeNodeImpl<T> *>(current)->value)) {
                current = current->left;
                continue;
            }
            if (comp(static_cast<TreeNodeImpl<T> *>(current)->value, value)) {
                current = current->right;
                continue;
            }
            // value == curent->value
            return current;
        }
        return nullptr;
    }

    template <class T, class Tv, class Compare>
    TreeNode *_M_lower_bound(Tv &&value, Compare comp) const noexcept {
        TreeNode *current = block->root;
        TreeNode *result = nullptr;
        while (current != nullptr) {
            if (!(comp(static_cast<TreeNodeImpl<T> *>(current)->value, value))) { // current->value >= value
                result = current;
                current = current->left;
            } else {
                current = current->right;
            }
        }
        return result;
    }

    template <class T, class Tv, class Compare>
    TreeNode *_M_upper_bound(Tv &&value, Compare comp) const noexcept {
        TreeNode *current = block->root;
        TreeNode *result = nullptr;
        while (current != nullptr) {
            if (comp(value, static_cast<TreeNodeImpl<T> *>(current)->value)) { // current->value > value
                result = current;
                current = current->left;
            } else {
                current = current->right;
            }
        }
        return result;
    }

    template <class T, class Tv, class Compare>
    std::pair<TreeNode *, TreeNode *> _M_equal_range(Tv &&value, Compare comp) const noexcept {
        return {this->_M_lower_bound<T>(value, comp), this->_M_upper_bound<T>(value, comp)};
    }

    static void _M_transplant(TreeNode *node, TreeNode *replace) noexcept {
        *node->pparent = replace;
        if (replace != nullptr) {
            replace->parent = node->parent;
            replace->pparent = node->pparent;
        }
    }

    static void _M_delete_fixup(TreeNode *node) noexcept {
        if (node == nullptr)
            return;
        while (node->parent != nullptr && node->color == BLACK) {
            TreeChildDir dir = node->pparent == &node->parent->left ? LEFT : RIGHT;
            TreeNode *sibling = dir == LEFT ? node->parent->right : node->parent->left;
            if (sibling->color == RED) {
                sibling->color = BLACK;
                node->parent->color = RED;
                if (dir == LEFT) {
                    TreeBase::_M_rotate_left(node->parent);
                } else {
                    TreeBase::_M_rotate_right(node->parent);
                }
                sibling = dir == LEFT ? node->parent->right : node->parent->left;
            }
            if (sibling->left->color == BLACK && sibling->right->color == BLACK) {
                sibling->color = RED;
                node = node->parent;
            } else {
                if (dir == LEFT && sibling->right->color == BLACK) {
                    sibling->left->color = BLACK;
                    sibling->color = RED;
                    TreeBase::_M_rotate_right(sibling);
                    sibling = node->parent->right;
                } else if (dir == RIGHT && sibling->left->color == BLACK) {
                    sibling->right->color = BLACK;
                    sibling->color = RED;
                    TreeBase::_M_rotate_left(sibling);
                    sibling = node->parent->left;
                }
                sibling->color = node->parent->color;
                node->parent->color = BLACK;
                if (dir == LEFT) {
                    sibling->right->color = BLACK;
                    TreeBase::_M_rotate_left(node->parent);
                } else {
                    sibling->left->color = BLACK;
                    TreeBase::_M_rotate_right(node->parent);
                }
                while (node->parent != nullptr) {
                    node = node->parent;
                }
                // break;
            }
        }
        node->color = BLACK;
    }

    static void _M_erase_node(TreeNode *node) noexcept {
        if (node->left == nullptr) {
            TreeNode *right = node->right;
            TreeColor color = node->color;
            TreeBase::_M_transplant(node, right);
            if (color == BLACK) {
                TreeBase::_M_delete_fixup(right);
            }
        } else if (node->right == nullptr) {
            TreeNode *left = node->left;
            TreeColor color = node->color;
            TreeBase::_M_transplant(node, left);
            if (color == BLACK) {
                TreeBase::_M_delete_fixup(left);
            }
        } else {
            TreeNode *replace = node->right;
            while (replace->left != nullptr) {
                replace = replace->left;
            }
            TreeNode *right = replace->right;
            TreeColor color = replace->color;
            if (replace->parent == node) {
                right->parent = replace;
                right->pparent = &replace->right;
            } else {
                TreeBase::_M_transplant(replace, right);
                replace->right = node->right;
                replace->right->parent = replace;
                replace->right->pparent = &replace->right;
            }
            TreeBase::_M_transplant(node, replace);
            replace->left = node->left;
            replace->left->parent = replace;
            replace->left->pparent = &replace->left;
            if (color == BLACK) {
                TreeBase::_M_delete_fixup(right);
            }
        }
    }

    template <class T, class Compare>
    TreeNode *_M_single_insert_node(TreeNode *node, Compare comp) {
        TreeNode **pparent = &block->root;
        TreeNode *parent = nullptr;
        while (*pparent != nullptr) {
            parent = *pparent;
            if (comp(static_cast<TreeNodeImpl<T> *>(node)->value, static_cast<TreeNodeImpl<T> *>(parent)->value)) {
                pparent = &parent->left;
                continue;
            }
            if (comp(static_cast<TreeNodeImpl<T> *>(parent)->value, static_cast<TreeNodeImpl<T> *>(node)->value)) {
                pparent = &parent->right;
                continue;
            }
            return parent;
        }

        node->left = nullptr;
        node->right = nullptr;
        node->color = RED;

        node->parent = parent;
        node->pparent = pparent;
        *pparent = node;
        TreeBase::_M_fix_violation(node);
        return nullptr;
    }

    template <class T, class Compare>
    void _M_multi_insert_node(TreeNode *node, Compare comp) {
        TreeNode **pparent = &block->root;
        TreeNode *parent = nullptr;
        while (*pparent != nullptr) {
            parent = *pparent;
            if (comp(static_cast<TreeNodeImpl<T> *>(node)->value, static_cast<TreeNodeImpl<T> *>(parent)->value)) {
                pparent = &parent->left;
                continue;
            }
            if (comp(static_cast<TreeNodeImpl<T> *>(parent)->value, static_cast<TreeNodeImpl<T> *>(node)->value)) {
                pparent = &parent->right;
                continue;
            }
            pparent = &parent->right;
        }

        node->left = nullptr;
        node->right = nullptr;
        node->color = RED;

        node->parent = parent;
        node->pparent = pparent;
        *pparent = node;
        TreeBase::_M_fix_violation(node);
    }
};

template <class T, class Compare>
struct TreeImpl : TreeBase {
    [[no_unique_address]] Compare _M_comp;

    TreeImpl() noexcept : TreeBase(new TreeRoot) {}

    explicit TreeImpl(Compare comp) noexcept : TreeBase(new TreeRoot), _M_comp(comp) {}

    TreeImpl(TreeImpl &&that) noexcept : TreeBase(that.block) {
        that.block = nullptr;
    }

    TreeImpl &operator=(TreeImpl &&that) noexcept {
        std::swap(block, that.block);
        return *this;
    }

    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, InputIt)>
    void _M_single_insert(InputIt first, InputIt last) {
        while (first != last) {
            this->_M_single_insert(*first);
            ++first;
        }
    }

    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, InputIt)>
    void _M_multi_insert(InputIt first, InputIt last) {
        while (first != last) {
            this->_M_multi_insert(*first);
            ++first;
        }
    }

    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, InputIt)>
    void assign(InputIt first, InputIt last) {
        this->clear();
        this->_M_multi_insert(first, last);
    }

    using iterator = TreeIterator<T, false>;
    using reverse_iterator = TreeIterator<T, true>;
    using const_iterator = TreeIterator<T const, false>;
    using const_reverse_iterator = TreeIterator<T const, true>;

    static_assert(std::input_iterator<iterator>);

    template <class Tv>
    const_iterator _M_find(Tv &&value) const noexcept {
        return this->_M_prevent_end(this->_M_find_node<T>(value, _M_comp));
    }

    template <class Tv>
    iterator _M_find(Tv &&value) noexcept {
        return this->_M_prevent_end(this->_M_find_node<T>(value, _M_comp));
    }


    template <class ...Ts>
    iterator _M_multi_emplace(Ts &&...value) {
        TreeNodeImpl<T> *node = new TreeNodeImpl<T>;
        new (std::addressof(node->value)) T(std::forward<Ts>(value)...);
        this->_M_multi_insert_node<T>(node, _M_comp);
        return node;
    }

    template <class ...Ts>
    std::pair<iterator, bool> _M_single_emplace(Ts &&...value) {
        TreeNode *node = new TreeNodeImpl<T>;
        new (std::addressof(static_cast<TreeNodeImpl<T> *>(node)->value)) T(std::forward<Ts>(value)...);
        TreeNode *conflict = this->_M_single_insert_node<T>(node, _M_comp);
        if (conflict) {
            static_cast<TreeNodeImpl<T> *>(node)->value.~T();
            delete node;
            return {conflict, false};
        } else {
            return {node, true};
        }
    }

    void clear() noexcept {
        for (auto it = this->begin(); it != this->end(); ++it) {
            it = this->erase(it);
        }
    }

    static iterator erase(const_iterator it) noexcept {
        const_iterator tmp = it;
        ++tmp;
        TreeNode *node = it.node;
        TreeImpl::_M_erase_node(node);
        static_cast<TreeNodeImpl<T> *>(node)->value.~T();
        delete node;
        return iterator(tmp);
    }

    template <class Tv>
    size_t _M_single_erase(Tv &&value) noexcept {
        TreeNode *node = this->_M_find_node<T>(value, _M_comp);
        if (node != nullptr) {
            this->_M_erase_node(node);
            static_cast<TreeNodeImpl<T> *>(node)->value.~T();
            delete node;
            return 1;
        } else {
            return 0;
        }
    }

    static std::pair<iterator, size_t> _M_erase_range(const_iterator first, const_iterator last) noexcept {
        size_t num = 0;
        while (first != last) {
            first = TreeImpl::erase(first);
            ++num;
        }
        return {iterator(first), num};
    }

    template <class Tv>
    size_t _M_multi_erase(Tv &&value) noexcept {
        std::pair<iterator, iterator> range = this->equal_range(value);
        return this->_M_erase_range(range.first, range.second).second;
    }

    static iterator erase(const_iterator first, const_iterator last) noexcept {
        return TreeImpl::_M_erase_range(first, last).first;
    }

    template <class Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    iterator lower_bound(Tv &&value) noexcept {
        return this->_M_lower_bound<T>(value, _M_comp);
    }

    template <class Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    const_iterator lower_bound(Tv &&value) const noexcept {
        return this->_M_lower_bound<T>(value, _M_comp);
    }

    template <class Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    iterator upper_bound(Tv &&value) noexcept {
        return this->_M_upper_bound<T>(value, _M_comp);
    }

    template <class Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    const_iterator upper_bound(Tv &&value) const noexcept {
        return this->_M_upper_bound<T>(value, _M_comp);
    }

    template <class Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    std::pair<iterator, iterator> equal_range(Tv &&value) noexcept {
        return {this->lower_bound(value), this->upper_bound(value)};
    }

    template <class Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    std::pair<const_iterator, const_iterator> equal_range(Tv &&value) const noexcept {
        return {this->lower_bound(value), this->upper_bound(value)};
    }


    iterator lower_bound(T const &value) noexcept {
        return this->_M_prevent_end(this->_M_lower_bound<T>(value, _M_comp));
    }

    const_iterator lower_bound(T const &value) const noexcept {
        return this->_M_prevent_end(this->_M_lower_bound<T>(value, _M_comp));
    }

    iterator upper_bound(T const &value) noexcept {
        return this->_M_prevent_end(this->_M_upper_bound<T>(value, _M_comp));
    }

    const_iterator upper_bound(T const &value) const noexcept {
        return this->_M_prevent_end(this->_M_upper_bound<T>(value, _M_comp));
    }

    std::pair<iterator, iterator> equal_range(T const &value) noexcept {
        return {this->lower_bound(value), this->upper_bound(value)};
    }

    std::pair<const_iterator, const_iterator> equal_range(T const &value) const noexcept {
        return {this->lower_bound(value), this->upper_bound(value)};
    }

    template <class Tv>
    size_t _M_multi_count(Tv &&value) const noexcept {
        auto it = this->lower_bound(value);
        return it != end() ? std::distance(it, this->upper_bound()) : 0;
    }

    template <class Tv>
    bool _M_contains(Tv &&value) const noexcept {
        return this->template _M_find_node<T>(value, _M_comp) != nullptr;
    }

    iterator _M_prevent_end(TreeNode *node) noexcept {
        return node == nullptr ? end() : node;
    }

    reverse_iterator _M_prevent_rend(TreeNode *node) noexcept {
        return node == nullptr ? rend() : node;
    }

    const_iterator _M_prevent_end(TreeNode *node) const noexcept {
        return node == nullptr ? end() : node;
    }

    const_reverse_iterator _M_prevent_rend(TreeNode *node) const noexcept {
        return node == nullptr ? rend() : node;
    }

    iterator begin() noexcept {
        return this->_M_prevent_end(this->_M_min_node());
    }

    reverse_iterator rbegin() noexcept {
        return this->_M_prevent_rend(this->_M_max_node());
    }

    iterator end() noexcept {
        return &block->root;
    }

    reverse_iterator rend() noexcept {
        return &block->root;
    }

    const_iterator begin() const noexcept {
        return this->_M_prevent_end(this->_M_min_node());
    }

    const_reverse_iterator rbegin() const noexcept {
        return this->_M_prevent_rend(this->_M_max_node());
    }

    const_iterator end() const noexcept {
        return &block->root;
    }

    const_reverse_iterator rend() const noexcept {
        return &block->root;
    }

    void _M_print(TreeNode *node) {
        if (node) {
            T &value = static_cast<TreeNodeImpl<T> *>(node)->value;
            if constexpr (requires (T t) { t.first; t.second; }) {
                std::cout << value.first << ':' << value.second;
            } else {
                std::cout << value;
            }
            std::cout << '(' << (node->color == BLACK ? 'B' : 'R') << ' ';
            if (node->left) {
                if (node->left->parent != node || node->left->pparent != &node->left) {
                    std::cout << '*';
                }
            }
            _M_print(node->left);
            std::cout << ' ';
            if (node->right) {
                if (node->right->parent != node || node->right->pparent != &node->right) {
                    std::cout << '*';
                }
            }
            _M_print(node->right);
            std::cout << ')';
        } else {
            std::cout << '.';
        }
    }

    void _M_print() {
        _M_print(this->block->root);
        std::cout << '\n';
    }
};

template <class T, class Compare = std::less<T>>
struct Set : TreeImpl<T, Compare> {
    using typename TreeImpl<T, Compare>::const_iterator;
    using iterator = const_iterator;
    using value_type = T;

    Set() = default;
    explicit Set(Compare comp) : TreeImpl<T, Compare>(comp) {}

    Set(Set &&) = default;
    Set &operator=(Set &&) = default;

    Set(Set const &that) : TreeImpl<T, Compare>() {
        this->_M_single_insert(that.begin(), that.end());
    }

    Set &operator=(Set const &that) {
        if (&that != this) {
            this->assign(that.begin(), that.end());
        }
        return *this;
    }

    Set &operator=(std::initializer_list<T> ilist) {
        this->assign(ilist);
        return *this;
    }

    void assign(std::initializer_list<T> ilist) {
        this->clear();
        this->_M_single_insert(ilist.begin(), ilist.end());
    }

    Compare value_comp() const noexcept {
        return this->_M_comp;
    }

    template <class Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    const_iterator find(T &&value) const noexcept {
        return this->_M_find(value);
    }

    const_iterator find(T const &value) const noexcept {
        return this->_M_find(value);
    }

    std::pair<iterator, bool> insert(T &&value) {
        return this->_M_single_emplace(std::move(value));
    }

    std::pair<iterator, bool> insert(T const &value) {
        return this->_M_single_emplace(value);
    }

    template <class ...Ts>
    std::pair<iterator, bool> emplace(Ts &&...value) {
        return this->_M_single_emplace(std::forward<Ts>(value)...);
    }

    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, InputIt)>
    void insert(InputIt first, InputIt last) {
        return this->_M_single_insert(first, last);
    }

    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, InputIt)>
    void assign(InputIt first, InputIt last) {
        this->clear();
        return this->_M_single_insert(first, last);
    }

    using TreeImpl<T, Compare>::erase;

    template <class Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    size_t erase(Tv &&value) {
        return this->_M_single_erase(value);
    }

    size_t erase(T const &value) {
        return this->_M_single_erase(value);
    }

    template <class Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    size_t count(Tv &&value) const noexcept {
        return this->_M_contains(value) ? 1 : 0;
    }

    size_t count(T const &value) const noexcept {
        return this->_M_contains(value) ? 1 : 0;
    }

    template <class Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    bool contains(Tv &&value) const noexcept {
        return this->_M_contains(value);
    }

    bool contains(T const &value) const noexcept {
        return this->_M_contains(value);
    }
};

template <class T, class Compare = std::less<T>>
struct MultiSet : TreeImpl<T, Compare> {
    using typename TreeImpl<T, Compare>::const_iterator;
    using iterator = const_iterator;

    MultiSet() = default;
    explicit MultiSet(Compare comp) : TreeImpl<T, Compare>(comp) {}

    MultiSet(MultiSet &&) = default;
    MultiSet &operator=(MultiSet &&) = default;

    MultiSet(MultiSet const &that) : TreeImpl<T, Compare>() {
        this->_M_multi_insert(that.begin(), that.end());
    }

    MultiSet &operator=(MultiSet const &that) {
        if (&that != this) {
            this->assign(that.begin(), that.end());
        }
        return *this;
    }

    MultiSet &operator=(std::initializer_list<T> ilist) {
        this->assign(ilist);
        return *this;
    }

    void assign(std::initializer_list<T> ilist) {
        this->clear();
        this->_M_multi_insert(ilist.begin(), ilist.end());
    }

    Compare value_comp() const noexcept {
        return this->_M_comp;
    }

    template <class Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    const_iterator find(T &&value) const noexcept {
        return this->_M_find(value);
    }

    const_iterator find(T const &value) const noexcept {
        return this->_M_find(value);
    }

    iterator insert(T &&value) {
        return this->_M_multi_emplace(std::move(value));
    }

    iterator insert(T const &value) {
        return this->_M_multi_emplace(value);
    }

    template <class ...Ts>
    iterator emplace(Ts &&...value) {
        return this->_M_multi_emplace(std::forward<Ts>(value)...);
    }

    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, InputIt)>
    void insert(InputIt first, InputIt last) {
        return this->_M_multi_insert(first, last);
    }

    using TreeImpl<T, Compare>::assign;

    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, InputIt)>
    void assign(InputIt first, InputIt last) {
        this->clear();
        return this->_M_multi_insert(first, last);
    }

    using TreeImpl<T, Compare>::erase;

    template <class Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    size_t erase(Tv &&value) {
        return this->_M_multi_erase(value);
    }

    size_t erase(T const &value) {
        return this->_M_multi_erase(value);
    }

    template <class Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    size_t count(Tv &&value) const noexcept {
        return this->_M_multi_count(value);
    }

    size_t count(T const &value) const noexcept {
        return this->_M_multi_count(value);
    }

    template <class Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    bool contains(Tv &&value) const noexcept {
        return this->_M_contains(value);
    }

    bool contains(T const &value) const noexcept {
        return this->_M_contains(value);
    }
};

template <class Compare, class Value, class = void>
struct TreeValueCompare {
    [[no_unique_address]] Compare _M_comp;

    TreeValueCompare(Compare comp = {}) noexcept : _M_comp(comp) {}

    bool operator()(typename Value::first_type const &lhs, Value const &rhs) const noexcept {
        return this->_M_comp(lhs, rhs.first);
    }

    bool operator()(Value const &lhs, typename Value::first_type const &rhs) const noexcept {
        return this->_M_comp(lhs.first, rhs);
    }

    bool operator()(Value const &lhs, Value const &rhs) const noexcept {
        return this->_M_comp(lhs.first, rhs.first);
    }
};

template <class Compare, class Value>
struct TreeValueCompare<Compare, Value, decltype((void)static_cast<typename Compare::is_transparent *>(nullptr))> {
    [[no_unique_address]] Compare _M_comp;

    TreeValueCompare(Compare comp = {}) noexcept : _M_comp(comp) {}

    template <class Lhs>
    bool operator()(Lhs &&lhs, Value const &rhs) const noexcept {
        return this->_M_comp(lhs, rhs.first);
    }

    template <class Rhs>
    bool operator()(Value const &lhs, Rhs &&rhs) const noexcept {
        return this->_M_comp(lhs.first, rhs);
    }

    bool operator()(Value const &lhs, Value const &rhs) const noexcept {
        return this->_M_comp(lhs.first, rhs.first);
    }

    using is_transparent = typename Compare::is_transparent;
};

template <class Key, class Mapped, class Compare = std::less<Key>>
struct Map : TreeImpl<std::pair<const Key, Mapped>,
        TreeValueCompare<Compare, std::pair<const Key, Mapped>>> {
    using key_type = Key;
    using mapped_type = Mapped;
    using value_type = std::pair<const Key, Mapped>;

private:
    using ValueComp = TreeValueCompare<Compare, value_type>;

public:
    using typename TreeImpl<value_type, ValueComp>::iterator;
    using typename TreeImpl<value_type, ValueComp>::const_iterator;

    Map() = default;
    explicit Map(Compare comp) : TreeImpl<value_type, ValueComp>(comp) {}

    Map(std::initializer_list<value_type> ilist) {
        _M_single_insert(ilist.begin(), ilist.end());
    }

    explicit Map(std::initializer_list<value_type> ilist, Compare comp)
        : TreeImpl<value_type, ValueComp>(comp) {
        _M_single_insert(ilist.begin(), ilist.end());
    }

    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, InputIt)>
    explicit Map(InputIt first, InputIt last) {
        _M_single_insert(first, last);
    }

    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, InputIt)>
    explicit Map(InputIt first, InputIt last, Compare comp)
        : TreeImpl<value_type, ValueComp>(comp) {
        _M_single_insert(first, last);
    }

    Map(Map &&) = default;
    Map &operator=(Map &&) = default;

    Map(Map const &that) : TreeImpl<value_type, ValueComp>() {
        this->_M_single_insert(that.begin(), that.end());
    }

    Map &operator=(Map const &that) {
        if (&that != this) {
            this->assign(that.begin(), that.end());
        }
        return *this;
    }

    Map &operator=(std::initializer_list<value_type> ilist) {
        this->assign(ilist);
        return *this;
    }

    void assign(std::initializer_list<value_type> ilist) {
        this->clear();
        this->_M_single_insert(ilist.begin(), ilist.end());
    }

    Compare key_comp() const noexcept {
        return this->_M_comp->_M_comp;
    }

    ValueComp value_comp() const noexcept {
        return this->_M_comp;
    }

    template <class Kv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    iterator find(Kv &&key) noexcept {
        return this->_M_find(key);
    }

    template <class Kv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    const_iterator find(Kv &&key) const noexcept {
        return this->_M_find(key);
    }

    iterator find(Key const &key) noexcept {
        return this->_M_find(key);
    }

    const_iterator find(Key const &key) const noexcept {
        return this->_M_find(key);
    }

    std::pair<iterator, bool> insert(value_type &&value) {
        return this->_M_single_emplace(std::move(value));
    }

    std::pair<iterator, bool> insert(value_type const &value) {
        return this->_M_single_emplace(value);
    }

    template <class Kv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    Mapped const &at(Kv const &key) const {
        auto it = this->_M_find(key);
        if (it == this->end()) {
            throw std::out_of_range("map::at");
        }
        return it->second;
    }

    template <class Kv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    Mapped &at(Kv const &key) {
        auto it = this->_M_find(key);
        if (it == this->end()) {
            throw std::out_of_range("map::at");
        }
        return it->second;
    }

    Mapped const &at(Key const &key) const {
        auto it = this->_M_find(key);
        if (it == this->end()) {
            throw std::out_of_range("map::at");
        }
        return it->second;
    }

    Mapped &at(Key const &key) {
        auto it = this->_M_find(key);
        if (it == this->end()) {
            throw std::out_of_range("map::at");
        }
        return it->second;
    }

    template <class Kv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    Mapped &operator[](Kv const &key) {
        auto it = this->_M_find(key);
        if (it == this->end()) {
            it = this->_M_single_emplace(std::piecewise_construct,
                                         std::forward_as_tuple(key),
                                         std::forward_as_tuple()).first;
        }
        return it->second;
    }

    Mapped &operator[](Key const &key) {
        auto it = this->_M_find(key);
        if (it == this->end()) {
            it = this->_M_single_emplace(std::piecewise_construct,
                                         std::forward_as_tuple(key),
                                         std::forward_as_tuple()).first;
        }
        return it->second;
    }

    template <class M, class = std::enable_if_t<std::is_convertible_v<M, Mapped>>>
    std::pair<iterator, bool> insert_or_assign(Key const &key, M &&mapped) {
        auto result = this->_M_single_emplace(std::piecewise_construct,
                                              std::forward_as_tuple(key),
                                              std::forward_as_tuple(std::forward<M>(mapped)));
        if (!result.second) {
            result.first->second = std::forward<M>(mapped);
        }
        return result;
    }

    template <class M, class = std::enable_if_t<std::is_convertible_v<M, Mapped>>>
    std::pair<iterator, bool> insert_or_assign(Key &&key, M &&mapped) {
        auto result = this->_M_single_emplace(std::piecewise_construct,
                                              std::forward_as_tuple(std::move(key)),
                                              std::forward_as_tuple(std::forward<M>(mapped)));
        if (!result.second) {
            result.first->second = std::forward<M>(mapped);
        }
        return result;
    }

    template <class ...Vs>
    std::pair<iterator, bool> emplace(Vs &&...value) {
        return this->_M_single_emplace(std::forward<Vs>(value)...);
    }

    template <class ...Ms>
    std::pair<iterator, bool> try_emplace(Key &&key, Ms &&...mapped) {
        return this->_M_single_emplace(std::piecewise_construct,
                                       std::forward_as_tuple(std::move(key)),
                                       std::forward_as_tuple(std::forward<Ms>(mapped)...));
    }

    template <class ...Ms>
    std::pair<iterator, bool> try_emplace(Key const &key, Ms &&...mapped) {
        return this->_M_single_emplace(std::piecewise_construct,
                                       std::forward_as_tuple(key),
                                       std::forward_as_tuple(std::forward<Ms>(mapped)...));
    }

    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, InputIt)>
    iterator insert(InputIt first, InputIt last) {
        return this->_M_single_insert(first, last);
    }

    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, InputIt)>
    iterator assign(InputIt first, InputIt last) {
        this->clear();
        return this->_M_single_insert(first, last);
    }

    using TreeImpl<value_type, ValueComp>::erase;

    template <class Kv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    size_t erase(Kv &&key) {
        return this->_M_single_erase(key);
    }

    size_t erase(Key const &key) {
        return this->_M_single_erase(key);
    }

    template <class Kv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    size_t count(Kv &&value) const noexcept {
        return this->_M_contains(value) ? 1 : 0;
    }

    size_t count(Key const &value) const noexcept {
        return this->_M_contains(value) ? 1 : 0;
    }

    template <class Kv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    bool contains(Kv &&value) const noexcept {
        return this->_M_contains(value);
    }

    bool contains(Key const &value) const noexcept {
        return this->_M_contains(value);
    }
};

template <class Key, class Mapped, class Compare = std::less<Key>>
struct MultiMap : TreeImpl<std::pair<const Key, Mapped>,
        TreeValueCompare<Compare, std::pair<const Key, Mapped>>> {
    using key_type = Key;
    using mapped_type = Mapped;
    using value_type = std::pair<const Key, Mapped>;

private:
    using ValueComp = TreeValueCompare<Compare, value_type>;
public:

    using typename TreeImpl<value_type, ValueComp>::iterator;
    using typename TreeImpl<value_type, ValueComp>::const_iterator;

    MultiMap() = default;
    explicit MultiMap(Compare comp) : TreeImpl<value_type, ValueComp>(comp) {}

    MultiMap(std::initializer_list<value_type> ilist) {
        _M_multi_insert(ilist.begin(), ilist.end());
    }

    explicit MultiMap(std::initializer_list<value_type> ilist, Compare comp) : TreeImpl<value_type, ValueComp>(comp) {
        _M_multi_insert(ilist.begin(), ilist.end());
    }

    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, InputIt)>
    explicit MultiMap(InputIt first, InputIt last) {
        _M_multi_insert(first, last);
    }

    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, InputIt)>
    explicit MultiMap(InputIt first, InputIt last, Compare comp)
        : TreeImpl<value_type, ValueComp>(comp) {
        _M_multi_insert(first, last);
    }

    MultiMap(MultiMap &&) = default;
    MultiMap &operator=(MultiMap &&) = default;

    MultiMap(MultiMap const &that) : TreeImpl<value_type, ValueComp>() {
        this->_M_multi_insert(that.begin(), that.end());
    }

    MultiMap &operator=(MultiMap const &that) {
        if (&that != this) {
            this->assign(that.begin(), that.end());
        }
        return *this;
    }

    MultiMap &operator=(std::initializer_list<value_type> ilist) {
        this->assign(ilist);
        return *this;
    }

    void assign(std::initializer_list<value_type> ilist) {
        this->clear();
        this->_M_multi_insert(ilist.begin(), ilist.end());
    }

    Compare key_comp() const noexcept {
        return this->_M_comp->_M_comp;
    }

    ValueComp value_comp() const noexcept {
        return this->_M_comp;
    }

    template <class Kv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    iterator find(Kv &&key) noexcept {
        return this->_M_find(key);
    }

    template <class Kv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    const_iterator find(Kv &&key) const noexcept {
        return this->_M_find(key);
    }

    iterator find(Key const &key) noexcept {
        return this->_M_find(key);
    }

    const_iterator find(Key const &key) const noexcept {
        return this->_M_find(key);
    }

    std::pair<iterator, bool> insert(value_type &&value) {
        return this->_M_single_emplace(std::move(value));
    }

    std::pair<iterator, bool> insert(value_type const &value) {
        return this->_M_single_emplace(value);
    }

    template <class ...Ts>
    std::pair<iterator, bool> emplace(Ts &&...value) {
        return this->_M_single_emplace(std::forward<Ts>(value)...);
    }

    template <class ...Ts>
    std::pair<iterator, bool> try_emplace(Key &&key, Ts &&...value) {
        return this->_M_single_emplace(std::piecewise_construct,
                                       std::forward_as_tuple(std::move(key)),
                                       std::forward_as_tuple(std::forward<Ts>(value)...));
    }

    template <class ...Ts>
    std::pair<iterator, bool> try_emplace(Key const &key, Ts &&...value) {
        return this->_M_single_emplace(std::piecewise_construct,
                                       std::forward_as_tuple(key),
                                       std::forward_as_tuple(std::forward<Ts>(value)...));
    }

    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, InputIt)>
    iterator insert(InputIt first, InputIt last) {
        return this->_M_single_insert(first, last);
    }

    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, InputIt)>
    iterator assign(InputIt first, InputIt last) {
        this->clear();
        return this->_M_single_insert(first, last);
    }

    using TreeImpl<value_type, ValueComp>::erase;

    template <class Kv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    size_t erase(Kv &&key) {
        return this->_M_single_erase(key);
    }

    size_t erase(Key const &key) {
        return this->_M_single_erase(key);
    }

    template <class Kv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    size_t count(Kv &&value) const noexcept {
        return this->_M_multi_count(value);
    }

    size_t count(Key const &value) const noexcept {
        return this->_M_multi_count(value);
    }

    template <class Kv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    bool contains(Kv &&value) const noexcept {
        return this->_M_contains(value);
    }

    bool contains(Key const &value) const noexcept {
        return this->_M_contains(value);
    }
};
