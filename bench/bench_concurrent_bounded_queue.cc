#include <assert.h>
#include "baidu/streaming_log.h"
#include "base/comlog_sink.h"
#include "base/strings/stringprintf.h"
#include "com_log.h"
#include "cronoapd.h"

// concurrent:1 threads -------------
// stl_enqueue                                        35 ns      34 ns      33 ns
// moody_enqueue                                      42 ns      42 ns      42 ns
// babylon_enqueue                                    49 ns      46 ns      43 ns
// cbq_enqueue                                        43 ns      42 ns      40 ns
// concurrent:2 threads -------------
// stl_enqueue                                       164 ns     119 ns      35 ns
// moody_enqueue                                     250 ns     106 ns      44 ns
// babylon_enqueue                                   189 ns      98 ns      55 ns
// cbq_enqueue                                       108 ns      93 ns      63 ns
// concurrent:3 threads -------------
// stl_enqueue                                       107 ns      78 ns      35 ns
// moody_enqueue                                     263 ns     143 ns      97 ns
// babylon_enqueue                                   176 ns      90 ns      56 ns
// cbq_enqueue                                       128 ns      82 ns      46 ns
// concurrent:4 threads -------------
// stl_enqueue                                       166 ns     140 ns      84 ns
// moody_enqueue                                     279 ns     163 ns     101 ns
// babylon_enqueue                                   171 ns      87 ns      74 ns
// cbq_enqueue                                        93 ns      80 ns      73 ns
// concurrent:5 threads -------------
// stl_enqueue                                       195 ns     172 ns     146 ns
// moody_enqueue                                     243 ns     173 ns     116 ns
// babylon_enqueue                                   100 ns      81 ns      69 ns
// cbq_enqueue                                        85 ns      75 ns      70 ns
// concurrent:10 threads -------------
// stl_enqueue                                       248 ns     219 ns     204 ns
// moody_enqueue                                     236 ns     187 ns     119 ns
// babylon_enqueue                                   140 ns      97 ns      67 ns
// cbq_enqueue                                       117 ns      90 ns      68 ns
// concurrent:20 threads -------------
// stl_enqueue                                       273 ns     251 ns     205 ns
// moody_enqueue                                     230 ns     223 ns     219 ns
// babylon_enqueue                                   164 ns     115 ns      64 ns
// cbq_enqueue                                       118 ns      87 ns      63 ns


#undef DCHECK_IS_ON

#include <random>
#include <mutex>
#include <deque>

#include "gflags/gflags.h"

#define QUEUE_STATS false

#include "bench_common.h"
#include "concurrent/concurrent_bounded_queue.h"

#include "concurrent/concurrentqueue.h"

DEFINE_int32(ops_per_thread, 100000, "ops_per_thread");
DEFINE_int32(times, 10, "bench times");
DEFINE_int32(type, 0, "bench type");

struct Foobar {
    std::string name;
    char dummy[771];
};

void bench_stl_enqueue(std::string name, int concurrent) {
    int ops_each_time = FLAGS_ops_per_thread * concurrent;
    // bench一次
    auto benchFn = [&]() -> uint64_t {
        std::deque<int> q;
        std::mutex mutex;
        auto initFn = [&] {
            q.resize(FLAGS_ops_per_thread * concurrent);
        };

        // 定义每个线程干的活
        auto fn = [&]() {
            for (int i = 0; i < FLAGS_ops_per_thread; i++) {
                std::lock_guard<std::mutex> guard(mutex);
                q.push_back(i);
                q.pop_front();
            }
        };
        auto endFn = [] {};
        uint64_t cost = run_concurrent(initFn, fn, endFn, concurrent);
        return cost;
    };

    // bench多次，取最大值、平均值、最小值
    bench_many_times(name, benchFn, ops_each_time, FLAGS_times);
}

void bench_moody_enqueue(std::string name, int concurrent) {
    int ops_each_time = FLAGS_ops_per_thread * concurrent;
    // bench一次
    auto benchFn = [&]() -> uint64_t {
        moodycamel::ConcurrentQueue<int> q(FLAGS_ops_per_thread * concurrent);
        auto initFn = [&] {
        };

        // 定义每个线程干的活
        auto fn = [&]() {
            for (int i = 0; i < FLAGS_ops_per_thread; i++) {
                q.enqueue(i);
                int v;
                while(!q.try_dequeue(v));
            }
        };
        auto endFn = [] {};
        uint64_t cost = run_concurrent(initFn, fn, endFn, concurrent);
        return cost;
    };

    // bench多次，取最大值、平均值、最小值
    bench_many_times(name, benchFn, ops_each_time, FLAGS_times);
}

void bench_babylon_enqueue(std::string name, int concurrent) {
    int ops_each_time = FLAGS_ops_per_thread * concurrent;
    // bench一次
    auto benchFn = [&]() -> uint64_t {
        using rcu::FutexInterface;
        using Queue = ConcurrentBoundedQueue<int, FutexInterface, ConcurrentBoundedQueueOption::CACHELINE_ALIGNED>;
        Queue q(FLAGS_ops_per_thread*concurrent);
        auto initFn = [&] {
        };

        // 定义每个线程干的活
        auto fn = [&]() {
            for (int i = 0; i < FLAGS_ops_per_thread; i++) {
                q.push(i);
                int v;
                q.pop(v);
            }
        };
        auto endFn = [] {};
        uint64_t cost = run_concurrent(initFn, fn, endFn, concurrent);
        return cost;
    };

    // bench多次，取最大值、平均值、最小值
    bench_many_times(name, benchFn, ops_each_time, FLAGS_times);
}


void bench_cbq_enqueue(std::string name, int concurrent) {
    int ops_each_time = FLAGS_ops_per_thread * concurrent;
    // bench一次
    auto benchFn = [&]() -> uint64_t {
        using rcu::queue::ConcurrentBoundedQueue;
        using rcu::queue::ConcurrentBoundedQueueOption;
        using rcu::FutexInterface;
        using Queue = ConcurrentBoundedQueue<int, FutexInterface, ConcurrentBoundedQueueOption::CACHELINE_ALIGNED>;
        Queue q(FLAGS_ops_per_thread*concurrent);
        auto initFn = [&] {};

        int c = 1;

        // 定义每个线程干的活
        auto fn = [&]() {
            for (int i = 0; i < FLAGS_ops_per_thread; i++) {
                q.push(i);
                int v;
                q.pop(v);
            }
        };
        auto endFn = [&] {
            //rcu::queue::PRINT_STATS();
        };
        uint64_t cost = run_concurrent(initFn, fn, endFn, concurrent);
        return cost;
    };

    // bench多次，取最大值、平均值、最小值
    bench_many_times(name, benchFn, ops_each_time, FLAGS_times);
}


// 分别用多少种并发来压测
std::vector<int> concurrent_list = {1, 2, 3, 4, 5, 10, 20};
int32_t run_bench() {
    for (auto concurrent : concurrent_list) {
        std::cout << "concurrent:" << concurrent << " threads -------------" << std::endl;
        switch(FLAGS_type) {
        case 1:
            bench_stl_enqueue("stl_enqueue", concurrent);
            break;
        case 2:
            bench_moody_enqueue("moody_enqueue", concurrent);
            break;
        case 3:
            bench_babylon_enqueue("babylon_enqueue", concurrent);
            break;
        case 4:
            bench_cbq_enqueue("cbq_enqueue", concurrent);
            break;
        default:
            bench_stl_enqueue("stl_enqueue", concurrent);
            bench_moody_enqueue("moody_enqueue", concurrent);
            bench_babylon_enqueue("babylon_enqueue", concurrent);
            bench_cbq_enqueue("cbq_enqueue", concurrent);
            break;
        }
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

