#include <glog/logging.h>
#include <gtest/gtest.h>
#include <libpmemobj.h>
#include <memory>
#include <random>
#include "../pcas.h"
#include "bench_common.h"

POBJ_LAYOUT_BEGIN(benchmark);
POBJ_LAYOUT_TOID(benchmark, char);
POBJ_LAYOUT_END(benchmark);

static const constexpr uint32_t kItemCnt = 48 * 2;
static const constexpr uint32_t kArrayLen = 10000;
static const constexpr uint32_t kOpCnt = 100000;

struct BaseBench : public PerformanceTest {
  PMEMobjpool* pool{nullptr};
  BaseBench() {
    static const char* pool_name = "/mnt/pmem0/pcas_pool.data";
    static const char* layout_name = "benchmark";
    static const uint64_t pool_size = 1024 * 1024 * 1024;
    if (!pm_tool::FileExists(pool_name)) {
      pool = pmemobj_create(pool_name, layout_name, pool_size,
                            pm_tool::CREATE_MODE_RW);
    } else {
      pool = pmemobj_open(pool_name, layout_name);
    }
    EXPECT_NE(pool, nullptr);
  }
  ~BaseBench() { pmemobj_close(pool); }

  std::mt19937 rng{42};

  size_t read_count_{0};

  uint64_t* array{nullptr};

  void WorkLoadInit() {
    auto table = (pm_tool::DirtyTable*)ZAlloc(
        sizeof(pm_tool::DirtyTable) +
        sizeof(pm_tool::DirtyTable::Item) * kItemCnt);
    pm_tool::DirtyTable::Initialize(table, kItemCnt);
    array = (uint64_t*)ZAlloc(pm_tool::kCacheLineSize * kArrayLen);
  }

  void* ZAlloc(size_t size) {
    PMEMoid ptr;
    pmemobj_zalloc(pool, &ptr, pm_tool::PMDK_PADDING + size,
                   TOID_TYPE_NUM(char));
    auto abs_ptr = (char*)pmemobj_direct(ptr) + pm_tool::PMDK_PADDING;
    return abs_ptr;
  }

  void Teardown() override {
    uint64_t sum{0};
    for (uint32_t i = 0; i < kArrayLen; i += 1) {
      uint64_t* target = array + i * pm_tool::kCacheLineSize / sizeof(uint64_t);
      sum += *target;
    }
    std::cout << "Succeeded CAS: " << sum << std::endl;

    std::cout << "Read operation compeleted: " << read_count_ << std::endl;
    auto oid = pmemobj_oid((char*)pm_tool::DirtyTable::GetInstance() -
                           pm_tool::PMDK_PADDING);
    pmemobj_free(&oid);
    oid = pmemobj_oid((char*)array - pm_tool::PMDK_PADDING);
    pmemobj_free(&oid);
  }
};

struct PCASBench : public BaseBench {
  const char* GetBenchName() override { return "PCASBench"; }

  PCASBench() : BaseBench() {}

  void Entry(size_t thread_idx, size_t thread_count) override {
    if (thread_idx == 0) {
      WorkLoadInit();
    }

    std::uniform_int_distribution<std::mt19937::result_type> dist(
        0, kArrayLen - 1);

    WaitForStart();

    if (thread_idx == 0) {
      size_t lc_read{0};
      while (GetThreadsFinished() != (thread_count - 1)) {
        uint32_t pos = dist(rng);
        uint64_t* target =
            array + pos * pm_tool::kCacheLineSize / sizeof(uint64_t);
        volatile uint64_t value = *target;
        lc_read += 1;
      }
      read_count_ = lc_read;
      return;
    }

    for (uint32_t i = 0; i < kOpCnt; i += 1) {
      uint32_t pos = dist(rng);
      uint64_t* target =
          array + pos * pm_tool::kCacheLineSize / sizeof(uint64_t);
      uint64_t value = *target;
      pm_tool::PersistentCAS(target, value, value + 1);
    }
  }
};

struct CASBench : public BaseBench {
  const char* GetBenchName() override { return "CASBench"; }
  CASBench() : BaseBench() {}

  void Entry(size_t thread_idx, size_t thread_count) override {
    if (thread_idx == 0) {
      WorkLoadInit();
    }

    std::uniform_int_distribution<std::mt19937::result_type> dist(
        0, kArrayLen - 1);

    WaitForStart();

    if (thread_idx == 0) {
      size_t lc_read{0};
      while (GetThreadsFinished() != (thread_count - 1)) {
        uint32_t pos = dist(rng);
        uint64_t* target =
            array + pos * pm_tool::kCacheLineSize / sizeof(uint64_t);
        volatile uint64_t value = *target;
        lc_read += 1;
      }
      read_count_ = lc_read;
      return;
    }

    for (uint32_t i = 0; i < kOpCnt; i += 1) {
      uint32_t pos = dist(rng);
      uint64_t* target =
          array + pos * pm_tool::kCacheLineSize / sizeof(uint64_t);
      uint64_t value = *target;

      pm_tool::CompareExchange64(target, value + 1, value);
      pm_tool::flush(target);
      pm_tool::fence();
    }
  }
};

struct DirtyCASBench : public BaseBench {
  const char* GetBenchName() override { return "DirtyCASBench"; }
  DirtyCASBench() : BaseBench() {}

  static const uint64_t kDirtyBitMask = 0x8000000000000000ull;

  void Entry(size_t thread_idx, size_t thread_count) override {
    if (thread_idx == 0) {
      WorkLoadInit();
    }

    std::uniform_int_distribution<std::mt19937::result_type> dist(
        0, kArrayLen - 1);

    WaitForStart();

    if (thread_idx == 0) {
      size_t lc_read{0};
      while (GetThreadsFinished() != (thread_count - 1)) {
        uint32_t pos = dist(rng);
        uint64_t* target =
            array + pos * pm_tool::kCacheLineSize / sizeof(uint64_t);
        volatile uint64_t old_v = *target;
        while ((old_v & kDirtyBitMask) != 0) {
          uint64_t cleared = old_v & (~kDirtyBitMask);
          pm_tool::CompareExchange64(target, cleared, old_v);
          old_v = *target;
        }
        lc_read += 1;
      }
      read_count_ = lc_read;
      return;
    }

    for (uint32_t i = 0; i < kOpCnt; i += 1) {
      uint32_t pos = dist(rng);
      uint64_t* target =
          array + pos * pm_tool::kCacheLineSize / sizeof(uint64_t);

      uint64_t old_v = *target;
      while ((old_v & kDirtyBitMask) != 0) {
        uint64_t cleared = old_v & (~kDirtyBitMask);
        pm_tool::CompareExchange64(target, cleared, old_v);
        old_v = *target;
      }

      uint64_t new_v = old_v + 1;
      uint64_t dirty_value = new_v | kDirtyBitMask;

      pm_tool::CompareExchange64(target, dirty_value, old_v);
      pm_tool::flush(target);
      pm_tool::fence();
      pm_tool::CompareExchange64(target, new_v, dirty_value);
    }
  }
};

int main(int argc, char** argv) {
  if (argc == 2) {
    int32_t bench_to_run = atoi(argv[1]);
    switch (bench_to_run) {
      case 1: {
        auto pcas_bench = std::make_unique<PCASBench>();
        pcas_bench->Run(16);
        break;
      }

      case 2: {
        auto cas_bench = std::make_unique<CASBench>();
        cas_bench->Run(16);
        break;
      }
      case 3: {
        auto dirty_cas_bench = std::make_unique<DirtyCASBench>();
        dirty_cas_bench->Run(16);
        break;
      }
      default:
        break;
    }
    return 0;
  }

  {
    auto pcas_bench = std::make_unique<PCASBench>();
    pcas_bench->Run(2)->Run(3)->Run(5)->Run(9)->Run(17)->Run(24);
  }
  {
    auto cas_bench = std::make_unique<CASBench>();
    cas_bench->Run(2)->Run(3)->Run(5)->Run(8)->Run(17)->Run(24);
  }
  {
    auto dirty_cas_bench = std::make_unique<DirtyCASBench>();
    dirty_cas_bench->Run(2)->Run(3)->Run(5)->Run(9)->Run(17)->Run(24);
  }
}
