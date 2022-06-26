#include "bench_common.h"
#include "my_hazptr.h"
#include <iostream>

#include "gflags/gflags.h"

DEFINE_int32(ops, 1000, "ops_per_thread");
DEFINE_int32(times, 10, "bench times");

void holder_bench(std::string name, int concurrent) {
    int ops_per_thread = FLAGS_ops;
    int times = FLAGS_times;
    int ops_each_time = ops_per_thread * concurrent;

    // bench一次
    auto benchFn = [&]() -> uint64_t {
        auto initFn = [] {};
        // 定义每个线程干的活
        auto fn = [&]() {
            for (int i = 0; i < ops_per_thread; i++) {
                rcu::HazptrHolder holder[10];
            }
        };
        auto endFn = [] {};
        // 启动concurrent个线程压测
        uint64_t cost = run_concurrent(initFn, fn, endFn, concurrent);
        return cost;
    };

    // bench多次，取最大值、平均值、最小值
    bench_many_times(name, benchFn, ops_each_time, times);
}

void retire_bench(std::string name, int concurrent) {
    int ops_per_thread = FLAGS_ops;
    int times = FLAGS_times;
    int ops_each_time = ops_per_thread * concurrent;

    struct FooNode : public rcu::HazptrNode<FooNode> {
        int x;
    };

    // bench一次
    auto benchFn = [&]() -> uint64_t {
        auto initFn = [] {};
        // 定义每个线程干的活
        auto fn = [&]() {
            for (int i = 0; i < ops_per_thread; i++) {
                FooNode* node = new FooNode();
                node->retire();
            }
        };
        auto endFn = [] {};
        // 启动concurrent个线程压测
        uint64_t cost = run_concurrent(initFn, fn, endFn, concurrent);
        return cost;
    };

    // bench多次，取最大值、平均值、最小值
    bench_many_times(name, benchFn, ops_each_time, times);
}

// 分别用多少种并发来压测
std::vector<int> concurrent_list = {1, 10};

int32_t run_bench() {
    for (auto concurrent : concurrent_list) {
        std::cout << "concurrent:" << concurrent << " threads -------------" << std::endl;
        holder_bench("bench_holder", concurrent);
        retire_bench("bench_retire", concurrent);
    }
    return 0;
}
