#include <iostream>
#include <mutex>
#include <cassert>
#include <memory>

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

struct PmImpl {
    std::shared_ptr<Image> bg_image;
    int image_changes = 0;
};

class PrettyMenu {
public:
    std::string default_image_src = "/home/work/menu.jpg";

    PrettyMenu() {
        PmImpl* new_impl = new PmImpl;
        new_impl->bg_image = std::make_shared<Image>(default_image_src, false);
        impl.reset(new_impl);
    }

    // 使用copy and swap技术, 实现了异常安全
    void change_background(const std::string& img_src) {
        Locker locker(&mutex);

        // Step1: Copy
        std::shared_ptr<PmImpl> new_impl(new PmImpl(*impl));
        // Step2: Modify
        new_impl->bg_image.reset(new Image(img_src, true));
        new_impl->image_changes++;
        // Step3: Swap
        swap(impl, new_impl);
    }

    Image& get_image() {
        return *(impl->bg_image);
    }
private:
    std::shared_ptr<PmImpl> impl;
    std::mutex mutex;
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
    assert(width == 0);
    return 0;
}
