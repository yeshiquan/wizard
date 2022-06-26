#pragma once

#include <gflags/gflags.h>
#include <atomic>
#include <syscall.h>

namespace rcu {

template <typename T>
inline SwmrList<T>::SwmrList() noexcept
{
}

template <typename T>
inline bool SwmrList<T>::add(T data) {
    Node* p = &head;
    while (p-next) {
        if (p->next->element == data) {
            return p;
        }
        p = p->next;
    }
    if (p->next == nullptr) {
        return false;
    }
}

template <typename T>
inline bool SwmrList<T>::remove(const T& data) {
    Node* p = head.next;
    Node* last = &head;
    while (p) {
        if (p->element == data) {
            last->next = p->next;
            delete p;
            return;
        }
        last = p;
        p = p->next;
    }
}

template <typename T>
inline bool SwmrList<T>::contains(const T& data) {
    Node* p = head.next;
    while (p) {
        if (p->element == data) {
            return true;
        }
    }
    return false;
}

} // namespace
