#include <iostream>
#include <fstream>
#include <memory>
#include "gtest/gtest.h"

#define private public
#define protected public
#include "concurrent/thread_pool.h"
#include "concurrent/thread_pool_executor.h"
#undef private
#undef protected

#include <typeinfo>       // operator typeid

class ThreadPoolTest : public ::testing::Test {
private:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
protected:
};

int32_t handler(int foo, std::string bar) {
    LOG(NOTICE) << "foo:" << foo << " bar:" << bar;
    return 0;
}

class IndexCache {
public:
    int32_t async_update_cache_with_executor() {
        std::string query = "播放以父之名";
        std::string key = "bvc:index_cache:" + query;
        auto future = ThreadPoolExecutor::instance().execute(&IndexCache::update_cache, this, key);
        LOG(NOTICE) << "ret -> " << future.get();
        return 0;
    }
    int32_t async_update_cache_with_thread_pool() {
        std::string query = "播放以父之名";
        std::string key = "bvc:index_cache:" + query;
        auto thread_pool = std::make_shared<ThreadPool>(5, 2000000);
        auto future = thread_pool->enqueue(&IndexCache::update_cache, this, key); 
        LOG(NOTICE) << "ret -> " << future.get();
        return 0;
    }
    int32_t update_cache(std::string key) {
        LOG(NOTICE) << "update cache key -> " << key;
        return 5;
    }
};


TEST_F(ThreadPoolTest, test_thread_pool) {
    fprintf(stderr, ">>> ThreadPoolTest start...\n");
    {
        auto thread_pool = std::make_shared<ThreadPool>(5, 2000000);
        auto future = thread_pool->enqueue(handler, 3, "hello"); 
        ASSERT_TRUE(future.get() == 0);
    }

    {
        // 测试没有任务的时候能正常析构退出
        auto thread_pool = std::make_shared<ThreadPool>(5, 2000000);
    }

    {
        // 测试优雅退出，等所有任务处理完才退出
        auto thread_pool = std::make_shared<ThreadPool>(1, 2000000);
        for (int i = 0; i < 2; ++i) {
            thread_pool->enqueue([]() {
                for (int j = 0; j < 10000; ++j) {
                    std::string output = std::to_string(j);
                }
                std::cout << "I'm done" << std::endl;
            }); 
        }
    }

    {
        // 测试多线程多次调用stop_and_wait，程序正常不出core
        auto thread_pool = std::make_shared<ThreadPool>(5, 2000000);
        std::vector<std::thread> threads;
        for (size_t i = 0; i < 10; ++i) {
            threads.emplace_back([&, i] {
                thread_pool->stop_and_wait();
            });
        }
        for (auto& th : threads) {
            th.join();
        }
    }
    //thread_pool->wait_and_stop();
}


// 测试正确的析构
std::atomic<int> des_cnt;

struct Foobar {
    ~Foobar() {
        des_cnt++;
    }
};

class Worker {
public:
    void work(std::shared_ptr<Foobar> foobar) {
    }
};

TEST_F(ThreadPoolTest, test_destructor) {
    int task_cnt = 100;
    {
        Worker w;
        des_cnt = 0;
        auto thread_pool = std::make_shared<ThreadPool>(5, 2000000);
        {
            for (size_t i = 0; i < task_cnt; ++i) {
                auto foobar = std::make_shared<Foobar>();
                thread_pool->enqueue(&Worker::work, &w, foobar);
            }
        }
        // 任务执行完后，传入的参数确保会自动析构
        LOG(NOTICE) << "shutdown begin..." ;
        thread_pool->stop_and_wait();
        LOG(NOTICE) << "shutdown done" ;
    }
    LOG(NOTICE) << "des_cnt -> " << des_cnt.load() ;
    ASSERT_EQ(des_cnt.load(), task_cnt);
}


TEST_F(ThreadPoolTest, test_thread_pool_executor) {
    ThreadPoolExecutor::instance().createThreadPool(5, 2000000);
    ThreadPoolExecutor::instance().execute(handler, 3, "hello");
    thread_pool_async(handler, 3, "hello");
    IndexCache index_cache;
    std::string query = "播放以父之名";
    std::string key = "bvc:index_cache:" + query;
    thread_pool_async(&IndexCache::update_cache, &index_cache, key);
}

TEST_F(ThreadPoolTest, test_thread_pool_executor_class) {
    ThreadPoolExecutor::instance().createThreadPool(5, 2000000);
    IndexCache index_cache;
    index_cache.async_update_cache_with_executor();
    index_cache.async_update_cache_with_thread_pool();
    sleep(2);
}
