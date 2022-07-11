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

#include <fstream>
#include <memory>
#include "gtest/gtest.h"

#define private public
#define protected public
#include "defer.h"
#undef private
#undef protected

#include <typeinfo>       // operator typeid

namespace base::defer {

class DeferTest : public ::testing::Test {
private:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
protected:
};

TEST_F(DeferTest, test_shutdown) {
    // 1次
    int cnt = 1;
    {
        DEFER({
            cnt--;
        });
    }
    ASSERT_EQ(cnt, 0);

    // 2次
    int cnt2 = 2;
    {
        // 执行顺序和安装顺序是相反的
        DEFER({
            std::cout << "first defer..." << std::endl;
            cnt2--;
        });
        DEFER({
            std::cout << "second defer..." << std::endl;
            cnt2--;
        });
    }
    ASSERT_EQ(cnt2, 0);
}

}
