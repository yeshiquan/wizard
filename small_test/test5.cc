#include <iostream>
#include <functional>

class Base {
public:
    std::function<void(Base*)> deleter;
};

class Derive : public Base {
public:
    Derive() {
        // 在子类设置父类的deleter
        // 子类完全清楚要如何析构
        deleter = [](Base* p) {
            Derive *d = static_cast<Derive*>(p);
            delete d;
        };
    }
    ~Derive() {
        std::cout << "~Derive()" << std::endl;
    }
};

int main() {
    Base* p = new Derive;
    (p->deleter)(p); // 调用父类的deleter，但是deleter是在子类设置的
    // delete p; // 直接delete父类指针不会析构子类的对象
}
