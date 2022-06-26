#include <iostream>
#include <vector>
#include <random>

int intRand(const int & min, const int & max) {
    static thread_local std::mt19937 generator;
    std::uniform_int_distribution<int> distribution(min,max);
    return distribution(generator);
}

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
        // 操作基础数据类型肯定可以确保异常安全
        std::swap(impl, rhs.impl);
    }
    void say() {
        impl->say();
    }
private:
    WidgetTpImpl<T>* impl = nullptr;
};

// 尝试重载呢？也不行
namespace std {
    // 对函数模板进行重载是可以的，但是std是一个特殊的命名空间，
    // 使用它的规则也很特殊。在std中进行全特化是可以的，
    // 但是添加新的模板（类，函数或其他任何东西）不可以。
    // std的内容完全由C++标准委员会来决定。越过这条线的程序肯定可以通过编译并且能运行，
    // 但是行为未定义。如果你想你的软件有可预测的行为，不要向std中添加新东西。
    template<typename T>
    void swap(WidgetTp<T>& a, WidgetTp<T>& b) {  // an overloading of std::swap
        a.swap(b);
    }

    // template<typename T>          // typical implementation of std::swap;
    // void swap(T& a, T& b)         // swaps a's and b's values
}

int main() {
    WidgetTp<int> w1;
    WidgetTp<int> w2;
    w1.say();
    w2.say();
    std::swap(w1, w2);
    //w1.swap(w2);
    std::cout << "after swap -> \n";
    w1.say();
    w2.say();
    return 0;
}
