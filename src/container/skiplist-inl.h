#pragma once

#include <algorithm>
#include <atomic>
#include <limits>
#include <iostream>
#include <memory>
#include <type_traits>
#include <assert.h>

namespace rcu::detail {

template<typename T>
class SkipListNode {
public:
    using ValueType = T;

    template<typename NodeAlloc, typename U>
    static SkipListNode* create(NodeAlloc& alloc, uint8_t height, U&& data) {
        size_t bytes = sizeof(SkipListNode) + height * sizeof(SkipListNode*);
        auto* node = (SkipListNode*)alloc.allocate(bytes);
        new(node) SkipListNode(height, std::forward<U>(data));
        return node;
    }

    template<typename NodeAlloc>
    static void destroy(NodeAlloc& alloc, SkipListNode* node) {
        size_t bytes = sizeof(SkipListNode) + node->height_ * sizeof(SkipListNode*);
        node->~SkipListNode();
        alloc.deallocate((uint8_t*)node, bytes);        
    }    

    template<typename U>
    SkipListNode(uint8_t height, U&& data) 
                : height_(height)
                , data_(std::forward<U>(data)){
        for (uint8_t i = 0; i < height; ++i) {
            skip_[i] = nullptr;
        }
    }

    ~SkipListNode() {
    }

    uint8_t height() const { return height_; }
    const ValueType& data() const { return data_; }
    SkipListNode* skip(int i) const { 
        assert(i < height_);
        return skip_[i]; 
    }
    void set_skip(int i, SkipListNode* node) {
        assert(i < height_);
        skip_[i] = node; 
    }
private:
    uint8_t height_{0};
    ValueType data_;
    SkipListNode* skip_[0];
};

} // namespace
