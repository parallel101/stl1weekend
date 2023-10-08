#pragma once

#include <iostream>
#include <utility>
#include <stdexcept>
#include <memory>
#include <type_traits>
#include <functional>

template <class FnSig>
struct Function {
    // 只在使用了不符合 Ret(Args...) 模式的 FnSig 时出错
    static_assert(!std::is_same_v<FnSig, FnSig>, "not a valid function signature");
};

template <class Ret, class ...Args>
struct Function<Ret(Args...)> {
private:
    struct FuncBase {
        virtual Ret call(Args ...args) = 0; // 类型擦除后的统一接口
        virtual ~FuncBase() = default; // 应对F可能有非平凡析构的情况
    };

    template <class F>
    struct FuncImpl : FuncBase { // FuncImpl 会被实例化多次，每个不同的仿函数类都产生一次实例化
        F m_f;

        FuncImpl(F f) : m_f(std::move(f)) {}

        virtual Ret call(Args ...args) override {
            // 完美转发所有参数给构造时保存的仿函数对象
            return m_f(std::forward<Args>(args)...);
            // 更规范的写法其实是：
            // return std::invoke(m_f, std::forward<Args>(args)...);
            // 但为了照顾初学者依然采用朴素的调用方法
        }
    };

    std::shared_ptr<FuncBase> m_base; // 使用智能指针管理仿函数对象，用shared而不是unique是为了让Function支持拷贝

public:
    Function() = default; // m_base 初始化为 nullptr

    // 此处 enable_if_t 的作用：阻止 Function 从不可调用的对象中初始化
    template <class F, class = std::enable_if_t<std::is_invocable_r_v<Ret, F &, Args...>>>
    Function(F f) // 没有 explicit，允许 lambda 表达式隐式转换成 Function
    : m_base(std::make_shared<FuncImpl<F>>(std::move(f)))
    {}

    Ret operator()(Args ...args) const {
        if (!m_base) [[unlikely]]
            throw std::runtime_error("function not initialized");
        // 完美转发所有参数，这样即使 Args 中具有引用，也能不产生额外的拷贝开销
        return m_base->call(std::forward<Args>(args)...);
    }
};
