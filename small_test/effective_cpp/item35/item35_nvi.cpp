#include <iostream>

// NVI手法
// Non-Virtual Interface手法实现Template Method模式
// 通过non-virtual成员函数间接调用private virtual函数

class GameCharacter {
public:
    // 相当于virtual do_health_value的一个Wrapper
    int health_value() const {
        std::cout << "log helth_value start" << std::endl;
        int ret_val = do_health_value();
        std::cout << "log helth_value end" << std::endl;
        return ret_val;
    }
private:
    virtual int do_health_value() const {
        constexpr int default_value = 55;
        return default_value;
    }
};

class EvialBadGuy : public GameCharacter {
private:
    virtual int do_health_value() const override {
        constexpr int default_value = 55;
        return default_value - 11;
    }
};

int main() {
    EvialBadGuy b;
    int value = b.health_value();
    std::cout << "health_value -> " <<  value << std::endl;
    return 0;
}
