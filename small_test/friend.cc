#include <iostream>

class Guest;

class Host {
public:
    Host(std::string dinner) : _dinner(dinner) {}
private:
    std::string _dinner;
    friend class Guest;
};

class Guest {
public:
    Host* h;
public:
    void test() {
        std::cout << "What dinner tonight? -> " << h->_dinner << std::endl;
    }
private:
    std::string _gift;
};

int main() {
    Host h("Fish");
    Guest g;
    g.h = &h;

    g.test();
}
