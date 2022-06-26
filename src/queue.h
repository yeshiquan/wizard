#pragma once

#include <atomic>
#include <memory>
#include <assert.h> 
#include <iostream> 
#include "base/comlog_sink.h"

namespace rcu {

template<typename T>
class SPSCQueue {
public:
    struct Node {
        std::shared_ptr<T> data ;
        Node* next {nullptr};
        Node() {
            next = nullptr;
        }
    };
public:
    SPSCQueue() {
        Node* dummy = new Node();
        head.store(dummy);
        tail.store(dummy);
    }
    SPSCQueue(SPSCQueue const&) = delete;             // Copy construct
    SPSCQueue(SPSCQueue&&) = delete;                  // Move construct
    SPSCQueue& operator=(SPSCQueue const&) = delete;  // Copy assign
    SPSCQueue& operator=(SPSCQueue &&) = delete;      // Move assign

    Node* pop_head();
    void push(T x);
    std::shared_ptr<T> pop();
    int size();
    bool empty();
private:
    std::atomic<Node*> head {nullptr};
    std::atomic<Node*> tail {nullptr};
    std::atomic<int> _size {0};
};

}

#include "queue.hpp"
