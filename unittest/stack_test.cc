#include "gtest/gtest.h"
#include "gflags/gflags.h"

#define private public
#define protected public
#include "stack.h"
#undef private
#undef protected

class LockFreeStackTest : public ::testing::Test {
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

TEST_F(LockFreeStackTest, case1) {
    fprintf(stderr, ">>> LockFreeStackTest start...\n");
    rcu::LockFreeStack<int> stack;
    int data1 = 5;
    int data2 = 9;
    stack.push(data1);
    stack.push(data2);
    ASSERT_TRUE(*stack.pop() == data2);
    ASSERT_TRUE(*stack.pop() == data1);
    //ASSERT_TRUE(stack.empty() == true);
}

TEST_F(LockFreeStackTest, case2) {
    fprintf(stderr, ">>> LockFreeStackTest start...\n");
    rcu::LockFreeStack<int> stack;

    int data1 = 5;

    {
        stack.push(data1);
        ASSERT_TRUE(*stack.pop() == data1);
    }

    {
        stack.push(data1);
        ASSERT_TRUE(*stack.pop() == data1);
    }

    //ASSERT_TRUE(stack.empty() == true);
}

int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    google::ParseCommandLineFlags(&argc, &argv, false);
    return RUN_ALL_TESTS();
}
