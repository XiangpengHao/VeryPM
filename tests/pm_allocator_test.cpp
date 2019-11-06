#include "../pm_allocator.h"
#include <glog/logging.h>
#include <gtest/gtest.h>
#include <libpmemobj.h>


int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
