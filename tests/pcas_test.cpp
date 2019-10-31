#include "../pcas.h"
#include <glog/logging.h>
#include <gtest/gtest.h>

GTEST_TEST(PCASTest, SmokeTest) {
  LOG(INFO) << "running tests";
  ASSERT_TRUE(true);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
