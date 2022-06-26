#include "gtest/gtest.h"
#include "gflags/gflags.h"
#include <thread>
#include <chrono>
#include <random>

#define  DCHECK_IS_ON

#define private public
#define protected public
#include "concurrent/vector.h"
#undef private
#undef protected

using namespace rcu;

int intRand(const int & min, const int & max) {
    static thread_local std::mt19937 generator;
    std::uniform_int_distribution<int> distribution(min,max);
    return distribution(generator);
}

class ConcurrentVectorTest : public ::testing::Test {
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

TEST_F(ConcurrentVectorTest, ConcurrentVectorPool) {
    {
        rcu::ConcurrentVector<int> vec;
        vec.reserve(2004);

        vec[0] = 4241;
        ASSERT_EQ(vec[0], 4241);
        vec[2003] = 4242;
        ASSERT_EQ(vec[2003], 4242);
    }

    {
        rcu::ConcurrentVector<int> vec(232);  // num_per_shard
        vec.reserve(4238);  // size
        vec[4237] = 9433;
        ASSERT_EQ(vec[4237], 9433);
    }

    {
        rcu::ConcurrentVector<std::string> vec;
        vec.ensure(9999).assign("Hello World");
        ASSERT_EQ(vec[9999], "Hello World");
    }

    {
        rcu::ConcurrentVector<std::string> vec;
        vec.ensure(1000) = "Hello";
        vec.ensure(2000) = "World";
        ASSERT_EQ(vec[2000], "World");
        ASSERT_EQ(vec[1000], "Hello");
    }

    {
        rcu::ConcurrentVector<std::string> vec;
        vec.reserve(2004);
        /*
        vec[9999].assign("Hello World");
        ASSERT_EQ(vec[9999], "Hello World");
        */
    }

    //std::vector<::std::string> data = {"0", "1", "2", "3", "4", "5"};
    //vec.copy_n(data.begin() + 1, 4, 1);
}

struct Foobar {
    char data[189];
    std::string message;
};

template<typename T>
int print_list(std::string name, T* head) {
    int cnt = 0;
    std::cout << "PRINT_LIST " << name << " -> ";
    for (auto p = head; p != nullptr; p = p->next) {
        cnt++;
        std::cout << p << " -> ";
    }
    std::cout << " list_size:" << cnt << std::endl;
    return cnt;
}

TEST_F(ConcurrentVectorTest, test_retire_list) {
    rcu::RetireList<Foobar> list;
    using Node = rcu::RetireList<Foobar>::Node;
    auto now = list.get_current_timestamp();
    for (int i = 0; i < 5; i++) {
        Foobar* data = new Foobar;
        data->message = "Hello, World";
        auto* node = new Node;
        node->data = data;
        list.push(node, now);
    }
    auto head = list._head.load();
    int cnt = 0;
    cnt = print_list("before drop:", list.get_head());
    ASSERT_EQ(cnt, 5);
    auto* dropped_list = list.try_drop(head);
    cnt = print_list("after drop:", list.get_head());
    ASSERT_EQ(cnt, 0);
    cnt = print_list("dropped_list:", dropped_list);
    ASSERT_EQ(cnt, 5);
}

TEST_F(ConcurrentVectorTest, test_snapshot) {
    rcu::ConcurrentVector<std::string> vec;
    vec.ensure_table(100);
    auto snapshot = vec.snapshot();
}

TEST_F(ConcurrentVectorTest, test_fill_n) {
    rcu::ConcurrentVector<int> vec(4);
    size_t index = 2;
    size_t length = 8;
    int the_value = 666;
    vec.fill_n(index, length, the_value);
    for (int i = index; i < index+length; ++i) {
        ASSERT_EQ(vec[i], the_value);
    }
    ASSERT_EQ(vec[index-1], 0);
    ASSERT_EQ(vec[index+length], 0);
}

TEST_F(ConcurrentVectorTest, test_copy_n) {
    rcu::ConcurrentVector<int> vec(4);
    std::vector<int> src;
    size_t length = 8;
    size_t start_index = 2;
    for (int i = 0; i < (length+length); ++i) {
        src.emplace_back(i);
    }
    vec.copy_n(src.begin()+start_index, length, start_index);
    for (int i = start_index; i < (start_index+length); ++i) {
        std::cout << i << " " << vec[i] << std::endl;
        ASSERT_EQ(vec[i], i);
    }
    ASSERT_EQ(vec[start_index-1], 0);
    ASSERT_EQ(vec[start_index+length], 0);
}

TEST_F(ConcurrentVectorTest, test_for_each) {
    rcu::ConcurrentVector<std::string> vec(4);
    vec.reserve(2004);
    for (int i = 0; i < 1000; ++i) {
        vec[i] = std::to_string(i);
    }

    int cnt = 0;
    auto print = [&cnt](std::string* iter, std::string* end) {
        std::cout << "callback -> ";
        for (; iter != end; ++iter) {
            std::cout << *iter << ",";
            cnt++;
        }
        std::cout << std::endl;
    };

    auto test = [&](size_t start_index, size_t end_index) {
        std::cout << "\nfor_each(" << start_index << "," << end_index << ") -> " << std::endl;
        cnt = 0;
        vec.for_each(start_index, end_index, print);
        ASSERT_EQ(cnt, end_index-start_index);
    };

    test(2, 4);
    test(2, 6);
    test(2, 10);
}

/*
TEST_F(ConcurrentVectorTest, bench_ensure_table) {
    rcu::ConcurrentVector<std::string> vec;
    std::vector<std::thread> threads;
    for (size_t i = 0; i < 10; ++i) {
        threads.emplace_back([&, i] {
            size_t shard_num = intRand(1, 100);
            vec.ensure_table(shard_num);
            std::this_thread::sleep_for(std::chrono::milliseconds(70000));
            shard_num = intRand(100, 200);
            vec.ensure_table(shard_num);
        });
    }
    for (auto& th : threads) {
        th.join();
    }
}

TEST_F(ConcurrentVectorTest, bench_retire_list) {
    rcu::RetireList<Foobar> list;
    using Node = rcu::RetireList<Foobar>::Node;
    std::vector<std::thread> threads;
    for (size_t i = 0; i < 10; ++i) {
        threads.emplace_back([&, i] {
            while (true) {
                for (int j = 0; j < 1000; ++j) {
                    Foobar* data = new Foobar;
                    data->message = "Hello, World";
                    list.retire(data);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }
}
*/
