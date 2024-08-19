#include <iostream>
#include "Map.hpp"
#include <string>

int main() {
    std::cout << std::boolalpha;
    Map<std::string, int> table;
    table["delay"] = 12;
    if (!table.contains("delay"))
        table["delay"] = 32;
    table["timeout"] = 42;
    
    for (auto it = table.begin(); it != table.end(); ++it)
        std::cout << it->first << "=" << it->second << '\n';

    std::cout << "at(delay): " << table.at("delay") << '\n';
    std::cout << "size: " << table.size() << '\n';

    return 0;
}
