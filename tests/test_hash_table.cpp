#include "test_framework.hpp"
#include "adaptive_ds/HashTable.hpp"
#include <string>

using namespace adaptive_ds;

TEST(HashTable, BasicInsertAndFind) {
    HashTable<int> ht;
    ASSERT_EQ(ht.size(), 0);
    ASSERT_TRUE(ht.empty());

    ASSERT_TRUE(ht.insert(42));
    ASSERT_TRUE(ht.insert(99));
    ASSERT_FALSE(ht.insert(42)); // Duplicate
    ASSERT_EQ(ht.size(), 2);

    ASSERT_TRUE(ht.contains(42));
    ASSERT_TRUE(ht.contains(99));
    ASSERT_FALSE(ht.contains(100));

    const int* val = ht.find(42);
    ASSERT_TRUE(val != nullptr);
    ASSERT_EQ(*val, 42);
}

TEST(HashTable, RehashingOnScale) {
    HashTable<int> ht(8, 0.75); // Small table to force multiple rehashes
    size_t initial_buckets = ht.bucket_count();

    for (int i = 0; i < 200; ++i) {
        ASSERT_TRUE(ht.insert(i * 3));
    }
    ASSERT_EQ(ht.size(), 200);
    ASSERT_TRUE(ht.bucket_count() > initial_buckets);

    // Verify all inserted items are intact after rehashing
    for (int i = 0; i < 200; ++i) {
        ASSERT_TRUE(ht.contains(i * 3));
    }
}

TEST(HashTable, EraseOperations) {
    HashTable<std::string> ht;
    ht.insert("alpha");
    ht.insert("beta");
    ht.insert("gamma");

    ASSERT_TRUE(ht.erase("beta"));
    ASSERT_EQ(ht.size(), 2);
    ASSERT_FALSE(ht.contains("beta"));
    ASSERT_TRUE(ht.contains("alpha"));
    ASSERT_TRUE(ht.contains("gamma"));

    // Erase non-existent
    ASSERT_FALSE(ht.erase("delta"));
    ASSERT_EQ(ht.size(), 2);
}

TEST(HashTable, RangeQuery) {
    HashTable<int> ht;
    for (int v : {10, 5, 20, 15, 30, 25}) {
        ht.insert(v);
    }

    auto r = ht.range(10, 25);
    // Expected sorted range: 10, 15, 20, 25
    ASSERT_EQ(r.size(), 4);
    ASSERT_EQ(r[0], 10);
    ASSERT_EQ(r[1], 15);
    ASSERT_EQ(r[2], 20);
    ASSERT_EQ(r[3], 25);
}
