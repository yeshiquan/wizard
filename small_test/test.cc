#include <iostream>

class HazptrManager;
HazptrManager& get_default_manager();

class HazptrPrivate {
public:
    void hello() {
        _manager = &get_default_manager();
    }
private:
    HazptrManager* _manager;
};

class HazptrManager {
public:
    static HazptrManager& instance() {
        static HazptrManager instance;
        return instance;
    }
public:
    int val {0};
};

HazptrManager& get_default_manager() {
    static HazptrManager manager;
    return manager;
}


int main() {
    HazptrPrivate priv;
    return 0;
}





