#pragma once

#include <atomic>
#include <memory>
#include <assert.h> 
#include <iostream> 
#include "base/comlog_sink.h"

namespace rcu {

template<typename T>
class LockFreeQueue {
public:
    struct Node;
    struct CountedNodePtr {
        int external_count;
        Node* ptr;
    };
    struct NodeCounter {
        unsigned internal_count:30;
        unsigned external_counters:2;
    };
    struct Node {
        std::atomic<T*> data;
        std::atomic<NodeCounter> count;
        std::atomic<CountedNodePtr> next;

        Node() {
            NodeCounter new_count;
            new_count.internal_count = 0;
            new_count.external_counters = 2;
            count.store(new_count);

            CountedNodePtr new_next;
            new_next.ptr = nullptr;
            new_next.external_count = 0;
            next.store(new_next);
        }

        void release_ref() {
            NodeCounter old_counter = count.load(std::memory_order_relaxed);
            NodeCounter new_counter;
            do {
                new_counter = old_counter;
                --new_counter.internal_count;
            } while (!count.compare_exchange_strong(old_counter, new_counter, 
                                    std::memory_order_acquire, 
                                    std::memory_order_relaxed));

            if (new_counter.internal_count == 0 && new_counter.external_counters == 0) {
                delete this;
            }
        }
    };
public:
    LockFreeQueue() {
        auto* dummy = new Node;
        CountedNodePtr tmp;
        tmp.ptr = dummy;
        head = tmp;
        tail = head.load();
    }
    LockFreeQueue(LockFreeQueue const&) = delete;             // Copy construct
    LockFreeQueue(LockFreeQueue&&) = delete;                  // Move construct
    LockFreeQueue& operator=(LockFreeQueue const&) = delete;  // Copy assign
    LockFreeQueue& operator=(LockFreeQueue &&) = delete;      // Move assign

    void set_new_tail(CountedNodePtr& old_tail, CountedNodePtr const &new_tail);
    Node* pop_head();
    void push(T x);
    std::unique_ptr<T> pop();
    int size();
    bool empty();
private:
    static void increase_external_count(std::atomic<CountedNodePtr>& counter, CountedNodePtr& old_counter) {
        CountedNodePtr new_counter;
        do {
            new_counter = old_counter;
            ++new_counter.external_count;
        } while (!counter.compare_exchange_strong(old_counter, new_counter, 
                                    std::memory_order_acquire, 
                                    std::memory_order_relaxed));

        old_counter.external_count = new_counter.external_count;
    }

    static void free_external_counter(CountedNodePtr& old_node_ptr) {
        Node* ptr = old_node_ptr.ptr;
        int count_increase = old_node_ptr.external_count - 2;
        NodeCounter old_counter = ptr->count.load(std::memory_order_relaxed);

        NodeCounter new_counter;
        do {
            new_counter = old_counter;
            --new_counter.external_counters;
            new_counter.internal_count += count_increase;
        } while (!ptr->count.compare_exchange_strong(old_counter, new_counter,
                                        std::memory_order_acquire, std::memory_order_relaxed));

        if (new_counter.internal_count == 0 && new_counter.external_counters == 0) {
            delete ptr;
        }
    }
private:
    std::atomic<CountedNodePtr> head;
    std::atomic<CountedNodePtr> tail;
    std::atomic<int> _size {0};
};

}

#include "lock_free_queue.hpp"
