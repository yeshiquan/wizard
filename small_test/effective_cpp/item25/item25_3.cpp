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
        std::swap(impl, rhs.impl);
    }
    void say() {
        impl->say();
    }
private:
    WidgetTpImpl<T>* impl = nullptr;
};

// 对于模板类，对std::swap无法实现特化, 编译不通过
namespace std {
    // error: non-class, non-variable partial specialization ‘swap<WidgetTp<T> >’ is not allowed
    template<typename T>
    void swap<WidgetTp<T>>(WidgetTp<T>& a, WidgetTp<T>& b) {
        a.swap(b);
    }
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
