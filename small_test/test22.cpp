#include <memory>
#include <iostream>

struct Context {
    std::shared_ptr<std::string> p_info;
};

class A {
public:
    void func() {
        if (!context.p_info) {
            // 指针为空才初始化
            context.p_info = std::make_shared<std::string>();
            *context.p_info = "Hello";
        }
    }

    void check() {
        if (context.p_info) {
            std::cout << "info is alive:" << *context.p_info << std::endl;
        }
    }
private:
    Context context;
};

int main() {
    A a;
    a.func();
    // 检查info, 是否依然存在
    a.check();

    // a析构后会自动释放p_info
    return 0;
}
