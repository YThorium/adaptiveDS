#include "test_framework.hpp"
#include "adaptive_ds/AdaptationEngine.hpp"

using namespace adaptive_ds;

TEST(AdaptationEngine, SmallDatasetPreference) {
    AdaptationConfig cfg;
    cfg.small_dataset_threshold = 30;
    AdaptationEngine engine(cfg);

    WorkloadStats stats;
    stats.window_size = 100;
    stats.lookup_freq = 0.7;
    stats.insert_freq = 0.2;
    stats.erase_freq = 0.1;

    // For N = 10 (small dataset), DynamicArray should have lower or highly competitive cost due to cache locality
    auto cost_arr = engine.estimate_cost(StructureType::DynamicArray, 10, stats);
    auto cost_avl = engine.estimate_cost(StructureType::AVLTree, 10, stats);
    ASSERT_TRUE(cost_arr.total_estimated_cost < cost_avl.total_estimated_cost);
}

TEST(AdaptationEngine, RangeQueryTriggersAVL) {
    AdaptationConfig cfg;
    cfg.min_operations_before_adaptation = 50;
    cfg.cooldown_operations = 50;
    AdaptationEngine engine(cfg);

    WorkloadStats stats;
    stats.window_size = 200;
    stats.range_freq = 0.8;
    stats.lookup_freq = 0.1;
    stats.insert_freq = 0.1;

    // In a range-heavy workload with N=1000, AVLTree must be selected over HashTable
    auto dec = engine.evaluate(StructureType::HashTable, 1000, stats, 100);
    ASSERT_TRUE(dec.should_adapt);
    ASSERT_EQ(dec.target_structure, StructureType::AVLTree);
}

TEST(AdaptationEngine, LookupQueryTriggersHashTable) {
    AdaptationConfig cfg;
    cfg.min_operations_before_adaptation = 50;
    cfg.cooldown_operations = 50;
    AdaptationEngine engine(cfg);

    WorkloadStats stats;
    stats.window_size = 200;
    stats.lookup_freq = 0.9;
    stats.insert_freq = 0.1;
    stats.range_freq = 0.0;

    // Point lookup heavy workload with N=2000, HashTable must be selected over DynamicArray
    auto dec = engine.evaluate(StructureType::DynamicArray, 2000, stats, 100);
    ASSERT_TRUE(dec.should_adapt);
    ASSERT_EQ(dec.target_structure, StructureType::HashTable);
}

TEST(AdaptationEngine, CooldownSuppressesAdaptation) {
    AdaptationConfig cfg;
    cfg.min_operations_before_adaptation = 50;
    cfg.cooldown_operations = 100;
    AdaptationEngine engine(cfg);

    WorkloadStats stats;
    stats.window_size = 200;
    stats.range_freq = 0.8;

    // Only 20 ops since last switch (in cooldown of 100)
    auto dec = engine.evaluate(StructureType::HashTable, 1000, stats, 20);
    ASSERT_FALSE(dec.should_adapt);
}
