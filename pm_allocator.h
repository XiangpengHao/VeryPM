#pragma once
#include <libpmemobj.h>
#include "utils.h"
namespace very_pm {
static const char* layout_name = "very_pm_alloc_layout";
class Allocator {
 public:
  static void Initialize(const char* pool_name, size_t pool_size) {
    allocator_ = new Allocator(pool_name, pool_size);
    LOG(INFO) << "pool opened at: " << std::hex << allocator_->pm_pool_
              << std::dec << std::endl;
  }

  static void Allocate() {}

 private:
  Allocator(const char* pool_name, size_t pool_size) {
    if (!very_pm::FileExists(pool_name)) {
      LOG(INFO) << "creating a new pool" << std::endl;
      pm_pool_ =
          pmemobj_create(pool_name, layout_name, pool_size, CREATE_MODE_RW);
      if (pm_pool_ == nullptr) {
        LOG(FATAL) << "failed to create a pool" << std::endl;
      }
      return;
    } else {
      LOG(INFO) << "opening an existing pool, and trying to map to same address"
                << std::endl;
      pm_pool_ = pmemobj_open(pool_name, layout_name);
      if (pm_pool_ == nullptr) {
        LOG(FATAL) << "failed to open the pool" << std::endl;
      }
    }
    very_pm::flush(pm_pool_);
  }

  static Allocator* allocator_;
  PMEMobjpool* pm_pool_{nullptr};
};

Allocator* Allocator::allocator_{nullptr};

}  // namespace very_pm
