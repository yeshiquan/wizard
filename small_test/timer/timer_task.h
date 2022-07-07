#pragma once

#include <chrono>
#include <atomic>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <vector>
#include <time.h>
#include <cstring>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <sys/eventfd.h>

namespace base::timer {

// 表示一个时间点，类似时间戳
using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;

class Task {
public:
    explicit Task(std::string key_, TimePoint expired_pt_, uint32_t interval_)  noexcept
            : key(key_), expired_pt(expired_pt_), interval(interval_) {}

    Task(Task const&) = delete;             // Copy construct
    Task(Task&&) = delete;                  // Move construct
    Task& operator=(Task const&) = delete;  // Copy assign
    Task& operator=(Task &&) = delete;      // Move assign

    template<class F, class... Args>
    bool exec(F&& f, Args&&... args) noexcept {
        _func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        is_ready.store(true, std::memory_order_release);
        return true;
    }

    void update_expired_pt(TimePoint& now) {
        expired_pt = now + std::chrono::milliseconds(interval);
    }

    void run() {
        _func();
    }

private:
    std::function<void(void)> _func;
public:
    uint32_t interval = {1000};
    TimePoint expired_pt;
    std::string key;
    std::atomic<bool> is_ready{false};
};

// 一个wakeup_fd, 一个task_fd
constexpr size_t kMaxEvent = 2;

// Thread safe Task Manager
class TimerTask {
public:
    TimerTask(TimerTask const&) = delete;             // Copy construct
    TimerTask(TimerTask&&) = delete;                  // Move construct
    TimerTask& operator=(TimerTask const&) = delete;  // Copy assign
    TimerTask& operator=(TimerTask &&) = delete;      // Move assign

    static TimerTask& instance() noexcept {
        static TimerTask instance;
        return instance;
    }

    // 每隔interval毫秒执行一次任务
    template<class F, class... Args>
    bool run_every(uint32_t interval, F&& f, Args&&... args) noexcept {
        //CHECK(interval > 0);
        uint32_t task_id = anonymous_task_id.fetch_add(std::memory_order_relaxed);
        std::string key = "anonymous_task_" + std::to_string(task_id);
        add_task(key, interval)->exec(std::forward<F>(f), std::forward<Args>(args)...);
        return true;
    }

    // 指定将来某个时间点执行一次任务
    template<class F, class... Args>
    bool run_at(TimePoint expired_pt, F&& f, Args&&... args) noexcept {
        uint32_t task_id = anonymous_task_id.fetch_add(std::memory_order_relaxed);
        std::string key = "anonymous_task_" + std::to_string(task_id);
        _add_task_internal(key, expired_pt, 0)->exec(std::forward<F>(f), std::forward<Args>(args)...);
        return true;
    }

    // 指定delay_ms毫秒后执行一次任务
    template<class F, class... Args>
    bool run_after(uint32_t delay_ms, F&& f, Args&&... args) noexcept {
        //CHECK(delay_ms > 0);
        TimePoint now = std::chrono::steady_clock::now();
        TimePoint expired_pt = now + std::chrono::milliseconds(delay_ms);
        uint32_t task_id = anonymous_task_id.fetch_add(std::memory_order_relaxed);
        std::string key = "anonymous_task_" + std::to_string(task_id);
        _add_task_internal(key, expired_pt, 0)->exec(std::forward<F>(f), std::forward<Args>(args)...);
        return true;
    }

    // 添加1个名字为key，间隔为interval毫秒的定时器
    std::shared_ptr<Task> add_task(std::string key, uint32_t interval) noexcept;

    // 根据key删除定时器
    void remove_task(const std::string& key) noexcept;

    void loop() noexcept;
    void stop() noexcept;
    void clear() noexcept;
    ~TimerTask();

private:
    TimerTask() noexcept;
    void _wakeup() noexcept;
    void _check_and_run_tasks() noexcept;
    void _poll() noexcept;
    struct timespec _how_much_time_left(TimePoint expired_pt) noexcept;
    void _setup_a_task(int task_fd, TimePoint expired_pt) noexcept;
    int _create_event_fd() noexcept;
    std::shared_ptr<Task> _add_task_internal(std::string key, 
                    TimePoint expired_pt, uint32_t interval) noexcept;

private:
    static std::atomic<uint32_t> anonymous_task_id;
    std::unordered_map<std::string, std::shared_ptr<Task>> _task_map;
    std::mutex _mutex;
    int _task_fd = -1;
    int _epoll_fd = -1;
    int _wakeup_fd = -1;
    struct epoll_event _ev;
    std::vector<struct epoll_event> _event_vec;
    std::atomic<bool> _is_stop = false;
};

} // namespace
