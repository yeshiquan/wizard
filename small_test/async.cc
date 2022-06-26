#include <thread>
#include <iostream>
#include <functional>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <random>
#include <atomic>
#include <queue>
#include <future>
#include <memory>

namespace safe_queue_ns {

template <typename T>
class SafeQueue {
private:
    std::queue<T> m_queue; //利用模板函数构造队列
    std::mutex m_mutex; // 访问互斥信号量
public:
    SafeQueue() {}
    SafeQueue(SafeQueue &&other) {}
    ~SafeQueue() {}

    /**
     * 返回队列是否为空
     * @return
     */
    bool empty() {
        // 互斥信号变量加锁，防止m_queue被改变
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    /**
     * 返回队列长度
     * @return
     */
    int size() {
        // 互斥信号变量加锁，防止m_queue被改变
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

    /**
     * 队列添加元素
     */
    void enqueue(T& t) {
        // 互斥信号变量加锁，防止m_queue被改变
        std::unique_lock<std::mutex> lock(m_mutex);
        m_queue.emplace(t);
    }

    /**
     * 队列取出元素
     * @param t
     * @return
     */
    bool dequeue(T &t) {
        // 互斥信号变量加锁，防止m_queue被改变
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_queue.empty())
            return false;
        t = std::move(m_queue.front()); // 取出队首元素，返回队首元素值，并进行右值引用
        std::cout << "trace 1..." << std::endl;
        m_queue.pop(); // 弹出入队的第一个元素

        return true;
    }
};
} // namespace

namespace thread_pool_ns {

template<size_t WorkerNum = 20>
class ThreadPool {
private:
    // 内置线程工作类
    class ThreadWorker {
    private:
        int m_id; // 工作id
        ThreadPool *m_pool; // 所属线程池
    public:
        // 构造函数
        ThreadWorker(ThreadPool *pool, const int id) : m_pool(pool), m_id(id) {
        }

        // 重载()操作
        void operator()() {
            std::function<void()> func; // 定义基础函数类func
            bool dequeued; // 是否正在取出队列中元素
            while (!m_pool->m_shutdown) {
                {
                    // 为线程环境加锁，互访问工作线程的休眠和唤醒
                    std::unique_lock<std::mutex> lock(m_pool->m_conditional_mutex);
                    // 如果任务队列为空，阻塞当前线程
                    if (m_pool->m_queue.empty()) {
                        std::cout << "wait task..." << std::endl;
                        m_pool->m_conditional_lock.wait(lock); // 等待条件变量通知，开启线程
                        std::cout << "got task" << std::endl;
                    }
                    // 取出任务队列中的元素
                    dequeued = m_pool->m_queue.dequeue(func);
                }
                // 如果成功取出，执行工作函数
                if (dequeued) {
                    m_pool->idle_thread_num.fetch_sub(1, std::memory_order_relaxed);
                    try {
                        std::cout << "trace 2..." << std::endl;
                        func();
                        std::cout << "trace 3..." << std::endl;
                    } catch (std::future_error&) {
                    }
                    m_pool->idle_thread_num.fetch_add(1, std::memory_order_relaxed);
                }
            }
        }
    };

    ThreadPool() :
            m_threads(std::vector<std::thread>(WorkerNum)) {
        init();
    }

    std::atomic_int idle_thread_num = {0};
    std::atomic<bool> m_shutdown = {false}; // 线程池是否关闭
    safe_queue_ns::SafeQueue<std::function<void()>> m_queue; // 执行函数安全队列，即任务队列
    std::vector<std::thread> m_threads; // 工作线程队列
    std::mutex m_conditional_mutex; // 线程休眠锁互斥变量
    std::condition_variable m_conditional_lock; // 线程环境锁，可以让线程处于休眠或者唤醒状态
public:
    // 线程池构造函数
    static ThreadPool* instance() {
        static std::unique_ptr<ThreadPool> ins(new ThreadPool());
        return ins.get();
    }

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;

    /**
     * 初始化线程池
     */
    void init() {
        idle_thread_num = m_threads.size();
        for (int i = 0; i < m_threads.size(); ++i) {
            // 分配工作线程
            m_threads.at(i) = std::thread(ThreadWorker(this, i));
        }
    }

    /**
     * 获取空闲线程数
     * @return int
     */
    int get_idle_thread_num() {
        return idle_thread_num;
    }

    /**
     * 退出
     */
    void shutdown() {
        bool expected_value = false;
        if (m_shutdown.compare_exchange_strong(expected_value, true, std::memory_order_seq_cst)) {
            // 通知-唤醒所有工作线程
            m_conditional_lock.notify_all();
            for (int i = 0; i < m_threads.size(); ++i) {
                // 判断线程是否在等待
                if (m_threads.at(i).joinable()) {
                    // 将线程加入到等待队列
                    m_threads.at(i).join();
                }
            }
            std::cout << "shutdown" << std::endl;
        } else {
            std::cout << "do nothing" << std::endl;
        }
    }

    ~ThreadPool() {
        shutdown();
    }

    // Submit a function to be executed asynchronously by the pool
    template <typename F, typename... Args>
    auto submit(F &&f, Args &&...args) -> std::future<typename std::result_of<F(Args...)>::type>
    {
        using return_type = typename std::result_of<F(Args...)>::type;
        // Encapsulate it into a shared pointer in order to be able to copy construct
        // 连接函数和参数定义，特殊函数类型，避免左右值错误
        auto task_ptr = std::make_unique<std::packaged_task<return_type()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        // Warp packaged task into void function
        std::function<void()> warpper_func = [task_ptr]() {
            (*task_ptr)();
        };
        // 队列通用安全封包函数，并压入安全队列
        m_queue.enqueue(warpper_func);
        // 唤醒一个等待中的线程, 无等待即阻塞
        m_conditional_lock.notify_one();
        // 返回先前注册的任务指针
        return task_ptr->get_future();
    }
};

// 实现2个语义更明确的异步语法糖

// 多线程并发异步执行任务
template <typename F, typename... Args>
auto async_exec(F &&f, Args &&...args) -> std::future<typename std::result_of<F(Args...)>::type> {
    return ThreadPool<>::instance()->submit(std::forward<F>(f), std::forward<Args>(args)...);
}

// 单线程串行异步执行任务
template <typename F, typename... Args>
auto single_async_exec(F &&f, Args &&...args) -> std::future<typename std::result_of<F(Args...)>::type> {
    return ThreadPool<1>::instance()->submit(std::forward<F>(f), std::forward<Args>(args)...);
}

} // namespace

struct AIActionCommon {
    bool is_debug;
    std::string course_id;
    int action_id;
    std::string source;
    std::string action_type;
};

class AiClient {
public:
    int do_something(std::string message) {
        std::cout << "message -> " << message << std::endl;
        return 2;
    }

    int sync_update(std::shared_ptr<AIActionCommon> common) {
        std::cout << "sync_update course_id -> " << common->course_id << std::endl;
        return 1;
    }
};

using thread_pool_ns::async_exec;
using thread_pool_ns::single_async_exec;
using thread_pool_ns::ThreadPool;

class LogData {
public:
    LogData() {
        std::cout << "LogData()" << std::endl;
    }
    ~LogData() {
        std::cout << "~LogData()" << std::endl;
    }
    LogData(LogData const& o) {
        line = o.line;
        std::cout << "LogData Copy construct()" << std::endl;
    }
    LogData(LogData&& o) {
        std::cout << "LogData Move construct()" << std::endl;
        line = std::move(o.line);
    }
    LogData& operator=(LogData const& o) {
        std::cout << "LogData Copy assign()" << std::endl;
        line = o.line;
        return *this;
    }
    LogData& operator=(LogData && o) {
        std::cout << "LogData Move assign()" << std::endl;
        line = std::move(o.line);
        return *this;
    }
public:
    std::string line;
};

void do_log(const LogData& logdata) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "log line -> " << logdata.line << std::endl;
}

void test_log_data() {
    LogData logdata;
    logdata.line = "load model failed!";
    auto future = std::async(do_log, std::move(logdata));
    std::cout << "trace 2..." << std::endl;
    future.get();
}

int main() {
    {
        ThreadPool<>::instance();
        ThreadPool<>::instance()->shutdown();
    }
    /*
    {
        AiClient ai_client;
        async_exec(&AiClient::do_something, &ai_client, "Hello, World");

        auto common = std::make_shared<AIActionCommon>();
        common->course_id = "5893984";
        auto future = single_async_exec(&AiClient::sync_update, &ai_client, common);
        std::cout << "ret -> " << future.get() << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    }

    {
        AiClient ai_client;
        ThreadPool<>::instance()->submit(&AiClient::do_something, &ai_client, "Hello, World");

        std::vector<std::thread> threads;
        for (size_t i = 0; i < 10; ++i) {
            threads.emplace_back([&, i] {
                ThreadPool<>::instance()->shutdown();
            });
        }
        for (auto& th : threads) {
            th.join();
        }
        ThreadPool<>::instance()->shutdown();
    }

    test_log_data();
    */
    return 0;
}
