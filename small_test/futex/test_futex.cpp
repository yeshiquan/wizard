#include "futex_interface.h"

#include <iostream>

#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <bitset>

Futex<FutexInterface> _futex;
bool USE_FUTEX_WAIT = true;

void wait_until_reach_expected_version_slow(int threadid, uint16_t expected_version) noexcept {
    uint32_t current_version_and_waiters = _futex.value().load(::std::memory_order_relaxed);
    // 版本未就绪则循环等待
    while (current_version_and_waiters != expected_version) {
        _futex.wait(current_version_and_waiters, nullptr);
        current_version_and_waiters = _futex.value().load(::std::memory_order_relaxed);
    }
}

void wait_until_reach_expected_version_slow(int threadid, uint16_t expected_version) noexcept {
    uint32_t current_version_and_waiters = _futex.value().load(::std::memory_order_relaxed);
    uint16_t version = current_version_and_waiters;
    // 版本未就绪则循环等待
    while (version != expected_version) {
        if (USE_FUTEX_WAIT) {
            // 进入阻塞等待前需要先确保已经完成了等待者注册
            if (current_version_and_waiters <= UINT16_MAX) {
                uint32_t wait_version_and_waiters = current_version_and_waiters + UINT16_MAX + 1;
                if (_futex.value().compare_exchange_weak(current_version_and_waiters,
                            wait_version_and_waiters, ::std::memory_order_relaxed)) {
                    _futex.wait(wait_version_and_waiters, nullptr);
                } else {
                    continue;
                }
            } else {
                _futex.wait(current_version_and_waiters, nullptr);
            }
        } else {
            // 自旋等待无需注册等待者
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        current_version_and_waiters = _futex.value().load(::std::memory_order_relaxed);
        version = current_version_and_waiters;
    }
}



/*
void wait_until_reach_expected_version_slow(int threadid, uint16_t expected_version) noexcept {
    uint32_t current_version_and_waiters = _futex.value().load(::std::memory_order_relaxed);
    uint16_t version = current_version_and_waiters;
    std::cout << "threadid:" << threadid 
                << " current_version_and_waiters:" << current_version_and_waiters
                << " version:" << version
                << " expected_version:" << expected_version
                << std::endl;
    // 版本未就绪则循环等待
    while (version != expected_version) {
        std::cout << "threadid:" << threadid << " while begin...current_version_and_waiters:" << current_version_and_waiters << std::endl;
        if (USE_FUTEX_WAIT) {
            // 进入阻塞等待前需要先确保已经完成了等待者注册
            if (current_version_and_waiters <= UINT16_MAX) {
                uint32_t wait_version_and_waiters = current_version_and_waiters + UINT16_MAX + 1;
                std::cout << "threadid:" << threadid << " wait_version_and_waiters:" << wait_version_and_waiters << std::endl;

                if (_futex.value().compare_exchange_weak(current_version_and_waiters,
                            wait_version_and_waiters, ::std::memory_order_relaxed)) {
                    std::cout << "threadid:" << threadid << " wait trace 1" << std::endl;
                    _futex.wait(wait_version_and_waiters, nullptr);
                    std::cout << "threadid:" << threadid << " wait done trace 1" << std::endl;
                } else {
                    std::cout << "threadid:" << threadid << " cas failed, continue..." << std::endl;
                }
            } else {
                std::cout << "threadid:" << threadid << " wait trace 2" << std::endl;
                _futex.wait(current_version_and_waiters, nullptr);
                std::cout << "threadid:" << threadid << " wait done trace 2" << std::endl;
            }
        } else {
            // 自旋等待无需注册等待者
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        current_version_and_waiters = _futex.value().load(::std::memory_order_relaxed);
        version = current_version_and_waiters;
        std::cout << "threadid:" << threadid << " while end, current_version_and_waiters:" << current_version_and_waiters << " version:" << version << std::endl;
    }
}
*/

int main() {
    /*
    uint16_t aaa = 49;    
    uint32_t bbb = aaa + UINT16_MAX + 1;
    uint16_t ccc = bbb;
    std::cout << "ccc:" << ccc << std::endl
            << "aaa:" << std::bitset<32>(aaa) << std::endl
            << "bbb:" << std::bitset<32>(bbb) << std::endl
            << "ccc:" << std::bitset<32>(ccc) << std::endl
            << std::endl;
    */

    auto& v = _futex.value();
    v.store(0);
    std::cout << v.load() << std::endl;
    v.fetch_add(1);
    std::cout << v.load() << std::endl;
    v.fetch_add(1);
    std::cout << v.load() << std::endl;

    std::vector<std::thread> threads;
    int i;
    for (i = 0; i < 3; ++i) {
        threads.emplace_back([&, i] {
            wait_until_reach_expected_version_slow(i, 3);
        });
    }

    threads.emplace_back([&, i] {
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        std::cout << "wake futex thread_id:" << i << std::endl;
        v.fetch_add(1);
        _futex.wake_all();
        std::cout << "wake done futex thread_id:" << i << std::endl;
    });


    for (auto& th : threads) {
        th.join();
    }

    return 1;

    int thread_id1 = 444;
    int thread_id2 = 555;

    uint32_t _value = 99;
    auto& atomic = reinterpret_cast<::std::atomic<uint32_t>&>(_value);
    atomic.store(454);
    atomic.fetch_add(1);

    std::cout << "sizeof(_value):" << sizeof(_value) << std::endl;
    std::cout << "sizeof(atomic<uint32_t>):" << sizeof(atomic) << std::endl;
    std::cout << "sizeof(atomic<uint32_t>):" << sizeof(std::atomic<uint32_t>) << std::endl;
    std::cout << "sizeof(atomic<uint64_t>):" << sizeof(std::atomic<uint64_t>) << std::endl;
    std::cout << "atomic:" << atomic.load() << std::endl;
    atomic.store(454);
    std::cout << "atomic:" << atomic.load() << std::endl;
    atomic.fetch_add(1);
    std::cout << "atomic:" << atomic.load() << std::endl;



    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&, i] {
            std::cout << "wait futex thread_id:" << i << std::endl;
            uint32_t current_value = _futex.value().load();
            _futex.wait(current_value, nullptr);
            std::cout << "wait return futex thread_id:" << i << std::endl;
        });
    }

    threads.emplace_back([&, thread_id2] {
        std::this_thread::sleep_for(std::chrono::milliseconds(4000));
        std::cout << "wake futex thread_id:" << thread_id2 << std::endl;
        _futex.wake_all();
        std::cout << "wake done futex thread_id:" << thread_id2 << std::endl;
    });

    for (auto& th : threads) {
        th.join();
    }

    return 0;
}
