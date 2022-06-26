#include "gtest/gtest.h"
#include "gflags/gflags.h"

#define private public
#define protected public
#include "nonlock_free_queue.h"
//#include "lock_free_queue.h"
#undef private
#undef protected

class LockFreeQueueTest : public ::testing::Test {
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

TEST_F(LockFreeQueueTest, case1) {
    fprintf(stderr, ">>> LockFreeQueueTest start...\n");
    rcu::LockFreeQueue<int> queue;
    int data1 = 5;
    int data2 = 9;

    queue.push(data1);
    queue.push(data2);

    ASSERT_TRUE(*queue.pop() == data1);
    ASSERT_TRUE(*queue.pop() == data2);
    ASSERT_TRUE(queue.pop().get() == nullptr);

    //ASSERT_TRUE(queue.empty() == true);
}

TEST_F(LockFreeQueueTest, case2) {
    fprintf(stderr, ">>> LockFreeQueueTest start...\n");
    rcu::LockFreeQueue<int> queue;

    int data1 = 5;

    queue.push(data1);

    ASSERT_TRUE(*queue.pop() == data1);
    ASSERT_TRUE(queue.pop().get() == nullptr);

    //ASSERT_TRUE(queue.empty() == true);
}

int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    google::ParseCommandLineFlags(&argc, &argv, false);
    return RUN_ALL_TESTS();
}
