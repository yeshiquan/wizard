#include <iostream>
#include <chrono>
#include <future>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <functional>
#include <thread>
#include <vector>

enum class TimerType {
    EXACT_TIMER,  // 精确定时器，把任务本身的耗时也会算到等待的时间
    APPROXIMATE_TIMER, // 近似精确的定时器，不考虑任务本身的耗时
};

class Timer {
public:
    explicit Timer(uint32_t interval, TimerType timer_type)  noexcept
            : _interval(interval), _timer_type(timer_type) {}

    Timer(Timer const&) = delete;             // Copy construct
    Timer(Timer&&) = delete;                  // Move construct
    Timer& operator=(Timer const&) = delete;  // Copy assign
    Timer& operator=(Timer &&) = delete;      // Move assign

    // 启动1个新的线程定时异步执行任务，不能多次调用
    template<class F, class... Args>
    void exec(F&& f, Args&&... args) noexcept {
        _func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        _running = true;
        _thread = std::thread([&]() {
            while (_running) {
                auto wake_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(_interval);
                _func();
                if (_timer_type == TimerType::EXACT_TIMER) {
                    std::this_thread::sleep_until(wake_time);
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(_interval));
                }
            }
        });
        //std::cout << "detach()..." << std::endl;
        _thread.detach();
    }

    // 延迟等待一段时间，然后启动1个新的线程异步执行任务
    template<class F, class... Args>
    void delay(F&& f, Args&&... args) noexcept {
        //TODO 增加实现
    }

    void stop() noexcept {
        bool expected_running = true;
        // 只有任务本身处于运行状态才会停止，并且只有1个线程能竞争成功，停止这个任务
        if (_running.compare_exchange_strong(expected_running, false, std::memory_order_seq_cst)) {
            std::cout << "I'am going to stop, wait for task exit..." << std::endl;
            if (_thread.joinable()) {
                std::cout << "thread can joinable" << std::endl;
                _thread.join();
            } else {
                std::cout << "thread can not joinable" << std::endl;
            }
        } else {
            std::cout << "task is stopped by other thread,do nothing" << std::endl;
        }
    }
   
    ~Timer() noexcept {
        std::cout << "~Timer()" << std::endl;
        stop();
    }

private:
    std::function<void(void)> _func;
    uint32_t _interval = {1000};
    std::thread _thread;
    TimerType  _timer_type = {TimerType::APPROXIMATE_TIMER};
    std::atomic<bool> _running = {false};
};

// Thread safe Timer Manager
class TimerManager {
public:
    TimerManager() = default;            
    TimerManager(TimerManager const&) = delete;             // Copy construct
    TimerManager(TimerManager&&) = delete;                  // Move construct
    TimerManager& operator=(TimerManager const&) = delete;  // Copy assign
    TimerManager& operator=(TimerManager &&) = delete;      // Move assign

    static TimerManager& instance() {
        static TimerManager instance;
        return instance;
    }

    std::shared_ptr<Timer> add_timer(std::string key, uint32_t interval, 
                    TimerType timer_type = TimerType::APPROXIMATE_TIMER) noexcept {
        std::lock_guard<std::mutex> guard(_mutex);

        // 先删除原来的timer
        auto iter = _timer_map.find(key);
        if (iter != _timer_map.end()) {
            _timer_map.erase(iter);
        }

        auto timer = std::make_shared<Timer>(interval, timer_type);
        _timer_map.emplace(std::move(key), timer);
        return timer;
    }

    void remove_timer(const std::string& key) noexcept {
        std::lock_guard<std::mutex> guard(_mutex);
        std::cout << "remove_timer" << std::endl;
        _timer_map.erase(key);
    }

    ~TimerManager() {
        std::lock_guard<std::mutex> guard(_mutex);
        _timer_map.clear();
    }
private:
    std::unordered_map<std::string, std::shared_ptr<Timer>> _timer_map;
    std::mutex _mutex;
};

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

    int timer_async_callback_mock(std::shared_ptr<AIActionCommon> common) {
        std::cout << "timer_async_callback_mock course_id -> " << common->course_id << std::endl;
        return 1;
    }
};

int main() {
    /*
    {   // Test Timer
        std::cout << "Test Timer =======================" << std::endl;
        Timer t1(1000, TimerType::APPROXIMATE_TIMER);
        AiClient ai_client;
        t1.exec(&AiClient::do_something, &ai_client, "Hello, World");
        std::cout << "start done" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
        std::cout << "stop..." << std::endl;
        t1.stop();
        std::cout << "stop done" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    {   // Test TimerManager
        std::cout << "\nTest TimerManager =======================" << std::endl;
        AiClient ai_client;

        // 添加一个间隔1s的定时器，并且执行任务
        TimerManager::instance().add_timer("unique_id_1", 1000)->exec(&AiClient::do_something, &ai_client, "Hello World");

        // 添加一个间隔500ms的定时器，并且执行任务
        auto common = std::make_shared<AIActionCommon>();
        common->course_id = "5893984";
        TimerManager::instance().add_timer("unique_id_2", 500)->exec(&AiClient::timer_async_callback_mock, &ai_client, common);

        std::this_thread::sleep_for(std::chrono::milliseconds(4000));
        TimerManager::instance().remove_timer("unique_id_1");

        std::this_thread::sleep_for(std::chrono::milliseconds(4000));
        TimerManager::instance().remove_timer("unique_id_2");

        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    }
    */

    {   // Stress Test
        AiClient ai_client;
        std::vector<std::thread> threads;
        for (size_t i = 0; i < 10; ++i) {
            threads.emplace_back([&, i] {
                for (int i = 0; i < 20; i++) {
                    auto common = std::make_shared<AIActionCommon>();
                    common->course_id = "5893984";
                    TimerManager::instance().add_timer(std::to_string(i), 100)->exec(&AiClient::timer_async_callback_mock, &ai_client, common);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(4000));
                for (int i = 0; i < 20; i++) {
                    TimerManager::instance().remove_timer(std::to_string(i));
                }
            });
        }

        for (auto& th : threads) {
            th.join();
        }
    }

    /*
    {   // Test Concurrent stop
        std::cout << "\nTest Concurrent stop =======================" << std::endl;
        AiClient ai_client;
        auto timer = TimerManager::instance().add_timer("unique_id_1", 1000, TimerType::EXACT_TIMER);
        timer->exec(&AiClient::do_something, &ai_client, "Hello World");
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));

        std::vector<std::thread> threads;
        for (size_t i = 0; i < 10; ++i) {
            threads.emplace_back([&, i] {
                timer->stop();
            });
        }
        for (auto& th : threads) {
            th.join();
        }
        timer->stop();
    }
        */

    return 0;
}
