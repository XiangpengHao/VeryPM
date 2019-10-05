#include <glog/logging.h>
#include <gtest/gtest.h>
#include <map>
#include <memory>
#include <random>
#include "../epoch_manager.h"

GTEST_TEST(EpochTest, EpochSmoke) {
  LOG(INFO) << "running tests";
  auto epm = std::make_unique<EpochManager>();
  epm->Initialize();
  epm->Protect();
  ASSERT_TRUE(true);
  epm->Unprotect();
}

class EpochManagerTest : public ::testing::Test {
 public:
  EpochManagerTest() {}

 protected:
  EpochManager mgr_;

  virtual void SetUp() { mgr_.Initialize(); }

  virtual void TearDown() {
    mgr_.Uninitialize();
    Thread::ClearRegistry(true);
  }
};

TEST_F(EpochManagerTest, Initialize) {
  // Initialize() called by the unit test framework.
  ASSERT_TRUE(mgr_.Initialize());
  EXPECT_EQ(1llu, mgr_.current_epoch_.load());
  EXPECT_EQ(0llu, mgr_.safe_to_reclaim_epoch_.load());
  // At least make sure table initializer was called.
  ASSERT_NE(nullptr, mgr_.epoch_table_);
}

TEST_F(EpochManagerTest, Uninitialize) {
  EXPECT_TRUE(mgr_.Uninitialize());

  EpochManager default_mgr;
  ASSERT_TRUE(default_mgr.epoch_table_ == mgr_.epoch_table_);
  EXPECT_EQ(nullptr, mgr_.epoch_table_);
  EXPECT_EQ(default_mgr.current_epoch_.load(), mgr_.current_epoch_.load());
  EXPECT_EQ(default_mgr.safe_to_reclaim_epoch_.load(),
            mgr_.safe_to_reclaim_epoch_.load());

  EXPECT_TRUE(mgr_.Uninitialize());
}

TEST_F(EpochManagerTest, Protect) {
  mgr_.BumpCurrentEpoch();
  EXPECT_TRUE(mgr_.Protect());
  // Make sure the table is clear except the one new entry.
  auto* table = mgr_.epoch_table_->table_;
  for (uint64_t i = 0; i < mgr_.epoch_table_->size_; ++i) {
    const auto& entry = table[i];
    if (entry.thread_id != 0) {
      EXPECT_EQ(pthread_self(), entry.thread_id.load());
      EXPECT_EQ(2llu, entry.protected_epoch.load());
      EXPECT_EQ(0llu, entry.last_unprotected_epoch);
      return;
    }
    EXPECT_EQ(0lu, entry.thread_id.load());
    EXPECT_EQ(0llu, entry.protected_epoch.load());
    EXPECT_EQ(0llu, entry.last_unprotected_epoch);
  }
}

TEST_F(EpochManagerTest, Unprotect) {
  mgr_.BumpCurrentEpoch();
  EXPECT_TRUE(mgr_.Protect());
  mgr_.BumpCurrentEpoch();
  EXPECT_TRUE(mgr_.Unprotect());
}

TEST_F(EpochManagerTest, BumpCurrentEpoch) {
  EXPECT_EQ(1llu, mgr_.GetCurrentEpoch());
  mgr_.BumpCurrentEpoch();
  EXPECT_EQ(2llu, mgr_.GetCurrentEpoch());
}

TEST_F(EpochManagerTest, ComputeNewSafeToReclaimEpoch) {
  mgr_.epoch_table_->table_[0].protected_epoch = 98;
  mgr_.current_epoch_ = 99;
  mgr_.ComputeNewSafeToReclaimEpoch(99);
  EXPECT_EQ(97llu, mgr_.safe_to_reclaim_epoch_.load());
  mgr_.epoch_table_->table_[0].protected_epoch = 0;
  EXPECT_EQ(97llu, mgr_.safe_to_reclaim_epoch_.load());
  mgr_.ComputeNewSafeToReclaimEpoch(99);
  EXPECT_EQ(98llu, mgr_.safe_to_reclaim_epoch_.load());
}

class MinEpochTableTest : public ::testing::Test {
 public:
  MinEpochTableTest() {}

 protected:
  typedef EpochManager::MinEpochTable MinEpochTable;
  MinEpochTable table_;

  virtual void SetUp() { table_.Initialize(); }

  virtual void TearDown() { table_.Uninitialize(); }
};

TEST_F(MinEpochTableTest, Initialize) {
  EXPECT_NE(nullptr, table_.table_);
  EXPECT_TRUE(table_.Initialize());
}

TEST_F(MinEpochTableTest, Initialize_SizeNotAPowerOfTwo) {
  EXPECT_TRUE(table_.Uninitialize());
  auto s = table_.Initialize(3lu);
  EXPECT_FALSE(s);
}

TEST_F(MinEpochTableTest, Uninitialize) {
  EXPECT_TRUE(table_.Uninitialize());
  EXPECT_EQ(0llu, table_.size_);
  EXPECT_EQ(nullptr, table_.table_);
  EXPECT_TRUE(table_.Uninitialize());
}

TEST_F(MinEpochTableTest, Protect) {
  EXPECT_TRUE(table_.Protect(99));
  size_t entry_slot = Murmur3_64(pthread_self()) % table_.size_;

  // Make sure the slot got reserved.
  const MinEpochTable::Entry& entry = table_.table_[entry_slot];
  EXPECT_EQ(pthread_self(), entry.thread_id.load());
  EXPECT_EQ(99llu, entry.protected_epoch.load());
  EXPECT_EQ(0llu, entry.last_unprotected_epoch);

  // Make sure none of the other slots got touched.
  for (uint64_t i = 0; i < table_.size_; ++i) {
    const MinEpochTable::Entry& local_entry = table_.table_[i];
    if (entry_slot == i) continue;
    EXPECT_EQ(0lu, local_entry.thread_id.load());
    EXPECT_EQ(0llu, local_entry.protected_epoch.load());
    EXPECT_EQ(0llu, local_entry.last_unprotected_epoch);
  }
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
