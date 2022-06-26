#include <iostream>
#include <chrono>
#include <thread>
#include <bitset>

struct Node {
    Node* next;
    int data;
};

struct Table {
    size_t size;
    Node* nodes[0];
};

uint16_t get_current_timestamp() {
    ::timespec spec;
    ::clock_gettime(CLOCK_MONOTONIC, &spec);
    // 按照64s一个时间单位
    return spec.tv_sec >> 6;
}

uint16_t extract_tag(uint64_t tagged_ptr) {
    return head >> 48;
}

Node* extract_ptr(uint64_t tagged_ptr) {
    return reinterpret_cast<Node*>(tagged_ptr & 0x0000FFFFFFFFFFFFUL);
}

uint64_t make_tagged_ptr(Node* ptr, uint64_t tag) {
    return (tag << 48) | reinterpret_cast<uint64_t>(ptr);
}

bool expire(uint64_t head, uint16_t now) {
    return static_cast<uint16_t>(extract_tag(head) - now) > 1;
}

int main() {
    Node* node = new Node;
    node->data = 55343;

    uint16_t timestamp = get_current_timestamp();
    uint64_t head = make_tagged_ptr(node, timestamp); 

    std::cout << "node -> " << std::bitset<64>(reinterpret_cast<uint64_t>(node)) << std::endl;
    std::cout << "timestamp -> " << std::bitset<64>(timestamp) << std::endl;
    std::cout << "head -> " << std::bitset<64>(head) << std::endl << std::endl;

    Node* node2 = extract_ptr(head);
    uint16_t ts = extract_tag(head);

    std::cout << reinterpret_cast<uint64_t>(node2) << std::endl;
    std::cout << ts << std::endl;
    //std::this_thread::sleep_for(std::chrono::milliseconds(80000));
    std::cout << get_current_timestamp() << std::endl;

    std::cout << "\n";
    constexpr size_t CACHE_LINE = 64;
    //auto* nodes = reinterpret_cast<Table*>(aligned_alloc(CACHE_LINE, sizeof(Node) * 100));
    auto* nodes = new Node[100];
    auto addr0 = reinterpret_cast<uint64_t>(&nodes[0]);
    auto addr1 = reinterpret_cast<uint64_t>(&nodes[1]);
    auto addr2 = reinterpret_cast<uint64_t>(&nodes[2]);
    auto addr3 = reinterpret_cast<uint64_t>(&nodes[3]);

    std::cout << std::bitset<64>(addr0) << std::endl;
    std::cout << std::bitset<64>(addr1) << std::endl;
    std::cout << std::bitset<64>(addr2) << std::endl;
    std::cout << std::bitset<64>(addr3) << std::endl;
    std::cout << addr1 << std::endl;
    std::cout << (addr1 - addr0) << std::endl;
    std::cout << (addr2 - addr1) << std::endl;

    std::cout << "\n" << sizeof(Table) << std::endl;

    std::cout << "sizeof(Table) -> " << sizeof(Table) << std::endl;
    auto* table1 = reinterpret_cast<Table*>(aligned_alloc(CACHE_LINE, sizeof(Table) + sizeof(Node) * 0));
    auto* table2 = reinterpret_cast<Table*>(aligned_alloc(CACHE_LINE, sizeof(Table) + sizeof(Node) * 0));
    auto* table3 = reinterpret_cast<Table*>(aligned_alloc(CACHE_LINE, sizeof(Table) + sizeof(Node) * 0));
    auto* table4 = reinterpret_cast<Table*>(aligned_alloc(CACHE_LINE, sizeof(Table) + sizeof(Node) * 0));
    std::cout << std::bitset<64>(reinterpret_cast<uint64_t>(table1)) << std::endl;
    std::cout << std::bitset<64>(reinterpret_cast<uint64_t>(table2)) << std::endl;
    std::cout << std::bitset<64>(reinterpret_cast<uint64_t>(table3)) << std::endl;
    std::cout << std::bitset<64>(reinterpret_cast<uint64_t>(table4)) << std::endl;
    std::cout << (reinterpret_cast<uint64_t>(table2)) - (reinterpret_cast<uint64_t>(table1)) << std::endl;

    return 0;
}
