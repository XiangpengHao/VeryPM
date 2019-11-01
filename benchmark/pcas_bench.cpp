#include <glog/logging.h>
#include <gtest/gtest.h>
#include "../pcas.h"
#include "bench_common.h"

POBJ_LAYOUT_BEGIN(garbagelist);
POBJ_LAYOUT_TOID(garbagelist, char)
POBJ_LAYOUT_END(garbagelist)

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
  static const constexpr uint32_t array_size = 1000;
  static const constexpr uint32_t op_cnt_ = 10000;

  uint64_t* array{nullptr};

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
          pm_tool::PMDK_PADDING + sizeof(pm_tool::kCacheLineSize) * array_size,
          TOID_TYPE_NUM(char));
      array = (uint64_t*)((char*)pmemobj_direct(ptr) + pm_tool::PMDK_PADDING);
    }

    WaitForStart();

    for (uint32_t i = 0; i < op_cnt_; i += 1) {
    }
  }

  void Teardown() override {
    auto oid = pmemobj_oid((char*)pm_tool::DirtyTable::GetInstance() -
                           pm_tool::PMDK_PADDING);
    pmemobj_free(&oid);
    oid = pmemobj_oid((char*)array - pm_tool::PMDK_PADDING);
    pmemobj_free(&oid);
  }
};

int main() {}
