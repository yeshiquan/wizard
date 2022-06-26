#include "inline.h"

#include <iostream>

// 如果将函数的实现放在cpp文件中，并且没有标记为inline
// 那么该函数可以被连接到其他编译单元中。
void Foobar::bar() {
    std::cout << "bar()" << std::endl;
}

// 如果将函数的实现放在cpp文件中，并且标记为inline,
// 那么该函数对其他编译单元不可见（类似static的效果），
// 也就是其他cpp文件不能链接该函数库，这就是
// undefined reference to

inline void Foobar::foo() {
    std::cout << "foo()" << std::endl;
}
