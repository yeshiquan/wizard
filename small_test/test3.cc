#include <iostream>

// 要记得把父类的析构函数变成虚函数
// 否则子类将无法析构
class Base {
public:
    // ~Base() {}      // delete base无法调用子类的析构
    virtual ~Base() {} // delete base可以调用子类的析构
};

class Derive : public Base {
public:
    ~Derive() {
        std::cout << "~Derive()"<< std::endl;
    }
};

int main() {
    Base* p = new Derive();
    delete p;
}
