#include <iostream>
#include <typeinfo>

class Transaction {
public:
    Transaction() {
        //log();  // undefined reference to `Transaction::log() const'
        std::cout << typeid(this).name() << std::endl;
        init();
    }
    void init() {
        log(); // pure virtual method called. terminate called without an active exception(core dumped)
    }
    virtual void log() const = 0;
};

class BuyTransaction : public Transaction {
public:
    BuyTransaction() {
        std::cout << typeid(this).name() << std::endl;
    }
    virtual void log() const override {
        std::cout << "buy transaction done" << std::endl;
    }
};

class SellTransaction : public Transaction {
public:
    SellTransaction() {
        std::cout << typeid(this).name() << std::endl;
    }
    virtual void log() const override {
        std::cout << "sell transaction done" << std::endl;
    }
};

int main() {
    BuyTransaction b;
    return 0;
}


