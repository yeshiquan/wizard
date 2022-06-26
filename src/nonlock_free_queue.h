#pragma once

#include <atomic>
#include <memory>
#include <assert.h> 
#include <iostream> 
#include "base/comlog_sink.h"

namespace rcu {

template<typename T>
class LockFreeQueue {
private:
    struct Node;
    struct CountedNodePtr {
        int external_count;
        Node* ptr;
    };
    std::atomic<CountedNodePtr> head;
    std::atomic<CountedNodePtr> tail;
    struct NodeCounter {
        unsigned internal_count:30;
        unsigned external_counters:2;
    };
    struct Node {
        std::atomic<T*> data;
        std::atomic<NodeCounter> count;
        CountedNodePtr next;

        Node() {
            NodeCounter new_count;
            new_count.internal_count = 0;
            new_count.external_counters = 2;
            count.store(new_count);
            next.ptr = nullptr;
            next.external_count = 0;
            data = nullptr;
        }

        void release_ref() {
            LOG(NOTICE) << "release_ref";
            NodeCounter old_counter = count.load(std::memory_order_relaxed);
            NodeCounter new_counter;
            do {
                new_counter = old_counter;
                --new_counter.internal_count;
            }
            while(!count.compare_exchange_strong(
                      old_counter,new_counter,
                      std::memory_order_acquire,std::memory_order_relaxed));
            if(!new_counter.internal_count &&
               !new_counter.external_counters) {
                delete this;
            }
        }

    };
public:
    LockFreeQueue() {
        CountedNodePtr node_ptr;
        node_ptr.external_count = 0;
        node_ptr.ptr = new Node;
        head.store(node_ptr);
        tail.store(node_ptr);
    }
    static void increase_external_count(
        std::atomic<CountedNodePtr>& counter,
        CountedNodePtr& old_counter) {
        CountedNodePtr new_counter;
        LOG(NOTICE) << "increase_external_count";
        do {
            new_counter = old_counter;
            ++new_counter.external_count;
        } while(!counter.compare_exchange_strong(
                  old_counter,new_counter,
                  std::memory_order_acquire,std::memory_order_relaxed));
        old_counter.external_count = new_counter.external_count;
    }
    static void free_external_counter(CountedNodePtr &old_node_ptr) {
        LOG(NOTICE) << "free_external_counter";
        Node* const ptr = old_node_ptr.ptr;
        int const count_increase = old_node_ptr.external_count-2;
        NodeCounter old_counter = ptr->count.load(std::memory_order_relaxed);
        NodeCounter new_counter;
        do {
            new_counter=old_counter;
            --new_counter.external_counters;
            new_counter.internal_count += count_increase;
        } while(!ptr->count.compare_exchange_strong(
                  old_counter,new_counter,
                  std::memory_order_acquire,std::memory_order_relaxed));
        if(!new_counter.internal_count &&
           !new_counter.external_counters) {
            delete ptr;
        }
    }
    void push(T new_value) {
        std::unique_ptr<T> new_data(new T(new_value));
        CountedNodePtr new_next;
        new_next.ptr = new Node;
        new_next.external_count = 1;
        CountedNodePtr old_tail=tail.load();
        for(;;) {
            increase_external_count(tail,old_tail);
            T* old_data = nullptr;
            LOG(NOTICE) << "old_tail.ptr->data: " << old_tail.ptr->data  ;
            LOG(NOTICE) << "old_data: nullptr" ;
            LOG(NOTICE) << "new_data: " << *new_data ;
            Node* dummy_node = old_tail.ptr;
            // There’s still a performance issue. 
            // Once one thread has started a push() operation by successfully completing the 
            // compare_exchange_strong() on old_tail.ptr->data no other thread
            // can perform a push() operation. 
            // Any thread that tries will see the new value rather than nullptr, 
            // which will cause the compare_exchange_strong() call to fail and
            // make that thread loop again. This is a busy wait, which consumes CPU cycles without achieving anything.
            // Consequently, this is effectively a lock. The first push() call
            // blocks other threads until it has completed, so this code is no longer lock-free. 
            // Not only that, but whereas the operating system can give priority to the thread that holds
            // the lock on a mutex if there are blocked threads, it can’t do so in this case, so the
            // blocked threads will waste CPU cycles until the first thread is done. This calls for the
            // next trick from the lock-free bag of tricks: the waiting thread can help the thread
            // that’s doing the push().
            if(dummy_node->data.compare_exchange_strong(old_data, new_data.get())) {
                dummy_node->next = new_next;
                old_tail = tail.exchange(new_next);
                free_external_counter(old_tail);
                new_data.release();
                break;
            }
            old_tail.ptr->release_ref();
        }
    }
    std::unique_ptr<T> pop() {
        CountedNodePtr old_head=head.load(std::memory_order_relaxed);
        for(;;) {
            increase_external_count(head,old_head);
            Node* const ptr = old_head.ptr;
            if(ptr == tail.load().ptr) {
                ptr->release_ref();
                return std::unique_ptr<T>();
            }
            assert(ptr != nullptr);
            if(head.compare_exchange_strong(old_head, ptr->next)) {
                T* const res = ptr->data.exchange(nullptr);
                free_external_counter(old_head);
                return std::unique_ptr<T>(res);
            }
            ptr->release_ref();
        }
    }
};

}

