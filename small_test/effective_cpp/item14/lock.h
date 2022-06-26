#pragma once 

#include <mutex>
#include <iostream>

class Lock {
public:
    explicit Lock(std::mutex* mutex) : _mutex(mutex) {
        std::cout << "Lock() _mutex:" << _mutex << std::endl;
        _mutex->lock();
    }
    ~Lock() {
        std::cout << "~Lock() _mutex:" << _mutex << std::endl;
        _mutex->unlock();
    }
private:
    std::mutex* _mutex = nullptr;
};

