#pragma once

namespace rcu {

template<typename T>
typename NonLockFreeQueue<T>::Node* NonLockFreeQueue<T>::pop_head() {
    Node* const old_head = head.load();
    if (old_head == tail.load()) {
        return nullptr;
    }
    head.store(old_head->next);

    return old_head;
}

template<typename T>
std::unique_ptr<T> NonLockFreeQueue<T>::pop() {
    CountedNodePtr old_head = head.load(std::memory_order_relaxed);
    for (;;) {
        increase_external_count(head, old_head);
        Node* ptr = old_head.ptr;
        if (ptr == tail.load().ptr) {
            return std::unique_ptr<T>();
        }
        CountedNodePtr next = ptr->next.load();
        if (head.compare_exchange_strong(old_head, next)) {
            T* res = ptr->data.exchange(nullptr);
            free_external_counter(old_head);
            return std::unique_ptr<T>(res);
        }
        ptr->release_ref();
    }
}


template<typename T>
void NonLockFreeQueue<T>::set_new_tail(CountedNodePtr& old_tail, CountedNodePtr const &new_tail) {
    Node* current_tail_ptr = old_tail.ptr;

    while (!tail.compare_exchange_weak(old_tail, new_tail)) {
        if (old_tail.ptr == current_tail_ptr) {
            break;
        }
    }

    if (old_tail.ptr == current_tail_ptr) {
        free_external_counter(old_tail);
    } else {
        current_tail_ptr->release_ref();
    }
}

template<typename T>
void NonLockFreeQueue<T>::push(T new_value) {
    // 总是要假设有2个线程同时调用push
    std::unique_ptr<T> new_data(new T(new_value));
    CountedNodePtr new_next;
    new_next.ptr = new Node;
    new_next.external_count = 1;
    CountedNodePtr old_tail = tail.load();

    for(;;) {
        // 增加tail的引用次数
        increase_external_count(tail, old_tail);
        T* old_data = nullptr;
        // Step1 setup dummy_node's data
        Node* dummy_node = old_tail.ptr;
        if (dummy_node->data.compare_exchange_strong(old_data, new_data.get())) {
            // 线程A成功
            CountedNodePtr old_next = {0};
            // Step2 setup dummy_node's next
            if (!dummy_node->next.compare_exchange_strong(old_next, new_next)) {
                // 此时在线程A，线程A修改next失败，说明线程B在Step4成功了
                delete new_next.ptr;
                new_next = old_next;
            }
            // Step3 change head
            set_new_tail(old_tail, new_next);
            new_data.release();
            break;
        } else {
            // 线程A修改data成功了, 线程B失败，线程B进入这里
            // 为了实现真正的lock-free，即使线程B设置data失败了，它也不能loop忙等，要干点有用的事情
            // 那就帮助Thread A做点事情吧 make progress
            // dummy_node的data修改失败
            CountedNodePtr old_next = {0};
            // Step4 修改next
            if (dummy_node->next.compare_exchange_strong(old_next, new_next)) {
                // 如果这里成功了，说明线程A在Step2没有成功
                old_next = new_next;
                new_next.ptr = new Node;
            }
            set_new_tail(old_tail, old_next);
        }
    }
}

template<typename T>
int NonLockFreeQueue<T>::size() {
    return this->_size.load(std::memory_order_relaxed);
}

template<typename T>
bool NonLockFreeQueue<T>::empty() {
    return (this->head.load().ptr == this->tail.load().ptr);
}

}
