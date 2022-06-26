#pragma once

// TODO
// 1 分离.h和.hpp文件 done
// 2 析构函数 done
// 3 内存align done
// 4 memory barrier done
// 5 List HashMap的应用
// 6 public改成private，使用friend class

#include <atomic>
#include <unordered_set>

namespace rcu {

#ifndef HAZPTR_AMB
#define HAZPTR_AMB true
#endif

#ifndef USE_THREAD_LOCAL_CACHE
#define USE_THREAD_LOCAL_CACHE false
#endif

#ifndef HAZPTR_RECORD_CACHE_SIZE
#define HAZPTR_RECORD_CACHE_SIZE 10
#endif

#ifndef HAZPTR_SCAN_MULT
#define HAZPTR_SCAN_MULT 2
#endif

#ifndef HAZPTR_SCAN_THRESHOLD
#define HAZPTR_SCAN_THRESHOLD 5
#endif


template<typename T>
class DoubleLinkedList {
public:
    void push(T* node) {
        node->next = nullptr;
        if (tail) {
            tail->next = node;    
        } else {
            head = node;
        }
        tail = node;
        count++;
    }
    void clear() {
        head = nullptr;
        tail = nullptr;
        count = 0;
    }
public:
    T* head = {nullptr};
    T* tail = {nullptr};
    int count = {0};
};

class HazptrObj {
public:
    const void* get_ptr() {
        return this;
    }
    virtual ~HazptrObj() {}
public:
    HazptrObj* next = {nullptr};
};

class HazptrPrivate;
class HazptrManager;
HazptrManager& get_default_manager();

class HazptrRecord {
public:
    void clear() {
        realptr.store(nullptr);
    }
public:
    HazptrRecord* next {nullptr};
    std::atomic<bool> active {true};
    std::atomic<const void*> realptr {nullptr};
};

class HazptrPool {
public:
    HazptrRecord* get_from_pool() {
        if (_available_cnt == 0) {
            return nullptr;
        }
        return _record_cache[--_available_cnt];
    }
    ~HazptrPool() {
        LOG(NOTICE) << "~HazptrPool() thread_id:" << std::this_thread::get_id();
        for (int i = 0; i < _available_cnt.load(); ++i) {
            auto* p = _record_cache[i];
             _record_cache[i] = nullptr;
             p->active.store(false, std::memory_order_release);
        }
        _available_cnt = 0;
    }
    void release_to_pool(HazptrRecord* rec) {
        if (_available_cnt < HAZPTR_RECORD_CACHE_SIZE) {
            _record_cache[_available_cnt++] = rec;
        }
    }

    void reset() {
        _available_cnt = 0;
    }
private:
    std::atomic<int> _available_cnt = {0};
    HazptrRecord* _record_cache[HAZPTR_RECORD_CACHE_SIZE];
};

class HazptrPrivate {
public:
    static HazptrPrivate& instance() {
        static thread_local class HazptrPrivate instance;
        return instance;
    }
public:
    HazptrPrivate() {
        _manager = &get_default_manager();
    }
    ~HazptrPrivate();
    HazptrRecord* get_cached_rec() {
        return pool.get_from_pool();
    }
    void free_cached_rec(HazptrRecord* rec) {
        pool.release_to_pool(rec);
    }

    void push(HazptrObj* node);
    void pushAlltoDomain();

    void reset() {
        pool.reset();
        priv_list.clear();
    }
public:
    DoubleLinkedList<HazptrObj> priv_list;
    HazptrPool pool;
    HazptrManager* _manager = {nullptr};
};


template<typename T>
class HazptrNode : public HazptrObj {
public:
    void retire() {
        HazptrPrivate::instance().push(this);
    }
    virtual ~HazptrNode() {}
private:
};

class HazptrManager {
public:
    ~HazptrManager();
    bool reached_threshold(int rcount);
    void append(HazptrObj* head, HazptrObj* tail, int count);
    void tryBulkReclaim();
    void bulkReclaim();
    HazptrRecord* acquire_record();
    void release_record(HazptrRecord* rec);

    void reset() {
        _hazptr_list.store(nullptr);
        _retired_list.store(nullptr);
        _hazptr_count.store(0);
        _retired_count.store(0);
    }
private:
    std::atomic<HazptrRecord*> _hazptr_list = {nullptr};
    std::atomic<HazptrObj*> _retired_list = {nullptr};
    std::atomic<int> _retired_count = {0};
    std::atomic<int> _hazptr_count = {0};
};

class HazptrHolder {
public:
    HazptrHolder() {
        _manager = &get_default_manager();
        _rec = _manager->acquire_record();
    }

    ~HazptrHolder() {
        _rec->clear();
        if (_manager) {
            _manager->release_record(_rec);
        }
    }
    void swap(HazptrHolder& other) {
        std::swap(_manager, other._manager);
        std::swap(_rec, other._rec);
    }

    template<typename T>
    void reset(const T* ptr) {
        _rec->realptr.store(ptr);
    }

    void reset() {
        _rec->realptr.store(nullptr);
    }

    template<typename T>
    bool try_protect(T* &p, const std::atomic<T*>& src);

    template<typename T>
    T* get_protected(const std::atomic<T*>& src);

private:
    HazptrManager* _manager;
    HazptrRecord* _rec;
};

} // namespace

#include "hazptr.hpp"
