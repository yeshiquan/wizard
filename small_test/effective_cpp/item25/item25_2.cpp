#include <iostream>
#include <vector>
#include <random>

int intRand(const int & min, const int & max) {
    static thread_local std::mt19937 generator;
    std::uniform_int_distribution<int> distribution(min,max);
    return distribution(generator);
}

class WidgetImpl {
public:
    WidgetImpl() {
        a = intRand(1, 1000000);
    }
    void say() {
        std::cout << "Hello World a:" << a << std::endl;
    }
private:
    int a = 0;
    int b = 0;
    int c = 0;
    std::vector<int> v;
};

class Widget {
public:
    Widget() {
        impl = new WidgetImpl;
    }
    Widget(const Widget& rhs) {
        std::cout << "Copy constructor\n";
        impl = new WidgetImpl(*rhs.impl);
    }
    Widget& operator=(const Widget& rhs) {
        std::cout << "Assign constructor\n";
        *impl = *(rhs.impl);
        return *this;
    }
    void swap(Widget& rhs) {
        // 操作基础数据类型肯定可以确保异常安全
        std::swap(impl, rhs.impl);
    }
    void say() {
        impl->say();
    }
private:
    WidgetImpl* impl = nullptr;
};

// 对于非模板类，可以特化std::swap
namespace std {
    template<>
    void swap<Widget>(Widget& a, Widget& b) {
        a.swap(b);
    }
}

int main() {
    Widget w1;
    Widget w2;
    w1.say();
    w2.say();
    std::swap(w1, w2); // 使用std::swap依然可以获得高效的实现
    //w1.swap(w2);
    std::cout << "after swap -> \n";
    w1.say();
    w2.say();
    return 0;
}
