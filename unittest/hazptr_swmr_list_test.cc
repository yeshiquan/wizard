#include "gtest/gtest.h"
#include "gflags/gflags.h"
#include <thread>
#include <chrono>
#include <random>

#include <assert.h>
#include "baidu/streaming_log.h"
#include "base/comlog_sink.h"
#include "base/strings/stringprintf.h"
#include "com_log.h"
#include "cronoapd.h"

#define private public
#define protected public
#include "swmr_list.h"
#undef private
#undef protected

template<typename T>
int print_list(std::string name, T* head) {
    int cnt = 0;
    std::cout << "PRINT_LIST " << name << " -> ";
    for (auto p = head; p != nullptr; p = p->next.load()) {
        cnt++;
        std::cout << p->element << " -> ";
    }
    std::cout << " list_size:" << cnt << std::endl;
    return cnt;
}

class HazptrSWMRListSetTest : public ::testing::Test {
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

TEST_F(HazptrSWMRListSetTest, priv_push) {
    rcu::SWMRListSet<int> set;
    bool ret;
    std::cout << "insert 3..." << std::endl;
    ret = set.add(3);
    int cnt = print_list("set_list", set.head.load());
    std::cout << "insert 4..." << std::endl;
    ret = set.add(4);
    cnt = print_list("set_list", set.head.load());
    std::cout << "insert 5..." << std::endl;
    ret = set.add(5);
    cnt = print_list("set_list", set.head.load());
    std::cout << "insert 1..." << std::endl;
    ret = set.add(1);
    cnt = print_list("set_list", set.head.load());
    std::cout << "insert 2..." << std::endl;
    ret = set.add(2);
    cnt = print_list("set_list", set.head.load());
    std::cout << "remove 3..." << std::endl;
    ret = set.remove(3);
    cnt = print_list("set_list", set.head.load());
    ret = set.contains(4);
    std::cout << "contains 4: " << ret << std::endl;
    ret = set.contains(3);
    std::cout << "contains 3: " << ret << std::endl;
}

// 线程安全
int intRand(const int & min, const int & max) {
    static thread_local std::mt19937 generator;
    std::uniform_int_distribution<int> distribution(min,max);
    return distribution(generator);
}


TEST_F(HazptrSWMRListSetTest, multi_thread) {
    rcu::SWMRListSet<int> set;

    std::vector<std::thread> threads;

    // Reader
    for (size_t i = 0; i < 10; ++i) {
        threads.emplace_back([&, i] {
            while (true) {
                int v = intRand(1, 1000);    
                bool ret = set.contains(v);
                if (ret) {
                    //LOG(NOTICE) << "contains " << v << " -> " << ret;
                }
            }
        });
    }

    // Writer
    for (size_t i = 0; i < 10; ++i) {
        threads.emplace_back([&] {
            while (true) {
                int v = intRand(1, 10000000);    
                int flag = intRand(1, 2000);    
                if (flag % 4 == 0) {
                    set.remove(v);
                } else {
                    set.add(v);
                }
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }
}

int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    google::ParseCommandLineFlags(&argc, &argv, false);
    return RUN_ALL_TESTS();
}
