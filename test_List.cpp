#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <list>
#include <string>
#include <iostream>
#include <vector>
#include "List.hpp"

int main() {
    List<int> arr{1, 2, 4, 5, 6};
    arr.erase(arr.cbegin(), std::next(arr.cbegin(), 2));
    arr.insert(arr.begin(), {40, 41, 42});
    for (int i = 0; i < 3; i++) {
        arr.push_back(100 + i); // O(1)
    }
    for (int i = 0; i < 3; i++) {
        arr.push_front(200 + i); // O(1)
    }
    size_t i = 0;
    for (auto it = arr.cbegin(); it != arr.cend(); ++it) {
        int const &val = *it;
        printf("arr[%zd] = %d\n", i, val);
        ++i;
    }
    List<int> arr2 = arr;
    for (auto it = arr2.crbegin(); it != arr2.crend(); ++it) {
        int const &val = *it;
        --i;
        printf("arr[%zd] = %d\n", i, val);
    }
    printf("arr.size() = %zd\n", arr.size());
}
