#include <assert.h>
#include "baidu/streaming_log.h"
#include "base/comlog_sink.h"
#include "base/strings/stringprintf.h"
#include "com_log.h"
#include "cronoapd.h"

#include <baidu/feed/mlarch/babylon/lite/concurrent/vector.h>


#undef DCHECK_IS_ON

#include <random>

#include "gflags/gflags.h"

#include "bench_common.h"
#include "concurrent/vector.h"

DEFINE_int32(ops_per_thread, 10000, "ops_per_thread");
DEFINE_int32(times, 10, "bench times");

using ::baidu::feed::mlarch::babylon::ConcurrentVector;

int intRand(const int & min, const int & max) {
    static thread_local std::mt19937 generator;
    std::uniform_int_distribution<int> distribution(min,max);
    return distribution(generator);
}

void vec_ensure_concurrent_random_read_bench(std::string name, int concurrent) {
    int ops_each_time = FLAGS_ops_per_thread * concurrent;
    // bench一次
    auto benchFn = [&]() -> uint64_t {
        std::vector<int> idxs;
        std::vector<size_t> vec;
        auto initFn = [&] {
            vec.reserve(FLAGS_ops_per_thread);
            for (int j = 0; j < FLAGS_ops_per_thread; ++j) {
                auto idx = intRand(0, FLAGS_ops_per_thread-1);
                idxs.push_back(idx);
            }
        };
        // 定义每个线程干的活
        auto fn = [&]() {
            size_t sum = 0;
            for (auto idx : idxs) {
                sum += vec[idx];
            }
        };
        auto endFn = [] {};
        uint64_t cost = run_concurrent(initFn, fn, endFn, concurrent);
        return cost;
    };

    // bench多次，取最大值、平均值、最小值
    bench_many_times(name, benchFn, ops_each_time, FLAGS_times);
}


void cvec_ensure_concurrent_random_read_bench(std::string name, int concurrent) {
    int ops_each_time = FLAGS_ops_per_thread * concurrent;
    // bench一次
    auto benchFn = [&]() -> uint64_t {
        std::vector<int> idxs;
        rcu::ConcurrentVector<size_t> vec;
        auto initFn = [&] {
            vec.reserve(FLAGS_ops_per_thread);
            for (int j = 0; j < FLAGS_ops_per_thread; ++j) {
                auto idx = intRand(0, FLAGS_ops_per_thread-1);
                idxs.push_back(idx);
            }
        };
        // 定义每个线程干的活
        auto fn = [&]() {
            size_t sum = 0;
            for (auto idx : idxs) {
                sum += vec[idx];
            }
        };
        auto endFn = [] {};
        uint64_t cost = run_concurrent(initFn, fn, endFn, concurrent);
        return cost;
    };

    // bench多次，取最大值、平均值、最小值
    bench_many_times(name, benchFn, ops_each_time, FLAGS_times);
}



void cvec_ensure_concurrent_random_write_bench(std::string name, int concurrent) {
    int ops_each_time = FLAGS_ops_per_thread * concurrent;
    // bench一次
    auto benchFn = [&]() -> uint64_t {
        std::vector<int> idxs;
        rcu::ConcurrentVector<size_t> vec;
        auto initFn = [&] {
            vec.reserve(FLAGS_ops_per_thread);
            for (int j = 0; j < FLAGS_ops_per_thread; ++j) {
                auto idx = intRand(0, FLAGS_ops_per_thread-1);
                idxs.push_back(idx);
            }
        };
        // 定义每个线程干的活
        auto fn = [&]() {
            for (auto idx : idxs) {
                vec.ensure(idx) = 5555;
            }
        };
        auto endFn = [] {};
        uint64_t cost = run_concurrent(initFn, fn, endFn, concurrent);
        return cost;
    };

    // bench多次，取最大值、平均值、最小值
    bench_many_times(name, benchFn, ops_each_time, FLAGS_times);
}

void cvec_snapshot_concurrent_random_write_bench(std::string name, int concurrent) {
    int ops_each_time = FLAGS_ops_per_thread * concurrent;
    // bench一次
    auto benchFn = [&]() -> uint64_t {
        std::vector<int> idxs;
        rcu::ConcurrentVector<size_t> vec;
        auto initFn = [&] {
            vec.reserve(FLAGS_ops_per_thread);
            for (int j = 0; j < FLAGS_ops_per_thread; ++j) {
                auto idx = intRand(0, FLAGS_ops_per_thread-1);
                idxs.push_back(idx);
            }
        };
        // 定义每个线程干的活
        auto fn = [&]() {
            auto snapshot = vec.snapshot();
            for (auto idx : idxs) {
                snapshot[idx] = 5555;
            }
        };
        auto endFn = [] {};
        uint64_t cost = run_concurrent(initFn, fn, endFn, concurrent);
        return cost;
    };

    // bench多次，取最大值、平均值、最小值
    bench_many_times(name, benchFn, ops_each_time, FLAGS_times);
}

void cvec_concurrent_random_write_bench(std::string name, int concurrent) {
    int ops_each_time = FLAGS_ops_per_thread * concurrent;
    // bench一次
    auto benchFn = [&]() -> uint64_t {
        std::vector<int> idxs;
        rcu::ConcurrentVector<size_t> vec;
        auto initFn = [&] {
            vec.reserve(FLAGS_ops_per_thread);
            for (int j = 0; j < FLAGS_ops_per_thread; ++j) {
                auto idx = intRand(0, FLAGS_ops_per_thread-1);
                idxs.push_back(idx);
            }
        };
        // 定义每个线程干的活
        auto fn = [&]() {
            for (auto idx : idxs) {
                vec[idx] = 5555;
            }
        };
        auto endFn = [] {};
        uint64_t cost = run_concurrent(initFn, fn, endFn, concurrent);
        return cost;
    };

    // bench多次，取最大值、平均值、最小值
    bench_many_times(name, benchFn, ops_each_time, FLAGS_times);
}

void vector_concurrent_random_write_bench(std::string name, int concurrent) {
    int ops_each_time = FLAGS_ops_per_thread * concurrent;
    // bench一次
    auto benchFn = [&]() -> uint64_t {
        std::vector<int> idxs;
        std::vector<size_t> vec(FLAGS_ops_per_thread);
        auto initFn = [&] {
            for (int j = 0; j < FLAGS_ops_per_thread; ++j) {
                auto idx = intRand(0, FLAGS_ops_per_thread-1);
                idxs.push_back(idx);
            }
        };
        // 定义每个线程干的活
        auto fn = [&]() {
            for (auto idx : idxs) {
                vec[idx] = 5555;
            }
        };
        auto endFn = [] {};
        uint64_t cost = run_concurrent(initFn, fn, endFn, concurrent);
        return cost;
    };

    // bench多次，取最大值、平均值、最小值
    bench_many_times(name, benchFn, ops_each_time, FLAGS_times);
}


void cvec_sequence_write_bench(std::string name) {
    // bench一次
    auto benchFn = [&]() -> uint64_t {
        auto initFn = [] {
        };
        // 定义每个线程干的活
        auto fn = [&]() {
            rcu::ConcurrentVector<size_t> vec;
            vec.reserve(FLAGS_ops_per_thread);
            for (int i = 0; i < FLAGS_ops_per_thread; i++) {
                vec[i] = 1;
            }
        };
        auto endFn = [] {};
        uint64_t cost = run_single(initFn, fn, endFn);
        return cost;
    };

    // bench多次，取最大值、平均值、最小值
    bench_many_times(name, benchFn, FLAGS_ops_per_thread, FLAGS_times);
}


void vector_sequence_write_bench(std::string name) {
    // bench一次
    auto benchFn = [&]() -> uint64_t {
        auto initFn = [] {};
        // 定义每个线程干的活
        auto fn = [&]() {
            std::vector<size_t> vec;
            vec.reserve(FLAGS_ops_per_thread);
            for (int i = 0; i < FLAGS_ops_per_thread; i++) {
                vec[i] = 1;
            }
        };
        auto endFn = [] {};
        uint64_t cost = run_single(initFn, fn, endFn);
        return cost;
    };

    // bench多次，取最大值、平均值、最小值
    bench_many_times(name, benchFn, FLAGS_ops_per_thread, FLAGS_times);
}

// 分别用多少种并发来压测
std::vector<int> concurrent_list = {1, 10};
int32_t run_bench() {
    std::cout << "non concurrent: -------------" << std::endl;
    vector_sequence_write_bench("vector_sequence_write_bench");
    cvec_sequence_write_bench("cvec_sequence_write_bench");

    for (auto concurrent : concurrent_list) {
        std::cout << "concurrent:" << concurrent << " threads -------------" << std::endl;
        vector_concurrent_random_write_bench("vector_concurrent_random_write_bench", concurrent);
        cvec_concurrent_random_write_bench("cvec_concurrent_random_write_bench", concurrent);
        cvec_snapshot_concurrent_random_write_bench("cvec_snapshot_concurrent_random_write_bench", concurrent);
        cvec_ensure_concurrent_random_write_bench("cvec_ensure_concurrent_random_write_bench", concurrent);

        vec_ensure_concurrent_random_read_bench("vec_ensure_concurrent_random_read_bench", concurrent);
        cvec_ensure_concurrent_random_read_bench("cvec_ensure_concurrent_random_read_bench", concurrent);
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

