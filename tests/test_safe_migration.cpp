#include "test_framework.hpp"
#include "adaptive_ds/AdaptiveDS.hpp"
#include <vector>
#include <algorithm>

using namespace adaptive_ds;

TEST(SafeMigration, AllPairwiseTransitions) {
    std::vector<StructureType> all_types = {
        StructureType::DynamicArray,
        StructureType::DoublyLinkedList,
        StructureType::HashTable,
        StructureType::AVLTree
    };

    const size_t NUM_ELEMENTS = 150;

    for (StructureType from_type : all_types) {
        for (StructureType to_type : all_types) {
            if (from_type == to_type) continue;

            AdaptiveDS<int> ds(from_type);
            ds.set_adaptive_enabled(false); // Controlled test

            for (size_t i = 0; i < NUM_ELEMENTS; ++i) {
                ds.insert(static_cast<int>(i * 7 + 3));
            }
            ASSERT_EQ(ds.size(), NUM_ELEMENTS);

            // Force adaptation
            bool migrated = ds.force_adapt(to_type);
            ASSERT_TRUE(migrated);
            ASSERT_EQ(ds.current_structure_type(), to_type);
            ASSERT_EQ(ds.size(), NUM_ELEMENTS);

            // Verify all elements are intact
            for (size_t i = 0; i < NUM_ELEMENTS; ++i) {
                ASSERT_TRUE(ds.contains(static_cast<int>(i * 7 + 3)));
            }

            // Verify non-existent element
            ASSERT_FALSE(ds.contains(999999));

            // Verify range query works after migration
            auto rng = ds.range(100, 300);
            ASSERT_TRUE(!rng.empty());
            for (size_t k = 1; k < rng.size(); ++k) {
                ASSERT_TRUE(rng[k-1] <= rng[k]); // strictly sorted
            }
        }
    }
}
