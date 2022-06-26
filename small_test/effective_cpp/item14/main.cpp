#include "lock.h"


int main() {
    std::mutex* mutex = new std::mutex;

    Lock lock1(mutex);
    delete mutex;
    mutex = nullptr;

    std::cout << "--------" << std::endl;

    Lock lock2(lock1);
    // lock2析构的时候，_mutex已经被delete了，调用它的unlock方法是危险的
    // 这就是为什么RAII资源管理器为什么要禁用拷贝
    return 0;
}


