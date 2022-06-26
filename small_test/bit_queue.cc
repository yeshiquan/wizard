#include <iostream>

inline constexpr size_t bit_ceil(size_t n) noexcept {
    if (n < 2) {
        return 1;
    }
    auto leading_zero = __builtin_clzll(n - 1);
    auto tailing_zero = 64 - leading_zero;
    return static_cast<size_t>(1) << tailing_zero;
}

size_t slot_mask = 0;
size_t slot_bits = 0;

uint16_t push_version_for_index(size_t index) {
    return (index >> slot_bits) << 1;
}

uint16_t pop_version_for_index(size_t index) {
    return push_version_for_index(index) + 1;
}

int main() {
    size_t capacity = 7; 
    size_t new_capacity = bit_ceil(capacity);
    slot_mask = new_capacity - 1;
    slot_bits = __builtin_ctzll(new_capacity); 

    std::cout << "capacity:" << capacity
            << " new_capacity:" << new_capacity
            << " slot_mask:" << slot_mask
            << " slot_bits:" << slot_bits
            << std::endl;

    for (size_t index = 0; index <= 128; ++index) {
        size_t v = push_version_for_index(index);
        size_t v2 = pop_version_for_index(index);
        size_t slot_index = index & slot_mask;
        if (slot_index == 0) {
            std::cout << std::endl;
        }
        std::cout << "index:" << index
                 << "  push_version:" << v
                 << "  pop_version:" << v2
                 << "  shard:" << (index / new_capacity)
                 << "  slot_index:" << slot_index
                 << std::endl;
    }

    return 0;
}
