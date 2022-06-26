#include <iostream>
#include <atomic>

template <typename T>
T* get_ptr(uint64_t tagged_ptr) {
    return reinterpret_cast<T*>(tagged_ptr & 0x0000FFFFFFFFFFFFUL);
}

template <typename T>
uint64_t make_tagged_ptr(T* pointer) {
    static std::atomic<uint16_t> version = {0};
    uint16_t v = version.fetch_add(1, std::memory_order_acq_rel);
    return reinterpret_cast<uint64_t>(pointer) | (static_cast<uint64_t>(v) << 48);
}

struct Node {
    Node* next;
};

std::atomic<uint64_t> top = {0};

template <typename T>
T* pop() {
    T* top_node = nullptr;
    uint64_t top_tagged_ptr = 0;
    uint64_t new_top_tagged_ptr = 0;

    do {
        top_tagged_ptr = top.load(std::memory_order_acquire);
        Node* top_node = get_ptr<Node>(top_tagged_ptr);
        if (!top_node) {
            return nullptr;
        }
        new_top_tagged_ptr = make_tagged_ptr(top_node->next);
    } while (!top.compare_exchange_weak(
                    top_tagged_ptr, 
                    new_top_tagged_ptr, 
                    std::memory_order_release,
                    std::memory_order_acquire));

    return top_node;
}

int main() {
    Node* node = new Node;
    uint64_t combine1 = make_tagged_ptr(node);
    uint64_t combine2 = make_tagged_ptr(node);

    std::cout << combine1 << std::endl;
    std::cout << combine2 << std::endl;

    Node* p1 = get_ptr<Node>(combine1);
    Node* p2 = get_ptr<Node>(combine2);

    std::cout << "\n";
    std::cout << node << std::endl;
    std::cout << p1 << std::endl;
    std::cout << p2 << std::endl;
}
