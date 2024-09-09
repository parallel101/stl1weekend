#pragma once

#include <utility>

template <class _Tp, class _Up>
_Tp exchange(_Tp &dst, _Up &&val) { // 同标准库的 std::exchange
    _Tp tmp = std::move(dst);
    dst = std::forward<_Up>(val);
    return tmp;
}
