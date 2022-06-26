#include "gtest/gtest.h"
#include "gflags/gflags.h"
#include <thread>
#include <chrono>
#include <random>
#include <vector>
#include <mutex>

#include <assert.h>
#include "baidu/streaming_log.h"
#include "base/comlog_sink.h"
#include "base/strings/stringprintf.h"
#include "com_log.h"
#include "cronoapd.h"

#define  DCHECK_IS_ON

#define private public
#define protected public
#include "concurrent/concurrent_bounded_queue.h"
#undef private
#undef protected


using rcu::queue::ConcurrentBoundedQueue;
using rcu::queue::ConcurrentBoundedQueueOption;
using rcu::FutexInterface;

int intRand(const int & min, const int & max) {
    static thread_local std::mt19937 generator;
    std::uniform_int_distribution<int> distribution(min,max);
    return distribution(generator);
}

class ConcurrentBoundedQueueTest : public ::testing::Test {
private:
    virtual void SetUp() {
        //p_obj.reset(new ExprBuilder);
    }
    virtual void TearDown() {
        //if (p_obj != nullptr) p_obj.reset();
    }
protected:
    //std::shared_ptr<ExprBuilder>  p_obj;
};

TEST_F(ConcurrentBoundedQueueTest, test_basic) {
    //using Queue = ConcurrentBoundedQueue<int, FutexInterface, ConcurrentBoundedQueueOption::CACHELINE_ALIGNED>;
    using Queue = ConcurrentBoundedQueue<int, FutexInterface, ConcurrentBoundedQueueOption::CACHELINE_ALIGNED>;
    Queue vec(53);

    std::cout << "test_basic" << std::endl;
    for (int i = 0; i < 10; ++i) {
        vec.push(i);
    }
    ASSERT_EQ(vec.size(), 10);

    for (int i = 0; i < 5; ++i) {
        int v;
        vec.pop(v);
        ASSERT_EQ(v, i);
    }
    ASSERT_EQ(vec.size(), 5);

    for (int i = 10; i < 20; ++i) {
        vec.push(i);
    }
    ASSERT_EQ(vec.size(), 15);

    for (int i = 5; i < 20; ++i) {
        int v;
        vec.pop(v);
        ASSERT_EQ(v, i);
    }
    ASSERT_EQ(vec.size(), 0);

    vec.push([](int& value) {
        value = 111;
    });
    vec.pop([](int& value) {
        ASSERT_EQ(value, 111);
    });
}

TEST_F(ConcurrentBoundedQueueTest, test_stress) {
    ConcurrentBoundedQueue<int> vec(54);

    std::vector<std::thread> threads;

    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&] {
            for (int k = 0; k < 100000; ++k) {
                int v = intRand(1, 1000000);
                //std::lock_guard<std::mutex> guard(mutex);
                vec.push(v);
                //std::cout << "push v:" << v << std::endl;
                //std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        });
    }

    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&] {
            std::this_thread::sleep_for(std::chrono::milliseconds(48));
            for (int k = 0; k < 100000; ++k) {
                //std::lock_guard<std::mutex> guard(mutex);
                int value;
                vec.pop(value);
                //std::cout << "pop v:" << v << std::endl;
                //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }
} 

int main(int argc, char* argv[]) {
    std::string log_conf_file = "./conf/log_afile.conf";

    com_registappender("CRONOLOG", comspace::CronoAppender::getAppender,
                comspace::CronoAppender::tryAppender);

    auto logger = logging::ComlogSink::GetInstance();
    if (0 != logger->SetupFromConfig(log_conf_file.c_str())) {
        LOG(FATAL) << "load log conf failed";
        return 0;
    }


    testing::InitGoogleTest(&argc, argv);
    google::ParseCommandLineFlags(&argc, &argv, false);
    return RUN_ALL_TESTS();
}
