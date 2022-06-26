#include "gtest/gtest.h"
#include "gflags/gflags.h"
#include <thread>
#include <chrono>
#include <random>

#include <baidu/feed/mlarch/babylon/pool.h>

#define  DCHECK_IS_ON
#define POOL_STATS true

#define private public
#define protected public
#include "pool.h"
#undef private
#undef protected

int intRand(const int & min, const int & max) {
    static thread_local std::mt19937 generator;
    std::uniform_int_distribution<int> distribution(min,max);
    return distribution(generator);
}

class PoolTest : public ::testing::Test {
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


TEST_F(PoolTest, test_tagged_index) {
    for (uint64_t index = 0; index <= 1000; ++index) {
        uint64_t tag = index;
        auto tagged_index = rcu::helper::make_tagged_index(index, tag);
        ASSERT_EQ(index, rcu::helper::get_index(tagged_index));
        ASSERT_EQ(tag, rcu::helper::get_tag(tagged_index));
    }
}

TEST_F(PoolTest, test_linked_list) {
    constexpr size_t TC_SIZE = 5;
    using Pool = rcu::ObjectPool<std::string, false, TC_SIZE>;
    using Node = Pool::Node;
    rcu::helper::LinkedList<Node> list;
    std::vector<Node> nodes(100);
    for (uint64_t i = 0; i < 10; ++i) {
        auto* node = &nodes[i];
        node->index = i;
        list.push_back(node);
    }
    auto cnt = rcu::helper::print_list("test list", nodes, list.head->index);
    ASSERT_EQ(cnt, 10);
}

TEST_F(PoolTest, test_thread_cache) {
    constexpr size_t TC_SIZE = 5;
    using Pool = rcu::ObjectPool<std::string, false, TC_SIZE>;
    using ThreadCache = Pool::ThreadCache;
    using Node = Pool::Node;

    auto& cache = ThreadCache::instance();
    for (int i = 0; i < TC_SIZE; ++i) {
        auto* node = new Node;
        bool ret = cache.release_to_tc(node);
        ASSERT_EQ(ret, true);
    }
    auto* node = new Node;
    bool ret = cache.release_to_tc(node);
    ASSERT_EQ(ret, false);

    for (int i = 0; i < TC_SIZE; ++i) {
        Node* node = cache.get_from_tc();
        ASSERT_TRUE(node != nullptr);
    }
    node = cache.get_from_tc();
    ASSERT_TRUE(node == nullptr);
}

TEST_F(PoolTest, test_babylon_pool) {
    using ::baidu::feed::mlarch::babylon::ObjectPool;
    size_t num = 20;
    size_t reserve_global = 7;
    size_t cache_per_thread = 3;
    ObjectPool<::std::string> pool(num, cache_per_thread, reserve_global);

    {
        std::vector<ObjectPool<std::string>::PooledObject> list;
        for (int i = 0; i < 24; i++) {
            auto object = pool.get();  // 返回池化对象
            list.emplace_back(std::move(object));
        }
        std::cout << "finish =================" << std::endl;
    }
    {
        auto obj = pool.get();
    }

    /*
    std::vector<std::thread> threads;
    for (size_t i = 0; i < 20; ++i) {
        threads.emplace_back([&, i] {
            while (true) {
                auto object = pool.get();  // 返回池化对象
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }
    */
}

TEST_F(PoolTest, test_pool) {
    rcu::ObjectPool<std::string> pool;
    pool.print();
}

std::atomic<uint32_t> constructor_cnt = {0};
std::atomic<uint32_t> destructor_cnt = {0};
struct Foobar {
    Foobar() {
        constructor_cnt++;
    }
    ~Foobar() {
        destructor_cnt++;
    }
    char dummy[274];
    std::string name;
};

TEST_F(PoolTest, test_pool_get_release) {
    using Pool = rcu::ObjectPool<Foobar>;
    using Node = Pool::Node;
    using PooledObject = Pool::PooledObject;
    size_t num = 20;
    size_t collective_num = 9;
    size_t tc_size = 7;
    size_t extra = 2;
    Pool pool(num, collective_num);
    pool.print();
    {
        std::vector<PooledObject> nodes;
        for (int i = 0; i < (num+extra); ++i) {
            auto pooled_obj = pool.get();
            ASSERT_TRUE(pooled_obj.get_obj() != nullptr);
            nodes.emplace_back(std::move(pooled_obj));
        }
    }
    rcu::g_stat.print();

    ASSERT_EQ(rcu::g_stat._create.load(), extra);
    ASSERT_EQ(rcu::g_stat._pop_market.load(), num-collective_num);
    ASSERT_EQ(rcu::g_stat._pop_collective.load(), collective_num);

    ASSERT_EQ(rcu::g_stat._push_local.load(), tc_size); //优先自己私有化
    ASSERT_EQ(rcu::g_stat._push_collective.load(), collective_num); //集体的必须保证归还
    ASSERT_EQ(rcu::g_stat._push_market.load(), num-collective_num-tc_size); // 剩下的流到市场
    ASSERT_EQ(rcu::g_stat._destroy.load(), extra);

    // 测试构造和析构是否正确
    std::cout << "constructor_cnt:" << constructor_cnt << std::endl;
    std::cout << "destructor_cnt:" << destructor_cnt << std::endl;
    ASSERT_EQ(constructor_cnt, num+extra); 
    ASSERT_EQ(destructor_cnt, num+extra);

    // 再来一次，这次会优先从thread local cache分配
    ASSERT_EQ(rcu::g_stat._pop_local.load(), 0);
    {
        std::vector<PooledObject> nodes;
        for (int i = 0; i < (num+extra); ++i) {
            auto pooled_obj = pool.get();
            nodes.emplace_back(std::move(pooled_obj));
        }
    }
    rcu::g_stat.print();
    ASSERT_EQ(rcu::g_stat._pop_local.load(), tc_size);
}

/*
TEST_F(PoolTest, test_pool_get_release_thread) {
    using Pool = rcu::ObjectPool<std::string>;
    using Node = Pool::Node;
    Pool pool;
    std::vector<std::thread> threads;
    for (size_t i = 0; i < 20; ++i) {
        threads.emplace_back([&, i] {
            while (true) {
                auto object = pool.get();  // 返回池化对象
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }
}
*/
