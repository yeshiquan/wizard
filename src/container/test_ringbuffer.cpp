#include <iostream>
#include <type_traits>
#include <algorithm>
#include <cstring>
#include <vector>
#include <functional>
#include <assert.h>

#include <sys/time.h>
#include <chrono>
#include <thread>

#include <fstream>
#include <memory>
#include "gtest/gtest.h"

#define DCHECK_IS_ON 
#include "check.h"

#define private public
#define protected public
#include "container/ringbuffer.h"
#undef private
#undef protected

#include <typeinfo>       // operator typeid

using base::container::RingBuffer;
using base::window::ItemHolder;

class WindowTest : public ::testing::Test {
private:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
protected:
};

int64_t get_system_millsec() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    int64_t factor = 1;

    return (int64_t)((factor * tv.tv_sec * 1000) + (tv.tv_usec / 1000));
}

struct FrameData {
    float x;
    float y;
    std::string name = "";

    FrameData() {
        //std::cout << " [Construct 1] " << std::endl;
    }
    ~FrameData() {
        std::cout << " [Destruct] ";
    }
    FrameData(float x_, float y_, std::string name_) :
        x(x_), y(y_), name(name_) {
        std::cout << " [Construct 2] ";
    }
};

TEST_F(WindowTest, test_ringbuffer) {
    {
        RingBuffer<int> buffer(4);
        int cnt = 0;
        for (auto& v : buffer) {
            cnt++;
        }
        ASSERT_EQ(cnt, 0);
    }
    {
        for (int capacity = 1; capacity <= 105; ++capacity) {
            RingBuffer<int> buffer(capacity);
            //auto& head = buffer.front();  coredump
            //auto& tail = buffer.back();  
            for (int v = 1; v <= 100; ++v) {
                buffer.emplace_back(v);
            }
            int front_value = (100 >= capacity) ? (100-capacity+1) : 1;
            int value = front_value;
            for (auto& v : buffer) {
                ASSERT_EQ(v, value);
                value++;
            }
            value = front_value;
            while (!buffer.empty()) {
                auto& v = buffer.front();
                ASSERT_EQ(v, value);
                value++;
                buffer.pop_front();
            }
        }
    }
}

TEST_F(WindowTest, test_window) {
    {   // Test RingBuffer
        std::cout << "Test RingBuffer ======================" << std::endl;
        RingBuffer<int> buffer3(4);
        for (int v = 1; v <= 5; ++v) {
            buffer3.emplace_back(v);
        }

        int value = 2;
        std::cout << "begin -> end" << std::endl;
        for (auto& v : buffer3) {
            ASSERT_EQ(v, value++);
            std::cout << v << std::endl;
        }

        std::cout << "rbegin -> rend" << std::endl;
        value = 5;
        for (auto iter = buffer3.rbegin(); iter != buffer3.rend(); ++iter) {
            ASSERT_EQ(*iter, value--);
            std::cout << *iter << std::endl;
        }

        auto iter = buffer3.cbegin();
        std::cout << "iter++ -> " << *(iter++) << std::endl;

        iter = buffer3.cbegin();
        std::cout << "++iter -> " << *(++iter) << std::endl;
    }
}
