#include <iostream>
#include <functional>

class GameCharacter;
using HealthCalcFunc = std::function<int(const GameCharacter&)>;

int default_health_calc(const GameCharacter&);

class GameCharacter {
public:
    explicit GameCharacter(HealthCalcFunc hcf) : 
                _health_func(hcf) {}
    int health_value() const {
        std::cout << "log helth_value start" << std::endl;
        int ret_val = _health_func(*this);
        std::cout << "log helth_value end" << std::endl;
        return ret_val;
    }
private:
    HealthCalcFunc _health_func;
};

int default_health_calc(const GameCharacter&) {
    return 55;
}

class EvialBadGuy : public GameCharacter {
public:
    explicit EvialBadGuy(HealthCalcFunc hcf) : 
                GameCharacter(hcf) {}
};

struct HealthCalculatorV2 {
    int operator()(const GameCharacter& c) const {
        return 66;
    }
};

class GameLevel {
public:
    int health(const GameCharacter& c) const {
        return 77;
    }
};

int main() {
    // 使用普通函数
    EvialBadGuy role_a(default_health_calc);
    int value = role_a.health_value();
    std::cout << "health_value -> " <<  value << "\n\n";

    // 使用函数对象
    HealthCalcFunc calc_func = HealthCalculatorV2();
    EvialBadGuy role_b(calc_func);
    value = role_b.health_value();
    std::cout << "health_value -> " <<  value << "\n\n";

    // 使用其他类的成员函数
    GameLevel level;
    HealthCalcFunc calc_func_2 = std::bind(&GameLevel::health, &level, std::placeholders::_1);
    EvialBadGuy role_c(calc_func_2);
    value = role_c.health_value();
    std::cout << "health_value -> " <<  value << "\n\n";

    return 0;
}
