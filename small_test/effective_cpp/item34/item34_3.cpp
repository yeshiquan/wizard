#include <iostream>

class Airport {};

class Airplane {
public:
    // 接口部分：子类必须使用
    virtual void fly(const Airport& dest) = 0;
};

// 尽管fly被声明为pure virtual函数，但是依然可以定义它的实现
// 定义部分：子类可以有选择的使用，明确指明要调用才可以
void Airplane::fly(const Airport& dest) {
    std::cout << "default fly() only for ModelA & ModelB" << std::endl;
}

class ModelA : public Airplane {
public:
    virtual void fly(const Airport& dest) override {
        // 调用父类的fly
        Airplane::fly(dest);
    }
};

class ModelB : public Airplane {
public:
    virtual void fly(const Airport& dest) override {
        // 调用父类的fly
        Airplane::fly(dest);
    }
};

class ModelC : public Airplane {
public:
    virtual void fly(const Airport& dest) override {
        std::cout << "fly() only for ModelC" << std::endl;
    }
};

int main() {
    Airport pdx;

    Airplane* pa = new ModelC;
    pa->fly(pdx);

    return 0;
}

