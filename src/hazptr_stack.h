#pragma once

#include <atomic>
#include <memory>
#include <assert.h> 
#include <iostream> 
#include "base/comlog_sink.h"

namespace rcu {

template<typename T>
class HazptrStack {
private:
    struct Node {
        std::shared_ptr<T> data;
        Node* next;
        Node(T const& data_):
            data(std::make_shared<T>(data_))
        {}
    };
public:
    HazptrStack(HazptrStack const&) = delete;             // Copy construct
    HazptrStack(HazptrStack&&) = delete;                  // Move construct
    HazptrStack& operator=(HazptrStack const&) = delete;  // Copy assign
    HazptrStack& operator=(HazptrStack &&) = delete;      // Move assign

    HazptrStack() {
    }

    ~HazptrStack() {
    }

    void push(T const& data) {
        Node* node = new Node(data);
        node->next = _head.load();
        while(!_head.compare_exchange_weak(node->next, node, std::memory_order_seq_cst));
    }

    std::shared_ptr<T> pop() {
        Node* old_head = _head.load();
        while (old_head && !_head.compare_exchange_weak(old_head, old_head->next, std::memory_order_seq_cst));
        auto p_data = old_head ? old_head->data : std::shared_ptr<T>();
        delete old_head;
        return p_data;
    }

private:
    std::atomic<Node*> _head = {nullptr};
};

}
