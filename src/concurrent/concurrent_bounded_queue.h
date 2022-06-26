//TODO
// 1 单测各种情况
// 2 析构 done
// 3 性能优化 & benchmark  done
// 4 丰富接口
// 5 DCHECK
#pragma once

#include <memory>
#include "futex_interface.h"
#include <chrono>

namespace rcu::queue {

#ifndef QUEUE_STATS
#define QUEUE_STATS false
#endif

#if QUEUE_STATS
#define INC_STATS(x) g_stat.x()
#define PRINT_STATS() g_stat.print()
#else 
#define INC_STATS(x)
#define PRINT_STATS() g_stat.do_nothing()
#endif

class QueueStat {
public:
    std::atomic<uint64_t> _wait{0};
    std::atomic<uint64_t> _wake{0};
    std::atomic<uint64_t> _futex_compacted{0};
    std::atomic<uint64_t> _futex_no_compacted{0};
    std::atomic<uint64_t> _loop{0};
public:
    void wait() { _wait++; }
    void wake() { _wake++; }
    void futex_compacted() { _futex_compacted++; }
    void futex_no_compacted() { _futex_no_compacted++; }
    void loop() { _loop++; }
    void do_nothing() {}
    void print() {
        std::cout << "QueueStat -> " 
                    << " wait:" << _wait.load()
                    << " wake:" << _wake.load()
                    << " futex_compacted:" << _futex_compacted.load()
                    << " futex_no_compacted:" << _futex_no_compacted.load()
                    << " loop:" << _loop.load()
                    << std::endl;
    }
};
static QueueStat g_stat;

using rcu::FutexInterface;

constexpr size_t CACHELINE_SIZE = 64;

template <class T>
inline constexpr
typename std::enable_if<
  (std::is_integral<T>::value &&
   std::is_unsigned<T>::value &&
   sizeof(T) > sizeof(unsigned int) &&
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

inline uint16_t get_raw_version(uint32_t tagged_version) noexcept {
    return (uint16_t)(tagged_version);
}

inline uint32_t make_tagged_version(uint32_t version) noexcept {
    return version + UINT16_MAX + 1;
}

inline bool is_tagged_version(uint32_t version) noexcept {
    return version > UINT16_MAX;
}

enum ConcurrentBoundedQueueOption {
    CACHELINE_ALIGNED = 0x1,
    DEFAULT = 0x0,
}; 

template <typename T, typename F = FutexInterface, uint32_t OPTION = ConcurrentBoundedQueueOption::DEFAULT>
class ConcurrentBoundedQueue {
    class BaseSlotFutex {
    public:
        void wait_until_reach_expected_version(uint16_t expected_version) noexcept {
            uint32_t current_version = _futex.value().load(std::memory_order_relaxed);
            uint16_t current_raw_version = get_raw_version(current_version);
            if (current_raw_version == expected_version) {
                return;
            }

            // version不满足，需要wait
            while (current_raw_version != expected_version) {
                INC_STATS(loop);
                // wait之前先把version标记一下，表明现在有等待者，这样别人版本推进后才会唤醒
                if (!is_tagged_version(current_version)) {
                    uint32_t tagged_version = make_tagged_version(current_version);
                    // 只需要1个线程负责标记就可以了，用CAS操作
                    if (_futex.value().compare_exchange_weak(
                                                current_version, 
                                                tagged_version,
                                                std::memory_order_release,
                                                std::memory_order_acquire)) {
                        INC_STATS(wait);
                        _futex.wait(tagged_version, nullptr);
                    } 
                } else {
                    INC_STATS(wait);
                    _futex.wait(current_version, nullptr);
                }
                // 被唤醒了，看看最新的version是不是我想要的
                current_version = _futex.value().load(std::memory_order_relaxed);
                current_raw_version = get_raw_version(current_version); 
            }
        }

        void advance_version(uint16_t new_version) noexcept {
            _futex.value().exchange(new_version, std::memory_order_relaxed);
        }

        void advance_version_and_wakeup_waiters(uint16_t new_version) noexcept {
            // 推进版本
            uint32_t old_version = _futex.value().exchange(new_version, std::memory_order_release);
            if (is_tagged_version(old_version)) {
                // 只有version被标记了才会唤醒, 被标记了说明有别的线程在等待
                INC_STATS(wake);
                _futex.wake_all();
            } else {
                // futex开销大，为了节省开销，没有等待者就不调用futex
            }
        }

        void reset() noexcept {
            _futex.value().store(0, std::memory_order_relaxed);
        }
    private:
        Futex<F> _futex {0};
    };
    struct alignas(CACHELINE_SIZE) AlignedSlotFutex : public BaseSlotFutex {
    };
    struct BaseSlot {
        T value;
    };
    struct alignas(CACHELINE_SIZE) CompactedSlot : public BaseSlot {
        BaseSlotFutex futex;
    };
    struct alignas(CACHELINE_SIZE) AlignedSlot : public BaseSlot {
    };
    static constexpr bool CACHELINE_ALIGNED = OPTION & ConcurrentBoundedQueueOption::CACHELINE_ALIGNED;
    static constexpr bool COMPACTED = CACHELINE_ALIGNED && CACHELINE_SIZE >=  
                                    ((sizeof(T) % CACHELINE_SIZE) + sizeof(Futex<F>));
    using Slot = typename std::conditional_t<COMPACTED, 
                                            CompactedSlot, 
                                            typename std::conditional_t<CACHELINE_ALIGNED, 
                                                                        AlignedSlot, 
                                                                        BaseSlot>>;
    using SlotFutex = typename std::conditional_t<CACHELINE_ALIGNED, AlignedSlotFutex, BaseSlotFutex>;
public:
    explicit ConcurrentBoundedQueue(size_t capacity) noexcept {
        reserve_and_clear(capacity);
    }
    // 禁止拷贝和移动
    ConcurrentBoundedQueue(ConcurrentBoundedQueue&&) = delete;
    ConcurrentBoundedQueue(const ConcurrentBoundedQueue&) = delete;
    ConcurrentBoundedQueue& operator=(ConcurrentBoundedQueue&&) = delete;
    ConcurrentBoundedQueue& operator=(const ConcurrentBoundedQueue&) = delete;

    void reserve_and_clear(size_t min_capacity) noexcept {
        auto new_capacity = nextPowTwo(min_capacity);
        if (new_capacity != capacity()) {
            _round_bits = __builtin_popcount(new_capacity - 1) ;
            _capacity = new_capacity;
            _slots.resize(new_capacity);
            _next_push_index.store(0, std::memory_order_relaxed);
            _next_pop_index.store(0, std::memory_order_relaxed);
            for (size_t i = 0; i < new_capacity; ++i) {
                _slots.get_futex(i).reset();
            }
        } else {
            clear();
        }
    }

    size_t capacity() noexcept {
        return _capacity;
    }

    void clear() noexcept {
        while (size() > 0) {
            pop([](T& v){});
        }
    }

    size_t size() noexcept {
        auto pop_idx = _next_pop_index.load(std::memory_order_relaxed);
        auto push_idx = _next_push_index.load(std::memory_order_relaxed);
        return push_idx > pop_idx ? push_idx - pop_idx : 0;
    }

    uint16_t push_version_for_index(size_t index) noexcept {
        return (index >> _round_bits) << 1;
    }

    uint16_t pop_version_for_index(size_t index) noexcept {
        return push_version_for_index(index) + 1;
    }

    template<bool IS_PUSH, typename C>
    void process(size_t index, C&& callback) noexcept {
        uint16_t expected_version = 0;
        if constexpr (IS_PUSH) {
            expected_version = push_version_for_index(index);
        } else {
            expected_version = pop_version_for_index(index);
        }
        uint32_t slot_id = index & (_capacity - 1);
        Slot& slot = _slots.get_slot(slot_id);
        BaseSlotFutex& slot_futex = _slots.get_futex(slot_id);

        slot_futex.wait_until_reach_expected_version(expected_version);

        std::atomic_thread_fence(::std::memory_order_acquire);
        callback(slot.value);

        slot_futex.advance_version_and_wakeup_waiters(expected_version + 1);
    }

    void push(const T& value) noexcept {
        push([&](T& v) __attribute__((always_inline)) {
            v = value;
        });
    }

    void push(T&& value) noexcept {
        push([&](T& v) __attribute__((always_inline)) {
            v = std::move(value);
        });
    }

    template<typename C, typename = std::enable_if_t<std::is_invocable_v<C, T&>>>
    void push(C&& callback) noexcept {
        size_t index = _next_push_index.fetch_add(1, std::memory_order_relaxed);
        process<true>(index, std::forward<C>(callback));
    }

    void pop(T& value) noexcept {
        pop([&](T& v) __attribute__((always_inline)) {
            value = std::move(v);
        });
    }

    void pop(T* value) noexcept {
        pop([&](T& v) __attribute__((always_inline)) {
            value = &v;
        });
    }

    template<typename C, typename = std::enable_if_t<std::is_invocable_v<C, T&>>>
    void pop(C&& callback) noexcept {
        size_t index = _next_pop_index.fetch_add(1, std::memory_order_relaxed);
        process<false>(index, std::forward<C>(callback));
    }
private:
    struct SlotVector {
    public:
        SlotVector() = default;

        size_t size() noexcept {
            return _size;
        }

        void init(size_t capacity) {
            _size = capacity;
            _slots = reinterpret_cast<Slot*>(::aligned_alloc(alignof(Slot), sizeof(Slot) * _size));
            if constexpr (!COMPACTED) {
                _futexes = reinterpret_cast<SlotFutex*>(::aligned_alloc(alignof(SlotFutex), sizeof(SlotFutex) * _size));
            }
            for (size_t i = 0; i < _size; ++i) {
                new (&_slots[i]) Slot();
                if constexpr (!COMPACTED) {
                    new (&_futexes[i]) SlotFutex();
                }
            }
        }

        void resize(size_t capacity) noexcept {
            clear();
            init(capacity);
        }

        ~SlotVector() noexcept {
            clear();
        }

        void clear() noexcept {
            for (size_t i = 0; i < _size; ++i) {
                _slots[i].~Slot();
                if constexpr (!COMPACTED) {
                    _futexes[i].~SlotFutex();
                }
            }
            if (_slots) {
                free(_slots);
                _slots = nullptr;
            }
            if constexpr (!COMPACTED) {
                if (_futexes) {
                    free(_futexes);
                    _futexes = nullptr;
                }
            }
            _size = 0;
        }

        BaseSlotFutex& get_futex(size_t slot_id) noexcept {
            if constexpr (COMPACTED) {
                INC_STATS(futex_compacted);
                return _slots[slot_id].futex;
            } else {
                INC_STATS(futex_no_compacted);
                return _futexes[slot_id];
            }
        }

        Slot& get_slot(size_t i) noexcept {
            return _slots[i];
        }
    private:
        Slot* _slots = nullptr;
        SlotFutex* _futexes = nullptr;
        size_t _size = 0;
    };
private:
    SlotVector _slots;
    size_t _capacity = 0;
    size_t _round_bits = 0;
    alignas(CACHELINE_SIZE) std::atomic<size_t> _next_push_index = ATOMIC_VAR_INIT(0);
    alignas(CACHELINE_SIZE) std::atomic<size_t> _next_pop_index = ATOMIC_VAR_INIT(0);
};

} // namespace
