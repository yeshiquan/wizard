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
#include "concurrent/concurrent_hashmap.h"
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

TEST_F(ConcurrentHashMapTest, insert_find) {
    rcu::ConcurrentHashMap<std::string, std::string> hs;
    hs.insert("Hello", "World1");
    hs.insert("kenby", "World2");
    hs.insert("aankaa", "World3");
    hs.insert("yudanqi", "World4");

    auto iter = hs.find("aankaa");
    std::string value = iter.it_.node_->value_holder_.value;
    ASSERT_EQ(value, "World3");
    std::cout << value << std::endl;
}

TEST_F(ConcurrentHashMapTest, segment_iterator) {
    using HashMap = rcu::ConcurrentHashMap<std::string, std::string, std::hash<std::string>, std::allocator<uint8_t>, 0>;
    HashMap hs;
    std::vector<std::string> keys;
    std::vector<std::string> values;
    std::set<std::string> set1;
    for (int i = 0; i < 100; ++i) {
        keys.emplace_back("key_" + std::to_string(i+1));
        values.emplace_back("value_" + std::to_string(i+1));
    }
    for (int i = 0; i < keys.size(); ++i) {
        hs.insert(keys[i], values[i]);
        set1.insert(values[i]);
    }

    {
        auto iter = hs.find("key_1");
        std::string value = iter.it_.node_->value_holder_.value;
        ASSERT_EQ(value, "value_1");
        std::cout << value << std::endl;
    }
    auto* segment = hs.ensureSegment(0);
    {
        HashMap::SegmentT::Iterator iter;
        std::string key = "key_not_exist";
        auto hash = std::hash<std::string>()(key);
        bool found = segment->find(hash, iter, key);
        ASSERT_TRUE(iter == segment->cend());
        ASSERT_EQ(found, false);
    }

    std::set<std::string> set2;
    for (auto iter = segment->cbegin(); iter != segment->cend(); iter++) {
        //std::cout << *iter << std::endl;
        set2.insert(*iter);
    }
    ASSERT_EQ(set1.size(), set2.size());
}

TEST_F(ConcurrentHashMapTest, hashmap_iterator) {
    rcu::ConcurrentHashMap<std::string, std::string> hs;
    std::vector<size_t> size_list = {1, 10, 100, 1000};

    auto iter = hs.find("key_not_exist");
    ASSERT_TRUE(iter == hs.cend());

    for (auto size_cnt : size_list) {
        std::vector<std::string> keys;
        std::vector<std::string> values;
        std::set<std::string> set1;
        for (int i = 0; i < size_cnt; ++i) {
            keys.emplace_back("key_" + std::to_string(i+1));
            values.emplace_back("value_" + std::to_string(i+1));
        }
        for (int i = 0; i < keys.size(); ++i) {
            hs.insert(keys[i], values[i]);
            set1.insert(values[i]);
        }

        auto iter = hs.find("key_1");
        std::string value = *iter;
        ASSERT_EQ(value, "value_1");
        //std::cout << value << std::endl;

        std::set<std::string> set2;
        for (auto iter = hs.cbegin(); iter != hs.cend(); iter++) {
            //std::cout << "value -> " << *iter << std::endl;
            set2.insert(*iter);
        }
        ASSERT_EQ(set1.size(), set2.size());
    }
}

TEST_F(ConcurrentHashMapTest, hashmap_erase) {
    rcu::ConcurrentHashMap<std::string, std::string, std::hash<std::string>, std::allocator<uint8_t>, 2> hs;
    std::vector<std::string> keys;
    std::vector<std::string> values;
    for (int i = 0; i < 100; ++i) {
        keys.emplace_back("key_" + std::to_string(i+1));
        values.emplace_back("value_" + std::to_string(i+1));
    }
    for (int i = 0; i < keys.size(); ++i) {
        hs.insert(keys[i], values[i]);
    }

    for (int i = 0; i < keys.size(); ++i) {
        hs.erase(keys[i]);
        auto iter = hs.find(keys[i]);
        ASSERT_TRUE(iter == hs.cend());
    }
}

TEST_F(ConcurrentHashMapTest, hashmap_rehash) {
    using HashMap = rcu::ConcurrentHashMap<std::string, std::string, std::hash<std::string>, std::allocator<uint8_t>, 0>;
    HashMap hs;
    std::vector<std::string> keys;
    std::vector<std::string> values;
    for (int i = 0; i < 100; ++i) {
        keys.emplace_back("key_" + std::to_string(i+1));
        values.emplace_back("value_" + std::to_string(i+1));
    }
    for (int i = 0; i < keys.size(); ++i) {
        hs.insert(keys[i], values[i]);
    }
    auto* segment = hs.ensureSegment(0);
    segment->rehash(segment->get_bucket_list()->bucket_count() << 1);
    for (int i = 0; i < 100; ++i) {
        auto iter = hs.find(keys[i]);
        ASSERT_TRUE(iter != hs.cend());
        ASSERT_TRUE(*iter == values[i]);
    }
}


TEST_F(ConcurrentHashMapTest, hashmap_emplace) {
    using HashMap = rcu::ConcurrentHashMap<std::string, std::string, std::hash<std::string>, std::allocator<uint8_t>, 4>;
    HashMap hs;
    std::vector<std::string> keys;
    std::vector<std::string> values;
    for (int i = 0; i < 100; ++i) {
        keys.emplace_back("key_" + std::to_string(i+1));
        values.emplace_back("value_" + std::to_string(i+1));
    }
    for (int i = 0; i < keys.size(); ++i) {
        hs.emplace(keys[i], values[i]);
    }
    for (int i = 0; i < keys.size(); ++i) {
        auto iter = hs.find(keys[i]);
        ASSERT_TRUE(*iter == values[i]);
    }
}

TEST_F(ConcurrentHashMapTest, hashmap_clear) {
    using HashMap = rcu::ConcurrentHashMap<std::string, std::string, std::hash<std::string>, std::allocator<uint8_t>, 4>;
    HashMap hs;
    std::vector<std::string> keys;
    std::vector<std::string> values;
    for (int i = 0; i < 100; ++i) {
        keys.emplace_back("key_" + std::to_string(i+1));
        values.emplace_back("value_" + std::to_string(i+1));
    }
    for (int i = 0; i < keys.size(); ++i) {
        hs.emplace(keys[i], values[i]);
    }
    hs.clear();
    ASSERT_EQ(hs.size(), 0);
    for (int i = 0; i < keys.size(); ++i) {
        auto iter = hs.find(keys[i]);
        ASSERT_TRUE(iter == hs.cend());
    }
}

TEST_F(ConcurrentHashMapTest, hashmap_reserve) {
    using HashMap = rcu::ConcurrentHashMap<std::string, std::string, std::hash<std::string>, std::allocator<uint8_t>, 4>;
    HashMap hs;
    hs.reserve(20000 << 4);
}

TEST_F(ConcurrentHashMapTest, hashmap_insert_or_assign) {
    using HashMap = rcu::ConcurrentHashMap<std::string, std::string, std::hash<std::string>, std::allocator<uint8_t>, 4>;
    HashMap hs;
    std::vector<std::string> keys;
    std::vector<std::string> values;
    std::vector<std::string> new_values;
    for (int i = 0; i < 100; ++i) {
        keys.emplace_back("key_" + std::to_string(i+1));
        values.emplace_back("value_" + std::to_string(i+1));
        new_values.emplace_back("new_value_" + std::to_string(i+1));
    }
    for (int i = 0; i < keys.size(); ++i) {
        hs.emplace(keys[i], values[i]);
    }
    ASSERT_EQ(hs.size(), keys.size());
    for (int i = 0; i < keys.size(); ++i) {
        hs.emplace(keys[i], new_values[i]);
        auto iter = hs.find(keys[i]);
        ASSERT_TRUE(*iter == new_values[i]);
    }
    ASSERT_EQ(hs.size(), keys.size());

    /*
    for (int i = 0; i < keys.size(); ++i) {
        hs.insert_or_assign(keys[i], new_values[i]);
        auto iter = hs.find(keys[i]);
        ASSERT_TRUE(*iter == new_values[i]);
    }
    ASSERT_EQ(hs.size(), keys.size());
    */
}

struct Foobar {
    char data[393];
    std::string message;
    Foobar() {}
    Foobar(std::string msg) {
        message = msg;
    }
};

TEST_F(ConcurrentHashMapTest, multi_thread) {
    std::string log_conf_file = "./conf/log_afile.conf";

    com_registappender("CRONOLOG", comspace::CronoAppender::getAppender,
                comspace::CronoAppender::tryAppender);

    auto logger = logging::ComlogSink::GetInstance();
    if (0 != logger->SetupFromConfig(log_conf_file.c_str())) {
        LOG(FATAL) << "load log conf failed";
        return ;
    }

    std::vector<std::thread> threads;
    using HashMap = rcu::ConcurrentHashMap<std::string, Foobar, std::hash<std::string>, std::allocator<uint8_t>, 0>;
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
        ASSERT_TRUE(iter->message == values[i].message);
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
                    value = *iter;
                    //LOG(NOTICE) << "find key:" << key << " value:" << *iter;
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
                    hs.emplace(key, value);
                    //hs.insert_or_assign(key, value);
                    //LOG(NOTICE) << "insert key:" << key << " value:" << value;
                }
            }
        });
    }

    // 遍历
    /*
    for (size_t i = 0; i < 5; ++i) {
        threads.emplace_back([&, i] {
            while (true) {
                int idx = intRand(1, key_size-1);    
                auto key = keys[idx];
                auto iter = hs.find(key);
                Foobar value;
                for (; iter != hs.end() && i < 20; ++iter) {
                    value = *iter;
                }
            }
        });
    }
    */


    for (auto& th : threads) {
        if (th.joinable()) {
            th.join();
        }
    }
}

