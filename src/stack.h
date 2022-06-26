#pragma once

#include <atomic>
#include <memory>
#include <assert.h> 
#include <iostream> 
#include "base/comlog_sink.h"

namespace rcu {

template<typename T>
class LockFreeStack {
private:
    struct Node;
    struct CountedNodePtr {
        int32_t external_count;
        Node* ptr;
    };
    struct Node {
        std::shared_ptr<T> data;
        std::atomic<int> internal_count;
        CountedNodePtr next;
        Node(T const& data_):
            data(std::make_shared<T>(data_)),
            internal_count(0)
        {}
    };
public:
    LockFreeStack(LockFreeStack const&) = delete;             // Copy construct
    LockFreeStack(LockFreeStack&&) = delete;                  // Move construct
    LockFreeStack& operator=(LockFreeStack const&) = delete;  // Copy assign
    LockFreeStack& operator=(LockFreeStack &&) = delete;      // Move assign

    LockFreeStack() {
    }

    void increase_head_count(CountedNodePtr& old_head) {
        CountedNodePtr new_head;
        do {
            new_head = old_head;
   e:
    std::atomic<CountedNodePtr> head;
};

}
