#include <iostream>

// 这个不是通用引用
int test1(int&& age) {
    std::cout << "test1: " << age << std::endl;
    return 1;
}

// Universal Reference
template<typename T>
int test2(T&& age) {
    std::cout << "test2: " << age << std::endl;
    return 1;
}

template<typename T>
struct Context {
    // 这个不是通用引用
    // Context is instantiated as Context<int>. Once this happens, the class template is instantiated
    int test3(T&& age) {
        std::cout << "test3: " << age << std::endl;
        return 1;
    }
};

int main() {
    int age = 5;
    // test1(age);      // cannot bind rvalue reference of type ‘int&&’ to lvalue of type ‘int’
    test2(age);
    Context<int> ctx;
    // ctx.test3(age);  // cannot bind rvalue reference of type ‘int&&’ to lvalue of type ‘int’
    return 0;
}

