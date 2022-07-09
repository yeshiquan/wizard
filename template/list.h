#pragma once

#include <algorithm>
#include <atomic>
#include <limits>
#include <iostream>
#include <memory>
#include <type_traits>

#include "list-detail.h"

namespace rcu {

template<typename T, 
         typename Comp = std::less<T>,
         typename NodeAlloc = std::allocator<uint8_t>, 
         uint32_t MAX_HEIGHT = 24>
class SkipList {
    using SkipListType = SkipList<T, Comp, NodeAlloc, MAX_HEIGHT>;
public:
    using NodeType = detail::SkipListNode<T> ;
    using ValueType = T;

    SkipList(uint8_t height) {
        head_ = NodeType::create(alloc_, height, ValueType());
    }
    ~SkipList() {}

    void insert(const ValueType& data) {

    }
    void remove(const ValueType& data) {

    }
    void find(const ValueType& data) {

    }

    void print() {

    }
private:
    NodeType* head_{nullptr};
    size_t size_{0};
    NodeAlloc alloc_;
};

} // namespace
