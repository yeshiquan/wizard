#include "gtest/gtest.h"
#include "gflags/gflags.h"
#include <thread>
#include <chrono>
#include <random>

#define USE_THREAD_LOCAL_CACHE false

#define private public
#define protected public
#include "hazptr_stack.h"
#include "concurrent/hazptr.h"
#undef private
#undef protected

int intRand(const int & min, const int & max) {
    static thread_local std::mt19937 generator;
    std::uniform_int_distribution<int> distribution(min,max);
    return distribution(generator);
}

class HazptrTest : public ::testing::Test {
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

class MyNode : public rcu::HazptrNode<MyNode> {
public:
    MyNode() {}
    ~MyNode() {
        std::cout << "~MyNode()" << std::endl;
    }
    char data[777];
    std::string name = "Hello World";
};


auto print_rec_list = [](std::string name, rcu::HazptrRecord* head) -> int {
    int cnt = 0;
    std::cout << "PRINT_LIST " << name << " -> ";
    for (auto p = head; p != nullptr; p = p->next) {
        cnt++; 
        std::cout << p->realptr << "(" << p->active << ") -> ";
    }
    std::cout << " list_size:" << cnt << std::endl;
    return cnt;
};

auto print_obj_list = [](std::string name, rcu::HazptrObj* head) -> int {
    int cnt = 0;
    std::cout << "PRINT_LIST " << name << " -> ";
    for (auto p = head; p != nullptr; p = p->next) {
        cnt++; 
        std::cout << p << " -> ";
    }
    std::cout << " list_size:" << cnt << std::endl;
    return cnt;
};

TEST_F(HazptrTest, HazptrPool) {
    using rcu::HazptrRecord;
    rcu::HazptrPool pool;

    HazptrRecord* rec = pool.get_from_pool();
    ASSERT_EQ(rec, nullptr);
    ASSERT_EQ(pool._available_cnt, 0);

    rec = new HazptrRecord();
    pool.release_to_pool(rec);
    ASSERT_EQ(pool._available_cnt, 1);

    rec = pool.get_from_pool();
    ASSERT_TRUE(rec != nullptr);
    ASSERT_EQ(pool._available_cnt, 0);
}

TEST_F(HazptrTest, priv_push) {
    auto& manager = rcu::get_default_manager();
    auto& priv = rcu::HazptrPrivate::instance();
    priv.reset();
    manager.reset();

    {
        auto* node = new MyNode();
        priv.push(node);
        int cnt = print_obj_list("priv_list", priv.priv_list.head);
        std::cout << "tail -> " << priv.priv_list.tail << std::endl;
        ASSERT_EQ(cnt, 1);
    }

    {
        auto* node = new MyNode();
        priv.push(node);
        int cnt = print_obj_list("priv_list", priv.priv_list.head);
        std::cout << "tail -> " << priv.priv_list.tail << std::endl;
        ASSERT_EQ(cnt, 2);
    }
}

TEST_F(HazptrTest, append) {
    auto& manager = rcu::get_default_manager();
    auto& priv = rcu::HazptrPrivate::instance();
    priv.reset();
    manager.reset();
 
    auto* node1 = new MyNode();
    auto* node2 = new MyNode();
    priv.push(node1);
    priv.push(node2);

    manager.append(priv.priv_list.head, priv.priv_list.tail, priv.priv_list.count);
    int cnt = print_obj_list("_retired_list", manager._retired_list.load());
    ASSERT_EQ(cnt, 2);
 
    priv.reset();
    auto* node3 = new MyNode();
    auto* node4 = new MyNode();
    priv.push(node3);
    priv.push(node4);

    manager.append(priv.priv_list.head, priv.priv_list.tail, priv.priv_list.count);
    cnt = print_obj_list("_retired_list", manager._retired_list.load());
    ASSERT_EQ(cnt, 4);
}

TEST_F(HazptrTest, acquire_record) {
    auto& manager = rcu::get_default_manager();
    auto& priv = rcu::HazptrPrivate::instance();
    priv.reset();
    manager.reset();

    {
        auto* rec = manager.acquire_record();
        ASSERT_TRUE(rec != nullptr);
        ASSERT_EQ(manager._hazptr_count, 1);
        int cnt = print_rec_list("_hazptr_list", manager._hazptr_list.load());
        ASSERT_EQ(cnt, 1);
    }

    {
        auto* rec = manager.acquire_record();
        ASSERT_TRUE(rec != nullptr);
        ASSERT_EQ(manager._hazptr_count, 2);
        int cnt = print_rec_list("_hazptr_list", manager._hazptr_list.load());
        ASSERT_EQ(cnt, 2);
    }
}

TEST_F(HazptrTest, private_push) {
    auto& manager = rcu::get_default_manager();
    auto& priv = rcu::HazptrPrivate::instance();
    priv.reset();
    manager.reset();

    MyNode* node = new MyNode;
    priv.push(node);
    ASSERT_EQ(priv.priv_list.head, node);
    ASSERT_EQ(priv.priv_list.tail, node);

    MyNode* node2 = new MyNode;
    priv.push(node2);
    ASSERT_EQ(priv.priv_list.tail, node2);
    ASSERT_TRUE(priv.priv_list.head != node2);

    int cnt = print_obj_list("_priv_list", priv.priv_list.head);
    ASSERT_EQ(cnt, 2);
}

TEST_F(HazptrTest, reset_pretect) {
    auto& manager = rcu::get_default_manager();
    auto& priv = rcu::HazptrPrivate::instance();
    priv.reset();
    manager.reset();

    int cnt = 1000;
    std::vector<MyNode*> nodes;
    for (int i = 0; i < cnt; ++i) {
        MyNode* node = new MyNode;
        nodes.push_back(node);
    }

    rcu::HazptrHolder holder;
    int id1 = intRand(1, cnt-1);
    holder.reset(nodes[id1]);

    rcu::HazptrHolder holder2;
    int id2 = intRand(1, cnt-1);
    holder2.reset(nodes[id2]);

    holder.swap(holder2);
    holder2.swap(holder);

    std::vector<std::thread> threads;
    for (size_t i = 0; i < cnt; ++i) {
        threads.emplace_back([&, i] {
            nodes[i]->retire();
        });
    }
    for (auto& th : threads) {
        th.join();
    }

    auto name1 = nodes[id1]->name;
    auto name2 = nodes[id2]->name;
    std::cout << id1 << " -> " << name1 << std::endl;
    std::cout << id2 << " -> " << name2 << std::endl;
    ASSERT_TRUE(name1 == "Hello World");
    ASSERT_TRUE(name2 == "Hello World");
    //auto name3 = nodes[5]->name;
    //ASSERT_TRUE(name3 == "Hello World");
}

TEST_F(HazptrTest, try_protect) {
    auto& manager = rcu::get_default_manager();
    auto& priv = rcu::HazptrPrivate::instance();
    priv.reset();
    manager.reset();

    print_rec_list("hazptr_list", manager._hazptr_list.load());

    auto count_active = [](rcu::HazptrRecord* head) -> int {
        int cnt = 0;
        for (auto p = head; p != nullptr; p = p->next) {
            if (p->active) cnt++; 
        }
        return cnt;
    };

    int cnt = 4;
    std::cout << "\nretire " << cnt << " element  ==================" << std::endl;
    std::atomic<MyNode*> last_node;
    for (int i = 0; i < cnt; ++i) {
        rcu::HazptrHolder holder;
        rcu::HazptrHolder holder2;
        MyNode* node = new MyNode;
        std::atomic<MyNode*> src;
        src.store(node);
        last_node.store(node);
        bool ret = holder.try_protect(node, src);
        ret = holder2.try_protect(node, src);
        ASSERT_TRUE(ret == true);
        node->retire();
        ASSERT_EQ(priv.priv_list.count,  i+1);
        print_rec_list("hazptr_list", manager._hazptr_list.load());
        print_obj_list("private retired_list", priv.priv_list.head);
        print_obj_list("global retired_list", manager._retired_list.load());
        if (!USE_THREAD_LOCAL_CACHE) {
            ASSERT_EQ(count_active(manager._hazptr_list.load()), 2);
        }
    }
    {
        std::cout << "\nretire another element ==================" << std::endl;
        print_rec_list("hazptr_list 1", manager._hazptr_list.load());
        rcu::HazptrHolder holder;
        rcu::HazptrHolder holder2;
        MyNode* node = new MyNode;
        std::atomic<MyNode*> src;
        src.store(node);
        MyNode* p_last_node = last_node.load();
        bool ret = holder2.try_protect(p_last_node, last_node);
        ret = holder.try_protect(node, src);
        print_rec_list("hazptr_list 2", manager._hazptr_list.load());
        ASSERT_TRUE(ret == true);
        if (!USE_THREAD_LOCAL_CACHE) {
            ASSERT_EQ(count_active(manager._hazptr_list.load()), 2);
        }
        // 达到阈值，触发bulkReclaim
        std::cout << "retire object -> " << node << std::endl;
        node->retire();
        print_rec_list("hazptr_list", manager._hazptr_list.load());
        int rcount = print_obj_list("global retired_list", manager._retired_list.load());
        LOG(NOTICE) << "priv.priv_list.count:" << priv.priv_list.count;
        ASSERT_TRUE(manager._retired_count == rcount);
    }
}

template <typename T>
class LockFreeLIFO {
    class Node : public rcu::HazptrNode<Node> {
    friend LockFreeLIFO;
   public:
    ~Node() {
    }
   private:
    Node(T v, Node* n) : value_(v), next_(n) {
    }
    T value_;
    Node* next_;
  };

 public:
  LockFreeLIFO() {
  }

  ~LockFreeLIFO() {
  }

  void push(T val) {
      auto pnode = new Node(val, head_.load());
      while (!head_.compare_exchange_weak(pnode->next_, pnode));
  }

  bool pop(T& val) {
      rcu::HazptrHolder hptr;
      Node* pnode = head_.load();
      do {
          if (pnode == nullptr) {
              return false;
          }
          if (!hptr.try_protect(pnode, head_)) {
              continue;
          }
          auto next = pnode->next_;
          if (head_.compare_exchange_weak(pnode, next)) break;
      } while (true);
      //hptr.reset();
      val = pnode->value_;
      pnode->retire();
      return true;
  }

private:
    std::atomic<Node*> head_ = {nullptr};
};


LockFreeLIFO<int> queue;


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

TEST_F(HazptrTest, case_queue) {
    /*
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
    */
}

int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    google::ParseCommandLineFlags(&argc, &argv, false);
    return RUN_ALL_TESTS();
}
