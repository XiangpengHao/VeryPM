#include <glog/logging.h>
#include <gtest/gtest.h>
#include <map>
#include <random>
#include "../epoch_manager.h"

GTEST_TEST(EpochTest, EpochSmoke) {
  LOG(INFO) << "running tests";
  auto epm = new EpochManager();
  epm->Initialize();
  epm->Protect();
  ASSERT_TRUE(true);
  epm->Unprotect();
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
