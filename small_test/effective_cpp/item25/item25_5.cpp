#include <iostream>
#include <vector>
#include <random>

int intRand(const int & min, const int & max) {
    static thread_local std::mt19937 generator;
    std::uniform_int_distribution<int> distribution(min,max);
    return distribution(generator);
}

namespace widgetstuff {

template<typename T>
class WidgetTpImpl {
public:
    WidgetTpImpl() {
        a = intRand(1, 1000000);
    }
    void say() {
        std::cout << "Hello World a:" << a << std::endl;
    }
private:
    int a = 0;
    int b = 0;
    int c = 0;
    std::vector<T> v;
};

template<typename T>
class WidgetTp {
public:
    WidgetTp() {
        impl = new WidgetTpImpl<T>;
    }
    WidgetTp(const WidgetTp& rhs) {
        std::cout << "Copy constructor\n";
        impl = new WidgetTpImpl<T>(*rhs.impl);
    }
    WidgetTp& operator=(const WidgetTp& rhs) {
        std::cout << "Assign constructor\n";
        *impl = *(rhs.impl);
        return *this;
    }
    void swap(WidgetTp& rhs) {
        std::swap(impl, rhs.impl);
    }
    void say() {
        impl->say();
    }
private:
    WidgetTpImpl<T>* impl = nullptr;
};

// 在另一个namespace下定义自己的swap
template<typename T>
void swap(WidgetTp<T>& a, WidgetTp<T>& b) {
    std::cout << "call widgetstuff::swap()\n";
    a.swap(b);
}

} // namespace

int main() {
    widgetstuff::WidgetTp<int> w1;
    widgetstuff::WidgetTp<int> w2;
    w1.say();
    w2.say();

    // 使用的时候先让std::swap变得可见
    // 当编译器看到了对swap的调用，它们会寻找swap的正确版本。
    // C++名字搜寻策略先在全局范围内或者同一个命名空间内搜寻swap的T特定版本。
    // （例如，如果T是命名空间WidgetStuff中的Widget，编译器会用参数依赖搜寻（argument-dependent
    // lookup）在WidgetStuff中寻找swap）.
    // 如果没有T特定的swap版本存在，编译器会使用std中的swap版本，多亏了using
    // std::swap使得std::swap在函数中可见。
    // 但是编译器更喜欢普通模板std::swap上的T指定特化版本，因此如果std::swap已经为T特化过了，特化版本将会调用。
    using std::swap;
    swap(w1, w2);

    //w1.swap(w2);
    std::cout << "after swap -> \n";
    w1.say();
    w2.say();
    return 0;
}

