#pragma once

#include <version>
#include <type_traits>
#include <utility>
#if __cpp_concepts && __cpp_lib_concepts
#include <concepts>
#endif
#if __cpp_lib_three_way_comparison
#include <compare>
#endif
#include <algorithm>

#if __cpp_concepts && __cpp_lib_concepts
# define _LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(__category, _Type) \
     __category _Type
#else
# define _LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(__category, _Type) \
     class _Type, std::enable_if_t<std::is_convertible_v< \
                      typename std::iterator_traits<_Type>::iterator_category, \
                      __category##_tag>>
#endif
#define _LIBPENGCXX_REQUIRES_TRANSPARENT_COMPARE(_Compare, _Tv, _Tp) \
    class _Compare##Tp = _Compare, \
          class = typename _Compare##Tp::is_transparent, \
          class = \
              decltype(std::declval<bool &>() = std::declval<_Compare##Tp>()( \
                           std::declval<_Tv>(), std::declval<_Tp>()) = \
                           std::declval<_Compare##Tp>()(std::declval<_Tp>(), \
                                                        std::declval<_Tv>()))

// #define _LIBPENGCXX_THROW_OUT_OF_RANGE(__i, __n) throw std::runtime_error("out of range at index " + std::to_string(__i) + ", size " + std::to_string(__n))
#define _LIBPENGCXX_THROW_OUT_OF_RANGE(__i, __n) throw std::out_of_range("")

#if defined(_MSC_VER)
#define _LIBPENGCXX_UNREACHABLE() __assume(0)
#elif defined(__clang__)
#define _LIBPENGCXX_UNREACHABLE() __builtin_unreachable()
#elif defined(__GNUC__)
#define _LIBPENGCXX_UNREACHABLE() __builtin_unreachable()
#else
#define _LIBPENGCXX_UNREACHABLE() do {} while (1)
#endif

#if __cpp_lib_three_way_comparison
#define _LIBPENGCXX_DEFINE_COMPARISON(_Type) \
    bool operator==(_Type const &__that) const noexcept { \
        return std::equal(this->begin(), this->end(), __that.begin(), __that.end()); \
    } \
    \
    auto operator<=>(_Type const &__that) const noexcept { \
        return std::lexicographical_compare_three_way(this->begin(), this->end(), __that.begin(), __that.end()); \
    }
#else
#define _LIBPENGCXX_DEFINE_COMPARISON(_Type) \
    bool operator==(_Type const &__that) const noexcept { \
        return std::equal(this->begin(), this->end(), __that.begin(), __that.end()); \
    } \
    \
    bool operator!=(_Type const &__that) const noexcept { \
        return !(*this == __that); \
    } \
    \
    bool operator<(_Type const &__that) const noexcept { \
        return std::lexicographical_compare(this->begin(), this->end(), __that.begin(), __that.end()); \
    } \
    \
    bool operator>(_Type const &__that) const noexcept { \
        return __that < *this; \
    } \
    \
    bool operator<=(_Type const &__that) const noexcept { \
        return !(__that < *this); \
    } \
    \
    bool operator>=(_Type const &__that) const noexcept { \
        return !(*this < __that); \
    }
#endif
