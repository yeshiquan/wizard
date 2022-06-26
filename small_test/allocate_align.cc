#include <iostream>
#include <cstddef>
#include <memory>

struct alignas(32) Foo {
    int a,b,c,d;
    char ch;
};

struct T {
    int a,b,c,d;
    char i;
};
typename std::aligned_storage<sizeof(T), alignof(T)>::type   data[100];

void* allocate(
    const size_t bytes,
    const size_t alignment = alignof(std::max_align_t)) {
  (void)alignment;
  void* p = static_cast<void*>(new char[bytes]);
  return p;
}

void deallocate(
    void* p,
    const size_t bytes,
    const size_t alignment = alignof(std::max_align_t)) {
  (void)alignment;
  (void)bytes;
  delete[] static_cast<char*>(p);
}

int main() {
    void* p = allocate(5);
    char* p2 = static_cast<char*>(p);
    p2[7] = 'c';
    deallocate(p, 5);

    T& t = *reinterpret_cast<T*>(&data[5]);
    t.a = 45;
    std::cout << t.a << std::endl;
    std::cout << t.b << std::endl;
    return 0;
}
