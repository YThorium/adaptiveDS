#include "test_framework.hpp"
#include "adaptive_ds/AdaptiveDS.hpp"
#include <string>

using namespace adaptive_ds;

TEST(AdaptiveDS, UnifiedInterface) {
    AdaptiveDS<std::string> ds(StructureType::DynamicArray);
    ASSERT_TRUE(ds.empty());

    ds.insert("apple");
    ds.insert("banana");
    ds.insert("cherry");
    ASSERT_EQ(ds.size(), 3);

    ASSERT_TRUE(ds.contains("banana"));
    ASSERT_FALSE(ds.contains("orange"));

    const std::string* ptr = ds.find("cherry");
    ASSERT_TRUE(ptr != nullptr);
    ASSERT_EQ(*ptr, "cherry");

    auto rng = ds.range("b", "d");
    // "banana", "cherry"
    ASSERT_EQ(rng.size(), 2);
    ASSERT_EQ(rng[0], "banana");
    ASSERT_EQ(rng[1], "cherry");

    ASSERT_TRUE(ds.erase("banana"));
    ASSERT_EQ(ds.size(), 2);
    ASSERT_FALSE(ds.contains("banana"));

    ds.clear();
    ASSERT_EQ(ds.size(), 0);
    ASSERT_TRUE(ds.empty());
}

TEST(AdaptiveDS, AutomaticAdaptationWorkflow) {
    AdaptationConfig cfg;
    cfg.sliding_window_size = 60;
    cfg.min_operations_before_adaptation = 30;
    cfg.cooldown_operations = 30;
    cfg.improvement_threshold = 0.20;
    cfg.projection_horizon = 500;
    cfg.small_dataset_threshold = 10;
    cfg.enable_automatic_adaptation = true;

    // Start with DynamicArray
    AdaptiveDS<int> ds(StructureType::DynamicArray, cfg);

    // Insert 100 elements
    for (int i = 0; i < 100; ++i) {
        ds.insert(i);
    }

    // Phase 1: Heavy Point Lookups (simulate 150 lookups)
    for (int i = 0; i < 150; ++i) {
        ds.contains(i % 100);
    }

    // Should have adapted from DynamicArray to HashTable!
    ASSERT_EQ(ds.current_structure_type(), StructureType::HashTable);

    // Phase 2: Shift to heavy Range Queries (simulate 150 range queries)
    for (int i = 0; i < 150; ++i) {
        auto r = ds.range(10, 40);
        (void)r;
    }

    // Should have adapted from HashTable to AVLTree!
    ASSERT_EQ(ds.current_structure_type(), StructureType::AVLTree);

    // Verify history records
    const auto& history = ds.get_adaptation_history();
    ASSERT_TRUE(history.size() >= 2);

    ASSERT_EQ(history[0].from_structure, StructureType::DynamicArray);
    ASSERT_EQ(history[0].to_structure, StructureType::HashTable);
    ASSERT_TRUE(!history[0].reason.empty());

    ASSERT_EQ(history[1].from_structure, StructureType::HashTable);
    ASSERT_EQ(history[1].to_structure, StructureType::AVLTree);
    ASSERT_TRUE(!history[1].reason.empty());
}

TEST(AdaptiveDS, ExportHistoryCSV) {
    AdaptiveDS<int> ds(StructureType::DynamicArray);
    for (int i = 0; i < 20; ++i) ds.insert(i);
    ds.force_adapt(StructureType::HashTable);
    ds.force_adapt(StructureType::AVLTree);

    std::string csv_path = "/tmp/test_adaptation_history.csv";
    ASSERT_TRUE(ds.export_history_csv(csv_path));
}
