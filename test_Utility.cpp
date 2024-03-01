#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <utility>
#include <memory>
#include <new>

void *operator new(size_t size) {
    printf("size=%zd\n", size);
    return malloc(size);
}

void *operator new(size_t size, std::align_val_t align) {
    printf("size=%zd align=%zd\n", size, (size_t)align);
    return aligned_alloc(size, (size_t)align);
}

void *operator new(size_t size, const char *arg) {
    printf("size=%zd arg=%s\n", size, arg);
    return malloc(size);
}

struct alignas(64) M {
};

int main (int argc, char *argv[]) {
    char buf[32];
    new ("hello") M;
    return 0;
}
