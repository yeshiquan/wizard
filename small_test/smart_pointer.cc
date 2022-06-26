#include <memory>
#include <iostream>

void hello(std::shared_ptr<int> p = nullptr) {
    if (p == nullptr) {
        std::cout << "null" << std::endl;
    }
}

int main() {
    hello();
}
