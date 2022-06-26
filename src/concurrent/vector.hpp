#pragma once

#include <gflags/gflags.h>
#include <atomic>

namespace rcu {

template <typename T>
inline ConcurrentVector<T>::ConcurrentVector(size_t num_per_shard) noexcept
{
    _meta.num_per_shard = folly::nextPowTwo(num_per_shard);
    _meta.shard_bit =  __builtin_popcount(_meta.num_per_shard - 1) ;
    _table.store(const_cast<Table*>(&EMPTY_TABLE), std::memory_order_relaxed);
}

template <typename T>
inline ConcurrentVector<T>::~ConcurrentVector() noexcept
{
    Table* table = _table.load();
    for (int i = 0; i < table->shard_num; ++i) {
        auto* shard = table->shards[i];
        delete_shard(shard);
    }
    delete_table(table);
    _retired_list.force_gc();
}


/*
template <typename T>
inline typename ConcurrentVector<T>::Commiter
ConcurrentVector<T>::get() {
    return Commiter{};
}
*/

} // namespace
