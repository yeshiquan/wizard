#include <iostream>

template <typename T>
class Array {
public:
    class Iterator {
        friend class Array;
    public:
        Iterator() {}
        ~Iterator() {}

        void init(T* data, size_t cnt) {
            data_ = data;
            cnt_ = cnt;
            curr = data_;
        }

        const T& operator*() const {
            return *curr;
        }

        const T* operator->() const {
            return curr;
        }

        // 前置操作重载
        const Iterator& operator++() {
            ++idx;
            if (idx >= cnt_) {
                curr = nullptr;
            } else {
                curr = &data_[idx];
            }
            return *this;
        }

        // 后置操作重载
        Iterator operator++(int) {
            auto prev = *this;
            ++*this;
            return prev;
        }

        bool operator==(const Iterator& o) const {
            return curr == o.curr;
        }

        bool operator!=(const Iterator& o) const {
            return !(*this == o);
        }

        // Asign
        Iterator& operator=(const Iterator& o) {
            data_ = o.data_;
            idx = o.idx;
            cnt_ = o.cnt_;
            curr = o.curr;
            return *this;
        }

        // Copy Constructor
        Iterator(const Iterator& o) {
            data_ = o.data_;
            idx = o.idx;
            cnt_ = o.cnt_;
            curr = o.curr;
        }

        // Move Constructor
        Iterator(Iterator&& o) {
            data_ = o.data_;
            idx = o.idx;
            cnt_ = o.cnt_;
            curr = o.curr;
        }
    private:
        T* data_ = {nullptr};
        T* curr = {nullptr};
        int idx = {0};
        size_t cnt_ = {0};
    };

    Iterator cbegin() {
        Iterator iter;
        iter.init(raw_data, cnt);
        return iter;
    }

    Iterator cend() {
        return Iterator();
    }

    Array(size_t n) {
        raw_data = new T[n];
        cnt = n;
    }

    void set(int i, const T& e) {
        raw_data[i] = e;
    }

    Iterator get(int i) {
        Iterator iter;
        iter.init(raw_data, cnt);
        if (i >= cnt) {
            iter.curr = nullptr;
        } else {
            iter.curr = &raw_data[i];
        }
        return iter;
    }
private:
    T* raw_data = {nullptr};
    size_t cnt = {0};
};

int main() {
    Array<int> arr(100);
    for (int i = 0; i < 100; ++i) {
        arr.set(i, i*2);
    }

    for (auto iter = arr.cbegin(); iter != arr.cend(); iter++) {
        std::cout << *iter << std::endl;
    }

    auto iter = arr.get(49);
    std::cout << "data -> " << *iter << std::endl;

    return 0;
}
