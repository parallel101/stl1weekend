#include <array>
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <string>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>
#include "Array.hpp"

// concept Functor:
// Class::operator()
// void (*fp)()

// concept RandomAccessorIterator:
// *p
// p->...
// ++p
// --p
// p += n
// p -= n
// p + n
// p - n
// p[n]
// p1 - p2
// p1 != p2
// p1 < p2

template <class T, size_t N>
void iota(Array<T, N> &a) noexcept {
    T count = 0;
    // for (auto it = a.begin(), eit = a.end(); it != eit; ++it)
    for (auto &ai: a) {
        ai = count++; // a[i] = i;
    }
}

int main() {
    auto a = Array{2, 1, 0};
    for (auto &ai: a) {
        std::cout << ai << '\n';
    }
    iota(a);
    for (auto &ai: a) {
        std::cout << ai << '\n';
    }
    std::cout << "front:" << a.front() << '\n';
    std::cout << "back:" << a.back() << '\n';
    return 0;
}
