#pragma once

#include <glog/logging.h>
#include <libpmem.h>
#include <sys/mman.h>
#include "utils.h"

#include <fcntl.h>
#include <unistd.h>
#include <memory>

#ifdef TEST_BUILD
#include <gtest/gtest.h>
#include <gtest/gtest_prod.h>
#endif

namespace very_pm {

/// 512 GB
#ifndef VERY_PM_POOL_ADDR
#define VERY_PM_POOL_ADDR (void*)0x01000000000ull
#endif

class PMPool {
 public:
  static void OpenPool(const char* pool_path, size_t pool_size) {
    if (!very_pm::FileExists(pool_path)) {
      LOG(FATAL) << "pool file does not exist!" << std::endl;
    }

    int fd = open(pool_path, O_CREAT | O_RDWR, 0666);
    /* create a pmem file */
    if (fd < 0) {
      LOG(FATAL) << "Failed to open pmem file" << std::endl;
      exit(1);
    }

    pm_pool_->pool_addr_ = MapFile(pool_size, fd);
  }

  static void CreatePool(const char* pool_path, size_t pool_size) {
    int fd = open(pool_path, O_CREAT | O_RDWR, 0666);

    /* create a pmem file */
    if (fd < 0) {
      LOG(FATAL) << "Failed to open pmem file" << std::endl;
      exit(1);
    }

    /* allocate the pmem */
    int err_no = posix_fallocate(fd, 0, pool_size);
    if (err_no != 0) {
      LOG(FATAL) << "posix_fallocate failed" << std::endl;
      exit(1);
    }

    pm_pool_->pool_addr_ = MapFile(pool_size, fd);
  }

 private:
  static void* MapFile(size_t pool_size, int fd) {
    auto pool_addr = mmap(
        VERY_PM_POOL_ADDR, pool_size, PROT_READ | PROT_WRITE,
        MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB | MAP_FIXED_NOREPLACE, fd, 0);

    if (pool_addr == nullptr) {
      LOG(FATAL) << "mmap failed" << std::endl;
    }

    if (pool_addr != VERY_PM_POOL_ADDR) {
      LOG(FATAL)
          << "mmap did not mapped to the desired address, try to restart "
             "and try again!"
          << std::endl;
    }

    LOG(INFO) << "Pool successfully mapped at: " << std::hex << pool_addr
              << std::dec << std::endl;

    close(fd);

    /// The return of pmem_is_pmem is only valid when using pmem_map_file
    //   int is_pmem = pmem_is_pmem(pool_addr, pool_size);
    //   if (is_pmem == 0) {
    //     LOG(WARNING) << "Mapped file is not persistent memory." << is_pmem
    //                  << std::endl;
    //   }

    return pool_addr;
  }

 private:
#ifdef TEST_BUILD
  FRIEND_TEST(PMPoolTest, CreatePool);
#endif
  static PMPool* pm_pool_;
  void* pool_addr_;
};

PMPool* PMPool::pm_pool_ = nullptr;
}  // namespace very_pm
