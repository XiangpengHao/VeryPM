#include "../pm_pool.h"

#include <glog/logging.h>
#include <gtest/gtest.h>
#include <fstream>

static const constexpr char* allocator_pool = "/mnt/pmem0/allocator0";
static const constexpr uint64_t pool_size = 1024 * 1024 * 1024;
static const constexpr uint64_t kCacheLineMask = 0x3F;

namespace very_pm {

GTEST_TEST(PMPoolTest, CreatePool) {
  PMPool::OpenPool(allocator_pool, pool_size);
  ASSERT_EQ(PMPool::pm_pool_, VERY_PM_POOL_ADDR);

  std::ifstream in(allocator_pool, std::ifstream::ate | std::ifstream::binary);
  ASSERT_EQ(pool_size, in.tellg());
}

}  // namespace very_pm

int main(int argc, char** argv) {
  FLAGS_logtostderr = 1;
  google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
