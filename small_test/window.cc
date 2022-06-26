#include <iostream>
#include <type_traits>
#include <algorithm>
#include <cstring>
#include <vector>
#include <functional>
#include <assert.h>

#include <sys/time.h>
#include <chrono>
#include <thread>

#define DCHECK_IS_ON 
#include "check.h"

namespace rcu {

template<typename T, bool Overwrite = true>
class RingBuffer;

template<bool v>
using bool_constant = std::integral_constant<bool, v>;

namespace detail {

template<typename T, bool C, bool Overwrite>
class RingBufferIterator {
    using buffer_t = typename std::conditional<!C, RingBuffer<T, Overwrite>*, RingBuffer<T, Overwrite> const*>::type;
public:
    friend class RingBuffer;
    using self_type = RingBufferIterator<T, C, Overwrite>;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    RingBufferIterator() noexcept = default;
    RingBufferIterator(buffer_t source, int index, size_type plus_count) noexcept
            : source_{source},
              index_{index},
              plus_count_{plus_count}
    {
    }
    RingBufferIterator(RingBufferIterator const& ) noexcept = default;
    RingBufferIterator& operator=(RingBufferIterator const& ) noexcept = default;

    template<bool Z = C, typename std::enable_if<(!Z), int>::type* = nullptr>
    T& operator*() noexcept {
        return (*source_)[index_];
    }
    template<bool Z = C, typename std::enable_if<(Z), int>::type* = nullptr>
    const T* operator*() const noexcept {
        return &(*source_)[index_];
    }
    template<bool Z = C, typename std::enable_if<(!Z), int>::type* = nullptr>
    T& operator->() noexcept {
        return &((*source_)[index_]);
    }
    template<bool Z = C, typename std::enable_if<(Z), int>::type* = nullptr>
    const T& operator->() const noexcept {
        return &((*source_)[index_]);
    }
    // ++iter
    self_type& operator++() noexcept {
        index_ = (index_ + 1) % source_->capacity();
        ++plus_count_;
        return *this;
    }
    // iter++
    self_type operator++(int) noexcept {
        auto result = *this;
        ++*this;
        return result;
    }
    size_type index() const noexcept {
        return index_;
    }
    size_type plus_count() const noexcept {
        return plus_count_;
    }
    ~RingBufferIterator() = default;
private:
    buffer_t source_{};
    int index_{};
    size_type plus_count_{}; // 迭代器++操作的次数
};

template<typename T, bool C, bool Overwrite>
bool operator==(RingBufferIterator<T,C,Overwrite> const& l,
                RingBufferIterator<T,C,Overwrite> const& r) noexcept {
    return l.plus_count() == r.plus_count();
}

template<typename T, bool C, bool Overwrite>
bool operator!=(RingBufferIterator<T,C,Overwrite> const& l,
                RingBufferIterator<T,C,Overwrite> const& r) noexcept {
    return l.plus_count() != r.plus_count();
}

} // detail namespace

template<typename T, bool Overwrite>
class RingBuffer {
    using self_type = RingBuffer<T, Overwrite>;
public:
    using size_type = size_t;
    using iterator = detail::RingBufferIterator<T, false, Overwrite>;
    using const_iterator = detail::RingBufferIterator<T, true, Overwrite>;

    explicit RingBuffer(size_t capacity) noexcept {
        elements_ = new T[capacity];
        capacity_ = capacity;
    }
    RingBuffer(RingBuffer const& rhs) = delete;
    RingBuffer& operator=(RingBuffer const& rhs) = delete;

    template<class... Args>
    void emplace_back(Args&&... args) {
        _emplace_back(std::forward<Args>(args)...);
    }

    void pop_front() noexcept{
        if(empty()) {
            return;
        }

        _destroy(head_, bool_constant<std::is_trivially_destructible<T>::value>{});

        --size_;
        head_ = (head_ + 1 ) % capacity_;
    }

    T& back() noexcept { 
        assert(size_ > 0);
        return reinterpret_cast<T&>(elements_[tail_]); 
    }
    const T& back() const noexcept {
        assert(size_ > 0);
        return const_cast<self_type*>(back)->back();
    }
    T& front() noexcept { 
        assert(size_ > 0);
        return reinterpret_cast<T&>(elements_[head_]); 
    }
    const T& front() const noexcept {
        assert(size_ > 0);
        return const_cast<self_type*>(this)->front();
    }

    T& operator[](size_type index) noexcept {
        return reinterpret_cast<T&>(elements_[index]);
    }
    const T& operator[](size_type index) const noexcept {
        return const_cast<self_type *>(this)->operator[](index);
    }

    T& at(size_type index) noexcept {
        assert(index < size_);
        return reinterpret_cast<T&>(elements_[(index+head_) % capacity_]);
    }

    const T& at(size_type index) const noexcept {
        assert(index < size_);
        return reinterpret_cast<const T&>(elements_[(index+head_) % capacity_]);
    }

    iterator begin() noexcept { 
        return iterator{this, head_, 0};
    }
    iterator end() noexcept { 
        return iterator{this, tail_, size_};
    }
    const_iterator cbegin() const noexcept { 
        return const_iterator{this, head_, 0};
    }
    const_iterator cend() const noexcept {
        return const_iterator{this, tail_, size_};
    }
    size_type size () {
        return size_;
    }
    bool empty() const noexcept {
        return size_ == 0; 
    }
    bool full() const noexcept {
        return size_ == capacity_; 
    }
    size_type capacity() const noexcept { 
        return capacity_; 
    }
    void clear() noexcept{
        _destroy_all(bool_constant<std::is_trivially_destructible<T>::value>{});
    }
    ~RingBuffer() {
        clear();
    };
private:
    // 平凡数据不需要析构
    void _destroy_all(std::true_type) { 
    }
    // 对象数据需要调用析构函数
    void _destroy_all(std::false_type) {
        while(!empty()) {
            _destroy(head_, bool_constant<std::is_trivially_destructible<T>::value>{});
            head_ = (head_ + 1 ) % capacity_;
            --size_;
        }
    }

    template<class... Args>
    void _emplace_back(Args&&... args) {
        if(full()) {
            if (Overwrite) {
                pop_front();
            } else {
                return;
            }
        }
        _emplace_back_impl(std::forward<Args>(args)...);
    }

    template<class... Args>
    void _emplace_back_impl(Args&&... args) {
        tail_ = (tail_ + 1 ) % capacity_;
        new(elements_ + tail_ ) T{std::forward<Args>(args)...};
        ++size_;
    }

    // 平凡数据不需要析构
    void _destroy(size_type index, std::true_type) noexcept {
        (void)index; // 消除编译报警
    }

    // 对象数据需要调用析构函数
    void _destroy(size_type index, std::false_type) noexcept {
        reinterpret_cast<T*>(&elements_[index])->~T();
    }

private:
    T* elements_;
    int tail_ = {-1};
    int head_ = {0};
    size_type size_ = {0};
    size_type capacity_;
};

template<typename ValueType>
struct ItemHolder {
    int64_t timestamp;
    ValueType value;

    ItemHolder() = default;

    // 原地构造
    template<class... Args>
    ItemHolder(int64_t ts, Args&&... args) : 
            timestamp(ts), 
            value(std::forward<Args>(args)...) {}
};

// 基于时间的窗口
template<typename T>
class TimeWindow {
public:
    explicit TimeWindow(size_t window_gap, size_t capacity) noexcept {
        _buffer = std::unique_ptr<RingBuffer<ItemHolder<T>>>(new RingBuffer<ItemHolder<T>>(capacity));
        _window_gap = window_gap;
    }
    TimeWindow(TimeWindow const&) = delete;             // Copy construct
    TimeWindow(TimeWindow&&) = delete;                  // Move construct
    TimeWindow& operator=(TimeWindow const&) = delete;  // Copy assign
    TimeWindow& operator=(TimeWindow &&) = delete;      // Move assign

    template<class... Args>
    void emplace_back(int64_t ts, Args&&... args) noexcept {
        _buffer->emplace_back(ts, std::forward<Args>(args)...);
        _end_timestamp = ts;
        remove_expired_data();
    }

    void remove_expired_data() noexcept {
        while (!_buffer->empty()) {
            if ((_end_timestamp - _buffer->front().timestamp) > (int64_t)_window_gap) {
                _buffer->pop_front();
            } else {
                break;
            }
        }
    }

    void for_each(std::function<void(const ItemHolder<T>&)> callback) {
        for (const auto& item : *_buffer) {
            callback(item);
        }
    }

    T& at(int i) noexcept { return _buffer->at(i).value; }
    const T& at(int i) const noexcept { return _buffer->at(i).value; }
    T& front() noexcept { return _buffer->front().value; }
    T& back() noexcept { return _buffer->back().value; }
    const T& front() const noexcept { return _buffer->front().value; }
    const T& back() const noexcept { return _buffer->back().value; }
    size_t size() noexcept { return _buffer->size(); }

    int64_t get_latest_timestamp() noexcept {
        return _end_timestamp;
    }

    RingBuffer<ItemHolder<T>>& get_data() noexcept {
        return *_buffer;
    }

    void clear() noexcept { _buffer->clear(); }
    ~TimeWindow() noexcept { clear(); }

private:
    std::unique_ptr<RingBuffer<ItemHolder<T>>> _buffer = {nullptr};
    int64_t _end_timestamp = {0};
    size_t _window_gap = {0};
};

// 固定长度的窗口
template<typename T>
class FixedWindow {
public:
    explicit FixedWindow(size_t window_size) noexcept {
        _window_size = window_size;
        _buffer = std::unique_ptr<RingBuffer<T>>(new RingBuffer<T>(_window_size));
    }
    FixedWindow(FixedWindow const&) = delete;             // Copy construct
    FixedWindow(FixedWindow&&) = delete;                  // Move construct
    FixedWindow& operator=(FixedWindow const&) = delete;  // Copy assign
    FixedWindow& operator=(FixedWindow &&) = delete;      // Move assign

    template<class... Args>
    void emplace_back(Args&&... args) noexcept {
        _buffer->emplace_back(std::forward<Args>(args)...);
        // remove_overflow_data(); no need remove because RingBuffer is Overwriteable
    }

    void remove_overflow_data() noexcept {
        while (_buffer->size() > _window_size) {
            _buffer->pop_front();
        }
    }

    void for_each(std::function<void(const T&)> callback) {
        for (const auto& item : *_buffer) {
            callback(item);
        }
    }

    void resize(size_t new_window_size) {
        auto* new_buffer = new RingBuffer<T>(new_window_size);
        for (auto& item : *_buffer) {
            new_buffer->emplace_back(item);
        }
        _buffer.reset(new_buffer);
        _window_size = new_window_size;
    }

    RingBuffer<T>& get_data() noexcept {
        return *_buffer;
    }

    T& at(int i) noexcept { return _buffer->at(i); }
    const T& at(int i) const noexcept { return _buffer->at(i); }
    T& front() noexcept { return _buffer->front(); }
    T& back() noexcept { return _buffer->back(); }
    const T& front() const noexcept { return _buffer->front(); }
    const T& back() const noexcept { return _buffer->back(); }
    size_t size() noexcept { return _buffer->size(); }

    void clear() noexcept { _buffer->clear(); }
    ~FixedWindow() noexcept { clear(); }
private:
    std::unique_ptr<RingBuffer<T>> _buffer = {nullptr};
    int64_t _end_timestamp = {0};
    size_t _window_size = {0};
};

} // rcu namespace


int64_t get_system_millsec() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    int64_t factor = 1;

    return (int64_t)((factor * tv.tv_sec * 1000) + (tv.tv_usec / 1000));
}

struct FrameData {
    float x;
    float y;
    std::string name = "";

    FrameData() {
        //std::cout << " [Construct 1] " << std::endl;
    }
    ~FrameData() {
        std::cout << " [Destruct] ";
    }
    FrameData(float x_, float y_, std::string name_) :
        x(x_), y(y_), name(name_) {
        std::cout << " [Construct 2] ";
    }
};


int main() {
    using namespace rcu;

    {   // Test RingBuffer
        std::cout << "Test RingBuffer ======================" << std::endl;
        RingBuffer<int> buffer3(3);
        buffer3.emplace_back(1);
        buffer3.emplace_back(2);
        buffer3.emplace_back(3);
        buffer3.emplace_back(4);

        for (auto& v : buffer3) {
            std::cout << v << std::endl;
        }

        auto iter = buffer3.cbegin();
        std::cout << "iter++ -> " << *(iter++) << std::endl;

        iter = buffer3.cbegin();
        std::cout << "++iter -> " << *(++iter) << std::endl;
    }

    {   // Test TimeWindow
        std::cout << "\nTest TimeWindow ======================" << std::endl;
        constexpr size_t WindowGap = 3000; // ms
        constexpr size_t Capacity = 50000;
        TimeWindow<FrameData> window(WindowGap, Capacity);

        auto print_window = [&window]() {
            std::cout << "window -> [";
            for (auto& item : window.get_data()) {
                std::cout << item.value.name << ", ";
            }
            std::cout << "]" << " size:" << window.size() << std::endl;
        };

        {   // 测试先用emplace_back构造空对象，再用back获取对象
            auto now = get_system_millsec();
            window.emplace_back(now);
            FrameData& new_frame_data = window.back();
            new_frame_data.x = 1.2f;
            new_frame_data.y = 1.3f;
            new_frame_data.name = "pp";
        }

        for (int i = 0; i < 10; ++i) {
            auto now = get_system_millsec();
            float x = i*1.0f;
            float y = i*2.0f;
            std::string name = std::to_string(i);
            window.emplace_back(now, x, y, name);            // 原地构造，没有移动和拷贝
            //window.emplace_back(now, FrameData(x, y, name)); // 使用移动构造，没有拷贝

            // 通过at获取第1个和最后1个元素
            std::cout << "first -> " << window.at(0).name << ", ";
            std::cout << "last -> " << window.at(window.size()-1).name << ", ";

            print_window();

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        std::cout << "for_each -> ";
        window.for_each([](const ItemHolder<FrameData>& item) {
            std::cout << item.value.name << ",";
        });
        std::cout << std::endl;
    }

    {   // Test FixedWindow
        std::cout << "\nTest FixedWindow ======================" << std::endl;
        constexpr size_t WindowSize = 5;
        FixedWindow<int> window(WindowSize);

        auto print_window = [&window](std::string label) {
            std::cout << label << " window -> [";
            for (auto& v : window.get_data()) {
                std::cout << v << ", ";
            }
            std::cout << "]" << " size:" << window.size() << std::endl;
        };

        for (int i = 0; i < 10; ++i) {
            int value = i;
            window.emplace_back(value);

            std::cout << "first -> " << window.at(0) << ", ";
            std::cout << "last -> " << window.at(window.size()-1) << ", ";
            
            print_window("");
        }

        std::cout << "for_each -> ";
        window.for_each([](const int& item) {
            std::cout << item << ",";
        });
        std::cout << std::endl;

        for (int i = 0; i < 10; i++) {
            int value = i;
            window.emplace_back(value);
        }
        print_window("before resize");
        window.resize(8);
        window.emplace_back(11);
        window.emplace_back(12);
        print_window("after resize");
    }

    return 0;
}
