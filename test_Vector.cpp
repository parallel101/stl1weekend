#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>
#include <iostream>
#include <vector>
#include "Vector.hpp"

int main() {
    Vector<int> arr; // data size cap
    // size=0 cap=0
    // size=1 cap=1 *
    // size=2 cap=2 *
    // size=3 cap=4 *
    // size=4 cap=4
    // size=5 cap=8 *
    // size=6 cap=8
    // size=7 cap=8
    // size=8 cap=8
    // size=9 cap=16 *
    // size=10 cap=16
    for (int i = 0; i < 16; i++) { // O(n)
        printf("arr.push_back(%d)\n", i);
        arr.push_back(i); // O(1)+
    }
    arr.insert(arr.begin() + 3, {40, 41, 42});
    for (size_t i = 0; i < arr.size(); i++) {
        printf("arr[%zd] = %d\n", i, arr[i]);
    }

    Vector<int> bar(3);
    printf("arr.size() = %zd\n", arr.size());
    printf("bar.size() = %zd\n", bar.size());
    bar = std::move(arr);
    printf("arr.size() = %zd\n", arr.size());
    printf("bar.size() = %zd\n", bar.size());
    printf("sizeof(Vector) = %zd\n", sizeof(Vector<int>));
}
