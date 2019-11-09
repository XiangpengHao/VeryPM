#include "../pm_pool.h"

#include <glog/logging.h>
#include <gtest/gtest.h>
#include <fstream>

static const constexpr char* allocator_pool = "/mnt/pmem0/pool_test";
static const constexpr uint64_t pool_size = 1024 * 1024 * 512;
static const constexpr uint64_t kCacheLineMask = 0x3F;

namespace very_pm {

GTEST_TEST(PMPoolTest, CreatePool) {
  PMPool::CreatePool(allocator_pool, pool_size);
  ASSERT_EQ(PMPool::pm_pool_->pool_addr_, VERY_PM_POOL_ADDR);

  ASSERT_EQ(PMPool::pm_pool_->pool_size_, pool_size);
  ASSERT_LE(PMPool::pm_pool_->free_page_list_,
            PMPool::pm_pool_->pool_high_addr_);

  PMPool::UnmapPool();
}

GTEST_TEST(PMPoolTest, OpenPool) {
  PMPool::OpenPool(allocator_pool, pool_size);
  ASSERT_EQ(PMPool::pm_pool_->pool_addr_, VERY_PM_POOL_ADDR);

  ASSERT_EQ(PMPool::pm_pool_->pool_size_, pool_size);
  PMPool::UnmapPool();
}

}  // namespace very_pm

int main(int argc, char** argv) {
  FLAGS_logtostderr = 1;
  google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
