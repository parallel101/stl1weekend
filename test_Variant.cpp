#include "Variant.hpp"
#include <iostream>

void print(Variant<std::string, int, double> v) {
    v.visit([&] (auto v) {
        std::cout << v << '\n';
    });
    // if (v.holds_alternative<std::string>()) {
    //     std::cout << v.get<std::string>() << '\n';
    // } else if (v.holds_alternative<int>()) {
    //     std::cout << v.get<int>() << '\n';
    // } else if (v.holds_alternative<double>()) {
    //     std::cout << v.get<double>() << '\n';
    // }
}

int main() {
    Variant<std::string, int, double> v1(inPlaceIndex<0>, "asas");
    print(v1);
    Variant<std::string, int, double> v2(42);
    print(v2);
    Variant<std::string, int, double> v3(3.14);
    print(v3);
}
