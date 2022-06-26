#include "gtest/gtest.h"
#include "gflags/gflags.h"
#include <thread>
#include <chrono>
#include <random>
#include <set>

#include <assert.h>
#include "baidu/streaming_log.h"
#include "base/comlog_sink.h"
#include "base/strings/stringprintf.h"
#include "com_log.h"
#include "cronoapd.h"

#define private public
#define protected public
#include "ConcurrentHashMap.h"
#undef private
#undef protected

// 线程安全
int intRand(const int & min, const int & max) {
    static thread_local std::mt19937 generator;
    std::uniform_int_distribution<int> distribution(min,max);
    return distribution(generator);
}

class ConcurrentHashMapTest : public ::testing::Test {
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

struct Foobar {
    char data[393];
    std::string message;
    Foobar() {}
    Foobar(std::string msg) {
        message = msg;
    }
};

TEST_F(ConcurrentHashMapTest, multi_thread) {
    LOG(NOTICE) << "trace 0...";
    std::string log_conf_file = "./conf/log_afile.conf";

    com_registappender("CRONOLOG", comspace::CronoAppender::getAppender,
                comspace::CronoAppender::tryAppender);

    auto logger = logging::ComlogSink::GetInstance();
    if (0 != logger->SetupFromConfig(log_conf_file.c_str())) {
        LOG(FATAL) << "load log conf failed";
        return ;
    }

    std::vector<std::thread> threads;
    using HashMap = folly::ConcurrentHashMap<
              std::string,
              Foobar,
              std::hash<std::string>,
              std::equal_to<std::string>,
              std::allocator<uint8_t>,
              0>;
    HashMap hs;
    size_t key_size = 10000;
    std::vector<std::string> keys;
    std::vector<Foobar> values;
    for (int i = 0; i < key_size; ++i) {
        keys.emplace_back("key_" + std::to_string(i+1));
        values.emplace_back(Foobar("value_" + std::to_string(i+1)));
    }
    for (int i = 0; i < key_size; ++i) {
        hs.emplace(keys[i], values[i]);
    }
    for (int i = 0; i < key_size; ++i) {
        auto iter = hs.find(keys[i]);
        ASSERT_TRUE(iter->second.message == values[i].message);
    }

    // Reader
    for (size_t i = 0; i < 10; ++i) {
        threads.emplace_back([&, i] {
            while (true) {
                int idx = intRand(1, key_size-1);    
                auto key = keys[idx];
                auto iter = hs.find(key);
                Foobar value;
                if (iter != hs.cend()) {
                    value = iter->second;
                    //LOG(NOTICE) << "find key:" << key << " value:" << iter->second.message;
                }
            }
        });
    }

    // Writer
    for (size_t i = 0; i < 10; ++i) {
        threads.emplace_back([&] {
            while (true) {
                int idx = intRand(1, key_size-1);    
                auto key = keys[idx];
                auto value = values[idx];
                int flag = intRand(1, 2000);    
                if (flag % 2 == 0) {
                    hs.erase(key);
                    //LOG(NOTICE) << "erase key:" << key;
                } else {
                    hs.insert_or_assign(key, value);
                    //LOG(NOTICE) << "insert key:" << key << " value:" << value.message;
                }
            }
        });
    }

    for (auto& th : threads) {
        if (th.joinable()) {
            th.join();
        }
    }
}

