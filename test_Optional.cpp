#include "Optional.hpp"
#include <memory>
#include <optional>
#include <iostream>
#include <vector>

struct C {
    C(int x, int y) : m_x(x), m_y(y) {}

    C(C const &) {
        printf("C(C const &)\n");
    }

    C(C &&) {
        printf("C(C &&)\n");
    }

    C &operator=(C const &) {
        printf("C &operator=(C const &)\n");
        return *this;
    }

    C &operator=(C &&) {
        printf("C &operator=(C &&)\n");
        return *this;
    }

    ~C() {
        printf("~C()\n");
    }

    int value() const {
        return m_x;
    }

    int m_x;
    int m_y;
};

namespace CCCP {
struct C {
    C(int x, int y) : m_x(x), m_y(y) {}

    C(C const &) {
        printf("C(C const &)\n");
    }

    C(C &&) {
        printf("C(C &&)\n");
    }

    C &operator=(C const &) {
        printf("C &operator=(C const &)\n");
        return *this;
    }

    C &operator=(C &&) {
        printf("C &operator=(C &&)\n");
        return *this;
    }

    ~C() {
        printf("~C()\n");
    }

    int value() const {
        return m_x;
    }

    int m_x;
    int m_y;
};

void swap(C &c1, C &c2) {
    (void)c1, (void)c2;
    printf("swap(C &, C &)\n");
}
}

void test_ours() {
    Optional<C> optc(nullopt);

    Optional<std::string> opts("ss");

    opts.value(); // std::string &
    std::move(opts).value(); // std::string &&

    auto s1 = std::move(opts.value());
    auto s2 = std::move(opts).value();

    const Optional<std::string> optcs("ss");
    optcs.value();

    Optional<int> opt(1);

    std::cout << opt.has_value() << '\n'; // true
    std::cout << opt.value() << '\n'; // 1
    std::cout << opt.value_or(0) << '\n'; // 1

    opt = nullopt;
    std::cout << opt.has_value() << '\n'; // false

    opt = 42;
    std::cout << opt.has_value() << '\n'; // true
    std::cout << opt.value() << '\n'; // 42
    std::cout << opt.value_or(0) << '\n'; // 42

    opt = Optional<int>(42);
    std::cout << opt.has_value() << '\n'; // true
    std::cout << opt.value() << '\n'; // 42
    std::cout << opt.value_or(0) << '\n'; // 42

    opt = Optional<int>(nullopt);
    std::cout << opt.has_value() << '\n'; // false

    Optional<int> opt2(nullopt);
    std::cout << opt2.has_value() << '\n'; // false
    try {
        opt2.value();
    } catch (BadOptionalAccess const &e) {
        std::cout << "opt2 exception: " << e.what() << '\n';
    }
    std::cout << opt2.value_or(0) << '\n'; // 0

    Optional<int> opt3 = opt2;
    std::cout << opt3.has_value() << '\n'; // false
    try {
        opt2.value();
    } catch (BadOptionalAccess const &e) {
        std::cout << "opt3 exception: " << e.what() << '\n';
    }
    std::cout << opt3.value_or(0) << '\n'; // 0
}

void test_std() {
    std::optional<int> opt(1);
    std::cout << opt.has_value() << '\n'; // true
    std::cout << opt.value() << '\n'; // 1
    std::cout << opt.value_or(0) << '\n'; // 1

    std::optional<int> opt2(std::nullopt);
    std::cout << opt2.has_value() << '\n'; // false
    try {
        opt2.value();
    } catch (std::bad_optional_access const &) {
        std::cout << "opt2 exception ok\n";
    }
    std::cout << opt2.value_or(0) << '\n'; // 0

    opt2 = 42;
    std::cout << opt2.has_value() << '\n'; // true
    std::cout << opt2.value() << '\n'; // 42
    std::cout << opt2.value_or(0) << '\n'; // 42
}

Optional<int> parseInt(std::string s) {
    try {
        return std::stoi(s);
    } catch (std::invalid_argument const &) {
        return nullopt;
    }
}

Optional<int> getInt(std::istream &is) {
    std::string s;
    is >> s;
    if (!is.good())
        return nullopt;
    return parseInt(s);
}

void test_emplace() {
    while (auto opt = getInt(std::cin)) {
        std::cout << *opt << '\n';
    }

    Optional<C> opt;

    Optional<C> optc;
    //optc = C(1, 2); // 1. C(int x, int y);   2. optional::operator=(C &&value) { new (&m_value) T(std::move(value)); }
    optc.emplace(1, 2);  // 1. optional::emplace(int x, int y) { new (&m_value) T(x, y); }

    optc.value();
    /* int x = (*optc).m_x; */
    /* int x2 = optc->m_x; */

    Optional<int> opti;
    opti.emplace(42); // new (&m_value) int(1);  // m_value = 42;
    opti.emplace(); // new (&m_value) int();  // m_value = int();

    int i = static_cast<bool>(opti);
    std::cout << i << '\n';

    Optional<std::string> opts = nullopt;
    opts.emplace(); // ""

    opts.reset(); // opts = nullopt;
}

void test_cmp() {
    Optional<int> o = nullopt;
    std::cout << (o != Optional(100)) << '\n';

    auto x = makeOptional(42);

    o = -42;
    auto o2 = o.and_then([&] (int i) -> Optional<int> {
        if (i < 0)
            return nullopt;
        return i + 1;
    });
    if (o2) {
        std::cout << *o2 << '\n';
    } else {
        std::cout << "nullopt\n";
    }

    o = -42;
    std::unique_ptr<int> up = std::make_unique<int>();
    auto o3 = o.transform([up = std::move(up)] (int i) -> int {
        return i + 1;
    });
    if (o3) {
        std::cout << o3.value() << '\n';
    } else {
        std::cout << "nullopt\n";
    }

    o = 42;
    auto o4 = o.or_else([] () -> Optional<int> {
        std::cout << "没值！\n";
        return 0;
    });
    if (o4) {
        std::cout << o4.value() << '\n';
    } else {
        std::cout << "nullopt\n";
    }
}

void test_in_place() {
    Optional<C> o1 = C(1, 2);
    o1.emplace(1, 2);
    
    Optional<C> o2(inPlace, 1, 2);
    Optional<C> o3(C(1, 2));
    Optional<std::vector<int>> ov(inPlace, {1, 2, 3});
    
    Optional<CCCP::C> cccp(inPlace, 1, 2);
    Optional<CCCP::C> ussr(inPlace, 3, 4);
    cccp.swap(ussr);
}

int main() {
    std::cout << std::boolalpha;
    /* test_std(); */
    /* test_ours(); */
    /* test_emplace(); */
    /* test_cmp(); */
    test_in_place();
}
