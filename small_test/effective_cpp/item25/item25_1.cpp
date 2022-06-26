#include <iostream>
#include <vector>
#include <random>

// namespace std {
//
// template<typename T>   // typical implementation of std::swap;
// void swap(T& a, T& b)  // swaps a's and b's values
// {
//     T temp(a);
//     a = b;
//     b = temp;
// }
// }

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
    void say() {
        impl->say();
    }
private:
    WidgetImpl* impl = nullptr;
};

int main() {
    Widget w1;
    Widget w2;
    w1.say();
    w2.say();
    std::swap(w1, w2); // 标准的swap会调用一次copy constructor和两次assign constructor, 效率很低
    //w1.swap(w2);
    std::cout << "after swap -> \n";
    w1.say();
    w2.say();
    return 0;
}
