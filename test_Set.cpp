#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <list>
#include <string>
#include <iostream>
#include <vector>
#include <set>

template <class T>
struct Set {
    struct RbNode {
        RbNode *left;
        RbNode *right;
        RbNode *parent;
        bool isRed;
        T value;
    };

    RbNode dummy;
};

int main() {
    Set<int> s;
}
