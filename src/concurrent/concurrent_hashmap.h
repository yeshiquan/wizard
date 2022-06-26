#pragma once

// TODO
// 1 clear() bug fix done
// 2 rehash() done
// 3 Destructor
// 4 Move Iterator
// 5 Lock-free done
// 6 align allocate
// 7 size()  done
// 8 benchmark
// 9 value's universal reference done
// 10 emplace() done
// 11 ValueHolder use std::pair for the interface is as close to std::map as possible
// 12 reserve() done
// 13 内存泄漏 done
// 14 lock的提前释放+引用计数+hazptr的使用
// 15 Allocator替换new和delete
// 16 根据iterator erase元素,  return an iterator to the element that follows the last element removed 
// 17 根据iterator insert元素 return an iterator pointing to either the 
//    newly inserted element or to the element that already had an equivalent key in the map

#include <memory>
#include <mutex>
#include <type_traits>
#include "concurrent_hashmap_detail.h"

namespace rcu {

template <
    typename KeyType, 
    typename ValueType, 
    typename HashFn = std::hash<KeyType>,
    typename Allocator = std::allocator<uint8_t>,
    uint8_t ShardBits = 8>
class ConcurrentHashMap {
public:
    using SegmentT = ConcurrentHashMapSegment<
          KeyType,
          ValueType,
          HashFn,
          ShardBits>;
    typedef ValueType value_type;

    static constexpr uint64_t NumShards = (1 << ShardBits);
    class ConstIterator;

    inline ConcurrentHashMap() noexcept {
    }

    inline ConcurrentHashMap(size_t init_capacity) noexcept {
        init_capacity_ = nextPowTwo(init_capacity);
    }
    // 禁止拷贝和移动
    inline ConcurrentHashMap(ConcurrentHashMap&&) = delete;
    inline ConcurrentHashMap(const ConcurrentHashMap&) = delete;
    inline ConcurrentHashMap& operator=(ConcurrentHashMap&&) = delete;
    inline ConcurrentHashMap& operator=(const ConcurrentHashMap&) = delete;

    template <typename Key, typename Value>
    std::pair<ConstIterator, bool> insert(Key&& key, Value&& value) {
        auto hash = HashFn()(key);
        auto segment_id = pickSegment(key);
        std::pair<ConstIterator, bool> res(
            std::piecewise_construct,
            std::forward_as_tuple(segment_id),
            std::forward_as_tuple(false));

        res.second = ensureSegment(segment_id)->insert(hash, std::forward<Key>(key), std::forward<Value>(value));

        return res;
    }

  template <typename Key, typename Value>
  std::pair<ConstIterator, bool> insert_or_assign(Key&& k, Value&& v) {
    auto hash = HashFn()(k);
    auto segment = pickSegment(k);
    std::pair<ConstIterator, bool> res(
        std::piecewise_construct,
        std::forward_as_tuple(this, segment),
        std::forward_as_tuple(false));
    res.second = ensureSegment(segment)->insert_or_assign(
        hash, std::forward<Key>(k), std::forward<Value>(v));
    return res;
  }

    template <typename... Args>
    std::pair<ConstIterator, bool> emplace(Args&&... args) {
        using Node = typename SegmentT::Node;
        Node* new_node = new Node(std::forward<Args>(args)...);
        const KeyType& key = new_node->value_holder_.key;
        auto hash = HashFn()(key);
        auto segment_id = pickSegment(key);

        std::pair<ConstIterator, bool> res(
            std::piecewise_construct,
            std::forward_as_tuple(segment_id),
            std::forward_as_tuple(false));

        bool ret = ensureSegment(segment_id)->emplace(hash, key, new_node);
        if (!ret) {
            delete new_node;
        }
        res.second = ret;
        return res;
    }

      ConstIterator find(const KeyType& k) const {
        auto hash = HashFn()(k);
        auto segment = pickSegment(k);
        ConstIterator res(this, segment);
        auto seg = segments_[segment].load(std::memory_order_acquire);
        //auto seg = segments_[segment];
        if (!seg || !seg->find(hash, res.it_, k)) {
          res.segment_ = NumShards;
        }
        return res;
      }

  void erase(const KeyType& k) {
    auto segment = pickSegment(k);
    auto hash = HashFn()(k);
    auto seg = segments_[segment].load(std::memory_order_acquire);
    //auto seg = segments_[segment];
    if (!seg) {
      return ;
    } else {
      seg->erase(hash, k);
    }
  }


  class ConstIterator {
   public:
    friend class ConcurrentHashMap;

    const value_type& operator*() const {
      return *it_;
    }

    const value_type* operator->() const {
      return &*it_;
    }

    ConstIterator& operator++() {
      it_++;
      next();
      return *this;
    }

    ConstIterator operator++(int) {
      auto prev = *this;
      ++*this;
      return prev;
    }

    bool operator==(const ConstIterator& o) const {
      return it_ == o.it_ && segment_ == o.segment_;
    }

    bool operator!=(const ConstIterator& o) const {
      return !(*this == o);
    }

    ConstIterator& operator=(const ConstIterator& o) {
      it_ = o.it_;
      segment_ = o.segment_;
      return *this;
    }

    ConstIterator(const ConstIterator& o) {
      it_ = o.it_;
      segment_ = o.segment_;
    }

    ConstIterator(const ConcurrentHashMap* parent, uint64_t segment)
        : segment_(segment), parent_(parent) {}

   private:
    // cbegin iterator
    explicit ConstIterator(const ConcurrentHashMap* parent)
        : it_(parent->ensureSegment(0)->cbegin()),
          segment_(0),
          parent_(parent) {
      // Always iterate to the first element, could be in any shard.
      next();
    }

    // cend iterator
    explicit ConstIterator(uint64_t shards) : segment_(shards) {}

    void next() {
      while (it_ == parent_->ensureSegment(segment_)->cend() &&
             segment_ < parent_->NumShards) {
        segment_++;
        auto seg = parent_->segments_[segment_].load(std::memory_order_acquire);
        //auto seg = parent_->segments_[segment_];
        if (segment_ < parent_->NumShards) {
          if (!seg) {
            continue;
          }
          it_ = seg->cbegin();
        }
      }
    }

    typename SegmentT::Iterator it_;
    uint64_t segment_;
    const ConcurrentHashMap* parent_;
  };

      ConstIterator cend() const noexcept {
        return ConstIterator(NumShards);
      }

      ConstIterator cbegin() const noexcept {
        return ConstIterator(this);
      }

    void clear() {
        for (int i = 0; i < NumShards; i++) {
            auto* segment = segments_[i].load(std::memory_order_acquire);
            //auto* segment = segments_[i];
            if (segment) {
                segment->clear();
            }
        }
    }

    void reserve(size_t capacity) {
        auto bucket_count = capacity >> ShardBits;
        for (int i = 0; i < NumShards; i++) {
            SegmentT* segment = segments_[i].load(std::memory_order_acquire);
            //SegmentT* segment = segments_[i];
            if (segment) {
                segment->rehash(bucket_count);
            }
        }
    }

    size_t size() const {
        size_t result = 0;
        for (int i = 0; i < NumShards; i++) {
            auto* segment = segments_[i].load(std::memory_order_acquire);
            //auto* segment = segments_[i];
            if (segment) {
                result += segment->size();
            }
        }
        return result;
    }
private:
  uint64_t pickSegment(const KeyType& k) const {
    auto h = HashFn()(k);
    // Use the lowest bits for our shard bits.
    //
    // This works well even if the hash function is biased towards the
    // low bits: The sharding will happen in the segments_ instead of
    // in the segment buckets, so we'll still get write sharding as
    // well.
    //
    // Low-bit bias happens often for std::hash using small numbers,
    // since the integer hash function is the identity function.
    return h & (NumShards - 1);
  }

  /*
  SegmentT* ensureSegment(uint64_t i) const {
        if (segments_[i] == nullptr) {
            size_t bucket_count = init_capacity_ >> ShardBits;
            //std::cout << "bucket_count -> " << bucket_count << std::endl;
            segments_[i] = new SegmentT(bucket_count, load_factor_);
        }
        return segments_[i];
  }
  */

  SegmentT* ensureSegment(uint64_t i) const {
    SegmentT* seg = segments_[i].load(std::memory_order_acquire);
    if (!seg) {
      SegmentT* newseg = (SegmentT*)Allocator().allocate(sizeof(SegmentT));
      newseg = new (newseg)
          SegmentT(init_capacity_ >> ShardBits, load_factor_);
      if (!segments_[i].compare_exchange_strong(seg, newseg)) {
        // seg is updated with new value, delete ours.
        newseg->~SegmentT();
        Allocator().deallocate((uint8_t*)newseg, sizeof(SegmentT));
      } else {
        seg = newseg;
      }
    }
    return seg;
  }
private:
    // 初始的时候，所有分片加起来，一共分配多少个bucket
    // init_capacity_ = bucket_count_per_shard * num_of_shard
    size_t init_capacity_ = {8};
    mutable std::atomic<SegmentT*> segments_[NumShards] {nullptr};
    //mutable SegmentT* segments_[NumShards] = {nullptr};
    float load_factor_ = {1.05};
};

}


