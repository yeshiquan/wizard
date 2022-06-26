#include <assert.h>
#include "baidu/streaming_log.h"
#include "base/comlog_sink.h"
#include "base/strings/stringprintf.h"
#include "com_log.h"
#include "cronoapd.h"

#include <baidu/feed/mlarch/babylon/pool.h>

#undef DCHECK_IS_ON

#include <random>

// concurrent:1 threads -------------
// babylon_pool                                      189 ns     187 ns     179 ns
// my_pool                                            91 ns      90 ns      90 ns
// my_pool_acc                                       129 ns     127 ns     124 ns
// babylon_pool_batch relea                          398 ns     388 ns     378 ns
// my_pool_batch relea                               422 ns     370 ns     346 ns
// my_pool_batch acc relea                           392 ns     362 ns     343 ns
// babylon_pool_batch                                266 ns     259 ns     250 ns
// my_pool_batch                                     171 ns     153 ns     148 ns
// my_pool_batch acc                                 183 ns     174 ns     168 ns
// concurrent:10 threads -------------
// babylon_pool                                       80 ns      47 ns      32 ns
// my_pool                                            57 ns      32 ns      18 ns
// my_pool_acc                                        42 ns      33 ns      24 ns
// babylon_pool_batch relea                          930 ns     347 ns     235 ns
// my_pool_batch relea                               311 ns     249 ns     192 ns
// my_pool_batch acc relea                           393 ns     276 ns     214 ns
// babylon_pool_batch                                298 ns     273 ns     248 ns
// my_pool_batch                                     388 ns     292 ns     232 ns
// my_pool_batch acc                                 308 ns     260 ns     232 ns
// concurrent:20 threads -------------
// babylon_pool                                       62 ns      44 ns      37 ns
// my_pool                                            40 ns      29 ns      17 ns
// my_pool_acc                                        56 ns      30 ns      18 ns
// babylon_pool_batch relea                          841 ns     419 ns     341 ns
// my_pool_batch relea                               379 ns     347 ns     305 ns
// my_pool_batch acc relea                           469 ns     373 ns     315 ns
// babylon_pool_batch                                403 ns     352 ns     323 ns
// my_pool_batch                                     377 ns     342 ns     309 ns
// my_pool_batch acc                                 369 ns     339 ns     305 ns

#include "gflags/gflags.h"

#include "bench_common.h"
#include "pool.h"

DEFINE_int32(ops_per_thread, 100000, "ops_per_thread");
DEFINE_int32(times, 10, "bench times");

struct Foobar {
    std::string name;
    char dummy[773];
};

void bench_my_pool_acc(std::string name, int concurrent) {
    int ops_each_time = FLAGS_ops_per_thread * concurrent;
    // bench一次
    auto benchFn = [&]() -> uint64_t {
        using Pool = rcu::ObjectPool<Foobar, true, 7>;
        Pool pool(FLAGS_ops_per_thread, 1000);
        auto initFn = [] {};

        // 定义每个线程干的活
        auto fn = [&]() {
            for (int i = 0; i < FLAGS_ops_per_thread; i++) {
                auto pool_obj = pool.get();
            }
        };
        auto endFn = [] {};
        uint64_t cost = run_concurrent(initFn, fn, endFn, concurrent);
        return cost;
    };

    // bench多次，取最大值、平均值、最小值
    bench_many_times(name, benchFn, ops_each_time, FLAGS_times);
}


void bench_my_pool(std::string name, int concurrent) {
    int ops_each_time = FLAGS_ops_per_thread * concurrent;
    // bench一次
    auto benchFn = [&]() -> uint64_t {
        using Pool = rcu::ObjectPool<Foobar, false, 7>;
        Pool pool(FLAGS_ops_per_thread, 1000);
        auto initFn = [] {};
        // 定义每个线程干的活
        auto fn = [&]() {
            for (int i = 0; i < FLAGS_ops_per_thread; i++) {
                auto pool_obj = pool.get();
            }
        };
        auto endFn = [] {};
        uint64_t cost = run_concurrent(initFn, fn, endFn, concurrent);
        return cost;
    };

    // bench多次，取最大值、平均值、最小值
    bench_many_times(name, benchFn, ops_each_time, FLAGS_times);
}

template<typename Pool>
void bench_my_pool_batch(std::string name, int concurrent, bool is_bench_release) {
    int ops_each_time = FLAGS_ops_per_thread * concurrent;
    // bench一次
    auto benchFn = [&]() -> uint64_t {
        using PooledObject = typename Pool::PooledObject;
        Pool pool(FLAGS_ops_per_thread, 1000);
        std::vector<std::vector<PooledObject>> vec(32768);
        auto initFn = [&] {};
        // 定义每个线程干的活
        auto fn = [&]() {
            auto tid = syscall(__NR_gettid);
            auto& objs = vec[tid];
            objs.reserve(FLAGS_ops_per_thread);
            for (int i = 0; i < FLAGS_ops_per_thread; i++) {
                auto pool_obj = pool.get();
                objs.emplace_back(std::move(pool_obj));
            }
            if (is_bench_release) {
                objs.clear();
            }
        };
        auto endFn = [] {};
        uint64_t cost = run_concurrent(initFn, fn, endFn, concurrent);
        return cost;
    };

    // bench多次，取最大值、平均值、最小值
    bench_many_times(name, benchFn, ops_each_time, FLAGS_times);
}

template<typename Pool>
void bench_babylon_pool_batch(std::string name, int concurrent, bool is_bench_release) {
    int ops_each_time = FLAGS_ops_per_thread * concurrent;
    // bench一次
    auto benchFn = [&]() -> uint64_t {
        using PooledObject = typename Pool::PooledObject;
        Pool pool(FLAGS_ops_per_thread, 7, 1000);
        std::vector<std::vector<PooledObject>> vec(32768);
        auto initFn = [&] {};
        // 定义每个线程干的活
        auto fn = [&]() {
            auto tid = syscall(__NR_gettid);
            auto& objs = vec[tid];
            objs.reserve(FLAGS_ops_per_thread);
            for (int i = 0; i < FLAGS_ops_per_thread; i++) {
                auto pool_obj = pool.get();
                objs.emplace_back(std::move(pool_obj));
            }
            if (is_bench_release) {
                objs.clear();
            }
        };
        auto endFn = [] {};
        uint64_t cost = run_concurrent(initFn, fn, endFn, concurrent);
        return cost;
    };

    // bench多次，取最大值、平均值、最小值
    bench_many_times(name, benchFn, ops_each_time, FLAGS_times);
}


void bench_babylon_pool(std::string name, int concurrent) {
    int ops_each_time = FLAGS_ops_per_thread * concurrent;
    // bench一次
    auto benchFn = [&]() -> uint64_t {
        using Pool = baidu::feed::mlarch::babylon::ObjectPool<Foobar>;
        Pool pool(FLAGS_ops_per_thread, 7, 1000);
        std::mutex _mutex;
        auto initFn = [] {};
        // 定义每个线程干的活
        auto fn = [&]() {
            for (int i = 0; i < FLAGS_ops_per_thread; i++) {
                auto pool_obj = pool.get();
            }
        };
        auto endFn = [] {};
        uint64_t cost = run_concurrent(initFn, fn, endFn, concurrent);
        return cost;
    };

    // bench多次，取最大值、平均值、最小值
    bench_many_times(name, benchFn, ops_each_time, FLAGS_times);
}


// 分别用多少种并发来压测
std::vector<int> concurrent_list = {1, 10, 20};
int32_t run_bench() {
    for (auto concurrent : concurrent_list) {
        std::cout << "concurrent:" << concurrent << " threads -------------" << std::endl;
        bench_babylon_pool("babylon_pool", concurrent);
        bench_my_pool("my_pool", concurrent);
        bench_my_pool_acc("my_pool_acc", concurrent);

        using baidu::feed::mlarch::babylon::ObjectPool;

        bench_babylon_pool_batch<ObjectPool<Foobar>>("babylon_pool_batch relea", concurrent, true);
        bench_my_pool_batch<rcu::ObjectPool<Foobar, false, 7>>("my_pool_batch relea", concurrent, true);
        bench_my_pool_batch<rcu::ObjectPool<Foobar, true, 7>>("my_pool_batch acc relea", concurrent, true);

        bench_babylon_pool_batch<ObjectPool<Foobar>>("babylon_pool_batch", concurrent, false);
        bench_my_pool_batch<rcu::ObjectPool<Foobar, false, 7>>("my_pool_batch", concurrent, false);
        bench_my_pool_batch<rcu::ObjectPool<Foobar, true, 7>>("my_pool_batch acc", concurrent, false);
    }
    return 0;
}

int main(int argc, char** argv) {
    google::ParseCommandLineFlags(&argc, &argv, true);

    std::string log_conf_file = "./conf/log_afile.conf";

    com_registappender("CRONOLOG", comspace::CronoAppender::getAppender,
                comspace::CronoAppender::tryAppender);

    auto logger = logging::ComlogSink::GetInstance();
    if (0 != logger->SetupFromConfig(log_conf_file.c_str())) {
        LOG(FATAL) << "load log conf failed";
        return -1;
    }

    return run_bench();
}

