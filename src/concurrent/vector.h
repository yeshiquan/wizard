// TODO
// 1 snapshot done
// 2 copy_n done
// 3 fill_n done
// 4 for_each done
// 5 retire_list done
// 5 destructor done
// 6 benchmark done

#pragma once

#include <memory>
#include <algorithm>
#include <bitset>

#include "debug.h"

constexpr size_t CACHELINE_SIZE = 64;

namespace rcu {

template <typename T>
inline T* get_ptr(uint64_t tagged_ptr) {
    return reinterpret_cast<T*>(tagged_ptr & 0x0000FFFFFFFFFFFFUL);
}

uint16_t get_tag(uint64_t tagged_ptr) {
    return tagged_ptr >> 48;
}

template <typename T>
inline uint64_t make_tagged_ptr(T* ptr, uint16_t tag) {
    // tagged_ptr
    // ----------------++++++++++++++++++++++++++++++++++++++++++++++++
    // |  tag(16bit)   |            ptr(48bit)                        |
    return reinterpret_cast<uint64_t>(ptr) | (static_cast<uint64_t>(tag) << 48);
}

template <typename T, typename D = std::default_delete<T>>
class RetireList {
public:
    struct Node {
        T* data = {nullptr};
        Node* next = {nullptr};    
    };
public:
    inline RetireList() noexcept {
    }
    uint16_t get_current_timestamp() {
        ::timespec spec;
        ::clock_gettime(CLOCK_MONOTONIC, &spec);
        // 按照64s一个时间单位
        return spec.tv_sec >> 6;
    }
    bool expire(uint64_t head, uint16_t now) {
        // return static_cast<uint16_t>(get_tag(head) - now) >= 1;
        return static_cast<uint16_t>(get_tag(head) - now) > 1;
    }
    void retire(T* data) {
        auto* new_node = new Node;
        new_node->data = data;
        uint16_t now = get_current_timestamp();
        try_gc(now);
        push(new_node, now);
    }
    void try_gc(uint16_t now) {
        uint64_t old_head = _head.load(std::memory_order_acquire);
        if (expire(old_head, now)) {
            auto* head = try_drop(old_head);
            LOG(NOTICE) << "head expired do gc..." << head;
            if (head) {
                delete_list(head);
            }
        }
    }
    void force_gc() {
        uint64_t old_head = _head.load(std::memory_order_acquire);
        auto* head = try_drop(old_head);
        LOG(NOTICE) << "force gc..." << head;
        if (head) {
            delete_list(head);
        }
    }

private:
    void push(Node* new_node, uint16_t now) noexcept {
        uint64_t new_head_tagged_ptr = make_tagged_ptr(new_node, now); 
        uint64_t old_head_tagged_ptr = _head.load();
        new_node->next = get_ptr<Node>(old_head_tagged_ptr);
        do {
            new_node->next = get_ptr<Node>(old_head_tagged_ptr);
        } while(!_head.compare_exchange_weak(
                                old_head_tagged_ptr,
                                new_head_tagged_ptr, 
                                std::memory_order_release, 
                                std::memory_order_acquire));
    }
    Node* try_drop(uint64_t old_head) noexcept {
        uint64_t old_head_tagged_ptr = old_head;
        uint64_t new_head_tagged_ptr = 0;
        Node* head = nullptr;
        if(_head.compare_exchange_weak(
                                old_head_tagged_ptr,
                                new_head_tagged_ptr,
                                std::memory_order_release,
                                std::memory_order_acquire)) {
            head = get_ptr<Node>(old_head_tagged_ptr);
        }
        return head;
    }
    void delete_list(Node* head) {
        Node* next = nullptr;
        int cnt = 0;
        for (; head != nullptr; head = next) {
            next = head->next;
            D()(head->data);
            delete head;
            cnt++;
        }
        LOG(NOTICE) << "delete_list -> " << cnt << " nodes";
    }
    Node* get_head() {
        uint64_t head_tagged_ptr = _head.load(std::memory_order_acquire);
        auto* node = get_ptr<Node>(head_tagged_ptr);
        return node;
    }
    // 禁止拷贝和移动
    inline RetireList(RetireList&&) = delete;
    inline RetireList(const RetireList&) = delete;
    inline RetireList& operator=(RetireList&&) = delete;
    inline RetireList& operator=(const RetireList&) = delete;
private:
    std::atomic<uint64_t> _head = {0};
};

template <typename T>
class ConcurrentVector {
    struct Table {
        size_t shard_num;
        T* shards[0];
    };
    struct TableDeleter {
        void operator()(Table* table) noexcept {
           delete_table(table); 
        }
    };
    struct Meta {
        size_t get_shard_id(size_t index) {
            return index >> shard_bit;
        }
        size_t get_shard_offset(size_t index) {
            return index & (num_per_shard - 1);
        }
        // 容纳size个元素需要多少个分片
        size_t require_shard_num(size_t size) {
            return get_shard_id(size-1) + 1;
        }
        size_t num_per_shard = {0};
        size_t shard_bit = {0};
    };
    class Snapshot {
    public:
        Snapshot(Table* table, Meta meta) : 
                _table(table), _meta(meta) {
        }
        T& operator[](size_t index) {
            DCHECK(_table);
            DCHECK(_table->shards);
            auto shard_id = _meta.get_shard_id(index);
            auto offset = _meta.get_shard_offset(index);
            return _table->shards[shard_id][offset];
        }
        const T& operator[](size_t index) const {
            DCHECK(_table);
            DCHECK(_table->shards);
            auto shard_id = _meta.get_shard_id(index);
            auto offset = _meta.get_shard_offset(index);
            return _table.load()->shards[shard_id][offset];
        }

        template<typename C>
        void for_each(size_t start_index, size_t end_index, C&& callback) {
            DCHECK(_table);
            DCHECK(_table->shards);

            auto start_shard_id = _meta.get_shard_id(start_index);
            auto start_offset = _meta.get_shard_offset(start_index);
            auto end_shard_id = _meta.get_shard_id(end_index);
            auto end_offset = _meta.get_shard_offset(end_index);
            T** shards = _table->shards;

            //std::cout << "for_each -> " << start_index << "," << end_index << std::endl;

            callback(shards[start_shard_id] + start_offset, shards[start_shard_id] + _meta.num_per_shard);

            for (auto shard_id = start_shard_id + 1; shard_id != end_shard_id; ++shard_id) {
                callback(shards[shard_id] + 0, shards[shard_id] + _meta.num_per_shard);
            }

            if (start_shard_id < end_shard_id && end_offset > 0) {
                callback(shards[end_shard_id] + 0, shards[end_shard_id] + end_offset);
            }
        }
        void fill_n(size_t start_index, size_t length, const T& value) {
            DCHECK(_table);
            DCHECK(_table->shards);

            for_each(start_index, start_index + length, [&value](T* iter, T* end) {
                std::fill(iter, end, value);
            });
        }

        template<typename IT>
        void copy_n(IT iter, size_t length, size_t start_index) {
            DCHECK(_table);
            DCHECK(_table->shards);

            for_each(start_index, start_index + length, [&](T* begin, T* end) {
                size_t l = (end - begin);
                std::copy_n(iter, l, begin);
                iter += l;
            });
        }
    private:
        Table* _table = {nullptr};
        Meta _meta;
    };
public:
    inline ConcurrentVector(size_t num_per_shard = 1024) noexcept;
    inline ~ConcurrentVector() noexcept;
    // 禁止拷贝和移动
    inline ConcurrentVector(ConcurrentVector&&) = delete;
    inline ConcurrentVector(const ConcurrentVector&) = delete;
    inline ConcurrentVector& operator=(ConcurrentVector&&) = delete;
    inline ConcurrentVector& operator=(const ConcurrentVector&) = delete;

    const T& operator[](size_t index) const {
        return snapshot()[index];
    }

    T& operator[](size_t index) {
        return snapshot()[index];
    }
    void reserve(size_t size) {
        auto shard_num = _meta.require_shard_num(size);
        ensure_table(shard_num);
    }
    T& ensure(size_t index) {
        auto shard_id = _meta.get_shard_id(index);
        auto offset = _meta.get_shard_offset(index);
        return ensure_table(shard_id + 1)->shards[shard_id][offset];
    }

    template<typename C>
    void for_each(size_t start_index, size_t end_index, C&& callback) {
        snapshot().for_each(start_index, end_index, std::forward<C>(callback));
    }

    void fill_n(size_t start_index, size_t length, const T& value) {
        reserved_snapshot(start_index+length).fill_n(start_index, length, value);
    }

    template<typename IT>
    void copy_n(IT iter, size_t length, size_t start_index) {
        reserved_snapshot(start_index+length).copy_n(iter, length, start_index);
    }
    Snapshot snapshot() {
        return Snapshot(_table.load(std::memory_order_acquire), _meta);
    }
    Snapshot reserved_snapshot(size_t size) {
        Table* table = ensure_table(size);
        return Snapshot(table, _meta);
    }
private:

    // 可能多个线程同时调用
    Table* ensure_table(size_t shard_num) {
        auto* old_table = _table.load(std::memory_order_acquire);
        if (old_table->shard_num >= shard_num) {
            return old_table;
        }
        size_t bytes = sizeof(Table) + shard_num * sizeof(T*);
        size_t size = (bytes + CACHELINE_SIZE) & ~static_cast<size_t>(CACHELINE_SIZE-1);
        auto* new_table = reinterpret_cast<Table*>(aligned_alloc(CACHELINE_SIZE, size));
        new_table->shard_num = shard_num;
        auto old_shard_num = old_table->shard_num;

        while (true) {
            // Copy Old Data
            for (int i = 0; i < old_table->shard_num; ++i) {
                new_table->shards[i] = old_table->shards[i];
            }

            // Create New Storage
            for (int i = old_shard_num; i < shard_num; ++i) {
                new_table->shards[i] = create_shard();
            }
            if (_table.compare_exchange_strong(
                                old_table, 
                                new_table, 
                                std::memory_order_release, 
                                std::memory_order_acquire)) {
                _retired_list.retire(old_table);
                LOG(NOTICE) << "trace 1...";
                return new_table;
            } else {
                // 被别的线程抢先一步扩容了，看看别人扩容后能否满足我的要求
                for (int i = old_shard_num; i < shard_num; ++i) {
                    delete_shard(new_table->shards[i]);
                }
                if (old_table->shard_num > shard_num) {
                    LOG(NOTICE) << "trace 2...";
                    delete_table(new_table);
                    return old_table;
                } else {
                    // capacity is still not enough, retry...
                    LOG(NOTICE) << "trace 3...";
                }
            }
        }
    }

    static void delete_table(Table* table) {
        if (table != &EMPTY_TABLE) {
            free(table);
        }
    }

    T* create_shard() {
        size_t bytes = _meta.num_per_shard * sizeof(T);
        size_t size = (bytes + CACHELINE_SIZE) & ~static_cast<size_t>(CACHELINE_SIZE-1);
        T* shard = reinterpret_cast<T*>(aligned_alloc(CACHELINE_SIZE, size));
        if constexpr (!std::is_trivial_v<T>) {
            for (int k = 0; k < _meta.num_per_shard; ++k) {
                new (shard + k) T;
            } 
        } else {
            __builtin_memset(shard, 0, size);
        }
        return shard;
    }

    void delete_shard(T* shard) {
        if constexpr (!std::is_trivial_v<T>) {
            for (int k = 0; k < _meta.num_per_shard; ++k) {
                shard[k].~T();
            } 
        } 
    }

private:
    static constexpr Table EMPTY_TABLE = {};
    RetireList<Table, TableDeleter> _retired_list;
    std::atomic<Table*> _table = {nullptr};
    Meta _meta;
};

} // namespace
#include "vector.hpp"
