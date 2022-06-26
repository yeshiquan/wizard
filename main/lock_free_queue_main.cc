#include "nonlock_free_queue.h"
//#include "lock_free_queue.h"

#include "gflags/gflags.h"
#include <thread>
#include <chrono>
#include <vector>
#include <random>
#include <iostream>

#include "baidu/streaming_log.h"
#include "base/comlog_sink.h"
#include "base/strings/stringprintf.h"
#include "com_log.h"
//#include "cronoapd.h"

#include <baidu/feed/mlarch/babylon/async_file_appender.h>

DEFINE_int32(threads, 1, "option.");
DEFINE_int32(n, 1000, "n");
DEFINE_int32(option, 0, "option");

using ::baidu::feed::mlarch::babylon::AsyncFileAppender;

int intRand(const int & min, const int & max) {
    //static thread_local std::mt19937 generator;
    static thread_local std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(min, max);
    return distribution(generator);
}

int torture1(rcu::LockFreeQueue<int> *queue) {
    for (int i = 0; i < FLAGS_n; ++i) {
        int n = intRand(1, 1000000);
        if (true) {
            queue->push(n);
            if (FLAGS_option == 1) {
                LOG(NOTICE) << "push -> " << n;
            }
        }
        int n2 = intRand(1, 1000000);
        if (n2 % 2 == 0) {
            auto p = queue->pop();
            if (p) {
                int x = *p;
                if (FLAGS_option == 1) { 
                    LOG(NOTICE) << "pop -> " << x << "  n2:" << n2 ;
                }
            } else {
                LOG(NOTICE) << "pop -> nullptr" ;
            }
        }
    }
    if (FLAGS_option == 1) {
        //LOG(NOTICE) << "queue size -> " << queue->size() ;
    }
    return 0;
}

int torture2(rcu::LockFreeQueue<int> *queue) {
    std::vector<std::thread> threads;
    for (int j = 0; j < FLAGS_threads; j++) {
        std::thread t(torture1, queue);
        threads.emplace_back(std::move(t));
    }
    for (auto& th : threads) {
        th.join();
    }
    return 0;
}

int main(int argc, char** argv) {
    google::ParseCommandLineFlags(&argc, &argv, false);

    std::string log_conf_file = "./conf/log.conf";

    com_registappender("BFILE", AsyncFileAppender::get_appender, 
                AsyncFileAppender::try_appender);

    auto logger = logging::ComlogSink::GetInstance();
    if (0 != logger->SetupFromConfig(log_conf_file.c_str())) {
        LOG(FATAL) << "Server init failed, load log conf failed";
        return -1;
    }

    // 要在SetupFromConfig之后执行，否则comlog_get_log_level()是错误的
    switch (comlog_get_log_level()) {
        case COMLOG_FATAL:
            ::logging::SetMinLogLevel(::logging::BLOG_FATAL);
            break;
        case COMLOG_WARNING:
            ::logging::SetMinLogLevel(::logging::BLOG_WARNING);
            break;
        case COMLOG_NOTICE:
            ::logging::SetMinLogLevel(::logging::BLOG_NOTICE);
            break;
        case COMLOG_TRACE:
            ::logging::SetMinLogLevel(::logging::BLOG_TRACE);
            break;
        case COMLOG_DEBUG:
            ::logging::SetMinLogLevel(::logging::BLOG_DEBUG);
            break;
        
        default:
            break;
    }

    //rcu::LockFreeQueue<int> queue1;
    //torture1(&queue1);

    rcu::LockFreeQueue<int> queue2;
    torture2(&queue2);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    return 0;
}
