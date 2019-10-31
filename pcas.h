// Copyright Xiangpeng Hao. All rights reserved.
// Licensed under the MIT license.
//
// Persistent CAS
#pragma once
#include <libpmemobj.h>
#include "utils.h"

namespace pm_tool {

class DirtyTable {
 private:
  /// Item is only padded to half cache line size,
  /// Don't want to waste too much memory
  struct Item {
    void* addr_;
    uint64_t old_;
    uint64_t new_;
    char paddings_[8];
  };

  DirtyTable(uint32_t item_cnt) : item_cnt_{item_cnt} {}

  Item* MyItem() {
    thread_local Item* my_item{nullptr};
    if (my_item != nullptr) {
      return my_item;
    }
    uint32_t next_id = next_free_object_.fetch_add(1);
    my_item = &items_[next_id];
    return my_item;
  }

  /// Why to flush my_item->addr_?
  ///   We employ lazy flush mechanism to hide the high cost of clwb (Oct.
  ///   2019), i.e. there's no flush after a CAS, need to make sure the previous
  ///   CASed value is properly persisted. When clwb is ideally implemented (not
  ///   evicting cache line), we can move the flush after a CAS and potenially
  ///   save a branch here.
  ///
  /// Why flush addr?
  ///   We're trying to assign a new value to addr, need to make sure previous
  ///   store has already persisted, o.w. the newly assigned CAS will fail
  ///   on recovery.
  ///
  /// Why non-temporal store is required?
  ///   We need to atomically and immediately write the value to persistent
  ///   memory, and not relying on the non-deterministic cache eviction policy
  void RegisterItem(void* addr, uint64_t old_v, uint64_t new_v) {
    Item* my_item = MyItem();
    if (my_item->addr_ != nullptr) {
      flush(my_item->addr_);
    }
    flush(addr);
    auto value = _mm256_set_epi64x(0, new_v, old_v, (uint64_t)addr);
    _mm256_stream_si256((__m256i*)(my_item), value);
  }

  /// Why a single CAS is not enough?
  ///   Checkout this post: https://blog.haoxp.xyz/posts/crash-consistency/
  ///
  /// The RegisterItem will record the CAS operation, and on recovery try to
  /// redo the CAS.
  ///   1. If a crash happens right after the RegisterItem, the new
  ///   value is not seen by any other thread and we are good to redo the CAS
  ///   during recovery.
  ///   2. If a crash happens right after the CAS, before the new value is
  ///   persisted, on recovery we can help finish the CAS, make it in a
  ///   consistent state
  ///   3. The only worry for me right now, is when CAS is finished and
  ///   persisted, on recovery we will still try to redo the CAS. This should be
  ///   fine in most cases, but the chances to hit ABA problem is much higher.
  bool PersistentCAS(void* addr, uint64_t old_v, uint64_t new_v) {
    RegisterItem(addr, old_v, new_v);
    __atomic_compare_exchange_n((uint64_t*)addr, &old_v, new_v, false,
                                __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  }

  std::atomic<uint32_t> next_free_object_{0};
  uint32_t item_cnt_{0};

  char paddings_[24];

  Item items_[0];
};

}  // namespace pm_tool
