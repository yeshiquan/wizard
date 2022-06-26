#pragma once

#include <mutex>
#include "my_hazptr.h"

namespace rcu {

/** Set implemented as an ordered singly-linked list.
 *
 *  A single writer thread may add or remove elements. Multiple reader
 *  threads may search the set concurrently with each other and with
 *  the writer's operations.
 */
template <typename T>
class SWMRListSet {
    class Node : public HazptrNode<Node> {
        friend SWMRListSet;
    public:
        Node(T data_, Node* next_, int seq_) :
            element(data_),
            next(next_),
            seq(seq_)
        {
        }
        ~Node() {
            //std::cout << "~Node()" << std::endl;
        }
    private:
        T element;
        int seq = {0};
        std::atomic<Node*> next = {nullptr};
    };

private:
    std::atomic<Node*>& find_first_small(const T& data) {
        std::atomic<Node*>* p_prev = &head;
        Node* curr = p_prev->load(std::memory_order_seq_cst);
        while (true) {
            if (curr == nullptr || curr->element >= data) {
                return *p_prev;
            }
            p_prev = &curr->next;
            curr = curr->next.load(std::memory_order_seq_cst);
        }
    }

public:
    SWMRListSet() {}
    ~SWMRListSet() {
        Node* curr = head.load(std::memory_order_seq_cst);
        while (curr) {
            Node* curr_next = curr->next.load(std::memory_order_seq_cst);
            curr->retire();
            curr = curr_next;
        }
    }

    bool add(T data) {
        std::lock_guard<std::mutex> guard(_mutex);
        static int seq = 0;
        std::atomic<Node*>& prev = find_first_small(data);
        Node* curr = prev.load(std::memory_order_seq_cst);
        if (curr && curr->element == data) {
            return false;
        }
        Node* node = new Node(data, curr, seq++);
        prev.store(node, std::memory_order_seq_cst);
        return true;
    }

    bool remove(const T& data) {
        std::lock_guard<std::mutex> guard(_mutex);
        std::atomic<Node*>& prev = find_first_small(data);
        Node* curr = prev.load(std::memory_order_seq_cst);
        if (!curr || curr->element != data) {
            return false;
        }
        prev.store(curr->next, std::memory_order_seq_cst);
        curr->next.store(nullptr, std::memory_order_seq_cst);
        curr->retire();
        return true;
    }

    bool contains(const T& data) {
        HazptrHolder holder_prev;
        HazptrHolder holder_curr;
        while (true) {
            std::atomic<Node*>* p_prev = &head;
            Node* curr = p_prev->load(std::memory_order_seq_cst);
            while (true) {
                if (!curr) return false;
                if (curr->element < data) return false;
                if(!holder_curr.try_protect(curr, *p_prev)) {
                    break;
                }
                Node* next = curr->next.load(std::memory_order_seq_cst);
                Node* latest_curr = p_prev->load(std::memory_order_seq_cst);
                if (latest_curr != curr) {
                    // 在读的时候可能curr被删除了，或者在prev和curr之间插入了1个新的节点
                    if (curr && latest_curr) {
                        LOG(NOTICE) << "prev is modify old_curr_seq:" << curr->element << " new_curr_seq:" << latest_curr->element;
                    } else if (curr) {
                        LOG(NOTICE) << "prev is modify old_curr_seq:" << curr->element << " new_curr_seq:" << latest_curr;
                    } else if (latest_curr) {
                        LOG(NOTICE) << "prev is modify old_curr_seq:" << curr << " new_curr_seq:" << latest_curr->element;
                    } else {
                        LOG(NOTICE) << "prev is modify old_curr_seq:" << curr << " new_curr_seq:" << latest_curr;
                    }
                    // 其实这里不break也没有关系，就是结果可能有点不太准确
                    // 假如curr.element == data, 本来curr被删除了，仍然会返回true
                    // 假如latest_curr.element == data, 本来latest_curr已经被插入了链表，仍然会返回false
                    // 在多线程环境下，本来就不存在绝对的一致，所以可以接受
                    break;
                }
                if (curr && curr->element == data) {
                    return true;
                }
                p_prev = &curr->next;
                curr = next;
                holder_curr.swap(holder_prev);
            }
        }
    }
private:
    std::atomic<Node*> head = {nullptr};
    std::mutex _mutex;
};

} // namespace rcu
