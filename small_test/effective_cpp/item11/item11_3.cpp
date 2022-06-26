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

    void swap(Widget& rhs) {
        std::swap(rhs.bitmap, this->bitmap);
    }

    Widget& operator=(const Widget& rhs) {
        // 使用 copy and swap技术
        Widget tmp(rhs);
        swap(tmp);
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
