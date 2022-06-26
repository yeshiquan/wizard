#include <iostream>
#include <atomic>

int main() {
    std::atomic<int*> a{nullptr};
    int* p = reinterpret_cast<int*>(0xFFFFFFFFFFFFFFFFL);
    int* p2 = nullptr;

    a.store(p);
    bool c = a.load();
    std::cout << c << std::endl;

    a.store(p2);
    bool d = a.load();
    std::cout << d << std::endl;
    return 0;
}
