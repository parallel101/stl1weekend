#include "SharedPtr.hpp"
#include <iostream>

// CRTP
struct Student : EnableSharedFromThis<Student> {
    const char *name;
    int age;

    explicit Student(const char *name_, int age_) : name(name_), age(age_) {
        std::cout << "Student 构造\n";
    }

    Student(Student &&) = delete;

    void func() {
        std::cout << (void *)shared_from_this().get() << '\n';
    }

    ~Student() {
        std::cout << "Student 析构\n";
    }
};

struct StudentDerived : Student {
    explicit StudentDerived(const char *name_, int age_) : Student(name_, age_) {
        std::cout << "StudentDerived 构造\n";
    }

    ~StudentDerived() {
        std::cout << "StudentDerived 析构\n";
    }
};

int main() {
    SharedPtr<Student> p0(new StudentDerived("彭于斌", 23));
    auto dp = staticPointerCast<StudentDerived>(p0);
    SharedPtr<Student const> bp = p0;
    p0 = constPointerCast<Student>(bp);
    std::cout << dp->name << '\n';

    SharedPtr<Student> p = makeShared<Student>("彭于斌", 23); // make_shared 一次性创建
    SharedPtr<Student> p2(new Student("彭于斌", 23));         // 用 new 创建指针后再构造（不推荐）
    SharedPtr<Student> p3(new Student("彭于斌", 23), [] (Student *p) { delete p; });
    Student *raw_p = p.get();        // 获取原始指针
    SharedPtr<Student> p4 = p;  // 浅拷贝
    SharedPtr<Student> p5 = p3; // 浅拷贝

    p5->func();

    p3 = p5;
    
    std::cout << p->name << ", " << p->age << '\n';
    std::cout << raw_p->name << ", " << raw_p->age << '\n';
}
