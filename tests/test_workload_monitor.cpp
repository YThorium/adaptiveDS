#include "test_framework.hpp"
#include "adaptive_ds/WorkloadMonitor.hpp"

using namespace adaptive_ds;

TEST(WorkloadMonitor, SlidingWindowAccounting) {
    WorkloadMonitor monitor(10); // Window capacity 10

    // Record 5 Inserts and 5 Finds
    for (int i = 0; i < 5; ++i) monitor.record_operation(OperationType::Insert);
    for (int i = 0; i < 5; ++i) monitor.record_operation(OperationType::Find);

    WorkloadStats stats = monitor.get_stats();
    ASSERT_EQ(stats.window_size, 10);
    ASSERT_EQ(stats.insert_count, 5);
    ASSERT_EQ(stats.find_count, 5);
    ASSERT_TRUE(stats.insert_freq >= 0.49 && stats.insert_freq <= 0.51);
    ASSERT_TRUE(stats.lookup_freq >= 0.49 && stats.lookup_freq <= 0.51);

    // Evict the 5 Inserts by adding 10 RangeQueries
    for (int i = 0; i < 10; ++i) monitor.record_operation(OperationType::RangeQuery);

    stats = monitor.get_stats();
    ASSERT_EQ(stats.window_size, 10);
    ASSERT_EQ(stats.range_count, 10);
    ASSERT_EQ(stats.insert_count, 0);
    ASSERT_EQ(stats.find_count, 0);
    ASSERT_TRUE(stats.range_freq >= 0.99);
}

TEST(WorkloadMonitor, ReadWriteRatio) {
    WorkloadMonitor monitor(100);
    for (int i = 0; i < 80; ++i) monitor.record_operation(OperationType::Contains);
    for (int i = 0; i < 20; ++i) monitor.record_operation(OperationType::Insert);

    WorkloadStats stats = monitor.get_stats();
    // 80 reads / (20 writes + 1) = ~3.8
    ASSERT_TRUE(stats.read_write_ratio > 3.5 && stats.read_write_ratio < 4.0);
}
