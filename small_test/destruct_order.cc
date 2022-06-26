#include <iostream>
#include <vector>
#include <thread>

class Manager {
public:
    Manager() {
        std::cout << "Manager()" << std::endl;
        vec = new std::vector<int>();
    }
    ~Manager() {
        std::cout << "~Manager()" << std::endl;
        delete vec;
    }
    void append(int v) {
        std::cout << "append " << v << std::endl;
        vec->push_back(v);
    }
public:
    std::vector<int>* vec;
};

Manager& get_default_manager() {
    static Manager manager;
    return manager;
}

class Local {
public:
    static Local& instance() {
        static thread_local Local ins;
        return ins;
    }
    Local() {
        std::cout << "Local()" << std::endl;
        manager = &get_default_manager();
    }
    ~Local() {
        std::cout << "~Local()" << std::endl;
        manager->append(242);
    }
public:
    Manager* manager;
};

int main() {
    Local::instance();
    return 0;
}
