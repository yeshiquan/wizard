#include <iostream>

class Airport {};

class Airplane {
public:
    virtual void fly(const Airport& dest);
};

void Airplane::fly(const Airport& dest) {
    std::cout << "default fly() only for ModelA & ModelB" << std::endl;
}

class ModelA : public Airplane {};
class ModelB : public Airplane {};

class ModelC : public Airplane {};

int main() {
    Airport pdx;

    Airplane* pa = new ModelC;
    pa->fly(pdx);

    return 0;
}
