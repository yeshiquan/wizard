#include <thread>
#include <iostream>
#include <functional>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <random>
#include <atomic>

// Executor，仅仅提供接口
class Executor {
public:
    Executor() = default;

    virtual ~Executor() = default;

    virtual void add(std::function<void()> fn) = 0;
};

// 实际的线程池，Executor的具体表现，可以忽略这部分的具体实现
class ThreadPool : public Executor {
public:
    explicit ThreadPool(size_t threadCnt = 4) : m_threadCnt(4), m_stop(false) {
        std::thread([this]() { run(); }).detach(); // 后台启动
    }

    ~ThreadPool() override {
        m_stop = true;
        m_cond.notify_all();
        for (auto &t: m_threads) {
            if (t.joinable()) {
                t.join();
            }
        }
    }

    ThreadPool(ThreadPool &) = delete;

    ThreadPool(ThreadPool &&) = delete;

    void add(std::function<void()> fn) override {
        m_mtx.lock();
        m_tasks.emplace_back(std::move(fn));
        m_mtx.unlock();
        m_cond.notify_one();
    }

private:
    void run() {
        for (size_t i = 0; i < m_threadCnt; ++i) {
            m_threads.emplace_back(std::thread([this]() {
                for (;;) {
                    std::unique_lock<std::mutex> lck(m_mtx);
                    m_cond.wait(lck, [this]() -> bool { return m_stop || !m_tasks.empty(); });
                    if (m_stop) {
                        return;
                    }
                    auto &fn = m_tasks.front();
                    m_tasks.pop_front();
                    lck.unlock(); // 一定要释放掉锁，否则无法并发

                    fn();
                }
            }));
        }
    }

private:
    std::vector<std::thread> m_threads;
    std::deque<std::function<void()>> m_tasks;
    std::condition_variable m_cond;
    std::mutex m_mtx;
    std::atomic<bool> m_stop{false};
    size_t m_threadCnt;
};

// 这里泛型提供了使用Exec的例子
template<typename Fn, typename Exec = Executor>
void async_exec(Fn fn, Exec &exec) {
    exec.add(fn);
}

// 工具函数，获取随机数
inline int get_random(int low, int high) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(low, high);
    return distrib(gen);
}

void foo(int n) {
    int t = get_random(100, 5000);
    std::this_thread::sleep_for(std::chrono::milliseconds(t)); // 模拟执行任务
    std::cout << "foo finish task " << n << " with " << t << " ms\n";
}

int main() {
    ThreadPool tp(4);
    for (int i = 0; i < 10; ++i) {
        async_exec([i]() { foo(i); }, tp); // 指定实际的Executor执行
    }
    std::this_thread::sleep_for(std::chrono::seconds(15));  // 休眠15s等所有任务完成
    std::cout << "stop everything\n";
    return 0;
}
