#include <iostream>

struct Context {
    Context() {
        std::cout << "Context()" << std::endl;
    }
    int value;
};

Context* test1() {
    // thread_local implys variable is static
    static thread_local Context ctx;
    //thread_local Context ctx;
    ctx.value = 3;
    return &ctx;
}

Context* test2() {
    static thread_local Context ctx;
    //thread_local Context ctx;
    ctx.value = 5;
    return &ctx;
}

int main() {
    {
        auto* ctx1 = test1();
        auto* ctx2 = test2();
        int c = 99;

        std::cout << ctx1->value << std::endl;
        std::cout << ctx2->value << std::endl;

        std::cout << ctx1 << std::endl;
        std::cout << ctx2 << std::endl;
        std::cout << &c << std::endl;
    }
    {
        auto* ctx1 = test1();
        auto* ctx2 = test2();
        int c = 99;

        std::cout << ctx1->value << std::endl;
        std::cout << ctx2->value << std::endl;

        std::cout << ctx1 << std::endl;
        std::cout << ctx2 << std::endl;
        std::cout << &c << std::endl;
    }
    return 0;
}
