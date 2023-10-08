#pragma once

#include <iostream>
#include <utility>
#include <memory>
#include <type_traits>
#include <functional>

template <class FnSig>
struct Function {
    static_assert(!std::is_same_v<FnSig, FnSig>, "not a valid function signature");
};

template <class Ret, class ...Args>
struct Function<Ret(Args...)> {
    struct FuncBase {
        virtual Ret call(Args ...args) = 0;
        virtual ~FuncBase() = default;
    };

    template <class F>
    struct FuncImpl : FuncBase {
        F f;

        FuncImpl(F f) : f(std::move(f)) {}

        virtual Ret call(Args ...args) override {
            return std::invoke(f, std::forward<Args>(args)...);
        }
    };

    std::shared_ptr<FuncBase> m_base;

    Function() = default;

    template <class F, class = std::enable_if_t<std::is_invocable_r_v<Ret, F, Args...> && !std::is_same_v<std::decay_t<F>, Function>>>
    Function(F f)
    : m_base(std::make_shared<FuncImpl<F>>(std::move(f)))
    {}

    Ret operator()(Args ...args) const {
        if (!m_base) [[unlikely]]
            throw std::runtime_error("function uninitialized");
        return m_base->call(std::forward<Args>(args)...);
    }
};
