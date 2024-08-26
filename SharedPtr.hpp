#pragma once

#include "UniquePtr.hpp"
#include <algorithm>
#include <atomic>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

struct _SpCounter {
    std::atomic<long> _M_refcnt;

    _SpCounter() noexcept : _M_refcnt(1) {}

    _SpCounter(_SpCounter &&) = delete;

    void _M_incref() noexcept {
        _M_refcnt.fetch_add(1, std::memory_order_relaxed);
    }

    void _M_decref() noexcept {
        if (_M_refcnt.fetch_sub(1, std::memory_order_relaxed) == 1) {
            delete this;
        }
    }

    long _M_cntref() const noexcept {
        return _M_refcnt.load(std::memory_order_relaxed);
    }

    virtual ~_SpCounter() = default;
};

template <class _Tp, class _Deleter>
struct _SpCounterImpl final : _SpCounter {
    _Tp *_M_ptr;
    [[no_unique_address]] _Deleter _M_deleter;

    explicit _SpCounterImpl(_Tp *__ptr) noexcept : _M_ptr(__ptr) {}

    explicit _SpCounterImpl(_Tp *__ptr, _Deleter __deleter) noexcept
        : _M_ptr(__ptr),
          _M_deleter(std::move(__deleter)) {}

    ~_SpCounterImpl() noexcept override {
        _M_deleter(_M_ptr);
    }
};

template <class _Tp, class _Deleter>
struct _SpCounterImplFused final : _SpCounter {
    _Tp *_M_ptr;
    void *_M_mem;
    [[no_unique_address]] _Deleter _M_deleter;

    explicit _SpCounterImplFused(_Tp *__ptr, void *__mem,
                                 _Deleter __deleter) noexcept
        : _M_ptr(__ptr),
          _M_mem(__mem),
          _M_deleter(std::move(__deleter)) {}

    ~_SpCounterImplFused() noexcept override {
        _M_deleter(_M_ptr);
#if __cpp_aligned_new
        ::operator delete(
            _M_mem, std::align_val_t(
                        std::max(alignof(_Tp), alignof(_SpCounterImplFused))));
#else
        ::operator delete(_M_mem);
#endif
    }

    void operator delete(void *) noexcept {}
};

template <class _Tp>
struct SharedPtr {
private:
    _Tp *_M_ptr;
    _SpCounter *_M_owner;

    template <class>
    friend struct SharedPtr;

    explicit SharedPtr(_Tp *__ptr, _SpCounter *__owner) noexcept
        : _M_ptr(__ptr),
          _M_owner(__owner) {}

public:
    using element_type = _Tp;
    using pointer = _Tp *;

    SharedPtr(std::nullptr_t = nullptr) noexcept : _M_owner(nullptr) {}

    template <class _Yp,
              std::enable_if_t<std::is_convertible_v<_Yp *, _Tp *>, int> = 0>
    explicit SharedPtr(_Yp *__ptr)
        : _M_ptr(__ptr),
          _M_owner(new _SpCounterImpl<_Yp, DefaultDeleter<_Yp>>(__ptr)) {}

    template <class _Yp, class _Deleter,
              std::enable_if_t<std::is_convertible_v<_Yp *, _Tp *>, int> = 0>
    explicit SharedPtr(_Yp *__ptr, _Deleter __deleter)
        : _M_owner(
              new _SpCounterImpl<_Yp, _Deleter>(__ptr, std::move(__deleter))) {}

    template <class _Yp, class _Deleter,
              std::enable_if_t<std::is_convertible_v<_Yp *, _Tp *>, int> = 0>
    explicit SharedPtr(UniquePtr<_Yp, _Deleter> &&__ptr)
        : SharedPtr(__ptr.release(), __ptr.get_deleter()) {}

    template <class _Yp>
    inline friend SharedPtr<_Yp>
    _S_makeSharedFused(_Yp *__ptr, _SpCounter *__owner) noexcept;

    SharedPtr(SharedPtr const &__that) noexcept
        : _M_ptr(__that._M_ptr),
          _M_owner(__that._M_owner) {
        if (_M_owner) {
            _M_owner->_M_incref();
        }
    }

    template <class _Yp,
              std::enable_if_t<std::is_convertible_v<_Yp *, _Tp *>, int> = 0>
    SharedPtr(SharedPtr<_Yp> const &__that) noexcept
        : _M_ptr(__that._M_ptr),
          _M_owner(__that._M_owner) {
        if (_M_owner) {
            _M_owner->_M_incref();
        }
    }

    SharedPtr(SharedPtr &&__that) noexcept
        : _M_ptr(__that._M_ptr),
          _M_owner(__that._M_owner) {
        __that._M_ptr = nullptr;
        __that._M_owner = nullptr;
    }

    template <class _Yp,
              std::enable_if_t<std::is_convertible_v<_Yp *, _Tp *>, int> = 0>
    SharedPtr(SharedPtr<_Yp> &&__that) noexcept
        : _M_ptr(__that._M_ptr),
          _M_owner(__that._M_owner) {
        __that._M_ptr = nullptr;
        __that._M_owner = nullptr;
    }

    template <class _Yp>
    SharedPtr(SharedPtr<_Yp> const &__that, _Tp *__ptr) noexcept
        : _M_ptr(__ptr),
          _M_owner(__that._M_owner) {
        if (_M_owner) {
            _M_owner->_M_incref();
        }
    }

    template <class _Yp>
    SharedPtr(SharedPtr<_Yp> &&__that, _Tp *__ptr) noexcept
        : _M_ptr(__ptr),
          _M_owner(__that._M_owner) {
        __that._M_ptr = nullptr;
        __that._M_owner = nullptr;
    }

    SharedPtr &operator=(SharedPtr const &__that) noexcept {
        if (this == &__that) {
            return *this;
        }
        if (_M_owner) {
            _M_owner->_M_decref();
        }
        _M_ptr = __that._M_ptr;
        _M_owner = __that._M_owner;
        if (_M_owner) {
            _M_owner->_M_incref();
        }
        return *this;
    }

    SharedPtr &operator=(SharedPtr &&__that) noexcept {
        if (this == &__that) {
            return *this;
        }
        if (_M_owner) {
            _M_owner->_M_decref();
        }
        _M_ptr = __that._M_ptr;
        _M_owner = __that._M_owner;
        __that._M_ptr = nullptr;
        __that._M_owner = nullptr;
        return *this;
    }

    template <class _Yp,
              std::enable_if_t<std::is_convertible_v<_Yp *, _Tp *>, int> = 0>
    SharedPtr &operator=(SharedPtr<_Yp> const &__that) noexcept {
        if (this == &__that) {
            return *this;
        }
        if (_M_owner) {
            _M_owner->_M_decref();
        }
        _M_ptr = __that._M_ptr;
        _M_owner = __that._M_owner;
        if (_M_owner) {
            _M_owner->_M_incref();
        }
        return *this;
    }

    template <class _Yp,
              std::enable_if_t<std::is_convertible_v<_Yp *, _Tp *>, int> = 0>
    SharedPtr &operator=(SharedPtr<_Yp> &&__that) noexcept {
        if (this == &__that) {
            return *this;
        }
        if (_M_owner) {
            _M_owner->_M_decref();
        }
        _M_ptr = __that._M_ptr;
        _M_owner = __that._M_owner;
        __that._M_ptr = nullptr;
        __that._M_owner = nullptr;
        return *this;
    }

    void reset() noexcept {
        if (_M_owner) {
            _M_owner->_M_decref();
        }
        _M_owner = nullptr;
        _M_ptr = nullptr;
    }

    template <class _Yp>
    void reset(_Yp *__ptr) {
        if (_M_owner) {
            _M_owner->_M_decref();
        }
        _M_ptr = nullptr;
        _M_owner = nullptr;
        _M_ptr = __ptr;
        _M_owner = new _SpCounterImpl<_Yp, DefaultDeleter<_Yp>>(__ptr);
    }

    template <class _Yp, class _Deleter>
    void reset(_Yp *__ptr, _Deleter __deleter) {
        if (_M_owner) {
            _M_owner->_M_decref();
        }
        _M_ptr = nullptr;
        _M_owner = nullptr;
        _M_ptr = __ptr;
        _M_owner =
            new _SpCounterImpl<_Yp, _Deleter>(__ptr, std::move(__deleter));
    }

    ~SharedPtr() noexcept {
        if (_M_owner) {
            _M_owner->_M_decref();
        }
    }

    long use_count() noexcept {
        return _M_owner ? _M_owner->_M_cntref() : 0;
    }

    bool unique() noexcept {
        return _M_owner ? _M_owner->_M_cntref() == 1 : true;
    }

    template <class _Yp>
    bool operator==(SharedPtr<_Yp> const &__that) const noexcept {
        return _M_ptr == __that._M_ptr;
    }

    template <class _Yp>
    bool operator!=(SharedPtr<_Yp> const &__that) const noexcept {
        return _M_ptr != __that._M_ptr;
    }

    template <class _Yp>
    bool operator<(SharedPtr<_Yp> const &__that) const noexcept {
        return _M_ptr < __that._M_ptr;
    }

    template <class _Yp>
    bool operator<=(SharedPtr<_Yp> const &__that) const noexcept {
        return _M_ptr <= __that._M_ptr;
    }

    template <class _Yp>
    bool operator>(SharedPtr<_Yp> const &__that) const noexcept {
        return _M_ptr > __that._M_ptr;
    }

    template <class _Yp>
    bool operator>=(SharedPtr<_Yp> const &__that) const noexcept {
        return _M_ptr >= __that._M_ptr;
    }

    template <class _Yp>
    bool owner_before(SharedPtr<_Yp> const &__that) const noexcept {
        return _M_owner < __that._M_owner;
    }

    template <class _Yp>
    bool owner_equal(SharedPtr<_Yp> const &__that) const noexcept {
        return _M_owner == __that._M_owner;
    }

    void swap(SharedPtr &__that) noexcept {
        std::swap(_M_ptr, __that._M_ptr);
        std::swap(_M_owner, __that._M_owner);
    }

    _Tp *get() const noexcept {
        return _M_ptr;
    }

    _Tp *operator->() const noexcept {
        return _M_ptr;
    }

    std::add_lvalue_reference_t<_Tp> operator*() const noexcept {
        return *_M_ptr;
    }

    explicit operator bool() const noexcept {
        return _M_ptr != nullptr;
    }
};

template <class _Tp>
inline SharedPtr<_Tp> _S_makeSharedFused(_Tp *__ptr,
                                         _SpCounter *__owner) noexcept {
    return SharedPtr<_Tp>(__ptr, __owner);
}

template <class _Tp>
struct SharedPtr<_Tp[]> : SharedPtr<_Tp> {
    using SharedPtr<_Tp>::SharedPtr;

    std::add_lvalue_reference_t<_Tp> operator[](std::size_t __i) {
        return this->get()[__i];
    }
};

template <class _Tp>
struct EnableSharedFromThis {
private:
    _SpCounter *_M_owner;

protected:
    EnableSharedFromThis() noexcept : _M_owner(nullptr) {}

    SharedPtr<_Tp> shared_from_this() {
        static_assert(std::is_base_of_v<EnableSharedFromThis, _Tp>,
                      "must be derived class");
        if (!_M_owner) {
            throw std::bad_weak_ptr();
        }
        _M_owner->_M_incref();
        return _S_makeSharedFused(static_cast<_Tp *>(this), _M_owner);
    }

    SharedPtr<_Tp const> shared_from_this() const {
        static_assert(std::is_base_of_v<EnableSharedFromThis, _Tp>,
                      "must be derived class");
        if (!_M_owner) {
            throw std::bad_weak_ptr();
        }
        _M_owner->_M_incref();
        return _S_makeSharedFused(static_cast<_Tp const *>(this), _M_owner);
    }

    template <class _Up>
    inline friend void
    _S_setEnableSharedFromThisOwner(EnableSharedFromThis<_Up> *, _SpCounter *);
};

template <class _Up>
inline void _S_setEnableSharedFromThisOwner(EnableSharedFromThis<_Up> *__ptr,
                                            _SpCounter *__owner) {
    __ptr->_M_owner = __owner;
}

template <class _Tp,
          std::enable_if_t<std::is_base_of_v<EnableSharedFromThis<_Tp>, _Tp>,
                           int> = 0>
void _S_setupEnableSharedFromThis(_Tp *__ptr, _SpCounter *__owner) {
    _S_setEnableSharedFromThisOwner(
        static_cast<EnableSharedFromThis<_Tp> *>(__ptr), __owner);
}

template <class _Tp,
          std::enable_if_t<!std::is_base_of_v<EnableSharedFromThis<_Tp>, _Tp>,
                           int> = 0>
void _S_setupEnableSharedFromThis(_Tp *, _SpCounter *) {}

template <class _Tp, class... _Args,
          std::enable_if_t<!std::is_unbounded_array_v<_Tp>, int> = 0>
SharedPtr<_Tp> makeShared(_Args &&...__args) {
    auto const __deleter = [](_Tp *__ptr) noexcept {
        __ptr->~_Tp();
    };
    using _Counter = _SpCounterImplFused<_Tp, decltype(__deleter)>;
    constexpr std::size_t __offset = std::max(alignof(_Tp), sizeof(_Counter));
    constexpr std::size_t __align = std::max(alignof(_Tp), alignof(_Counter));
    constexpr std::size_t __size = __offset + sizeof(_Tp);
#if __cpp_aligned_new
    void *__mem = ::operator new(__size, std::align_val_t(__align));
    _Counter *__counter = reinterpret_cast<_Counter *>(__mem);
#else
    void *__mem = ::operator new(__size + __align);
    _Counter *__counter = reinterpret_cast<_SpC *>(
        reinterpret_cast<std::size_t>(__mem) & __align);
#endif
    _Tp *__object =
        reinterpret_cast<_Tp *>(reinterpret_cast<char *>(__counter) + __offset);
    try {
        new (__object) _Tp(std::forward<_Args>(__args)...);
    } catch (...) {
#if __cpp_aligned_new
        ::operator delete(__mem, std::align_val_t(__align));
#else
        ::operator delete(__mem);
#endif
        throw;
    }
    new (__counter) _Counter(__object, __mem, __deleter);
    _S_setupEnableSharedFromThis(__object, __counter);
    return _S_makeSharedFused(__object, __counter);
}

template <class _Tp, std::enable_if_t<!std::is_unbounded_array_v<_Tp>, int> = 0>
SharedPtr<_Tp> makeSharedForOverwrite() {
    auto const __deleter = [](_Tp *__ptr) noexcept {
        __ptr->~_Tp();
    };
    using _Counter = _SpCounterImplFused<_Tp, decltype(__deleter)>;
    constexpr std::size_t __offset = std::max(alignof(_Tp), sizeof(_Counter));
    constexpr std::size_t __align = std::max(alignof(_Tp), alignof(_Counter));
    constexpr std::size_t __size = __offset + sizeof(_Tp);
#if __cpp_aligned_new
    void *__mem = ::operator new(__size, std::align_val_t(__align));
    _Counter *__counter = reinterpret_cast<_Counter *>(__mem);
#else
    void *__mem = ::operator new(__size + __align);
    _SpC *__counter = reinterpret_cast<_SpC *>(
        reinterpret_cast<std::size_t>(__mem) & __align);
#endif
    _Tp *__object =
        reinterpret_cast<_Tp *>(reinterpret_cast<char *>(__counter) + __offset);
    try {
        new (__object) _Tp;
    } catch (...) {
#if __cpp_aligned_new
        ::operator delete(__mem, std::align_val_t(__align));
#else
        ::operator delete(__mem);
#endif
        throw;
    }
    new (__counter) _Counter(__object, __mem, __deleter);
    _S_setupEnableSharedFromThis(__object, __counter);
    return _S_makeSharedFused(__object, __counter);
}

template <class _Tp, class... _Args,
          std::enable_if_t<std::is_unbounded_array_v<_Tp>, int> = 0>
SharedPtr<_Tp> makeShared(std::size_t __len) {
    std::remove_extent_t<_Tp> *__p = new std::remove_extent_t<_Tp>[__len];
    try {
        return SharedPtr<_Tp>(__p);
    } catch (...) {
        delete[] __p;
        throw;
    }
}

template <class _Tp, std::enable_if_t<std::is_unbounded_array_v<_Tp>, int> = 0>
SharedPtr<_Tp> makeSharedForOverwrite(std::size_t __len) {
    std::remove_extent_t<_Tp> *__p = new std::remove_extent_t<_Tp>[__len];
    try {
        return SharedPtr<_Tp>(__p);
    } catch (...) {
        delete[] __p;
        throw;
    }
}

template <class _Tp, class _Up>
SharedPtr<_Tp> staticPointerCast(SharedPtr<_Up> const &__ptr) {
    return SharedPtr<_Tp>(__ptr, static_cast<_Tp *>(__ptr.get()));
}

template <class _Tp, class _Up>
SharedPtr<_Tp> constPointerCast(SharedPtr<_Up> const &__ptr) {
    return SharedPtr<_Tp>(__ptr, const_cast<_Tp *>(__ptr.get()));
}

template <class _Tp, class _Up>
SharedPtr<_Tp> reinterpretPointerCast(SharedPtr<_Up> const &__ptr) {
    return SharedPtr<_Tp>(__ptr, reinterpret_cast<_Tp *>(__ptr.get()));
}

template <class _Tp, class _Up>
SharedPtr<_Tp> dynamicPointerCast(SharedPtr<_Up> const &__ptr) {
    _Tp *__p = dynamic_cast<_Tp *>(__ptr.get());
    if (__p != nullptr) {
        return SharedPtr<_Tp>(__ptr, __p);
    } else {
        return nullptr;
    }
}
