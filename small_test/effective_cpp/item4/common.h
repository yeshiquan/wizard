#pragma once

#include <iostream>

class Test2;
class Test1;
extern Test2 t2;
extern Test1 t1;

class Test2 {
public:
    static Test2& instance() {
        static Test2 ins;
        return ins;
    }
    Test2() : value(100) {
        std::cout << "Test2 construct" << std::endl;
    }
    void say() {
        std::cout << "  -> I am Test2 " << "value=" << value << std::endl;
    }
private:
    int value;
};


// 输出结果，Test1先初始化，Test2后初始化
// Test1 construct
//   -> I am Test2 value=0
// Test2 construct
// main()

class Test1 {
public:
    Test1() {
        std::cout << "Test1 construct" << std::endl;

        // 定义在不同编译单元内的non-local static对象其初始化相对次序是无明确的定义
        // t2可能没有初始化，你会调用1个未初始化对象的方法，非常危险
        t2.say();

        // 改成单例模式就可以确保Test2已经初始化OK
        //Test2::instance().say();
    }
};
