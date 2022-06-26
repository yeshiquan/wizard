#include <iostream>
#include <type_traits>

template <class T>
inline constexpr
typename std::enable_if<
  (std::is_integral<T>::value &&
   std::is_unsigned<T>::value &&
   sizeof(T) >= sizeof(unsigned int) &&
   sizeof(T) <= sizeof(unsigned long)),
  unsigned int>::type
  findLastSet(T x) {
  return x ? ((8 * sizeof(unsigned long) - 1) ^ __builtin_clzl(x)) + 1 : 0;
}

template <class T>
inline constexpr
typename std::enable_if<
  std::is_integral<T>::value && std::is_unsigned<T>::value,
  T>::type
nextPowTwo(T v) {
  return v ? (T(1) << findLastSet(v - 1)) : 1;
}


int main() {
    unsigned int hint = 6;
    auto NumPerShard = nextPowTwo(hint);
    auto ShardBit =  __builtin_popcount(NumPerShard - 1) ;
    std::cout << "index = shard_id * NumPerShard + offset" << std::endl;
    for (size_t index = 10000000000; index < 10000000005; index++) {
        // 把index分解为 index = shard_id * NumPerShard + offset
        auto shard_id = index >> ShardBit;
        auto offset = index & (NumPerShard - 1);
        std::cout << index << " -> NumPerShard:" << NumPerShard << " shard_id:" << shard_id << " offset:" << offset << std::endl;
    }

    size_t CACHELINE_SIZE = 64;
    for (size_t i = 10; i < 101; i++) {
        auto j = (i + CACHELINE_SIZE) & ~static_cast<size_t>(CACHELINE_SIZE-1);
        std::cout << i << " " << j << std::endl;
    }
}
