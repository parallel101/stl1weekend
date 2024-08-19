#include <iostream>
#include "Map.hpp"
#include <string>

int main() {
    std::cout << std::boolalpha;
    Map<std::string, int> table;
    table["delay"] = 12;
    if (!table.contains("delay"))
        table["delay"] = 32;
    
    for (auto it = table.begin(); it != table.end(); ++it)
        std::cout << it->first << "=" << it->second << '\n';

    auto it = table.begin();
    std::cout << "current it is " << it._M_node_ptr() << " " << (it != table.end() ? it->second : -1) << ", and tree is ";
    table._M_print(std::cout);
    it = table.erase(it);
    std::cout << "current it is " << it._M_node_ptr() << " " << (it != table.end() ? it->second : -1) << ", and tree is ";
    table._M_print(std::cout);
}
