#pragma once

#include <utility>
#include <typeinfo>
#include <memory>
#include <type_traits>
#include <functional>

template <class _FnSig>
struct Function {
    // 只在使用了不符合 Ret(Args...) 模式的 FnSig 时会进入此特化，导致报错
    // 此处表达式始终为 false，仅为避免编译期就报错，才让其依赖模板参数
    static_assert(!std::is_same_v<_FnSig, _FnSig>, "not a valid function signature");
};

template <class _Ret, class ..._Args>
struct Function<_Ret(_Args...)> {
private:
    struct _FuncBase {
        virtual _Ret _M_call(_Args ...__args) = 0; // 类型擦除后的统一接口
        virtual std::unique_ptr<_FuncBase> _M_clone() const = 0; // 原型模式，克隆当前函数对象
        virtual std::type_info const &_M_type() const = 0; // 获得函数对象类型信息
        virtual ~_FuncBase() = default; // 应对_Fn可能有非平凡析构的情况
    };

    template <class _Fn>
    struct _FuncImpl : _FuncBase { // FuncImpl 会被实例化多次，每个不同的仿函数类都产生一次实例化
        _Fn _M_f;

        template <class ..._CArgs>
        explicit _FuncImpl(std::in_place_t, _CArgs &&...__args) : _M_f(std::forward(__args)...) {}

        _Ret _M_call(_Args ...__args) override {
            // 完美转发所有参数给构造时保存的仿函数对象：
            // return _M_f(std::forward<Args>(__args)...);
            // 更规范的写法其实是：
            return std::invoke(_M_f, std::forward<_Args>(__args)...);
        }

        std::unique_ptr<_FuncBase> _M_clone() const override {
            return std::make_unique<_FuncImpl>(_M_f);
        }

        std::type_info const &_M_type() const override {
            return typeid(_Fn);
        }
    };

    std::unique_ptr<_FuncBase> _M_base; // 使用智能指针管理仿函数对象

public:
    Function() = default; // _M_base 初始化为 nullptr
    Function(std::nullptr_t) noexcept : Function() {}

    // 此处 enable_if_t 的作用：阻止 Function 从不可调用的对象中初始化
    // 另外标准要求 Function 还需要函数对象额外支持拷贝（用于 _M_clone）
    template <class _Fn, class = std::enable_if_t<std::is_invocable_r_v<_Ret, std::decay_t<_Fn>, _Args...> && std::is_copy_constructible_v<_Fn>>>
    Function(_Fn &&__f) // 没有 explicit，允许 lambda 表达式隐式转换成 Function
    : _M_base(std::make_unique<_FuncImpl<std::decay_t<_Fn>>>(std::in_place, std::move(__f)))
    {}

    Function(Function &&) = default;
    Function &operator=(Function &&) = default;

    Function(Function const &__that) : _M_base(__that._M_base ? __that._M_base->clone() : nullptr) {
    }

    Function &operator=(Function const &__that) {
        if (__that._M_base)
            _M_base = __that._M_base->clone();
        else
            _M_base = nullptr;
    }

    explicit operator bool() const noexcept {
        return _M_base != nullptr;
    }

    bool operator==(std::nullptr_t) const noexcept {
        return _M_base == nullptr;
    }

    bool operator!=(std::nullptr_t) const noexcept {
        return _M_base != nullptr;
    }

    _Ret operator()(_Args ...__args) const {
        if (!_M_base) [[unlikely]]
            throw std::bad_function_call();
        // 完美转发所有参数，这样即使 Args 中具有引用，也能不产生额外的拷贝开销
        return _M_base->_M_call(std::forward<_Args>(__args)...);
    }

    std::type_info const &target_type() const noexcept {
        return _M_base ? _M_base->_M_type() : typeid(void);
    }

    template <class _Fn>
    _Fn *target() const noexcept {
        return _M_base && typeid(_Fn) == _M_base->_M_type() ? std::addressof(static_cast<_FuncImpl<_Fn> *>(_M_base.get())->_M_f) : nullptr;
    }

    void swap(Function &__that) const noexcept {
        _M_base.swap(__that._M_base);
    }
};
