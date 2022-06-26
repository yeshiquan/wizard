#include <iostream>

// 只有源文件（.cpp/.c）才能被编译器编译。
// 预处理器首先递归包含头文件，
// 形成一个含所有必要信息的单个源文件，此源文件就是一个编译单元

template <typename T>
inline void Context<T>::hello() {
    std::cout << "hello" << std::endl;
}

// 在使用模板的时候，如果将模板成员函数分别放在.h文件和.cpp文件中则编译时会出现错误。
// 错误原因是找不到链接。因为当实例化一个模板时，
// 编译器必须看到模板确切的定义，而不仅仅是它的声明。
template <typename T>
void Context<T>::world() {
    std::cout << "world" << std::endl;
}

/*
inline void Foobar::foo() {
    std::cout << "foo()" << std::endl;
}
*/
