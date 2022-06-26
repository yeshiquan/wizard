#include <iostream>

class Bitmap {
public:
    Bitmap() {
        data = new int[24];
    }
    void print() {
        for (int i = 0; i < 24; ++i) {
            std::cout << data[i] << ",";
        }
        std::cout << std::endl;
    }
public:
    int* data = nullptr;
};

class Widget {
public:
    Widget(Bitmap* bitmap_) : bitmap(bitmap_) {
    }

    Widget& operator=(const Widget& rhs) {
        // 使用精心安排的语句实现异常安全
        // 即使new Bitmap抛出异常，也没有问题
        Bitmap* old = bitmap;
        bitmap = new Bitmap(*rhs.bitmap);
        delete old;
        return *this;
    }

    void print() {
        bitmap->print();
    }
public:
    Bitmap* bitmap = nullptr;
};

int main() {
    Bitmap* bitmap = new Bitmap;
    Widget w(bitmap);
    w = w;
    w.print();
    return 0;
}
