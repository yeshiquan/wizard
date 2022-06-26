#include <iostream>

class Airport {};

class Airplane {
public:
    virtual void fly(const Airport& dest) = 0;
protected:
    // 不能是public
    void default_fly(const Airport& dest);
};

void Airplane::default_fly(const Airport& dest) {
    std::cout << "default fly() only for ModelA & ModelB" << std::endl;
}

class ModelA : public Airplane {
public:
    virtual void fly(const Airport& dest) override {
        default_fly(dest);
    }
};

class ModelB : public Airplane {
public:
    virtual void fly(const Airport& dest) override {
        default_fly(dest);
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
