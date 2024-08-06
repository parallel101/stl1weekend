#include <iostream>
#include "Map.hpp"
#include <string>

int main() {
    Map<std::string, int> table;
    table["delay"] = 12;
    if (!table.contains("delay"))
        table["delay"] = 32;
    table["timeout"] = 42;
    // table._M_print();
    auto it = table.find("delay");
    while (it != table.end())
        std::cout << (it++)->second << '\n';
    std::cout << "at delay: " << table.at("delay") << '\n';
}
