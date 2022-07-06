#include <iostream>
#include "timer_task.h"
#include <sys/time.h>
#include <random>
#include <thread>

using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

// 线程安全, [min,max]区间的随机数
int intRand(const int & min, const int & max) {
    static thread_local std::mt19937 generator;
    std::uniform_int_distribution<int> distribution(min,max);
    return distribution(generator);
}

struct Foobar {
    void do_something(std::string msg) {
        char buff[20];
        time_t now = time(0);
        strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));
        buff[19] = '\0';
        printf(">>> %lld  %s do_something msg:[%s]\n",now, buff, msg.c_str());
        //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    void do_badthing(std::string msg) {
        char buff[20];
        time_t now = time(0);
        strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));
        buff[19] = '\0';
        printf(">>> %lld  %s do_badthing msg:[%s]\n",now, buff, msg.c_str());
    }
};

using ai_service::utils::TimerTask;

void test_two_TimerTask() {
    std::thread thd1([]() {
        TimerTask::instance().run_every(1000, [&](){
            std::cout << "run_every trace 1 thread_id:" << std::this_thread::get_id() << std::endl;
        });
    });
    std::thread thd2([]() {
        TimerTask::instance().run_every(2000, [&](){
            std::cout << "run_every trace 2 thread_id:" << std::this_thread::get_id() << std::endl;
        });
    });
    std::thread thd3([]() {
        TimerTask::instance().loop();
    });
    thd1.join();
    thd2.join();
    thd3.join();
}

void test_run_at() {
    Foobar foobar;
    int interval = 5000;
    TimePoint now = std::chrono::high_resolution_clock::now();
    // 故意设置一个过去的时间点
    TimePoint expired_pt = now - std::chrono::milliseconds(interval);

    int trace_cnt = 1;

    TimerTask::instance().run_at(expired_pt, [&](){
        trace_cnt += 4;
        std::cout << "run_after trace 2" << std::endl;
    });

    TimerTask::instance().run_after(2000, [&](){
        trace_cnt += 2;
        std::cout << "run_after trace 1" << std::endl;
    });

    std::thread thd([]() {
        TimerTask::instance().loop();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    std::cout << "trace_cnt:" << trace_cnt << std::endl;
    TimerTask::instance().stop();
    thd.join();
}

void test_stop() {
    Foobar foobar;
    TimerTask::instance().add_task("key1", 40)->exec(&Foobar::do_something, &foobar, "Hello");
    TimerTask::instance().run_after(518, &Foobar::do_something, &foobar, "World");
    TimerTask::instance().add_task("key2", 70)->exec(&Foobar::do_badthing, &foobar, "Raccoon");
    TimerTask::instance().run_every(90, &Foobar::do_badthing, &foobar, "Bear");

    std::thread thd([]() {
        TimerTask::instance().loop();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(10000));

    std::vector<std::thread> threads;
    int concurrent = 50;
    for (size_t i = 0; i < concurrent; ++i) {
        threads.emplace_back([&, i] {
            TimerTask::instance().stop();
        });
    }

    for (auto& th : threads) {
        th.join();
    }
    thd.join();
}

void stress_test() {
    Foobar foobar;

    /*
    std::thread thd([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        TimerTask::instance().remove_task("key2");
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        TimerTask::instance().remove_task("key1");
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        TimerTask::instance().stop();
    });
    */

    std::vector<std::thread> threads;
    int concurrent = 50;
    for (size_t i = 0; i < concurrent; ++i) {
        threads.emplace_back([&, i] {
            while (true) {
                int n = intRand(1, 100000000);
                if (n % 3 == 0) {
                    TimerTask::instance().remove_task("key2");
                } else if (n % 5 == 0) {
                    TimerTask::instance().add_task("key1", 10)->exec(&Foobar::do_something, &foobar, "Hello");
                } else if (n % 7 == 0) {
                    TimerTask::instance().run_after(51, &Foobar::do_something, &foobar, "World");
                } else if (n % 13 == 0 ){
                    TimerTask::instance().remove_task("key1");
                } else {
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });
    }

    TimerTask::instance().loop();
    //thd.join();

    for (auto& th : threads) {
        th.join();
    }
}

int main() {
    stress_test();
    //test_stop();
    //test_run_at();
    //test_two_TimerTask();
    return 0;
}
