#include <iostream>
#include <mutex>
#include <cassert>

class Image {
public:
    Image(std::string img_src, bool is_throw) {
        if (is_throw) {
            throw std::invalid_argument( "unknow number" );
        }
    }
public:
    size_t width = 0;
    size_t height = 0;
private:
    char data[256];
};

class Locker {
public:
    Locker(std::mutex* m) {
        mutex = m;
        mutex->lock();
        std::cout << "lock()" << std::endl;
    }
    ~Locker() {
        mutex->unlock();
        mutex = nullptr;
        std::cout << "unlock()" << std::endl;
    }
private:
    std::mutex* mutex = nullptr;
};

class PrettyMenu {
public:
    std::string default_image_src = "/home/work/menu.jpg";

    PrettyMenu() {
        bg_image = new Image(default_image_src, false);
    }

    // 使用资源管理器, 仍然不是异常安全
    // bg_image会被破坏
    void change_background(const std::string& img_src) {
        Locker locker(&mutex);
        delete bg_image;
        image_changes++;
        bg_image = new Image(img_src, true);
    }
    Image& get_image() {
        return *bg_image;
    }
private:
    std::mutex mutex;
    Image* bg_image = nullptr;
    int image_changes = 0;
};

int main() {
    PrettyMenu menu;
    try {
        menu.change_background("/home/work/new.jpg");
    } catch(std::exception& e) {
        std::cout << "exception -> " << e.what() << std::endl;
    }
    size_t width = menu.get_image().width;
    std::cout << "width -> " << width << std::endl;
    //assert(width == 0);
    return 0;
}
