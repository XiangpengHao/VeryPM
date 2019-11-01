#include "../pcas.h"
#include <glog/logging.h>
#include <gtest/gtest.h>

GTEST_TEST(PCASTest, SmokeTest) {
  LOG(INFO) << "running tests";
  ASSERT_TRUE(true);
}

namespace pm_tool {

class DirtyTablePMTest : public ::testing::Test {
 public:
  DirtyTablePMTest() {}

 protected:
  static const constexpr uint32_t item_cnt_ = 48 * 2;
  pm_tool::DirtyTable* table;
  virtual void SetUp() {
    posix_memalign((void**)&table, pm_tool::kCacheLineSize,
                   sizeof(pm_tool::DirtyTable) +
                       sizeof(pm_tool::DirtyTable::Item) * item_cnt_);
    pm_tool::DirtyTable::Initialize(table, item_cnt_);
  }

  virtual void TearDown() { delete table; }
};

TEST_F(DirtyTablePMTest, SimpleCAS) {
  uint64_t target{0};
  for (uint32_t i = 1; i < 100; i += 1) {
    table->next_free_object_ = 1;
    table->item_cnt_ = item_cnt_;
    table->items_[0].addr_ = &target;
    table->items_[0].old_ = i - 1;
    table->items_[0].new_ = i;
    auto rv = pm_tool::PersistentCAS(&target, i - 1, i);
    EXPECT_EQ(rv, i - 1);
    EXPECT_EQ(target, i);
  }
}

TEST_F(DirtyTablePMTest, SimpleRecovery) {
  uint64_t target{0};
  for (uint32_t i = 1; i < 100; i += 1) {
    pm_tool::PersistentCAS(&target, i - 1, i);
  }
  EXPECT_EQ(target, 99);
  table->Recovery(DirtyTable::GetInstance());
  EXPECT_EQ(target, 99);
}

}  // namespace pm_tool

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
