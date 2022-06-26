#include <iostream>

class Base {
public:
    virtual void mf1() = 0;
    virtual void mf1(int v) {
        std::cout << "Base::mf1" << std::endl;
    }
    virtual void mf2() {
    }
    void mf3() {
    }
    void mf3(int v) {
        std::cout << "Base::mf3" << std::endl;
    }
private:
    int x;
};

class Derived : public Base {
public:
    //using Base::mf1;
    //using Base::mf3;
    virtual void mf1() {
    }
    void mf3() {
    }
    void mf4() {
    }
};

int main() {
    Derived d;
    int v = 333;

    // 如果不适用using，Base的mf1和mf3将被遮蔽
    // error: no matching function for call to ‘Derived::mf1(int&)’
    // error: no matching function for call to ‘Derived::mf3(int&)’
    d.mf1(v);
    d.mf3(v);

    return 0;
}

