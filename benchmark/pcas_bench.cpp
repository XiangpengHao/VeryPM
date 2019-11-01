#include <glog/logging.h>
#include <gtest/gtest.h>
#include <libpmemobj.h>
#include <random>
#include "../pcas.h"
#include "bench_common.h"

POBJ_LAYOUT_BEGIN(pcas_layout);
POBJ_LAYOUT_TOID(pcas_layout, char);
POBJ_LAYOUT_END(pcas_layout);

struct PCASBench : public PerformanceTest {
  const char* GetBenchName() override { return "PCASBench"; }

  PMEMobjpool* pool{nullptr};
  PCASBench() {
    static const char* pool_name = "/mnt/pmem0/pcas_pool.data";
    static const char* layout_name = "pcas_layout";
    static const uint64_t pool_size = 1024 * 1024 * 1024;
    if (!pm_tool::FileExists(pool_name)) {
      pool = pmemobj_create(pool_name, layout_name, pool_size,
                            pm_tool::CREATE_MODE_RW);
    } else {
      pool = pmemobj_open(pool_name, layout_name);
    }
    EXPECT_NE(pool, nullptr);
  }

  ~PCASBench() { pmemobj_close(pool); }

  static const constexpr uint32_t item_cnt_ = 48 * 2;
  static const constexpr uint32_t array_len = 1000;
  static const constexpr uint32_t op_cnt_ = 10000;

  uint64_t* array{nullptr};
  std::mt19937 rng{42};

  void Entry(size_t thread_idx, size_t thread_count) override {
    if (thread_idx == 0) {
      PMEMoid ptr;
      pmemobj_zalloc(pool, &ptr,
                     pm_tool::PMDK_PADDING + sizeof(pm_tool::DirtyTable) +
                         sizeof(pm_tool::DirtyTable::Item) * item_cnt_,
                     TOID_TYPE_NUM(char));
      auto table = (pm_tool::DirtyTable*)((char*)pmemobj_direct(ptr) +
                                          pm_tool::PMDK_PADDING);
      pm_tool::DirtyTable::Initialize(table, item_cnt_);

      pmemobj_zalloc(
          pool, &ptr,
          pm_tool::PMDK_PADDING + sizeof(pm_tool::kCacheLineSize) * array_len,
          TOID_TYPE_NUM(char));
      array = (uint64_t*)((char*)pmemobj_direct(ptr) + pm_tool::PMDK_PADDING);
      memset(array, 0, pm_tool::kCacheLineSize * array_len);
    }

    std::uniform_int_distribution<std::mt19937::result_type> dist(
        0, array_len - 1);
    WaitForStart();

    for (uint32_t i = 0; i < op_cnt_; i += 1) {
      uint32_t pos = dist(rng);
      uint64_t* target =
          array + pos * pm_tool::kCacheLineSize / sizeof(uint64_t);
      uint64_t value = *target;
      pm_tool::PersistentCAS(target, value, value + 1);
    }
  }

  void Teardown() override {
    uint64_t sum{0};
    for (uint32_t i = 0; i < array_len; i += 1) {
      uint64_t* target = array + i * pm_tool::kCacheLineSize / sizeof(uint64_t);
      sum += *target;
    }
    std::cout << "sum: " << sum << std::endl;
    auto oid = pmemobj_oid((char*)pm_tool::DirtyTable::GetInstance() -
                           pm_tool::PMDK_PADDING);
    pmemobj_free(&oid);
    oid = pmemobj_oid((char*)array - pm_tool::PMDK_PADDING);
    pmemobj_free(&oid);
  }
};

int main() {
  auto pcas_bench = new PCASBench();
  pcas_bench->Run(1)->Run(2)->Run(4)->Run(8);
}
