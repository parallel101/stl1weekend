#include <cstdio>
#include <iostream>
#include "Set.hpp"

int main() {
    MultiSet<int> table;
    table.insert(1);
    table.insert(2);
    // table._M_print();
    table.insert(2);
    table.insert(2);
    table.insert(2);
    table.insert(-5);
    table.insert(3);
    table.insert(-5);
    // table._M_print();
    auto it = table.lower_bound(2);
    while (it != table.upper_bound(2))
        std::cout << *it++ << '\n';
    // table._M_print();
    table.erase(2);
    // table._M_print();
    for (auto const &entry: table) {
        std::cout << entry << '\n';
    }
    Set<int> s;
    s.insert(1);
    s.insert(3);
    s.insert(5);
    s.insert(4);
    printf("insert 4 = %d\n", s.insert(4).second); // 0
    printf("insert 6 = %d\n", s.insert(6).second); // 1
    printf("insert 7 = %d\n", s.insert(7).second); // 1
    printf("find 3 = %d\n", s.find(3) != s.end()); // 1
    printf("find 2 = %d\n", s.find(2) != s.end()); // 0
    printf("find 4 = %d\n", s.find(4) != s.end()); // 1
    // s._M_print();
    s.erase(3);
    printf("find 3 = %d\n", s.find(3) != s.end()); // 0
    // s._M_print();
    printf("min = %d\n", *s.begin());
    printf("max = %d\n", *s.rbegin());
    for (int i: s) {
        printf("%d\n", i);
    }
}
