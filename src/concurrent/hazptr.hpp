#pragma once

namespace rcu {

inline HazptrManager& get_default_manager() {
    static HazptrManager manager;
    return manager;
}

inline HazptrPrivate::~HazptrPrivate() {
    LOG(NOTICE) << "~HazptrPrivate() thread_id:" << std::this_thread::get_id();
    if (priv_list.count > 0) {
        pushAlltoDomain();
    }
}

inline void HazptrPrivate::push(HazptrObj* node) {
    priv_list.push(node);
    if (_manager->reached_threshold(priv_list.count)) {
        pushAlltoDomain();
    }
}

inline void HazptrPrivate::pushAlltoDomain() {
    _manager->append(priv_list.head, priv_list.tail, priv_list.count);
    priv_list.clear();
    _manager->tryBulkReclaim();
}

inline HazptrManager::~HazptrManager() {
    LOG(NOTICE) << "~HazptrManager()";
    { // free all remaining retired objects
        HazptrObj* next = nullptr;
        HazptrObj* p = _retired_list.exchange(nullptr);
        while (p) {
            for (; p != nullptr; p = next) {
                next = p->next;
                delete p;
            }
            p = _retired_list.exchange(nullptr);
        }
    }

    { // free all HazptrRecord
        HazptrRecord* next_rec = nullptr;
        HazptrRecord* rec = _hazptr_list.load(std::memory_order_acquire);
        for (; rec != nullptr; rec = next_rec) {
            next_rec = rec->next;
            delete rec;
        }
    }
}

inline bool HazptrManager::reached_threshold(int retire_cnt) {
    bool ret =  (
      retire_cnt >= HAZPTR_SCAN_THRESHOLD &&
      retire_cnt >= HAZPTR_SCAN_MULT * _hazptr_count.load(std::memory_order_acquire));
      //LOG(NOTICE) << "reached_threshold: " << ret;
    return ret;
}

inline HazptrRecord* HazptrManager::acquire_record() {
    HazptrRecord* rec = nullptr;
    if (USE_THREAD_LOCAL_CACHE) {
        rec = HazptrPrivate::instance().get_cached_rec();
        if (rec != nullptr) {
            return rec;
        }
    } else {
        for (rec = _hazptr_list.load(std::memory_order_seq_cst); rec != nullptr; rec = rec->next) {
            bool active = rec->active.load(std::memory_order_seq_cst);
            if (!active) {
                if (!rec->active.compare_exchange_strong(
                        active, 
                        true, 
                        std::memory_order_release, 
                        std::memory_order_relaxed)) {
                    continue;
                }
                //LOG(NOTICE) << "HazptrManager acquire_record get from hazptr_list " << rec;
                return rec;
            }
        }
    }
    rec = new HazptrRecord();
    rec->next = _hazptr_list.load(std::memory_order_seq_cst);
    //_hazptr_list.store(rec);
    while(!_hazptr_list.compare_exchange_weak(
                rec->next, 
                rec, 
                std::memory_order_release, 
                std::memory_order_acquire)) {
    }
    LOG(NOTICE) << "HazptrManager acquire_record allocate new HazptrRecord:" << _hazptr_list.load();
    _hazptr_count.fetch_add(1);

    return rec;
}

void HazptrManager::release_record(HazptrRecord* rec) {
    if (USE_THREAD_LOCAL_CACHE) {
        HazptrPrivate::instance().free_cached_rec(rec);
    } else {
        rec->active.store(false, std::memory_order_seq_cst);
    }
    // rec一旦分配就不会从全局链表释放
    //_hazptr_count.fetch_sub(1); 
}

void HazptrManager::bulkReclaim() {
    //为什么要加fence?
    std::atomic_thread_fence(std::memory_order_seq_cst);
    // 很容易出错的地方：一定要先读取retired_list，再读取hazptr_list
    HazptrObj* p = _retired_list.exchange(nullptr, std::memory_order_seq_cst);
    std::unordered_set<const void*> set;
    HazptrRecord* h = _hazptr_list.load(std::memory_order_seq_cst);
    int cnt = 0;
    for (; h != nullptr; h = h->next) {
        //LOG(NOTICE) << "HazptrManager bulkReclaim got a protected ptr:" << h->realptr;
        set.insert(h->realptr);
        cnt++;
    }
    //LOG(NOTICE) << "HazptrManager bulkReclaim hazptr_rec_size:" << cnt << " set_size:" << set.size();
    DoubleLinkedList<HazptrObj> left_list;
    HazptrObj* next = nullptr;
    int object_count = 0;
    for (; p != nullptr; p = next) {
        object_count++;
        next = p->next;
        if (set.count(p->get_ptr()) > 0) {
            left_list.push(p);
            //LOG(NOTICE) << "HazptrManager return the protected object " << p;
        } else {
            //LOG(NOTICE) << "HazptrManager delete object safely " << p;
            delete p;
        }
    }
    //LOG(NOTICE) << "HazptrManager bulkReclaim object_count " << object_count << " -> " << left_list.count;
    if (left_list.count > 0) {
        // 不能直接修改_retire_list，因为回收的时候也有别的线程在修改_retire_list
        // _retired_count.store(left_count);
        // _retired_list.exchange(left_list.head, std::memory_order_seq_cst);
        append(left_list.head, left_list.tail, left_list.count);
    }
}


void HazptrManager::append(HazptrObj* head, HazptrObj* tail, int count) {
    // 为什么要加fence?
    std::atomic_thread_fence(std::memory_order_seq_cst);
    tail->next = _retired_list.load(std::memory_order_seq_cst);
    //_retired_list.store(head);
    while(!_retired_list.compare_exchange_weak(
                tail->next, 
                head, 
                std::memory_order_release, 
                std::memory_order_acquire)) {
    }
    _retired_count.fetch_add(count);
}

void HazptrManager::tryBulkReclaim() {
    // 可能很多个线程同时达到阈值，触发bulk reclaim, 所以不能直接store
    //_retired_count.store(0);
    int retire_cnt = _retired_count.load(std::memory_order_acquire);
    do {
        int hazptr_cnt = _hazptr_count.load(std::memory_order_acquire);
        if (retire_cnt < HAZPTR_SCAN_THRESHOLD || retire_cnt < HAZPTR_SCAN_MULT * hazptr_cnt) {
            LOG(NOTICE) << "tryBulkReclaim retired_count not enough: " << retire_cnt;
            return;
        }
    } while(!_retired_count.compare_exchange_weak(
                    retire_cnt, 
                    0, 
                    std::memory_order_release, 
                    std::memory_order_acquire));

    bulkReclaim();
}


template<typename T>
bool HazptrHolder::try_protect(T* &p, const std::atomic<T*>& src) {
    _rec->realptr.store(p, std::memory_order_seq_cst);
    // 为什么要加fence?
    std::atomic_thread_fence(std::memory_order_seq_cst);
    T* latest_p = src.load(std::memory_order_seq_cst);
    if (p != latest_p) {
        p = latest_p;
        _rec->realptr.store(nullptr);
        return false;
    }
    return true;
}

template<typename T>
T* HazptrHolder::get_protected(const std::atomic<T*>& src) {
    T* p = src.load(std::memory_order_relaxed);
    while (!try_protect(p, src)) {
    }
    return p;
}


} // namespace
