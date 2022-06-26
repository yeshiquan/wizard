#pragma once

#include <memory>
#include <algorithm>
#include <bitset>

#include "debug.h"
//#include "concurrent/vector.h"

#include <baidu/feed/mlarch/babylon/concurrent/vector.h>

constexpr size_t CACHELINE_SIZE = 64;
namespace rcu {

using baidu::feed::mlarch::babylon::ConcurrentVector;

#ifndef DEFAULT_POOL_NUM
#define DEFAULT_POOL_NUM 1000
#endif

#ifndef DEFAULT_POOL_COLLECTIVE_NUM
#define DEFAULT_POOL_COLLECTIVE_NUM 100
#endif

#ifndef POOL_STATS
#define POOL_STATS false
#endif

#if POOL_STATS
#define INC_STATS(x) g_stat.x()
#else 
#define INC_STATS(x) 
#endif

using TaggedIndex = uint64_t;
constexpr uint64_t NULL_INDEX = 0xFFFFFFFFFFFFFFFFL;

// 补齐大小的结构体包装
template <typename T, size_t A>
class alignas(A) Aligned {
public:
    template <typename... Args>
    inline Aligned(Args&&... args) noexcept :
        _object(::std::forward<Args>(args)...) {}

    inline operator T&() noexcept {
        return _object;
    }

    inline operator const T&() const noexcept {
        return _object;
    }

    inline T& get() noexcept {
        return _object;
    }

private:
    T _object;
};

class PoolStat {
public:
    std::atomic<uint64_t> _push_local{0};
    std::atomic<uint64_t> _push_market{0};
    std::atomic<uint64_t> _push_collective{0};
    std::atomic<uint64_t> _pop_local{0};
    std::atomic<uint64_t> _pop_market{0};
    std::atomic<uint64_t> _pop_collective{0};
    std::atomic<uint64_t> _create{0};
    std::atomic<uint64_t> _destroy{0};
public:
    void push_local() { _push_local++; }
    void push_market() { _push_market++; }
    void push_collective() { _push_collective++; }
    void pop_local() { _pop_local++; }
    void pop_market() { _pop_market++; }
    void pop_collective() { _pop_collective++; }
    void create() { _create++; }
    void destroy() { _destroy++; }
    void print() {
        std::cout << "PoolStat -> " 
                    << " push_local:" << _push_local.load()
                    << " push_market:" << _push_market.load()
                    << " push_collective:" << _push_collective.load()
                    << " pop_local:" << _pop_local.load()
                    << " pop_market:" << _pop_market.load()
                    << " pop_collective:" << _pop_collective.load()
                    << " create:" << _create.load()
                    << " destroy:" << _destroy.load()
                    << std::endl;
    }
};
static PoolStat g_stat;

namespace helper {

inline uint64_t get_index(TaggedIndex tagged_index) {
    return tagged_index & 0x00000000FFFFFFFFL;
}

inline uint64_t get_tag(TaggedIndex tagged_index) {
    return tagged_index >> 32;
}

inline TaggedIndex make_tagged_index(uint64_t index, uint64_t tag) {
    return index | tag << 32;
}

template<typename NODE>
class LinkedList {
public:
    LinkedList() = default;
    LinkedList(NODE* node) noexcept 
            : head(node), tail(node) {
    }
    void push_back(NODE* node) noexcept {
        if (tail) {
            // 同一个node添加2次，版本要递增，否则会出现ABA问题
            // 链表为：head->A->B->A->tail
            // 线程1做head.cas(A,C)，被抢占; 
            // 线程2连续pop(A), pop(B)，新的head为A
            // 线程1继续执行，cas成功
            tail->next = make_tagged_index(node->index, node->advance_tag());
        } else {
            head = node;
        }
        tail = node;
        //std::cout << "push node:" << node << " index:" << node->index << std::endl;
    }
    void clear() {
        head = nullptr;
        tail = nullptr;
    }
public:
    NODE* head = {nullptr};
    NODE* tail = {nullptr};
};

template<typename NODE>
int print_list(std::string name, std::vector<NODE>& nodes, uint64_t index) {
    int cnt = 0;
    std::cout << "PRINT_LIST " << name << " -> ";
    while (index != helper::get_index(NULL_INDEX)) {
        cnt++;
        std::cout << index << " -> ";
        index = helper::get_index(nodes[index].next);
    }
    std::cout << " list_size:" << cnt << std::endl;
    return cnt;
}


} // namespace helper

template<typename T, bool USE_ACC = false, size_t TC_SIZE = 10>
class ObjectPool {
public:
    struct Node {
        T* object = {nullptr};
        TaggedIndex next = {NULL_INDEX};
        uint64_t index = 0;
        uint64_t tag = 0;
        bool is_collective = {false};
        size_t size = 0;
        uint64_t advance_tag() {
            return tag++;
        }
    }; // class Node

    struct ThreadCache {
        size_t size = 0;
        Node* nodes[0];

        static ThreadCache& instance() noexcept {
            static thread_local Aligned<ThreadCache, CACHELINE_SIZE> instance;
            return instance.get();
        }

        Node* get_from_tc() noexcept {
            if (size < 1)  {
                return nullptr;
            }
            return nodes[--size];
        }

        constexpr size_t get_capacity() noexcept {
            return std::min(TC_SIZE, MAX_NODE);
        }

        bool release_to_tc(Node* node) noexcept {
            if (size < get_capacity()) {
                nodes[size++] = node;
                return true;
            }
            return false;
        }
    }; // class ThreadCache

    // 线程缓存访问器
    class ThreadCacheAccessor {
    public:
        ThreadCacheAccessor(ObjectPool& pool) noexcept :
            _tid(syscall(__NR_gettid)),
            _cache(&static_cast<ThreadCache&>(pool._caches.ensure(_tid))),
            _pool_id(pool.id())
        {}
        ThreadCache* get(ObjectPool& pool) noexcept {
            if (_pool_id == pool.id()) {
                return _cache;
            }
            _cache = &static_cast<ThreadCache&>(pool._caches.ensure(_tid));
            _pool_id = pool.id();
            return _cache;
        }

    private:
        pid_t _tid;
        ThreadCache* _cache;
        size_t _pool_id;
    };

    class PooledObject {
    public:
        PooledObject(Node* node, ObjectPool* pool) noexcept : 
                _object(node->object), _node(node), _pool(pool) {
        }
        PooledObject(T* object) noexcept : 
                _object(object) {
        }
        ~PooledObject() noexcept {
            release();
        }
        T* get_obj() noexcept {
            return _object;
        }
        void release() noexcept {
            if (likely(_pool && _node)) {
                _pool->release(_node);
            } else if (unlikely(_object)) {
                INC_STATS(destroy);
                delete _object;
            } else {
            }
            _node = nullptr;
            _pool = nullptr;
            _object = nullptr;
        }
        PooledObject(PooledObject const&) = delete;             // Copy construct
        PooledObject& operator=(PooledObject const&) = delete;  // Copy assign
        PooledObject(PooledObject&& o) noexcept :
                _node(o._node), _object(o._object), _pool(o._pool) {
            o._node = nullptr;
            o._pool = nullptr;
            o._object = nullptr;
        }
        T& operator*() noexcept {
            return *_object;
        }
        T* operator->() noexcept {
            return _object;
        }
        PooledObject& operator=(PooledObject && o) noexcept {
            swap(o);
            o.release();
            return *this;
        }
        void swap(PooledObject& o) {
            std::swap(_object, o._object);
            std::swap(_node, o._node);
            std::swap(_pool, o._pool);
        }
    private:
        Node* _node = {nullptr}; 
        ObjectPool* _pool = {nullptr};
        T* _object = {nullptr};
    }; // class PooledObject

    static constexpr size_t MAX_NODE = (CACHELINE_SIZE - sizeof(ThreadCache)) / sizeof(Node*);

    ObjectPool() noexcept {
        init(DEFAULT_POOL_NUM, DEFAULT_POOL_COLLECTIVE_NUM);
    }
    ObjectPool(size_t num, size_t collective_num) noexcept {
        init(num, collective_num);
    }
    void init(size_t num, size_t collective_num) noexcept {
        LOG(NOTICE) << "pool init num:" << num << " collective_num:" << collective_num;
        _nodes.resize(num);
        decltype(_objects) objects(num);
        _objects.swap(objects);
        helper::LinkedList<Node> market_list;
        helper::LinkedList<Node> collective_list;
        // index = 0 保留，作为空index的标志，类似nullptr
        for (uint64_t i = 0; i < num; ++i) {
            Node* node = &_nodes[i];
            node->object = &_objects[i];
            node->index = i;
            if (i < collective_num) {
                node->is_collective = true;
                collective_list.push_back(node);
                //helper::print_list("collective_list", _nodes, collective_list.head->index);
            } else {
                node->is_collective = false;
                market_list.push_back(node);
                //helper::print_list("market_list", _nodes, market_list.head->index);
            }
        }
        push_into(_market_head, &market_list);
        push_into(_collective_head, &collective_list);
    }

    Node* try_pop() noexcept {
        Node* node = nullptr;

        if (USE_ACC) {
            node = try_pop_local();
        } else {
            node = ThreadCache::instance().get_from_tc();
        }

        if (node) {
            INC_STATS(pop_local);
            return node;
        }
        node = pop_one_node(_market_head);
        if (node) {
            INC_STATS(pop_market);
            return node;
        }
        node = pop_one_node(_collective_head);
        if (node) {
            INC_STATS(pop_collective);
            return node;
        }
        return node;
    }

    PooledObject try_get() noexcept {
        Node* node = try_pop();
        if (likely(node)) {
            return PooledObject(node, this);
        }
        return PooledObject();
    }

    PooledObject get() noexcept {
        Node* node = try_pop();
        if (likely(node)) {
            return PooledObject(node, this);
        }
        auto* obj = new T;
        INC_STATS(create);
        return PooledObject(obj);
    }

    void release(Node* node) noexcept {
        DCHECK(node);
        node->object->~T();
        if (node->is_collective) {
            INC_STATS(push_collective);
            push_one_node(_collective_head, node);
        } else {
            bool ret = false;
            if (USE_ACC) {
                ret = push_local(node);
            } else {
                ret = ThreadCache::instance().release_to_tc(node);
            }
            if (ret) {
                INC_STATS(push_local);
                return;
            }
            INC_STATS(push_market);
            push_one_node(_market_head, node);
        }
    }

    Node* pop_one_node(std::atomic<TaggedIndex>& head) noexcept {
        TaggedIndex old_head = head.load(std::memory_order_relaxed);
        TaggedIndex new_head;
        Node* head_node = nullptr;
        do  {
            if (unlikely(old_head == NULL_INDEX)) {
                return nullptr;
            }
            auto index = helper::get_index(old_head);
            head_node = &_nodes[index];
            new_head = head_node->next;
            // 不需要让new_head的版本递增, 只需要在插入节点的时候递增版本
        } while (!head.compare_exchange_weak(
                        old_head,
                        new_head,
                        std::memory_order_release,
                        std::memory_order_acquire));
        //std::cout << "pop_one_node:" << head_node << std::endl;
        return head_node;
    }

    void push_one_node(std::atomic<TaggedIndex>& head, Node* node) noexcept {
        DCHECK(node);
        helper::LinkedList<Node> list(node);
        push_into(head, &list);
    }

    bool push_local(Node* node) {
        static thread_local ThreadCacheAccessor acc(*this);
        ThreadCache* tc = acc.get(*this);
        return tc->release_to_tc(node);
    }

    Node* try_pop_local() {
        static thread_local ThreadCacheAccessor acc(*this);
        ThreadCache* tc = acc.get(*this);
        return tc->get_from_tc();
    }

    void push_into(std::atomic<TaggedIndex>& head, helper::LinkedList<Node>* list) noexcept {
        DCHECK(list);
        TaggedIndex old_head = head.load(std::memory_order_relaxed);
        auto tag = list->head->advance_tag();
        // ABA问题的关键是在插入操作的时候确保新head的版本能递增
        // 例如在t1 t2 t3的时候头结点分别为A B A，说明在t3做了一次插入A的操作
        // 如果有版本递增，t3时刻的版本为A1, 不等于A，就解决了ABA问题
        // 对于pop操作，下一个节点将成文头结点，但不需要把下1个节点的版本递增。
        TaggedIndex new_head = helper::make_tagged_index(list->head->index, tag);
        do  {
            list->tail->next = old_head;
        } while (!head.compare_exchange_weak(
                        old_head,
                        new_head,
                        std::memory_order_release,
                        std::memory_order_acquire));
    }
    size_t id() {
        return _id;
    }

    void print() {
        helper::print_list("collective_list", _nodes, helper::get_index(_collective_head.load()));
        helper::print_list("market_list", _nodes, helper::get_index(_market_head.load()));
    }
private:
    static std::atomic<size_t> _s_next_id;
    // 集体经济，可以借，但是借了必须归还给集体，不能私有保存
    std::atomic<TaggedIndex> _collective_head = {NULL_INDEX}; 
    std::vector<Node> _nodes;
    std::vector<T> _objects;
    size_t _id {_s_next_id.fetch_add(1, std::memory_order_relaxed)};
    std::vector<std::unique_ptr<T>> _object_ptrs;
    ConcurrentVector<Aligned<ThreadCache, CACHELINE_SIZE>, 32768> _caches {0};
    // 市场经济，可竞争获取，私有保存
    std::atomic<TaggedIndex> _market_head = {NULL_INDEX};     
};

template<typename T, bool USE_ACC, size_t TC_SIZE>
::std::atomic<size_t> ObjectPool<T, USE_ACC, TC_SIZE>::_s_next_id;

} // namespace
