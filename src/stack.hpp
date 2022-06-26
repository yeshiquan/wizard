#pragma once

namespace rcu {

template<typename T>
typename LockFreeStack<T>::Node* LockFreeStack<T>::pop_head() {
    Node* const old_head = head.load();
    if (old_head == tail.load()) {
        return nullptr;
    }
    head.store(old_head->next);

    return old_head;
}

template<typename T>
std::shared_ptr<T> LockFreeStack<T>::pop() {
    Node* old_head = pop_head();
    if (old_head == nullptr) {
        return std::shared_ptr<T>();
    }
    std::shared_ptr<T> const res(old_head->data);
    delete old_head;
    return res;
}

template<typename T>
void LockFreeStack<T>::push(T new_value) {
    auto new_data = std::make_shared<T>(new_value);
    Node* const old_tail = tail.load();
    old_tail->data.swap(new_data);

    Node* dummy = new Node();
    old_tail->next = dummy;
    tail.store(dummy);
}

template<typename T>
int LockFreeStack<T>::size() {
    return this->_size.load(std::memory_order_relaxed);
}

template<typename T>
bool LockFreeStack<T>::empty() {
    return (this->head.load() == this->tail.load());
}

}
