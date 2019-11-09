#pragma once

#include <glog/logging.h>
#include <libpmem.h>
#include <sys/mman.h>
#include "utils.h"

#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <clocale>
#include <cstring>
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
  /// Page size is 2MB as we force to use huge page;
  static const constexpr uint64_t kPoolPageSize = 1024 * 1024 * 2;

 public:
  static void UnmapPool() {
    LOG_IF(FATAL, pm_pool_ == nullptr) << "Pool is not initialized yet!\n";
    munmap(pm_pool_, pm_pool_->pool_size_);
    pm_pool_ = nullptr;
  }

  static void OpenPool(const char* pool_path, size_t pool_size) {
    if (!very_pm::FileExists(pool_path)) {
      LOG(FATAL) << "pool file does not exist!" << std::endl;
    }

    int fd = open(pool_path, O_RDWR, 0666);
    /* create a pmem file */
    if (fd < 0) {
      LOG(FATAL) << "Failed to open pmem file" << std::endl;
      exit(1);
    }

    pm_pool_ = reinterpret_cast<PMPool*>(MapFile(pool_size, fd));
    close(fd);

    LOG_IF(FATAL, pm_pool_ != pm_pool_->pool_addr_)
        << "Pool loaded address does not match with previous load!"
        << std::endl;
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

    pm_pool_ = reinterpret_cast<PMPool*>(MapFile(pool_size, fd));
    close(fd);
    auto value = _mm256_set_epi64x((uint64_t)((char*)pm_pool_ + kPoolPageSize),
                                   (uint64_t)((char*)pm_pool_ + kPoolPageSize),
                                   pool_size, (uint64_t)pm_pool_);
    _mm256_stream_si256((__m256i*)pm_pool_, value);
    fence();
  }

 private:
  static void* MapFile(size_t pool_size, int fd) {
    auto pool_addr =
        mmap(VERY_PM_POOL_ADDR, pool_size, PROT_READ | PROT_WRITE,
             MAP_FILE | MAP_SYNC | MAP_SHARED | MAP_FIXED_NOREPLACE, fd, 0);

    if (pool_addr == nullptr || (int64_t)pool_addr == -1) {
      LOG(FATAL) << "mmap failed: " << std::strerror(errno) << std::endl;
    }

    if (pool_addr != VERY_PM_POOL_ADDR) {
      LOG(FATAL)
          << "mmap did not mapped to the desired address, try to restart "
             "and try again!"
          << std::endl;
    }

    LOG(INFO) << "Pool successfully mapped at: " << std::hex << pool_addr
              << std::dec << std::endl;

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
  FRIEND_TEST(PMPoolTest, OpenPool);
#endif
  static PMPool* pm_pool_;
  void* pool_addr_;
  uint64_t pool_size_;
  void* pool_high_addr_;
  void* free_page_list_;
};

PMPool* PMPool::pm_pool_ = nullptr;
}  // namespace very_pm
