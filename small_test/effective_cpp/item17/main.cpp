#include <memory>
#include <iostream>
#include <stdexcept>

class Widget {
public:
    Widget() {
        std::cout << "Widget()" << std::endl;
    }
    ~Widget() {
        std::cout << "~Widget()" << std::endl;
    }
};

int get_priority() {
    throw std::invalid_argument( "unknow number" );
    return 0;
}

void processWidget(std::shared_ptr<Widget> p_widget, int priority) {
    std::cout << "processWidget()" << std::endl;
}

int main() {
    try {
        // 实际执行顺序有可能是
        // new Widget
        // get_priority() 抛出异常，后面的析构不会发生，导致内存泄漏
        // delete Widget
        processWidget(std::shared_ptr<Widget>(new Widget), get_priority());

        // 更好的写法 
        // Store newed objects in smart pointers in standalone statements.
        std::shared_ptr<Widget> p_widget(new Widget);
        processWidget(p_widget, get_priority());
    } catch (std::exception& e) {
        std::cout << "got exception:" << e.what() << std::endl;
    }
    return 0;
}


