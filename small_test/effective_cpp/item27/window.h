#include <iostream>

class Window {
public:
    virtual void onResize() {
        std::cout << "Window::onResize() this:" << this <<std::endl;
        width = 55;
    }
public:
    int width = 0;
};

class SpecialWindow: public Window {
public:
    virtual void onResize() {
        // 错误的写法，static_cast会创建一个拷贝对象，
        // 在拷贝对象上调用onResize()不会影响this的width
        // 输出 width:0 height:66
        static_cast<Window>(*this).onResize();

        // 正确的写法：
        // Window::onResize();
        // 输出 width:55 height:66
        height = 66;
        std::cout << "SpecialWindow::onResize() this:" << this <<std::endl;
    }
public:
    int height = 0;
};



