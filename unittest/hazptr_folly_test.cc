#include "gtest/gtest.h"
#include "gflags/gflags.h"
#include <thread>
#include <chrono>
#include <random>

#define private public
#define protected public
#include "folly_hazptr.h"
#undef private
#undef protected

using namespace folly::hazptr;

class HazptrStackTest : public ::testing::Test {
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

template <typename T>
class LockFreeLIFO {
  class Node : public hazptr_obj_base<Node> {
    friend LockFreeLIFO;
   public:
    ~Node() {
      DEBUG_PRINT(this);
    }
   private:
    Node(T v, Node* n) : value_(v), next_(n) {
      DEBUG_PRINT(this);
    }
    T value_;
    Node* next_;
  };

 public:
  LockFreeLIFO() {
    DEBUG_PRINT(this);
  }

  ~LockFreeLIFO() {
    DEBUG_PRINT(this);
  }

  void push(T val) {
    DEBUG_PRINT(this);
    auto pnode = new Node(val, head_.load());
    while (!head_.compare_exchange_weak(pnode->next_, pnode));
  }

  bool pop(T& val) {
    DEBUG_PRINT(this);
    hazptr_holder hptr;
    hazptr_holder hptr2;
    Node* pnode = head_.load();
    do {
      if (pnode == nullptr)
        return false;
      if (!hptr.try_protect(pnode, head_))
        continue;
      {
      if (!hptr2.try_protect(pnode, head_))
        continue;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      auto next = pnode->next_;
      if (head_.compare_exchange_weak(pnode, next)) break;
    } while (true);
    hptr.reset();
    val = pnode->value_;
    pnode->retire();
    return true;
  }

 private:
  std::atomic<Node*> head_ = {nullptr};
};


LockFreeLIFO<int> queue;

int intRand(const int & min, const int & max) {
    static thread_local std::mt19937 generator;
    std::uniform_int_distribution<int> distribution(min,max);
    return distribution(generator);
}

void producer() {
    for (int i = 0; i < 1000000; i++) {
        int val = intRand(1, 20000);
        queue.push(val);
    }
}

void consumer() {
    while (true) {
        int val;
        queue.pop(val);
    }
}

/*
TEST_F(HazptrStackTest, case1) {
    fprintf(stderr, ">>> HazptrStackTest start...\n");

    std::vector<std::thread> threads;
    for (int j = 0; j < 10; j++) {
        std::thread t(producer);
        threads.emplace_back(std::move(t));
    }
    for (int j = 0; j < 10; j++) {
        std::thread t(consumer);
        threads.emplace_back(std::move(t));
    }
    for (auto& th : threads) {
        th.join();
    }
}
*/
class MyNode : public hazptr_obj_base<MyNode> {
public:
    MyNode() {}
    ~MyNode() {
        LOG(NOTICE) << "~MyNode()";
    }
    char data[52];
};

TEST_F(HazptrStackTest, case2) {
    int cnt = 4;
    for (int i = 0; i < cnt; ++i) {
        hazptr_holder holder;
        hazptr_holder holder2;

        MyNode* node = new MyNode;
        std::cout << "node -> " << node << std::endl;
        std::atomic<MyNode*> src;
        src.store(node);
        bool ret = holder.try_protect(node, src);
        ret = holder2.try_protect(node, src);
        node->retire();
        std::cout << "\n";
    }
    hazptr_holder holder;
    MyNode* node = new MyNode;
    std::cout << "node -> " << node << std::endl;
    std::atomic<MyNode*> src;
    src.store(node);
    bool ret = holder.try_protect(node, src);
    ASSERT_TRUE(ret == true);
    node->retire();
}

