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
        // 如果不加这一行，会core dump
        // 使用了被删除的bitmap
        if (this == &rhs) return *this;
        delete bitmap;
        bitmap = new Bitmap(*rhs.bitmap);
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

