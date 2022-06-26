#include <iostream>
#include <atomic>

int main() {
    std::atomic<int> cnt = 0;
    cnt.fetch_add(5);
    std::cout << cnt.load() << std::endl;
}
