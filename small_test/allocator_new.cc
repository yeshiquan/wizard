#include <iostream>
#include <memory>

// allocator可以把申请内存和调用构造函数初始化2个步骤分离，从而带来更好的灵活性
// allocator可以避免将内存分配和对象构造组合在一起导致不必要的浪费

class Context {
public:
    Context() {}
    Context(std::string name, int age) {
        std::cout << "Context()" << std::endl;
        age_ = age;
        name_ = name;
    }
    ~Context() {
        std::cout << "~Context()" << std::endl;
    }
public:
    std::string name_;
    int age_;
};

// allocator用于模板
template <typename Allocator = std::allocator<uint8_t>>
Context* get_context() {
    auto ctx = (Context*)Allocator().allocate(sizeof(Context));  
    new (ctx) Context("kobe", 25); 
    return ctx;
}

int main() {
    std::allocator<uint8_t> alloc; // uint8_t是1个字节

    {   // 创建1个对象
        auto ctx = (Context*)alloc.allocate(sizeof(Context));  // 分配 sizeof(Context)个字节
        std::cout << "allocate done" << std::endl;
        new (ctx) Context("kobe", 25); // 调用构造函数
        std::cout << "constructor done" << std::endl;
        std::cout << ctx->name_ << " " << ctx->age_ << std::endl;
        ctx->~Context();
        alloc.deallocate((uint8_t*)ctx, sizeof(Context));
    }

    {   // 创建对象数组
        int count = 10;
        auto ctxs = alloc.allocate(sizeof(Context) * count);
        new (ctxs) Context[count];
        alloc.deallocate((uint8_t*)ctxs, sizeof(Context) * count);
    }

    // 模板函数
    Context* another = get_context();

    {   // 使用construct 和 destroy
        std::allocator<std::string> alloc;
        std::string* names = alloc.allocate(3); // 分配3个string对象
        alloc.construct(names, "Hello");
        alloc.construct(names+1, "World");
        alloc.construct(names+2, "Foobar");
        std::cout << names[0] << " " << names[1] << " " << names[2] << std::endl;

        alloc.destroy(names);
        alloc.destroy(names+1);
        alloc.destroy(names+2);
        alloc.deallocate(names, 3);
    }

    return 0;
}
