#pragma once

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

using ConcurrentBoundedQueue;

namespace duer::vc {

class ThreadPool {
public:
    explicit ThreadPool(uint32_t, uint32_t, bool is_graceful_stop = true) noexcept;

    ThreadPool(ThreadPool const&) = delete;             // Copy construct
    ThreadPool(ThreadPool&&) = delete;                  // Move construct
    ThreadPool& operator=(ThreadPool const&) = delete;  // Copy assign
    ThreadPool& operator=(ThreadPool &&) = delete;      // Move assign

    ~ThreadPool() noexcept;

    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) noexcept
        -> std::future<typename std::result_of<F(Args...)>::type>;

    int32_t enqueue(std::function<void(void)>&& function) noexcept;
    size_t task_size() noexcept;
    void wait_task_finish() noexcept;
    void stop_and_wait() noexcept;
private:
    std::condition_variable condition;
    std::mutex mutex;
    bool is_stop = {false};
    bool graceful_stop = {true};
    std::vector< std::thread > workers;
    ConcurrentBoundedQueue<std::function<void(void)>> tasks;
};

inline size_t ThreadPool::task_size() noexcept
{
    return tasks.size();
}
 
inline ThreadPool::ThreadPool(uint32_t threads, uint32_t queue_size, bool is_graceful_stop) noexcept
                    : graceful_stop(is_graceful_stop) 
{
    constexpr size_t batch_num = 10;
    using IT = ConcurrentBoundedQueue<std::function<void(void)>>::Iterator;
    tasks.reserve_and_clear(queue_size);
    for(uint32_t i = 0; i < threads; ++i) {
        workers.emplace_back([this] {
            while (true) {
                {
                    std::unique_lock<std::mutex> lock(this->mutex);
                    this->condition.wait(lock, [this]{
                        return this->is_stop || this->tasks.size() > 0;
                    });
                    if (this->is_stop) {
                        if (this->graceful_stop) {
                            // 等待所有任务都完成才退出
                            if (this->tasks.size() == 0) {
                                return;
                            }
                        } else {
                            // 收到信号直接退出，未完成的任务丢弃
                            return;
                        }
                    }
                }
                // 取出的task可能少于batch_num，队列为空时不会阻塞
                this->tasks.try_pop_n<true, true>([&] (IT iter, IT end) {
                    while (iter != end) {
                        (*iter++)();
                    }
                }, batch_num);    
            }
        });
    }
}

template<class F, class... Args>
inline auto ThreadPool::enqueue(F&& f, Args&&... args) noexcept
    -> std::future<typename std::result_of<F(Args...)>::type>
{
    using return_type = typename std::result_of<
            typename ::std::decay<F>::type(
                typename ::std::decay<Args>::type...)>::type;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
        
    std::future<return_type> res = task->get_future();
    // 队列满了会阻塞
    tasks.push([task](){ (*task)(); });

    condition.notify_one();

    return res;
}

inline int32_t ThreadPool::enqueue(std::function<void(void)>&& function) noexcept 
{
    tasks.push(std::move(function));
    condition.notify_one();
    return 0;
}


inline void ThreadPool::stop_and_wait() noexcept
{
    {
        std::unique_lock<std::mutex> lock(this->mutex);
        if (is_stop) {
            return;
        }
        is_stop = true;
    }
    LOG(NOTICE) << "ThreadPool is going to stop...left task_size:" << tasks.size();
    condition.notify_all();
    for(std::thread& worker: workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    LOG(NOTICE) << "ThreadPool stopped, left task_size:" << tasks.size();
}

inline ThreadPool::~ThreadPool() noexcept
{
    LOG(NOTICE) << "~ThreadPool";
    stop_and_wait();
}

} // namespace
