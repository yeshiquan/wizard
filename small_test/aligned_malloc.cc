#include <memory>
#include <iostream>
#include <bitset>

void test() {
    constexpr size_t CACHE_LINE = 64;
    int cnt = 13;
    int* data = reinterpret_cast<int*>(aligned_alloc(CACHE_LINE, sizeof(int)*cnt));
    int* data2 = reinterpret_cast<int*>(malloc(sizeof(int)*cnt));
    std::cout << std::bitset<64>(reinterpret_cast<uint64_t>(data)) << std::endl;
    std::cout << std::bitset<64>(reinterpret_cast<uint64_t>(data2)) << std::endl;
    data[14] = 89;
    std::cout << data[14] << std::endl;
}

int main() {
    test();
    return 0;
}
