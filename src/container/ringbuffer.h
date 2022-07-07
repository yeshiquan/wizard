#pragma once

#include <type_traits>
#include <functional>

#include "check.h"

namespace base {
namespace container {

template<typename T, bool Overwrite = true>
class RingBuffer;

template<bool v>
using bool_constant = std::integral_constant<bool, v>;
using Allocator = std::allocator<uint8_t>;

namespace detail {


template<typename T, bool C, bool R, bool Overwrite>
class RingBufferIterator {
    using buffer_t = typename std::conditional<!C, RingBuffer<T, Overwrite>*, RingBuffer<T, Overwrite> const*>::type;
public:
    friend class RingBuffer;
    using self_type = RingBufferIterator<T, C, R, Overwrite>;
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
    const T& operator*() const noexcept {
        return (*source_)[index_];
    }
    template<bool Z = C, typename std::enable_if<(!Z), int>::type* = nullptr>
    T* operator->() noexcept {
        return &((*source_)[index_]);
    }
    template<bool Z = C, typename std::enable_if<(Z), int>::type* = nullptr>
    const T* operator->() const noexcept {
        return &((*source_)[index_]);
    }
    // ++iter
    self_type& operator++() noexcept {
        auto c = source_->capacity();
        if (R) {
            index_ = (index_ + c - 1) %  c;
        } else {
            index_ = (index_ + 1) % c;
        }
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

template<typename T, bool C, bool R, bool Overwrite>
bool operator==(RingBufferIterator<T,C,R,Overwrite> const& l,
                RingBufferIterator<T,C,R,Overwrite> const& r) noexcept {
    return l.plus_count() == r.plus_count();
}

template<typename T, bool C, bool R, bool Overwrite>
bool operator!=(RingBufferIterator<T,C,R,Overwrite> const& l,
                RingBufferIterator<T,C,R,Overwrite> const& r) noexcept {
    return l.plus_count() != r.plus_count();
}

} // detail namespace

template<typename T, bool Overwrite>
class RingBuffer {
    using self_type = RingBuffer<T, Overwrite>;
public:
    using size_type = size_t;
    using iterator = detail::RingBufferIterator<T, false, false, Overwrite>;
    using reverse_iterator = detail::RingBufferIterator<T, false, true, Overwrite>;
    using const_iterator = detail::RingBufferIterator<T, true, false, Overwrite>;
    using reverse_const_iterator = detail::RingBufferIterator<T, true, true, Overwrite>;

    explicit RingBuffer(size_t capacity) noexcept {
        capacity_ = capacity;
        elements_ = (T*)Allocator().allocate(sizeof(T) * capacity_);
    }
    
    RingBuffer(RingBuffer const& rhs) {
        capacity_ = rhs.capacity_;
        tail_ = rhs.tail_;
        head_ = rhs.head_;
        size_ = rhs.size_;
        capacity_ = rhs.capacity_;
        elements_ = (T*)Allocator().allocate(sizeof(T) * capacity_);
        construct_all();
        // std::copy(rhs.elements_, rhs.elements_ + capacity_, elements_); //复现用这个
        std::copy(rhs.elements_, rhs.elements_ + size_, elements_);
    }

    RingBuffer& operator=(RingBuffer const& rhs) {
        RingBuffer tmp(rhs);
        swap(tmp);
        return *this;
    }

    void swap(RingBuffer& rhs) {
        std::swap(capacity_, rhs.capacity_);
        std::swap(tail_, rhs.tail_);
        std::swap(head_, rhs.head_);
        std::swap(size_, rhs.size_);
        std::swap(elements_, rhs.elements_);
    }

    void construct_all() {
        for (size_type i = 0; i < capacity_; ++i) {
            _construct(i, bool_constant<std::is_trivially_destructible<T>::value>{});
        }
    }

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
        DCHECK(size_ > 0);
        return reinterpret_cast<T&>(elements_[tail_]); 
    }
    const T& back() const noexcept {
        DCHECK(size_ > 0);
        return const_cast<self_type*>(back)->back();
    }
    T& front() noexcept { 
        DCHECK(size_ > 0);
        return reinterpret_cast<T&>(elements_[head_]); 
    }
    const T& front() const noexcept {
        DCHECK(size_ > 0);
        return const_cast<self_type*>(this)->front();
    }

    T& operator[](size_type index) noexcept {
        return reinterpret_cast<T&>(elements_[index]);
    }
    const T& operator[](size_type index) const noexcept {
        return const_cast<self_type *>(this)->operator[](index);
    }

    T& at(size_type index) noexcept {
        DCHECK(index < size_);
        return reinterpret_cast<T&>(elements_[(index+head_) % capacity_]);
    }

    const T& at(size_type index) const noexcept {
        DCHECK(index < size_);
        return reinterpret_cast<const T&>(elements_[(index+head_) % capacity_]);
    }

    iterator begin() noexcept { 
        return iterator{this, head_, 0};
    }
    iterator end() noexcept { 
        return iterator{this, tail_, size_};
    }
    reverse_iterator rbegin() noexcept { 
        return reverse_iterator{this, tail_, 0};
    }
    reverse_iterator rend() noexcept {
        return reverse_iterator{this, head_, size_};
    }
    const_iterator cbegin() const noexcept { 
        return const_iterator{this, head_, 0};
    }
    const_iterator cend() const noexcept {
        return const_iterator{this, tail_, size_};
    }
    reverse_const_iterator crbegin() const noexcept { 
        return reverse_const_iterator{this, tail_, 0};
    }
    reverse_const_iterator crend() const noexcept {
        return reverse_const_iterator{this, head_, size_};
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
        if (elements_ != nullptr) {
            Allocator().deallocate((uint8_t*)elements_, sizeof(T) * capacity_);
            elements_ = nullptr;
        }
    };
private:
    // 平凡数据不需要析构
    void _destroy_all(std::true_type) { 
        head_ = 0;
        tail_ = -1;
        size_ = 0;
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

    void _construct(size_type index, std::true_type) noexcept {
        (void)index; // 消除编译报警
    }

    void _construct(size_type index, std::false_type) noexcept {
        new(elements_ + index ) T();
    }
private:
    T* elements_ = nullptr;
    int tail_ = {-1};
    int head_ = {0};
    size_type size_ = {0};
    size_type capacity_ = {0};
};

} // window
} // base
