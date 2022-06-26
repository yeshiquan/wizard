#include <functional>
#include <thread>
#include <atomic>
#include <iostream>
#include <iomanip>

using Clock = std::chrono::high_resolution_clock;

template <typename InitFunc, typename Func, typename EndFunc>
inline uint64_t run_single(InitFunc initFn, Func&& fn, EndFunc endFunc) noexcept {
    initFn();

    auto start_time = Clock::now();
    fn();
    auto use_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                            Clock::now() - start_time).count();
    endFunc();

    return use_ns;
}

template <typename InitFunc, typename Func, typename EndFunc>
inline uint64_t run_concurrent(InitFunc initFn, Func&& fn, EndFunc endFunc, int concurrent) noexcept {
    ::std::vector<::std::thread> threads;
    ::std::atomic<bool> is_start(false);

    initFn();

    for (size_t i = 0; i < concurrent; ++i) {
        threads.emplace_back([&, i] {
            while (!is_start.load()) {
                ::std::this_thread::yield();
            }
            fn();
        });
    }

    auto start_time = Clock::now();
    is_start.store(true);

    for (auto& thread : threads) {
        thread.join();
    }

    auto use_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                            Clock::now() - start_time).count();
    endFunc();

    return use_ns;
}

template <typename BenchFunc>
void bench_many_times(std::string name, BenchFunc&& benchFn, int ops_each_time, int times) {
    uint64_t min = UINTMAX_MAX;
    uint64_t max = 0;
    uint64_t sum = 0;

    for (int i = 0; i < times; i++) {
        uint64_t cost = benchFn();
        sum += cost;
        max = std::max(cost, max);
        min = std::min(cost, min);
    }
    uint64_t avg = sum / times;
    std::string unit = " ns";

    // hazptr_retire           2499 ns   2460 ns   2420 ns
    std::cout << std::left << std::setw(45) << name;
    std::cout << "    " << std::right << std::setw(4) << max / ops_each_time << unit;
    std::cout << "    " << std::right << std::setw(4) << avg / ops_each_time << unit;
    std::cout << "    " << std::right << std::setw(4) << min / ops_each_time << unit;
    std::cout << std::endl;
}


int32_t run_bench();

