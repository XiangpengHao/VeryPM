#include <glog/logging.h>
#include <gtest/gtest.h>
#include <map>
#include <random>
#include "../epoch_manager.h"

GTEST_TEST(EpochTest, EpochSmoke) {
  LOG(INFO) << "running tests";
  ASSERT_TRUE(true);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
