#pragma once

#include <utility>
#include <memory>
#include <type_traits>
#include <functional>

template <class _FnSig>
struct MoveOnlyFunction {
    // 只在使用了不符合 Ret(Args...) 模式的 FnSig 时会进入此特化，导致报错
    // 此处表达式始终为 false，仅为避免编译期就报错，才让其依赖模板参数
    static_assert(!std::is_same_v<_FnSig, _FnSig>, "not a valid function signature");
};

template <class _Ret, class ..._Args>
struct MoveOnlyFunction<_Ret(_Args...)> {
private:
    struct _FuncBase {
        virtual _Ret _M_call(_Args ...__args) = 0; // 类型擦除后的统一接口
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
    };

    std::unique_ptr<_FuncBase> _M_base; // 使用智能指针管理仿函数对象

public:
    MoveOnlyFunction() = default; // _M_base 初始化为 nullptr
    MoveOnlyFunction(std::nullptr_t) noexcept : MoveOnlyFunction() {}

    // 此处 enable_if_t 的作用：阻止 MoveOnlyFunction 从不可调用的对象中初始化
    // MoveOnlyFunction 不要求支持拷贝
    template <class _Fn, class = std::enable_if_t<std::is_invocable_r_v<_Ret, _Fn &, _Args...>>>
    MoveOnlyFunction(_Fn __f) // 没有 explicit，允许 lambda 表达式隐式转换成 MoveOnlyFunction
    : _M_base(std::make_unique<_FuncImpl<_Fn>>(std::in_place, std::move(__f)))
    {}

    // 就地构造的版本
    template <class _Fn, class ..._CArgs>
    explicit MoveOnlyFunction(std::in_place_type_t<_Fn>, _CArgs &&...__args)
    : _M_base(std::make_unique<_FuncImpl<_Fn>>(std::in_place, std::forward<_CArgs>(__args)...))
    {}

    MoveOnlyFunction(MoveOnlyFunction &&) = default;
    MoveOnlyFunction &operator=(MoveOnlyFunction &&) = default;
    MoveOnlyFunction(MoveOnlyFunction const &) = delete;
    MoveOnlyFunction &operator=(MoveOnlyFunction const &) = delete;

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
        assert(_M_base);
        // 完美转发所有参数，这样即使 Args 中具有引用，也能不产生额外的拷贝开销
        return _M_base->_M_call(std::forward<_Args>(__args)...);
    }

    void swap(MoveOnlyFunction &__that) const noexcept {
        _M_base.swap(__that._M_base);
    }
};
