#include <cstdio>
#include <iostream>
#include "Function.hpp"

void func_hello(int i) {
    printf("#%d Hello\n", i);
}

struct func_printnum_t {
    void operator()(int i) const {
        printf("#%d Numbers are: %d, %d\n", i, x, y);
    }
    int x;
    int y;
};

void repeattwice(Function<void(int)> const &func) {
    func(1);
    func(2);
}

int main() {
    int x;
    int y;
    std::cin >> x >> y;
    repeattwice([=] (int i) {
        printf("#%d Numbers are: %d, %d\n", i, x, y);
    });
    func_printnum_t func_printnum{x, y};
    repeattwice(func_printnum);
    repeattwice(func_hello);
    return 0;
}
