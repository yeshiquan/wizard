#include <iostream>
#include <functional>
#include <thread>
#include <atomic>
#include <vector>
#include <iomanip>

// hw.cachelinesize: 128B
// 1个int = 4B
// 1个uint64_t = 8B
// 128B = 16个uint64_t

constexpr int M = 80;
constexpr int K = 80;
constexpr int N = 80;

int FLAGS_ops_per_thread = 100;
int FLAGS_times = 10;

using Matrix = std::vector<std::vector<uint64_t>>;

void printToCoordinates(int x, int y, const char *format, ...) {
    va_list args;
    va_start(args, format);
    printf("\033[%d;%dH", x, y);
    vprintf(format, args);
    va_end(args);
    fflush(stdout);
}

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

class MatrixMul {
public:
	void test1_ijk() {
		reset();
        for (int i = 0; i < M; ++i) {
            for (int j = 0; j < N; ++j) {
                for (int k = 0; k < K; ++k) {
                    C[i][j] += A[i][k] + B[k][j];
                }
            }
        }
    }

    void test2_jki() {
        reset();
        for (int j = 0; j < N; ++j) {
            for (int k = 0; k < K; ++k) {
                for (int i = 0; i < M; ++i) {
                    C[i][j] += A[i][k] + B[k][j];
                }
            }
        }
    }

    void test3_ikj() {
        // Cache局部性最好，性能最好
        reset();
        for (int i = 0; i < M; ++i) {
            for (int k = 0; k < K; ++k) {
                for (int j = 0; j < N; ++j) {
                    C[i][j] += A[i][k] + B[k][j];
                }
            }
        }
    }

    void test4_kij() {
        reset();
        for (int k = 0; k < K; ++k) {
            for (int i = 0; i < M; ++i) {
                for (int j = 0; j < N; ++j) {
                    C[i][j] += A[i][k] + B[k][j];
                }
            }
        }
    }


    void init() {
        A.resize(M);
        for (int i = 0; i < M; ++i) {
            A[i].resize(K);
            for (int j = 0; j < K; ++j) {
                A[i][j] = i+j;
            }
        }

        B.resize(K);
        for (int k = 0; k < K; ++k) {
            B[k].resize(N);
            for (int j = 0; j < N; ++j) {
                B[k][j] = k+j;
            }
        }

        C.resize(M);
        for (int i = 0; i < M; ++i) {
            C[i].resize(N);
        }
    }

    void reset() {
        for (int i = 0; i < M; ++i) {
            for (int j = 0; j < N; ++j) {
                C[i][j] = 0;
            }
        }
    }
public:
    Matrix A;
    Matrix B;
    Matrix C;
};

void test1_bench(std::string name, int concurrent) {
    int ops_each_time = FLAGS_ops_per_thread * concurrent;
    // bench一次
    auto benchFn = [&]() -> uint64_t {
        std::vector<int> idxs;
        MatrixMul mul;
        auto initFn = [&] {
            mul.init();
        };
        // 定义每个线程干的活
        auto fn = [&]() {
            for (int j = 0; j < FLAGS_ops_per_thread; ++j) {
                mul.test1_ijk();
            }
        };
        auto endFn = [] {};
        uint64_t cost = run_concurrent(initFn, fn, endFn, concurrent);
        return cost;
    };

    // bench多次，取最大值、平均值、最小值
    bench_many_times(name, benchFn, ops_each_time, FLAGS_times);
}

void test2_bench(std::string name, int concurrent) {
    int ops_each_time = FLAGS_ops_per_thread * concurrent;
    // bench一次
    auto benchFn = [&]() -> uint64_t {
        std::vector<int> idxs;
        MatrixMul mul;
        auto initFn = [&] {
            mul.init();
        };
        // 定义每个线程干的活
        auto fn = [&]() {
            for (int j = 0; j < FLAGS_ops_per_thread; ++j) {
                mul.test2_jki();
            }
        };
        auto endFn = [] {};
        uint64_t cost = run_concurrent(initFn, fn, endFn, concurrent);
        return cost;
    };

    // bench多次，取最大值、平均值、最小值
    bench_many_times(name, benchFn, ops_each_time, FLAGS_times);
}


void test3_bench(std::string name, int concurrent) {
    int ops_each_time = FLAGS_ops_per_thread * concurrent;
    // bench一次
    auto benchFn = [&]() -> uint64_t {
        std::vector<int> idxs;
        MatrixMul mul;
        auto initFn = [&] {
            mul.init();
        };
        // 定义每个线程干的活
        auto fn = [&]() {
            for (int j = 0; j < FLAGS_ops_per_thread; ++j) {
                mul.test3_ikj();
            }
        };
        auto endFn = [] {};
        uint64_t cost = run_concurrent(initFn, fn, endFn, concurrent);
        return cost;
    };

    // bench多次，取最大值、平均值、最小值
    bench_many_times(name, benchFn, ops_each_time, FLAGS_times);
}

void test4_bench(std::string name, int concurrent) {
    int ops_each_time = FLAGS_ops_per_thread * concurrent;
    // bench一次
    auto benchFn = [&]() -> uint64_t {
        std::vector<int> idxs;
        MatrixMul mul;
        auto initFn = [&] {
            mul.init();
        };
        // 定义每个线程干的活
        auto fn = [&]() {
            for (int j = 0; j < FLAGS_ops_per_thread; ++j) {
                mul.test4_kij();
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
std::vector<int> concurrent_list = {1, 10};
int32_t run_bench() {
    for (auto concurrent : concurrent_list) {
        std::cout << "concurrent:" << concurrent << " threads -------------" << std::endl;
        test4_bench("test4_kij_bench", concurrent);
        test3_bench("test3_ikj_bench", concurrent);
        test2_bench("test2_jki_bench", concurrent);
        test1_bench("test1_ijk_bench", concurrent);
    }
    return 0;
}

int main() {
    run_bench();
    return 0;
}
