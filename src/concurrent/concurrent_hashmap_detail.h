#pragma once

#define USE_THREAD_LOCAL_CACHE false
#define HAZPTR_RECORD_CACHE_SIZE 10
#define HAZPTR_SCAN_MULT 2
#define HAZPTR_SCAN_THRESHOLD 5

#include <memory>
#include <mutex>
#include <type_traits>
#include "hazptr.h"
//#include "folly_hazptr.h"
#include "memory_resource.h"

#define DEBUG 0

namespace rcu {

template<typename T>
int print_list(std::string name, T* head) {
    return 0;
    int cnt = 0;
    if (DEBUG) std::cout << "PRINT_LIST " << name << " -> ";
    for (auto p = head; p != nullptr; p = p->next_.load()) {
        cnt++;
        if (DEBUG) std::cout << p->value_holder_.key << " -> ";
    }
    if (DEBUG) std::cout << " list_size:" << cnt << std::endl;
    return cnt;
}

template <class T>
inline constexpr
typename std::enable_if<
  (std::is_integral<T>::value &&
   std::is_unsigned<T>::value &&
   sizeof(T) > sizeof(unsigned int) &&
   sizeof(T) <= sizeof(unsigned long)),
  unsigned int>::type
findLastSet(T x) {
    return x ? ((8 * sizeof(unsigned long) - 1) ^ __builtin_clzl(x)) + 1 : 0;
}

template <class T>
inline constexpr
typename std::enable_if<
  std::is_integral<T>::value && std::is_unsigned<T>::value,
  T>::type
nextPowTwo(T v) {
    return v ? (T(1) << findLastSet(v - 1)) : 1;
}

class HazptrDeleter {
 public:
  template <typename Node>
  void operator()(Node* node) {
    delete node;
  }
};

template <typename KeyType, typename ValueType>
struct ValueHolder {
    // 使用初始化列表，原地构造key和value
    template <typename Arg, typename... Args>
    explicit ValueHolder(Arg&& k, Args&&... args)
      : key(std::forward<Arg>(k)), value(std::forward<Args>(args)...) {
    }

    KeyType key;
    ValueType value;
};

template <typename KeyType, typename ValueType>
class NodeT : public rcu::HazptrNode<NodeT<KeyType,ValueType>> {
//class NodeT : public folly::hazptr::hazptr_obj_base<NodeT<KeyType, ValueType>, HazptrDeleter> {
//class NodeT : public TestObj {
//class NodeT {
    //friend ConcurrentHashMapSegment;
public:
    // 默认构造无效空实例
    inline NodeT() noexcept = default;

    // 使用初始化列表，原地构造
    template <typename...Args>
    explicit NodeT(Args&&... args) :
        value_holder_(std::forward<Args>(args)...) {
    }

    inline NodeT(const NodeT&) noexcept = delete;
    inline NodeT& operator=(const NodeT&) noexcept = delete;
    inline NodeT(NodeT&&) noexcept = delete;
    inline NodeT& operator=(NodeT&&) noexcept = delete;

    ValueHolder<KeyType, ValueType>* get_value_holder() {
        return &value_holder_;
    }

    void release() {
        //LOG(NOTICE) << "release refcount:" << (uint32_t)refcount_.load();
        DCHECK(refcount_.load() != 0);
        if (refcount_.fetch_sub(1) == 1) {
            this->retire();
            //this->retire(folly::hazptr::default_hazptr_domain(), HazptrDeleter());
        }
    }

    void acquire() {
        refcount_.fetch_add(1);
    }

    ~NodeT() {
        auto next = next_.load(std::memory_order_acquire);
        if (next) {
            next->release();
        }
    }
public:
    std::atomic<NodeT*> next_ = {nullptr};
    ValueHolder<KeyType, ValueType>  value_holder_;
    std::atomic<uint16_t>  refcount_{1}; //链表上的每个node初始化都被上一个node引用
};

template <
    typename KeyType, 
    typename ValueType, 
    typename HashFn,
    uint8_t ShardBits>
class ConcurrentHashMapSegment {
    enum class InsertType {
        DISCARD_IF_EXIST,
        REPLACE_IF_EXIST,
        ANY
    };
public:
    using Node = NodeT<KeyType, ValueType>;
    class BucketList : public HazptrNode<BucketList> {
    //class BucketList : public folly::hazptr::hazptr_obj_base<BucketList, HazptrDeleter> {
    public:
        explicit BucketList(size_t buckets_count) {
            buckets_count_ = buckets_count;
            buckets_ = new std::atomic<Node*>[bucket_count()];
            for (int i = 0; i < bucket_count(); ++i) {
                buckets_[i] = nullptr;
            }
        }
        ~BucketList() {
            for (int i = 0; i < bucket_count(); ++i) {
                Node* head = get_head(i);
                if (head) {
                    head->release();
                }
            }
            //delete buckets_;
        }
        Node* get_head(int bucket_id) {
            return buckets_[bucket_id].load(std::memory_order_acquire);
        }
        std::atomic<Node*>* get_atomic_head(int bucket_id) {
            return &buckets_[bucket_id];
        }
        void set_head(int bucket_id, Node* node) {
            buckets_[bucket_id].store(node, std::memory_order_release);
        }
        size_t bucket_count() {
            return buckets_count_;
        }
    private:
        size_t buckets_count_ = {0};
        std::atomic<Node*>* buckets_ = {nullptr};
    };
    class Iterator {
    public:
        Iterator() {}
        ~Iterator() {}

        void init(BucketList* bucket_list) {
            bucket_list_ = bucket_list;
            node_ = bucket_list_->get_head(bucket_id_);
            advanceBucketIfAtEnd();
        }

        const ValueType& operator*() const {
            DCHECK(node_);
            return node_->get_value_holder()->value;
        }

        const ValueType* operator->() const {
            DCHECK(node_);
            return &(node_->get_value_holder()->value);
        }

        void advanceBucketIfAtEnd() {
            LOG(NOTICE) << "advanceBucketIfAtEnd";
            while (node_ == nullptr) {
                bucket_id_++;
                if (bucket_id_ >= bucket_list_->bucket_count()) {
                    break;
                }
                DCHECK(bucket_list_);
                DCHECK(bucket_list_->buckets_);
                //auto* atomic_head = bucket_list_->get_atomic_head(bucket_id_);
                //node_ = holder_node.get_protected(*atomic_head);
                node_ = holder_node.get_protected(bucket_list_->buckets_[bucket_id_]);
            }
        }

        // 前置操作重载
        const Iterator& operator++() {
            DCHECK(node_);
            node_ = holder_node.get_protected(node_->next_);
            advanceBucketIfAtEnd();
            return *this;
        }

        // 后置操作重载
        Iterator operator++(int) {
            auto prev = *this;
            ++*this;
            return prev;
        }

        bool operator==(const Iterator& o) const {
            return node_ == o.node_;
        }

        bool operator!=(const Iterator& o) const {
            return !(*this == o);
        }

        Iterator& operator=(const Iterator& o) {
            node_ = o.node_;
            bucket_list_ = o.bucket_list_;
            bucket_id_ = o.bucket_id_;
            return *this;
        }

        Iterator(const Iterator& o) {
            node_ = o.node_;
            bucket_list_ = o.bucket_list_;
            bucket_id_ = o.bucket_id_;
        }

        Iterator(Iterator&& o) {
            node_ = o.node_;
            bucket_list_ = o.bucket_list_;
            bucket_id_ = o.bucket_id_;
        }
    private:
        Node* node_ = {nullptr};
        BucketList* bucket_list_ = {nullptr};
        uint64_t bucket_id_ = {0};
        HazptrHolder holder_bucket;
        HazptrHolder holder_node;
        //folly::hazptr::hazptr_holder holder_bucket;
        //folly::hazptr::hazptr_holder holder_node;
    };
public:
    ConcurrentHashMapSegment(size_t buckets_count, float load_factor) {
        buckets_count = nextPowTwo(buckets_count);
        bucket_list_ = new BucketList(buckets_count);
        set_load_factor(load_factor);
    }

    Iterator cbegin() {
        Iterator iter;
        iter.init(bucket_list_);
        return iter;
    }

    Iterator cend() {
        return Iterator();
    }

    size_t size() { return size_; }
    bool empty() { return size_ == 0; }

    template <typename Key, typename Value>
    bool insert(uint64_t hash, Key&& key, Value&& value) {
        Node* new_node = new Node(std::forward<Key>(key), std::forward<Value>(value));
        // Error: bool ret = insert_internal(hash, key, new_node); 不能使用被转发后的变量
        bool ret = insert_internal(hash, new_node->value_holder_.key, new_node);
        if (!ret) {
            delete new_node;
        }
        return ret;
    }

    bool emplace(uint64_t hash, const KeyType& key, Node* new_node) {
        return insert_internal(hash, key, new_node);
     }

    bool insert_internal(uint64_t hash, const KeyType& key, Node* new_node) {
        std::unique_lock<std::mutex> g(m_);

        BucketList* bucket_list = get_bucket_list();

        if (size_ >= load_factor_cnt_threshold_) {
            rehash(bucket_list->bucket_count() << 1);
            // reload latest bucket_list
            bucket_list = get_bucket_list();
        }
        auto bucket_id = (hash >> ShardBits) & (bucket_list->bucket_count() - 1);
        Node* head = bucket_list->get_head(bucket_id);
        Node* node = head;
        std::atomic<Node*>* prev = bucket_list->get_atomic_head(bucket_id);
        //std::cout << "size_:" << size_ << " load_factor_cnt_threshold_:" << load_factor_cnt_threshold_ << std::endl;
        // Check if key exist
        while (node) {
            if (key == node->value_holder_.key) {
                InsertType insert_type = InsertType::ANY;
                if (insert_type == InsertType::DISCARD_IF_EXIST) {
                    // Will not update existed node
                    return false;
                } else {
                    // Replace the existed node
                    Node* node_next = node->next_.load(std::memory_order_relaxed);
                    if (node_next) {
                        node_next->acquire();
                    }
                    new_node->next_.store(node_next, std::memory_order_relaxed);
                    prev->store(new_node, std::memory_order_release);
                    g.unlock();
                    // 回收不需要加锁
                    node->release();
                    return true;
                }
            }
            prev = &node->next_;
            node = node->next_.load(std::memory_order_relaxed);
        }
        size_++;
        new_node->next_.store(head, std::memory_order_relaxed);
        bucket_list->set_head(bucket_id, new_node);
        //print_list("bucket_id:" + std::to_string(bucket_id), new_node);
        return true;
    }

    void erase(uint64_t hash, const KeyType& key) {
        Node* node = nullptr;
        {
            std::lock_guard<std::mutex> g(m_);

            BucketList* bucket_list = get_bucket_list();
            auto bucket_id = (hash >> ShardBits) & (bucket_list->bucket_count() - 1);
            //print_list("before delete bucket_id:" + std::to_string(bucket_id) + " key:" + key, bucket_list_->get_head(bucket_id));
            Node* head = bucket_list->get_head(bucket_id);
            node = head;
            std::atomic<Node*>* prev = bucket_list->get_atomic_head(bucket_id);
            // find the node
            while (node) {
                if (key == node->value_holder_.key) {
                    break;
                }
                prev = &node->next_;
                node = node->next_.load(std::memory_order_relaxed);
            }
            if (node == nullptr) {
                // key not found
                return;
            }

            size_--;

            // separate node from list
            Node* node_next = node->next_.load(std::memory_order_relaxed);
            if (node_next) {
                node_next->acquire();
            }
            prev->store(node_next, std::memory_order_release);
        }
        // 回收不需要加锁
        if (node) {
            node->release();
        }
        //print_list("after delete bucket_id:" + std::to_string(bucket_id) + " key:" + key, bucket_list_->get_head(bucket_id));
    }

    bool find(size_t hash, Iterator& iter, const KeyType& key) {
        // 就算find加锁，也会出core
        // 因为iter拿到后锁就释放了，别的线程可以delete了，之后使用iter的时候可能已经被别的线程delete了
        // std::lock_guard<std::mutex> g(m_);
        // 链表遍历过程中一定要同时保护前后2个节点，否则会出core

        BucketList* bucket_list = iter.holder_bucket.get_protected(bucket_list_);
        auto bucket_id = (hash >> ShardBits) & (bucket_list->bucket_count() - 1);
        //std::atomic<Node*>& atomic_head = bucket_list->get_atomic_head(bucket_id);
        Node* node = iter.holder_node.get_protected(bucket_list->buckets_[bucket_id]);
        //folly::hazptr::hazptr_holder holder_next;
        HazptrHolder holder_next;
        while (node) {
            std::string node_key = node->value_holder_.key;
            if (key == node_key) {
                iter.node_ = node;
                iter.bucket_list_ = bucket_list;
                iter.bucket_id_ = bucket_id;
                return true;
            }
            //node = iter.holder_node.get_protected(node->next_);
            node = holder_next.get_protected(node->next_);
            holder_next.swap(iter.holder_node);
        }
        return false;
    }

    void rehash(size_t new_bucket_cnt) {
        BucketList* old_bucket_list = get_bucket_list();
        std::cout << "rehash -> old:" << old_bucket_list->bucket_count() << " new:" << new_bucket_cnt << std::endl;
        // hash     -> [47, 55, 35, 43, 51, 59]
        // hash % 4 -> [3, 3, 3, 3, 3, 3]  old bucket
        // hash % 8 -> [7, 7, 3, 3, 3, 3]  new bucket
        load_factor_cnt_threshold_ = new_bucket_cnt * load_factor_;
        auto* new_bucket_list = new BucketList(new_bucket_cnt);
        for (int i = 0; i < old_bucket_list->bucket_count(); ++i) {
            Node* head = old_bucket_list->get_head(i);
            for (auto* p = head; p != nullptr; p = p->next_.load(std::memory_order_relaxed)) {
                auto hash = HashFn()(p->value_holder_.key);
                auto bucket_id = (hash >> ShardBits) & (new_bucket_cnt - 1);
                Node* new_node = new Node(p->value_holder_.key, p->value_holder_.value);
                new_node->next_.store(new_bucket_list->get_head(bucket_id), std::memory_order_relaxed);
                new_bucket_list->set_head(bucket_id, new_node);
            }
        }
        bucket_list_.store(new_bucket_list, std::memory_order_release);
        old_bucket_list->retire();
        //old_bucket_list->retire(folly::hazptr::default_hazptr_domain(), HazptrDeleter());
    }

    float get_load_factor() const {
        return load_factor_;
    }

    void set_load_factor(float load_factor) {
        std::lock_guard<std::mutex> g(m_);
        BucketList* bucket_list = get_bucket_list();
        load_factor_ = load_factor;
        load_factor_cnt_threshold_ = bucket_list->bucket_count() * load_factor_;
    }

    void clear() {
        auto old_bucket_list = get_bucket_list();
        auto new_bucket_list = new BucketList(old_bucket_list->bucket_count());
        {
            std::lock_guard<std::mutex> g(m_);
            bucket_list_.store(new_bucket_list, std::memory_order_release);
            size_ = 0;
        }
        old_bucket_list->retire();
        //old_bucket_list->retire(folly::hazptr::default_hazptr_domain(), HazptrDeleter());
    }

    BucketList* get_bucket_list() {
        return  bucket_list_.load(std::memory_order_relaxed);
    }
private:
    std::mutex m_;
    std::atomic<BucketList*> bucket_list_ = {nullptr};
    size_t size_ = {0};
    size_t max_size_ = {0};
    float load_factor_;
    size_t load_factor_cnt_threshold_;
};

} // namespace


