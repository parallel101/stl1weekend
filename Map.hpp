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
// #include <iostream>
#include <stdexcept>
#include <type_traits>
#include <iterator>
#include <utility>

#if __cpp_concepts && __cpp_lib_concepts
#define _LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(__category, _Type) __category _Type
#else
#define _LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(__category, _Type) class _Type, \
    std::enable_if_t<std::is_convertible_v< \
        typename std::iterator_traits<_Type>::iterator_category, \
        __category##_tag>>
#endif
#define _LIBPENGCXX_REQUIRES_TRANSPARENT(_Compare) \
    class _Compare##Tp = _Compare, \
    class = typename _Compare##Tp::is_transparent

enum _RbTreeColor {
    _S_black,
    _S_red,
};

enum _RbTreeChildDir {
    _S_left,
    _S_right,
};

struct _RbTreeNode {
    _RbTreeNode *_M_left;     // 左子节点指针
    _RbTreeNode *_M_right;    // 右子节点指针
    _RbTreeNode *_M_parent;   // 父节点指针
    _RbTreeNode **_M_pparent; // 父节点中指向本节点指针的指针
    _RbTreeColor _M_color;    // 红或黑
};

template <class _T>
struct _RbTreeNodeImpl : _RbTreeNode {
    union {
        _T _M_value;
    }; // union 可以阻止里面成员的自动初始化，方便不支持 _T() 默认构造的类型

    _RbTreeNodeImpl() noexcept {}
    ~_RbTreeNodeImpl() noexcept {}
};

template <bool kReverse>
struct _RbTreeIteratorBase;

template <>
struct _RbTreeIteratorBase<false> {
    union {
        _RbTreeNode *_M_node;
        _RbTreeNode **_M_proot;
    };
    bool _M_off_by_one;

    _RbTreeIteratorBase(_RbTreeNode *__node) noexcept : _M_node(__node), _M_off_by_one(false) {
    }

    _RbTreeIteratorBase(_RbTreeNode **__proot) noexcept : _M_proot(__proot), _M_off_by_one(true) {}

    bool operator==(_RbTreeIteratorBase const &__that) const noexcept {
        return (!_M_off_by_one && !__that._M_off_by_one && _M_node == __that._M_node) || (_M_off_by_one && __that._M_off_by_one);
    }

    bool operator!=(_RbTreeIteratorBase const &__that) const noexcept {
        return !(*this == __that);
    }

    void operator++() noexcept {   // ++__it
        if (_M_off_by_one) {
            _M_off_by_one = false;
            _M_node = *_M_proot;
            assert(_M_node);
            while (_M_node->_M_left != nullptr) {
                _M_node = _M_node->_M_left;
            }
            return;
        }
        assert(_M_node);
        if (_M_node->_M_right != nullptr) {
            _M_node = _M_node->_M_right;
            while (_M_node->_M_left != nullptr) {
                _M_node = _M_node->_M_left;
            }
        } else {
            while (_M_node->_M_parent != nullptr && _M_node->_M_pparent == &_M_node->_M_parent->_M_right) {
                _M_node = _M_node->_M_parent;
            }
            if (_M_node->_M_parent == nullptr) {
                _M_off_by_one = true;
                return;
            }
            _M_node = _M_node->_M_parent;
        }
    }

    void operator--() noexcept {   // --__it
        // 为了支持 --end()
        if (_M_off_by_one) {
            _M_off_by_one = false;
            _M_node = *_M_proot;
            assert(_M_node);
            while (_M_node->_M_right != nullptr) {
                _M_node = _M_node->_M_right;
            }
            return;
        }
        assert(_M_node);
        if (_M_node->_M_left != nullptr) {
            _M_node = _M_node->_M_left;
            while (_M_node->_M_right != nullptr) {
                _M_node = _M_node->_M_right;
            }
        } else {
            while (_M_node->_M_parent != nullptr && _M_node->_M_pparent == &_M_node->_M_parent->_M_left) {
                _M_node = _M_node->_M_parent;
            }
            if (_M_node->_M_parent == nullptr) {
                _M_off_by_one = true;
                return;
            }
            _M_node = _M_node->_M_parent;
        }
    }

    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
};

template <>
struct _RbTreeIteratorBase<true> : _RbTreeIteratorBase<false> {
    using _RbTreeIteratorBase<false>::_RbTreeIteratorBase;

    void operator++() noexcept {   // ++__it
        _RbTreeIteratorBase<false>::operator--();
    }

    void operator--() noexcept {   // --__it
        _RbTreeIteratorBase<false>::operator++();
    }
};

template <class _T, bool kReverse>
struct _RbTreeIterator : _RbTreeIteratorBase<kReverse> {
    using _RbTreeIteratorBase<kReverse>::_RbTreeIteratorBase;

    template <class T0 = _T>
    explicit operator std::enable_if_t<std::is_const_v<T0>,
        _RbTreeIterator<std::remove_const_t<T0>, kReverse>>() const noexcept {
        return this->_M_node;
    }

    template <class T0 = _T>
    operator std::enable_if_t<!std::is_const_v<T0>,
        _RbTreeIterator<std::add_const_t<T0>, kReverse>>() const noexcept {
        return this->_M_node;
    }

    _RbTreeIterator &operator++() noexcept {   // ++__it
        _RbTreeIteratorBase<kReverse>::operator++();
        return *this;
    }

    _RbTreeIterator &operator--() noexcept {   // --__it
        _RbTreeIteratorBase<kReverse>::operator--();
        return *this;
    }

    _RbTreeIterator operator++(int) noexcept { // __it++
        _RbTreeIterator __tmp = *this;
        ++*this;
        return __tmp;
    }

    _RbTreeIterator operator--(int) noexcept { // __it--
        _RbTreeIterator __tmp = *this;
        --*this;
        return __tmp;
    }

    _T *operator->() const noexcept {
        return std::addressof(static_cast<_RbTreeNodeImpl<_T> *>(this->_M_node)->_M_value);
    }

    _T &operator*() const noexcept {
        return static_cast<_RbTreeNodeImpl<_T> *>(this->_M_node)->_M_value;
    }

    using value_type = std::remove_const_t<_T>;
    using reference = _T &;
    using pointer = _T *;
};

struct _RbTreeRoot {
    _RbTreeNode *_M_root;

    _RbTreeRoot() noexcept : _M_root(nullptr) {
    }
};

struct _RbTreeBase {
    _RbTreeRoot *_M_block;

    explicit _RbTreeBase(_RbTreeRoot *__block) : _M_block(__block) {}

    static void _M_rotate_left(_RbTreeNode *__node) noexcept {
        _RbTreeNode *__right = __node->_M_right;
        __node->_M_right = __right->_M_left;
        if (__right->_M_left != nullptr) {
            __right->_M_left->_M_parent = __node;
            __right->_M_left->_M_pparent = &__node->_M_right;
        }
        __right->_M_parent = __node->_M_parent;
        __right->_M_pparent = __node->_M_pparent;
        *__node->_M_pparent = __right;
        __right->_M_left = __node;
        __node->_M_parent = __right;
        __node->_M_pparent = &__right->_M_left;
    }

    static void _M_rotate_right(_RbTreeNode *__node) noexcept {
        _RbTreeNode *__left = __node->_M_left;
        __node->_M_left = __left->_M_right;
        if (__left->_M_right != nullptr) {
            __left->_M_right->_M_parent = __node;
            __left->_M_right->_M_pparent = &__node->_M_left;
        }
        __left->_M_parent = __node->_M_parent;
        __left->_M_pparent = __node->_M_pparent;
        *__node->_M_pparent = __left;
        __left->_M_right = __node;
        __node->_M_parent = __left;
        __node->_M_pparent = &__left->_M_right;
    }

    static void _M_fix_violation(_RbTreeNode *__node) noexcept {
        while (true) {
            _RbTreeNode *__parent = __node->_M_parent;
            if (__parent == nullptr) { // 等价于：__node == root，因为 root->__parent == nullptr
                // 情况 0: __node == root
                __node->_M_color = _S_black;
                return;
            }
            if (__node->_M_color == _S_black || __parent->_M_color == _S_black)
                return;
            _RbTreeNode *__uncle;
            _RbTreeNode *__grandpa = __parent->_M_parent;
            assert(__grandpa);
            _RbTreeChildDir __parent_dir = __parent->_M_pparent == &__grandpa->_M_left ? _S_left : _S_right;
            if (__parent_dir == _S_left) {
                __uncle = __grandpa->_M_right;
            } else {
                assert(__parent->_M_pparent == &__grandpa->_M_right);
                __uncle = __grandpa->_M_left;
            }
            _RbTreeChildDir __node_dir = __node->_M_pparent == &__parent->_M_left ? _S_left : _S_right;
            if (__uncle != nullptr && __uncle->_M_color == _S_red) {
                // 情况 1: 叔叔是红色人士
                __parent->_M_color = _S_black;
                __uncle->_M_color = _S_black;
                __grandpa->_M_color = _S_red;
                __node = __grandpa;
            } else if (__node_dir == __parent_dir) {
                if (__node_dir == _S_right) {
                    assert(__node->_M_pparent == &__parent->_M_right);
                    // 情况 2: 叔叔是黑色人士（RR）
                    _RbTreeBase::_M_rotate_left(__grandpa);
                } else {
                    // 情况 3: 叔叔是黑色人士（LL）
                    _RbTreeBase::_M_rotate_right(__grandpa);
                }
                std::swap(__parent->_M_color, __grandpa->_M_color);
                __node = __grandpa;
            } else {
                if (__node_dir == _S_right) {
                    assert(__node->_M_pparent == &__parent->_M_right);
                    // 情况 4: 叔叔是黑色人士（LR）
                    _RbTreeBase::_M_rotate_left(__parent);
                } else {
                    // 情况 5: 叔叔是黑色人士（RL）
                    _RbTreeBase::_M_rotate_right(__parent);
                }
                __node = __parent;
            }
        }
    }

    _RbTreeNode *_M_min_node() const noexcept {
        _RbTreeNode *__current = _M_block->_M_root;
        if (__current != nullptr) {
            while (__current->_M_left != nullptr) {
                __current = __current->_M_left;
            }
        }
        return __current;
    }

    _RbTreeNode *_M_max_node() const noexcept {
        _RbTreeNode *__current = _M_block->_M_root;
        if (__current != nullptr) {
            while (__current->_M_right != nullptr) {
                __current = __current->_M_right;
            }
        }
        return __current;
    }

    template <class _T, class _Tv, class _Compare>
    _RbTreeNode *_M_find_node(_Tv &&__value, _Compare __comp) const noexcept {
        _RbTreeNode *__current = _M_block->_M_root;
        while (__current != nullptr) {
            if (__comp(__value, static_cast<_RbTreeNodeImpl<_T> *>(__current)->_M_value)) {
                __current = __current->_M_left;
                continue;
            }
            if (__comp(static_cast<_RbTreeNodeImpl<_T> *>(__current)->_M_value, __value)) {
                __current = __current->_M_right;
                continue;
            }
            // __value == curent->_M_value
            return __current;
        }
        return nullptr;
    }

    template <class _T, class _Tv, class _Compare>
    _RbTreeNode *_M_lower_bound(_Tv &&__value, _Compare __comp) const noexcept {
        _RbTreeNode *__current = _M_block->_M_root;
        _RbTreeNode *__result = nullptr;
        while (__current != nullptr) {
            if (!(__comp(static_cast<_RbTreeNodeImpl<_T> *>(__current)->_M_value, __value))) { // __current->_M_value >= __value
                __result = __current;
                __current = __current->_M_left;
            } else {
                __current = __current->_M_right;
            }
        }
        return __result;
    }

    template <class _T, class _Tv, class _Compare>
    _RbTreeNode *_M_upper_bound(_Tv &&__value, _Compare __comp) const noexcept {
        _RbTreeNode *__current = _M_block->_M_root;
        _RbTreeNode *__result = nullptr;
        while (__current != nullptr) {
            if (__comp(__value, static_cast<_RbTreeNodeImpl<_T> *>(__current)->_M_value)) { // __current->_M_value > __value
                __result = __current;
                __current = __current->_M_left;
            } else {
                __current = __current->_M_right;
            }
        }
        return __result;
    }

    template <class _T, class _Tv, class _Compare>
    std::pair<_RbTreeNode *, _RbTreeNode *> _M_equal_range(_Tv &&__value, _Compare __comp) const noexcept {
        return {this->_M_lower_bound<_T>(__value, __comp), this->_M_upper_bound<_T>(__value, __comp)};
    }

    static void _M_transplant(_RbTreeNode *__node, _RbTreeNode *__replace) noexcept {
        *__node->_M_pparent = __replace;
        if (__replace != nullptr) {
            __replace->_M_parent = __node->_M_parent;
            __replace->_M_pparent = __node->_M_pparent;
        }
    }

    static void _M_delete_fixup(_RbTreeNode *__node) noexcept {
        if (__node == nullptr)
            return;
        while (__node->_M_parent != nullptr && __node->_M_color == _S_black) {
            _RbTreeChildDir __dir = __node->_M_pparent == &__node->_M_parent->_M_left ? _S_left : _S_right;
            _RbTreeNode *__sibling = __dir == _S_left ? __node->_M_parent->_M_right : __node->_M_parent->_M_left;
            if (__sibling->_M_color == _S_red) {
                __sibling->_M_color = _S_black;
                __node->_M_parent->_M_color = _S_red;
                if (__dir == _S_left) {
                    _RbTreeBase::_M_rotate_left(__node->_M_parent);
                } else {
                    _RbTreeBase::_M_rotate_right(__node->_M_parent);
                }
                __sibling = __dir == _S_left ? __node->_M_parent->_M_right : __node->_M_parent->_M_left;
            }
            if (__sibling->_M_left->_M_color == _S_black && __sibling->_M_right->_M_color == _S_black) {
                __sibling->_M_color = _S_red;
                __node = __node->_M_parent;
            } else {
                if (__dir == _S_left && __sibling->_M_right->_M_color == _S_black) {
                    __sibling->_M_left->_M_color = _S_black;
                    __sibling->_M_color = _S_red;
                    _RbTreeBase::_M_rotate_right(__sibling);
                    __sibling = __node->_M_parent->_M_right;
                } else if (__dir == _S_right && __sibling->_M_left->_M_color == _S_black) {
                    __sibling->_M_right->_M_color = _S_black;
                    __sibling->_M_color = _S_red;
                    _RbTreeBase::_M_rotate_left(__sibling);
                    __sibling = __node->_M_parent->_M_left;
                }
                __sibling->_M_color = __node->_M_parent->_M_color;
                __node->_M_parent->_M_color = _S_black;
                if (__dir == _S_left) {
                    __sibling->_M_right->_M_color = _S_black;
                    _RbTreeBase::_M_rotate_left(__node->_M_parent);
                } else {
                    __sibling->_M_left->_M_color = _S_black;
                    _RbTreeBase::_M_rotate_right(__node->_M_parent);
                }
                // while (__node->_M_parent != nullptr) {
                //     __node = __node->_M_parent;
                // }
                break;
            }
        }
        __node->_M_color = _S_black;
    }

    static void _M_erase_node(_RbTreeNode *__node) noexcept {
        if (__node->_M_left == nullptr) {
            _RbTreeNode *__right = __node->_M_right;
            _RbTreeColor __color = __node->_M_color;
            _RbTreeBase::_M_transplant(__node, __right);
            if (__color == _S_black) {
                _RbTreeBase::_M_delete_fixup(__right);
            }
        } else if (__node->_M_right == nullptr) {
            _RbTreeNode *__left = __node->_M_left;
            _RbTreeColor __color = __node->_M_color;
            _RbTreeBase::_M_transplant(__node, __left);
            if (__color == _S_black) {
                _RbTreeBase::_M_delete_fixup(__left);
            }
        } else {
            _RbTreeNode *__replace = __node->_M_right;
            while (__replace->_M_left != nullptr) {
                __replace = __replace->_M_left;
            }
            _RbTreeNode *__right = __replace->_M_right;
            _RbTreeColor __color = __replace->_M_color;
            if (__replace->_M_parent == __node) {
                __right->_M_parent = __replace;
                __right->_M_pparent = &__replace->_M_right;
            } else {
                _RbTreeBase::_M_transplant(__replace, __right);
                __replace->_M_right = __node->_M_right;
                __replace->_M_right->_M_parent = __replace;
                __replace->_M_right->_M_pparent = &__replace->_M_right;
            }
            _RbTreeBase::_M_transplant(__node, __replace);
            __replace->_M_left = __node->_M_left;
            __replace->_M_left->_M_parent = __replace;
            __replace->_M_left->_M_pparent = &__replace->_M_left;
            if (__color == _S_black) {
                _RbTreeBase::_M_delete_fixup(__right);
            }
        }
    }

    template <class _T, class _Compare>
    _RbTreeNode *_M_single_insert_node(_RbTreeNode *__node, _Compare __comp) {
        _RbTreeNode **__pparent = &_M_block->_M_root;
        _RbTreeNode *__parent = nullptr;
        while (*__pparent != nullptr) {
            __parent = *__pparent;
            if (__comp(static_cast<_RbTreeNodeImpl<_T> *>(__node)->_M_value, static_cast<_RbTreeNodeImpl<_T> *>(__parent)->_M_value)) {
                __pparent = &__parent->_M_left;
                continue;
            }
            if (__comp(static_cast<_RbTreeNodeImpl<_T> *>(__parent)->_M_value, static_cast<_RbTreeNodeImpl<_T> *>(__node)->_M_value)) {
                __pparent = &__parent->_M_right;
                continue;
            }
            return __parent;
        }

        __node->_M_left = nullptr;
        __node->_M_right = nullptr;
        __node->_M_color = _S_red;

        __node->_M_parent = __parent;
        __node->_M_pparent = __pparent;
        *__pparent = __node;
        _RbTreeBase::_M_fix_violation(__node);
        return nullptr;
    }

    template <class _T, class _Compare>
    void _M_multi_insert_node(_RbTreeNode *__node, _Compare __comp) {
        _RbTreeNode **__pparent = &_M_block->_M_root;
        _RbTreeNode *__parent = nullptr;
        while (*__pparent != nullptr) {
            __parent = *__pparent;
            if (__comp(static_cast<_RbTreeNodeImpl<_T> *>(__node)->_M_value, static_cast<_RbTreeNodeImpl<_T> *>(__parent)->_M_value)) {
                __pparent = &__parent->_M_left;
                continue;
            }
            if (__comp(static_cast<_RbTreeNodeImpl<_T> *>(__parent)->_M_value, static_cast<_RbTreeNodeImpl<_T> *>(__node)->_M_value)) {
                __pparent = &__parent->_M_right;
                continue;
            }
            __pparent = &__parent->_M_right;
        }

        __node->_M_left = nullptr;
        __node->_M_right = nullptr;
        __node->_M_color = _S_red;

        __node->_M_parent = __parent;
        __node->_M_pparent = __pparent;
        *__pparent = __node;
        _RbTreeBase::_M_fix_violation(__node);
    }
};

template <class _T, class _Compare>
struct _RbTreeImpl : _RbTreeBase {
    [[no_unique_address]] _Compare _M_comp;

    _RbTreeImpl() noexcept : _RbTreeBase(new _RbTreeRoot) {}

    explicit _RbTreeImpl(_Compare __comp) noexcept : _RbTreeBase(new _RbTreeRoot), _M_comp(__comp) {}

    _RbTreeImpl(_RbTreeImpl &&__that) noexcept : _RbTreeBase(__that._M_block) {
        __that._M_block = nullptr;
    }

    _RbTreeImpl &operator=(_RbTreeImpl &&__that) noexcept {
        std::swap(_M_block, __that._M_block);
        return *this;
    }

    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, _InputIt)>
    void _M_single_insert(_InputIt __first, _InputIt __last) {
        while (__first != __last) {
            this->_M_single_insert(*__first);
            ++__first;
        }
    }

    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, _InputIt)>
    void _M_multi_insert(_InputIt __first, _InputIt __last) {
        while (__first != __last) {
            this->_M_multi_insert(*__first);
            ++__first;
        }
    }

    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, _InputIt)>
    void assign(_InputIt __first, _InputIt __last) {
        this->clear();
        this->_M_multi_insert(__first, __last);
    }

    using iterator = _RbTreeIterator<_T, false>;
    using reverse_iterator = _RbTreeIterator<_T, true>;
    using const_iterator = _RbTreeIterator<_T const, false>;
    using const_reverse_iterator = _RbTreeIterator<_T const, true>;

    static_assert(std::input_iterator<iterator>);

    template <class _Tv>
    const_iterator _M_find(_Tv &&__value) const noexcept {
        return this->_M_prevent_end(this->_M_find_node<_T>(__value, _M_comp));
    }

    template <class _Tv>
    iterator _M_find(_Tv &&__value) noexcept {
        return this->_M_prevent_end(this->_M_find_node<_T>(__value, _M_comp));
    }


    template <class ...Ts>
    iterator _M_multi_emplace(Ts &&...__value) {
        _RbTreeNodeImpl<_T> *__node = new _RbTreeNodeImpl<_T>;
        new (std::addressof(__node->_M_value)) _T(std::forward<Ts>(__value)...);
        this->_M_multi_insert_node<_T>(__node, _M_comp);
        return __node;
    }

    template <class ...Ts>
    std::pair<iterator, bool> _M_single_emplace(Ts &&...__value) {
        _RbTreeNode *__node = new _RbTreeNodeImpl<_T>;
        new (std::addressof(static_cast<_RbTreeNodeImpl<_T> *>(__node)->_M_value)) _T(std::forward<Ts>(__value)...);
        _RbTreeNode *conflict = this->_M_single_insert_node<_T>(__node, _M_comp);
        if (conflict) {
            static_cast<_RbTreeNodeImpl<_T> *>(__node)->_M_value.~_T();
            delete __node;
            return {conflict, false};
        } else {
            return {__node, true};
        }
    }

    void clear() noexcept {
        for (iterator __it = this->begin(); __it != this->end(); ++__it) {
            __it = this->erase(__it);
        }
    }

    static iterator erase(const_iterator __it) noexcept {
        const_iterator __tmp = __it;
        ++__tmp;
        _RbTreeNode *__node = __it._M_node;
        _RbTreeImpl::_M_erase_node(__node);
        static_cast<_RbTreeNodeImpl<_T> *>(__node)->_M_value.~_T();
        delete __node;
        return iterator(__tmp);
    }

    template <class _Tv>
    size_t _M_single_erase(_Tv &&__value) noexcept {
        _RbTreeNode *__node = this->_M_find_node<_T>(__value, _M_comp);
        if (__node != nullptr) {
            this->_M_erase_node(__node);
            static_cast<_RbTreeNodeImpl<_T> *>(__node)->_M_value.~_T();
            delete __node;
            return 1;
        } else {
            return 0;
        }
    }

    static std::pair<iterator, size_t> _M_erase_range(const_iterator __first, const_iterator __last) noexcept {
        size_t num = 0;
        while (__first != __last) {
            __first = _RbTreeImpl::erase(__first);
            ++num;
        }
        return {iterator(__first), num};
    }

    template <class _Tv>
    size_t _M_multi_erase(_Tv &&__value) noexcept {
        std::pair<iterator, iterator> range = this->equal_range(__value);
        return this->_M_erase_range(range.first, range.second).second;
    }

    static iterator erase(const_iterator __first, const_iterator __last) noexcept {
        return _RbTreeImpl::_M_erase_range(__first, __last).first;
    }

    template <class _Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(_Compare)>
    iterator lower_bound(_Tv &&__value) noexcept {
        return this->_M_lower_bound<_T>(__value, _M_comp);
    }

    template <class _Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(_Compare)>
    const_iterator lower_bound(_Tv &&__value) const noexcept {
        return this->_M_lower_bound<_T>(__value, _M_comp);
    }

    template <class _Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(_Compare)>
    iterator upper_bound(_Tv &&__value) noexcept {
        return this->_M_upper_bound<_T>(__value, _M_comp);
    }

    template <class _Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(_Compare)>
    const_iterator upper_bound(_Tv &&__value) const noexcept {
        return this->_M_upper_bound<_T>(__value, _M_comp);
    }

    template <class _Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(_Compare)>
    std::pair<iterator, iterator> equal_range(_Tv &&__value) noexcept {
        return {this->lower_bound(__value), this->upper_bound(__value)};
    }

    template <class _Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(_Compare)>
    std::pair<const_iterator, const_iterator> equal_range(_Tv &&__value) const noexcept {
        return {this->lower_bound(__value), this->upper_bound(__value)};
    }


    iterator lower_bound(_T const &__value) noexcept {
        return this->_M_prevent_end(this->_M_lower_bound<_T>(__value, _M_comp));
    }

    const_iterator lower_bound(_T const &__value) const noexcept {
        return this->_M_prevent_end(this->_M_lower_bound<_T>(__value, _M_comp));
    }

    iterator upper_bound(_T const &__value) noexcept {
        return this->_M_prevent_end(this->_M_upper_bound<_T>(__value, _M_comp));
    }

    const_iterator upper_bound(_T const &__value) const noexcept {
        return this->_M_prevent_end(this->_M_upper_bound<_T>(__value, _M_comp));
    }

    std::pair<iterator, iterator> equal_range(_T const &__value) noexcept {
        return {this->lower_bound(__value), this->upper_bound(__value)};
    }

    std::pair<const_iterator, const_iterator> equal_range(_T const &__value) const noexcept {
        return {this->lower_bound(__value), this->upper_bound(__value)};
    }

    template <class _Tv>
    size_t _M_multi_count(_Tv &&__value) const noexcept {
        const_iterator __it = this->lower_bound(__value);
        return __it != end() ? std::distance(__it, this->upper_bound()) : 0;
    }

    template <class _Tv>
    bool _M_contains(_Tv &&__value) const noexcept {
        return this->template _M_find_node<_T>(__value, _M_comp) != nullptr;
    }

    iterator _M_prevent_end(_RbTreeNode *__node) noexcept {
        return __node == nullptr ? end() : __node;
    }

    reverse_iterator _M_prevent_rend(_RbTreeNode *__node) noexcept {
        return __node == nullptr ? rend() : __node;
    }

    const_iterator _M_prevent_end(_RbTreeNode *__node) const noexcept {
        return __node == nullptr ? end() : __node;
    }

    const_reverse_iterator _M_prevent_rend(_RbTreeNode *__node) const noexcept {
        return __node == nullptr ? rend() : __node;
    }

    iterator begin() noexcept {
        return this->_M_prevent_end(this->_M_min_node());
    }

    reverse_iterator rbegin() noexcept {
        return this->_M_prevent_rend(this->_M_max_node());
    }

    iterator end() noexcept {
        return &_M_block->_M_root;
    }

    reverse_iterator rend() noexcept {
        return &_M_block->_M_root;
    }

    const_iterator begin() const noexcept {
        return this->_M_prevent_end(this->_M_min_node());
    }

    const_reverse_iterator rbegin() const noexcept {
        return this->_M_prevent_rend(this->_M_max_node());
    }

    const_iterator end() const noexcept {
        return &_M_block->_M_root;
    }

    const_reverse_iterator rend() const noexcept {
        return &_M_block->_M_root;
    }

    // void _M_print(_RbTreeNode *__node) {
    //     if (__node) {
    //         _T &__value = static_cast<_RbTreeNodeImpl<_T> *>(__node)->_M_value;
    //         if constexpr (requires (_T t) { t.first; t.second; }) {
    //             std::cout << __value.first << ':' << __value.second;
    //         } else {
    //             std::cout << __value;
    //         }
    //         std::cout << '(' << (__node->_M_color == _S_black ? 'B' : 'R') << ' ';
    //         if (__node->_M_left) {
    //             if (__node->_M_left->_M_parent != __node || __node->_M_left->_M_pparent != &__node->_M_left) {
    //                 std::cout << '*';
    //             }
    //         }
    //         _M_print(__node->_M_left);
    //         std::cout << ' ';
    //         if (__node->_M_right) {
    //             if (__node->_M_right->_M_parent != __node || __node->_M_right->_M_pparent != &__node->_M_right) {
    //                 std::cout << '*';
    //             }
    //         }
    //         _M_print(__node->_M_right);
    //         std::cout << ')';
    //     } else {
    //         std::cout << '.';
    //     }
    // }
    //
    // void _M_print() {
    //     _M_print(this->_M_block->_M_root);
    //     std::cout << '\n';
    // }
};

template <class _T, class _Compare = std::less<_T>>
struct Set : _RbTreeImpl<_T, _Compare> {
    using typename _RbTreeImpl<_T, _Compare>::const_iterator;
    using iterator = const_iterator;
    using value_type = _T;

    Set() = default;
    explicit Set(_Compare __comp) : _RbTreeImpl<_T, _Compare>(__comp) {}

    Set(Set &&) = default;
    Set &operator=(Set &&) = default;

    Set(Set const &__that) : _RbTreeImpl<_T, _Compare>() {
        this->_M_single_insert(__that.begin(), __that.end());
    }

    Set &operator=(Set const &__that) {
        if (&__that != this) {
            this->assign(__that.begin(), __that.end());
        }
        return *this;
    }

    Set &operator=(std::initializer_list<_T> __ilist) {
        this->assign(__ilist);
        return *this;
    }

    void assign(std::initializer_list<_T> __ilist) {
        this->clear();
        this->_M_single_insert(__ilist.begin(), __ilist.end());
    }

    _Compare value_comp() const noexcept {
        return this->_M_comp;
    }

    template <class _Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(_Compare)>
    const_iterator find(_T &&__value) const noexcept {
        return this->_M_find(__value);
    }

    const_iterator find(_T const &__value) const noexcept {
        return this->_M_find(__value);
    }

    std::pair<iterator, bool> insert(_T &&__value) {
        return this->_M_single_emplace(std::move(__value));
    }

    std::pair<iterator, bool> insert(_T const &__value) {
        return this->_M_single_emplace(__value);
    }

    template <class ...Ts>
    std::pair<iterator, bool> emplace(Ts &&...__value) {
        return this->_M_single_emplace(std::forward<Ts>(__value)...);
    }

    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, _InputIt)>
    void insert(_InputIt __first, _InputIt __last) {
        return this->_M_single_insert(__first, __last);
    }

    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, _InputIt)>
    void assign(_InputIt __first, _InputIt __last) {
        this->clear();
        return this->_M_single_insert(__first, __last);
    }

    using _RbTreeImpl<_T, _Compare>::erase;

    template <class _Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(_Compare)>
    size_t erase(_Tv &&__value) {
        return this->_M_single_erase(__value);
    }

    size_t erase(_T const &__value) {
        return this->_M_single_erase(__value);
    }

    template <class _Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(_Compare)>
    size_t count(_Tv &&__value) const noexcept {
        return this->_M_contains(__value) ? 1 : 0;
    }

    size_t count(_T const &__value) const noexcept {
        return this->_M_contains(__value) ? 1 : 0;
    }

    template <class _Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(_Compare)>
    bool contains(_Tv &&__value) const noexcept {
        return this->_M_contains(__value);
    }

    bool contains(_T const &__value) const noexcept {
        return this->_M_contains(__value);
    }
};

template <class _T, class _Compare = std::less<_T>>
struct MultiSet : _RbTreeImpl<_T, _Compare> {
    using typename _RbTreeImpl<_T, _Compare>::const_iterator;
    using iterator = const_iterator;

    MultiSet() = default;
    explicit MultiSet(_Compare __comp) : _RbTreeImpl<_T, _Compare>(__comp) {}

    MultiSet(MultiSet &&) = default;
    MultiSet &operator=(MultiSet &&) = default;

    MultiSet(MultiSet const &__that) : _RbTreeImpl<_T, _Compare>() {
        this->_M_multi_insert(__that.begin(), __that.end());
    }

    MultiSet &operator=(MultiSet const &__that) {
        if (&__that != this) {
            this->assign(__that.begin(), __that.end());
        }
        return *this;
    }

    MultiSet &operator=(std::initializer_list<_T> __ilist) {
        this->assign(__ilist);
        return *this;
    }

    void assign(std::initializer_list<_T> __ilist) {
        this->clear();
        this->_M_multi_insert(__ilist.begin(), __ilist.end());
    }

    _Compare value_comp() const noexcept {
        return this->_M_comp;
    }

    template <class _Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(_Compare)>
    const_iterator find(_T &&__value) const noexcept {
        return this->_M_find(__value);
    }

    const_iterator find(_T const &__value) const noexcept {
        return this->_M_find(__value);
    }

    iterator insert(_T &&__value) {
        return this->_M_multi_emplace(std::move(__value));
    }

    iterator insert(_T const &__value) {
        return this->_M_multi_emplace(__value);
    }

    template <class ...Ts>
    iterator emplace(Ts &&...__value) {
        return this->_M_multi_emplace(std::forward<Ts>(__value)...);
    }

    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, _InputIt)>
    void insert(_InputIt __first, _InputIt __last) {
        return this->_M_multi_insert(__first, __last);
    }

    using _RbTreeImpl<_T, _Compare>::assign;

    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, _InputIt)>
    void assign(_InputIt __first, _InputIt __last) {
        this->clear();
        return this->_M_multi_insert(__first, __last);
    }

    using _RbTreeImpl<_T, _Compare>::erase;

    template <class _Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(_Compare)>
    size_t erase(_Tv &&__value) {
        return this->_M_multi_erase(__value);
    }

    size_t erase(_T const &__value) {
        return this->_M_multi_erase(__value);
    }

    template <class _Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(_Compare)>
    size_t count(_Tv &&__value) const noexcept {
        return this->_M_multi_count(__value);
    }

    size_t count(_T const &__value) const noexcept {
        return this->_M_multi_count(__value);
    }

    template <class _Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(_Compare)>
    bool contains(_Tv &&__value) const noexcept {
        return this->_M_contains(__value);
    }

    bool contains(_T const &__value) const noexcept {
        return this->_M_contains(__value);
    }
};

template <class _Compare, class _Value, class = void>
struct _RbTreeValueCompare {
    [[no_unique_address]] _Compare _M_comp;

    _RbTreeValueCompare(_Compare __comp = {}) noexcept : _M_comp(__comp) {}

    bool operator()(typename _Value::first_type const &__lhs, _Value const &__rhs) const noexcept {
        return this->_M_comp(__lhs, __rhs.first);
    }

    bool operator()(_Value const &__lhs, typename _Value::first_type const &__rhs) const noexcept {
        return this->_M_comp(__lhs.first, __rhs);
    }

    bool operator()(_Value const &__lhs, _Value const &__rhs) const noexcept {
        return this->_M_comp(__lhs.first, __rhs.first);
    }
};

template <class _Compare, class _Value>
struct _RbTreeValueCompare<_Compare, _Value, decltype((void)static_cast<typename _Compare::is_transparent *>(nullptr))> {
    [[no_unique_address]] _Compare _M_comp;

    _RbTreeValueCompare(_Compare __comp = {}) noexcept : _M_comp(__comp) {}

    template <class _Lhs>
    bool operator()(_Lhs &&__lhs, _Value const &__rhs) const noexcept {
        return this->_M_comp(__lhs, __rhs.first);
    }

    template <class _Rhs>
    bool operator()(_Value const &__lhs, _Rhs &&__rhs) const noexcept {
        return this->_M_comp(__lhs.first, __rhs);
    }

    bool operator()(_Value const &__lhs, _Value const &__rhs) const noexcept {
        return this->_M_comp(__lhs.first, __rhs.first);
    }

    using is_transparent = typename _Compare::is_transparent;
};

template <class _Key, class _Mapped, class _Compare = std::less<_Key>>
struct Map : _RbTreeImpl<std::pair<const _Key, _Mapped>,
        _RbTreeValueCompare<_Compare, std::pair<const _Key, _Mapped>>> {
    using key_type = _Key;
    using mapped_type = _Mapped;
    using value_type = std::pair<const _Key, _Mapped>;

private:
    using _ValueComp = _RbTreeValueCompare<_Compare, value_type>;

public:
    using typename _RbTreeImpl<value_type, _ValueComp>::iterator;
    using typename _RbTreeImpl<value_type, _ValueComp>::const_iterator;

    Map() = default;
    explicit Map(_Compare __comp) : _RbTreeImpl<value_type, _ValueComp>(__comp) {}

    Map(std::initializer_list<value_type> __ilist) {
        _M_single_insert(__ilist.begin(), __ilist.end());
    }

    explicit Map(std::initializer_list<value_type> __ilist, _Compare __comp)
        : _RbTreeImpl<value_type, _ValueComp>(__comp) {
        _M_single_insert(__ilist.begin(), __ilist.end());
    }

    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, _InputIt)>
    explicit Map(_InputIt __first, _InputIt __last) {
        _M_single_insert(__first, __last);
    }

    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, _InputIt)>
    explicit Map(_InputIt __first, _InputIt __last, _Compare __comp)
        : _RbTreeImpl<value_type, _ValueComp>(__comp) {
        _M_single_insert(__first, __last);
    }

    Map(Map &&) = default;
    Map &operator=(Map &&) = default;

    Map(Map const &__that) : _RbTreeImpl<value_type, _ValueComp>() {
        this->_M_single_insert(__that.begin(), __that.end());
    }

    Map &operator=(Map const &__that) {
        if (&__that != this) {
            this->assign(__that.begin(), __that.end());
        }
        return *this;
    }

    Map &operator=(std::initializer_list<value_type> __ilist) {
        this->assign(__ilist);
        return *this;
    }

    void assign(std::initializer_list<value_type> __ilist) {
        this->clear();
        this->_M_single_insert(__ilist.begin(), __ilist.end());
    }

    _Compare key_comp() const noexcept {
        return this->_M_comp->_M_comp;
    }

    _ValueComp value_comp() const noexcept {
        return this->_M_comp;
    }

    template <class _Kv, _LIBPENGCXX_REQUIRES_TRANSPARENT(_Compare)>
    iterator find(_Kv &&__key) noexcept {
        return this->_M_find(__key);
    }

    template <class _Kv, _LIBPENGCXX_REQUIRES_TRANSPARENT(_Compare)>
    const_iterator find(_Kv &&__key) const noexcept {
        return this->_M_find(__key);
    }

    iterator find(_Key const &__key) noexcept {
        return this->_M_find(__key);
    }

    const_iterator find(_Key const &__key) const noexcept {
        return this->_M_find(__key);
    }

    std::pair<iterator, bool> insert(value_type &&__value) {
        return this->_M_single_emplace(std::move(__value));
    }

    std::pair<iterator, bool> insert(value_type const &__value) {
        return this->_M_single_emplace(__value);
    }

    template <class _Kv, _LIBPENGCXX_REQUIRES_TRANSPARENT(_Compare)>
    _Mapped const &at(_Kv const &__key) const {
        const_iterator __it = this->_M_find(__key);
        if (__it == this->end()) {
            throw std::out_of_range("map::at");
        }
        return __it->second;
    }

    template <class _Kv, _LIBPENGCXX_REQUIRES_TRANSPARENT(_Compare)>
    _Mapped &at(_Kv const &__key) {
        iterator __it = this->_M_find(__key);
        if (__it == this->end()) {
            throw std::out_of_range("map::at");
        }
        return __it->second;
    }

    _Mapped const &at(_Key const &__key) const {
        const_iterator __it = this->_M_find(__key);
        if (__it == this->end()) {
            throw std::out_of_range("map::at");
        }
        return __it->second;
    }

    _Mapped &at(_Key const &__key) {
        iterator __it = this->_M_find(__key);
        if (__it == this->end()) {
            throw std::out_of_range("map::at");
        }
        return __it->second;
    }

    template <class _Kv, _LIBPENGCXX_REQUIRES_TRANSPARENT(_Compare)>
    _Mapped &operator[](_Kv const &__key) {
        iterator __it = this->_M_find(__key);
        if (__it == this->end()) {
            __it = this->_M_single_emplace(std::piecewise_construct,
                                         std::forward_as_tuple(__key),
                                         std::forward_as_tuple()).first;
        }
        return __it->second;
    }

    _Mapped &operator[](_Key const &__key) {
        iterator __it = this->_M_find(__key);
        if (__it == this->end()) {
            __it = this->_M_single_emplace(std::piecewise_construct,
                                         std::forward_as_tuple(__key),
                                         std::forward_as_tuple()).first;
        }
        return __it->second;
    }

    template <class _M, class = std::enable_if_t<std::is_convertible_v<_M, _Mapped>>>
    std::pair<iterator, bool> insert_or_assign(_Key const &__key, _M &&__mapped) {
        std::pair<iterator, bool> __result = this->_M_single_emplace(
            std::piecewise_construct,
            std::forward_as_tuple(__key),
            std::forward_as_tuple(std::forward<_M>(__mapped)));
        if (!__result.second) {
            __result.first->second = std::forward<_M>(__mapped);
        }
        return __result;
    }

    template <class _M, class = std::enable_if_t<std::is_convertible_v<_M, _Mapped>>>
    std::pair<iterator, bool> insert_or_assign(_Key &&__key, _M &&__mapped) {
        std::pair<iterator, bool> __result = this->_M_single_emplace(
            std::piecewise_construct,
            std::forward_as_tuple(std::move(__key)),
            std::forward_as_tuple(std::forward<_M>(__mapped)));
        if (!__result.second) {
            __result.first->second = std::forward<_M>(__mapped);
        }
        return __result;
    }

    template <class ...Vs>
    std::pair<iterator, bool> emplace(Vs &&...__value) {
        return this->_M_single_emplace(std::forward<Vs>(__value)...);
    }

    template <class ...Ms>
    std::pair<iterator, bool> try_emplace(_Key &&__key, Ms &&...__mapped) {
        return this->_M_single_emplace(std::piecewise_construct,
                                       std::forward_as_tuple(std::move(__key)),
                                       std::forward_as_tuple(std::forward<Ms>(__mapped)...));
    }

    template <class ...Ms>
    std::pair<iterator, bool> try_emplace(_Key const &__key, Ms &&...__mapped) {
        return this->_M_single_emplace(std::piecewise_construct,
                                       std::forward_as_tuple(__key),
                                       std::forward_as_tuple(std::forward<Ms>(__mapped)...));
    }

    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, _InputIt)>
    iterator insert(_InputIt __first, _InputIt __last) {
        return this->_M_single_insert(__first, __last);
    }

    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, _InputIt)>
    iterator assign(_InputIt __first, _InputIt __last) {
        this->clear();
        return this->_M_single_insert(__first, __last);
    }

    using _RbTreeImpl<value_type, _ValueComp>::erase;

    template <class _Kv, _LIBPENGCXX_REQUIRES_TRANSPARENT(_Compare)>
    size_t erase(_Kv &&__key) {
        return this->_M_single_erase(__key);
    }

    size_t erase(_Key const &__key) {
        return this->_M_single_erase(__key);
    }

    template <class _Kv, _LIBPENGCXX_REQUIRES_TRANSPARENT(_Compare)>
    size_t count(_Kv &&__value) const noexcept {
        return this->_M_contains(__value) ? 1 : 0;
    }

    size_t count(_Key const &__value) const noexcept {
        return this->_M_contains(__value) ? 1 : 0;
    }

    template <class _Kv, _LIBPENGCXX_REQUIRES_TRANSPARENT(_Compare)>
    bool contains(_Kv &&__value) const noexcept {
        return this->_M_contains(__value);
    }

    bool contains(_Key const &__value) const noexcept {
        return this->_M_contains(__value);
    }
};

template <class _Key, class _Mapped, class _Compare = std::less<_Key>>
struct MultiMap : _RbTreeImpl<std::pair<const _Key, _Mapped>,
        _RbTreeValueCompare<_Compare, std::pair<const _Key, _Mapped>>> {
    using key_type = _Key;
    using mapped_type = _Mapped;
    using value_type = std::pair<const _Key, _Mapped>;

private:
    using _ValueComp = _RbTreeValueCompare<_Compare, value_type>;
public:

    using typename _RbTreeImpl<value_type, _ValueComp>::iterator;
    using typename _RbTreeImpl<value_type, _ValueComp>::const_iterator;

    MultiMap() = default;
    explicit MultiMap(_Compare __comp) : _RbTreeImpl<value_type, _ValueComp>(__comp) {}

    MultiMap(std::initializer_list<value_type> __ilist) {
        _M_multi_insert(__ilist.begin(), __ilist.end());
    }

    explicit MultiMap(std::initializer_list<value_type> __ilist, _Compare __comp) : _RbTreeImpl<value_type, _ValueComp>(__comp) {
        _M_multi_insert(__ilist.begin(), __ilist.end());
    }

    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, _InputIt)>
    explicit MultiMap(_InputIt __first, _InputIt __last) {
        _M_multi_insert(__first, __last);
    }

    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, _InputIt)>
    explicit MultiMap(_InputIt __first, _InputIt __last, _Compare __comp)
        : _RbTreeImpl<value_type, _ValueComp>(__comp) {
        _M_multi_insert(__first, __last);
    }

    MultiMap(MultiMap &&) = default;
    MultiMap &operator=(MultiMap &&) = default;

    MultiMap(MultiMap const &__that) : _RbTreeImpl<value_type, _ValueComp>() {
        this->_M_multi_insert(__that.begin(), __that.end());
    }

    MultiMap &operator=(MultiMap const &__that) {
        if (&__that != this) {
            this->assign(__that.begin(), __that.end());
        }
        return *this;
    }

    MultiMap &operator=(std::initializer_list<value_type> __ilist) {
        this->assign(__ilist);
        return *this;
    }

    void assign(std::initializer_list<value_type> __ilist) {
        this->clear();
        this->_M_multi_insert(__ilist.begin(), __ilist.end());
    }

    _Compare key_comp() const noexcept {
        return this->_M_comp->_M_comp;
    }

    _ValueComp value_comp() const noexcept {
        return this->_M_comp;
    }

    template <class _Kv, _LIBPENGCXX_REQUIRES_TRANSPARENT(_Compare)>
    iterator find(_Kv &&__key) noexcept {
        return this->_M_find(__key);
    }

    template <class _Kv, _LIBPENGCXX_REQUIRES_TRANSPARENT(_Compare)>
    const_iterator find(_Kv &&__key) const noexcept {
        return this->_M_find(__key);
    }

    iterator find(_Key const &__key) noexcept {
        return this->_M_find(__key);
    }

    const_iterator find(_Key const &__key) const noexcept {
        return this->_M_find(__key);
    }

    std::pair<iterator, bool> insert(value_type &&__value) {
        return this->_M_single_emplace(std::move(__value));
    }

    std::pair<iterator, bool> insert(value_type const &__value) {
        return this->_M_single_emplace(__value);
    }

    template <class ...Ts>
    std::pair<iterator, bool> emplace(Ts &&...__value) {
        return this->_M_single_emplace(std::forward<Ts>(__value)...);
    }

    template <class ...Ts>
    std::pair<iterator, bool> try_emplace(_Key &&__key, Ts &&...__value) {
        return this->_M_single_emplace(std::piecewise_construct,
                                       std::forward_as_tuple(std::move(__key)),
                                       std::forward_as_tuple(std::forward<Ts>(__value)...));
    }

    template <class ...Ts>
    std::pair<iterator, bool> try_emplace(_Key const &__key, Ts &&...__value) {
        return this->_M_single_emplace(std::piecewise_construct,
                                       std::forward_as_tuple(__key),
                                       std::forward_as_tuple(std::forward<Ts>(__value)...));
    }

    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, _InputIt)>
    iterator insert(_InputIt __first, _InputIt __last) {
        return this->_M_single_insert(__first, __last);
    }

    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, _InputIt)>
    iterator assign(_InputIt __first, _InputIt __last) {
        this->clear();
        return this->_M_single_insert(__first, __last);
    }

    using _RbTreeImpl<value_type, _ValueComp>::erase;

    template <class _Kv, _LIBPENGCXX_REQUIRES_TRANSPARENT(_Compare)>
    size_t erase(_Kv &&__key) {
        return this->_M_single_erase(__key);
    }

    size_t erase(_Key const &__key) {
        return this->_M_single_erase(__key);
    }

    template <class _Kv, _LIBPENGCXX_REQUIRES_TRANSPARENT(_Compare)>
    size_t count(_Kv &&__value) const noexcept {
        return this->_M_multi_count(__value);
    }

    size_t count(_Key const &__value) const noexcept {
        return this->_M_multi_count(__value);
    }

    template <class _Kv, _LIBPENGCXX_REQUIRES_TRANSPARENT(_Compare)>
    bool contains(_Kv &&__value) const noexcept {
        return this->_M_contains(__value);
    }

    bool contains(_Key const &__value) const noexcept {
        return this->_M_contains(__value);
    }
};
