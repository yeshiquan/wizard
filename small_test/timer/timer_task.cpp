#include "timer_task.h"
#include <iostream>
#include "logging.h"


namespace base::timer {

TimerTask::TimerTask() noexcept {
    _event_vec.resize(kMaxEvent);
    _task_fd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (_task_fd == -1) {
        LOGW("timerfd_create() failed: errno=%d", errno);
    }

    _epoll_fd = epoll_create(EPOLL_CLOEXEC);
    if (_epoll_fd == -1) {
        LOGW("epoll_create() failed: errno=%d", errno);
    }

    _ev.events = EPOLLIN | EPOLLET;
    //_ev.events = EPOLLIN;
    if (::epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _task_fd, &_ev) == -1) {
        LOGW("epoll_ctl(ADD) failed: errno=%d", errno);
    }

    _wakeup_fd = _create_event_fd();
    memset(&_ev, 0, sizeof(_ev));
    _ev.events = EPOLLIN | EPOLLET;
    //_ev.events = EPOLLIN;
    if (::epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _wakeup_fd, &_ev) == -1) {
        LOGW("epoll_ctl(ADD) failed: errno=%", errno);
    }
}

std::shared_ptr<Task> TimerTask::add_task(std::string key, uint32_t interval) noexcept {
    //CHECK(interval > 0);
    TimePoint now = std::chrono::steady_clock::now();
    TimePoint expired_pt = now + std::chrono::milliseconds(interval);
    return _add_task_internal(key, expired_pt, interval);
}

void TimerTask::loop() noexcept {
    while (!_is_stop) {
        _poll();
        if (_is_stop)  break;
        _check_and_run_tasks();
        //std::cout << "-----------" << std::endl;
    }
}

void TimerTask::stop() noexcept {
    if (_is_stop) return;
    _is_stop = true;
    _wakeup();
}

void TimerTask::remove_task(const std::string& key) noexcept {
    std::lock_guard<std::mutex> guard(_mutex);
    //std::cout << "remove_task" << std::endl;
    _task_map.erase(key);
}

void TimerTask::clear() noexcept {
    std::lock_guard<std::mutex> guard(_mutex);
    _task_map.clear();        
    if (_task_fd > 0) {
        if(epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, _task_fd, NULL) != 0){
            SLOGW << "epoll_ctl delete task_fd failed";
        }
        close(_task_fd);
    }
    if (_wakeup_fd > 0) {
        if(epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, _wakeup_fd, NULL) != 0){
            SLOGW << "epoll_ctl delete wakeup_fd failed";
        }
        close(_wakeup_fd);
    } 
    if (_epoll_fd > 0) {
        close(_epoll_fd);
    }
}

TimerTask::~TimerTask() {
    clear();
}

void TimerTask::_wakeup() noexcept {
    uint64_t u = 0x0608; // 值无意义
    auto n = ::write(_wakeup_fd, &u, sizeof(uint64_t));
    if (n != sizeof(uint64_t)) {
        SLOGW << "wakeup failed";
    } else {
        //std::cout << "wakeup success" << std::endl;
    }
}

void TimerTask::_check_and_run_tasks() noexcept {
    //std::cout << "_check_and_run_tasks...task_size" << _task_map.size() << std::endl;
    TimePoint now = std::chrono::steady_clock::now();
    std::vector<std::string> to_delete_keys;

    std::lock_guard<std::mutex> guard(_mutex);
    to_delete_keys.reserve(_task_map.size());
    for (auto& [key, task] : _task_map) {
        if (task->is_ready.load(std::memory_order_acquire) && now >= task->expired_pt) {
            // TODO: 尝试改成异步执行任务
            // 执行任务的过程中如果耗时很长，停止、添加、删除计时器都会被阻塞
            task->run();
            if (task->interval <= 0) {
                to_delete_keys.emplace_back(key);
            } else {
                task->update_expired_pt(now);
            }
        } 
    }

    // 删除那些一次性的定时任务
    for (auto& key : to_delete_keys) {
        _task_map.erase(key);
    }

    // 从所有task中找出最早的时间点，作为下一个定时点
    TimePoint earliest_expired_pt = TimePoint::max();
    for (auto& [key, task] : _task_map) {
        if (task->expired_pt < earliest_expired_pt) {
            earliest_expired_pt = task->expired_pt;
        }	
    }
    if (earliest_expired_pt != TimePoint::max()) {
        _setup_a_task(_task_fd, earliest_expired_pt);
    }
}

void TimerTask::_poll() noexcept {
    constexpr int timeout_ms = 1000000;
    memset(&_event_vec[0], 0, _event_vec.size() * sizeof(struct epoll_event));
    int num_events = ::epoll_wait(_epoll_fd, &_event_vec[0], kMaxEvent, timeout_ms);
    int saved_errno = errno;
    if (num_events > 0) {
        uint64_t res;
        int ret = read(_task_fd, &res, sizeof(res));
        //printf("read() returned %d, res=%lld\n", ret, res);
    } else if (num_events == 0) {
        //std::cout << "epoll nothing happened" << std::endl;
    } else {
        if (saved_errno != EINTR) {
            errno = saved_errno;
            SLOGW << "epoll wait error:" << saved_errno;
        }
    }
}

struct timespec TimerTask::_how_much_time_left(TimePoint expired_pt) noexcept {
    constexpr long kMinGapMilliSeconds = 100;
    TimePoint now = std::chrono::steady_clock::now();
    auto duration = expired_pt - now;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    //std::cout << "ms -> " << ms << std::endl;
    if (ms < kMinGapMilliSeconds) {
        // 非常关键，否则很容易导致定时器断掉(因为下一次定时设置失败)
        ms = kMinGapMilliSeconds;
    }

    return timespec {
            .tv_sec = static_cast<time_t>(ms / 1000),
            .tv_nsec = static_cast<long>(ms % 1000 * 1000000)
    };
}

void TimerTask::_setup_a_task(int task_fd, TimePoint expired_pt) noexcept {
    struct itimerspec new_value;
    struct itimerspec old_value;

    memset(&new_value, 0, sizeof(new_value));
    memset(&old_value, 0, sizeof(old_value));
    
    new_value.it_value = _how_much_time_left(expired_pt);
    //std::cout << "timerfd_settime sec:" << new_value.it_value.tv_sec << " nsec:" << new_value.it_value.tv_nsec << std::endl;

    int ret = ::timerfd_settime(task_fd, 0, &new_value, &old_value);
    if (ret != 0) {
        SLOGW << "timerfd_settime() failed" 
                << " sec:" << new_value.it_value.tv_sec 
                << " nsec:" << new_value.it_value.tv_nsec;    
    }
}

int TimerTask::_create_event_fd() noexcept {
    int event_fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (event_fd < 0) {
        SLOGW << "Failed in eventfd()";
    }
    return event_fd;
}

std::shared_ptr<Task> TimerTask::_add_task_internal(std::string key, 
                        TimePoint expired_pt, uint32_t interval) noexcept {
    std::lock_guard<std::mutex> guard(_mutex);

    // 先删除原来的task
    auto iter = _task_map.find(key);
    if (iter != _task_map.end()) {
        _task_map.erase(iter);
    }

    bool need_update_taskfd = _task_map.empty();
    for (auto& [key, task] : _task_map) {
        if (expired_pt < task->expired_pt) {
            need_update_taskfd = true;
            break;
        }	
    }
    if (need_update_taskfd) {
        _setup_a_task(_task_fd, expired_pt);
    }

    auto task = std::make_shared<Task>(key, expired_pt, interval);
    _task_map.emplace(std::move(key), task);
    return task;
}

std::atomic<uint32_t> TimerTask::anonymous_task_id = 1;

} // namespace
