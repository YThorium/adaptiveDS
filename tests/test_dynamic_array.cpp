#include "test_framework.hpp"
#include "adaptive_ds/DynamicArray.hpp"
#include <string>

using namespace adaptive_ds;

TEST(DynamicArray, BasicInsertAndSize) {
    DynamicArray<int> arr;
    ASSERT_EQ(arr.size(), 0);
    ASSERT_TRUE(arr.empty());

    ASSERT_TRUE(arr.insert(10));
    ASSERT_TRUE(arr.insert(20));
    ASSERT_TRUE(arr.insert(30));
    ASSERT_EQ(arr.size(), 3);
    ASSERT_FALSE(arr.empty());

    // Disallow duplicates
    ASSERT_FALSE(arr.insert(20));
    ASSERT_EQ(arr.size(), 3);
}

TEST(DynamicArray, ContainsAndFind) {
    DynamicArray<std::string> arr;
    arr.insert("apple");
    arr.insert("banana");
    arr.insert("cherry");

    ASSERT_TRUE(arr.contains("banana"));
    ASSERT_FALSE(arr.contains("orange"));

    const std::string* ptr = arr.find("cherry");
    ASSERT_TRUE(ptr != nullptr);
    ASSERT_EQ(*ptr, "cherry");

    ASSERT_TRUE(arr.find("grape") == nullptr);
}

TEST(DynamicArray, EraseAndShrink) {
    DynamicArray<int> arr;
    for (int i = 0; i < 50; ++i) {
        arr.insert(i);
    }
    ASSERT_EQ(arr.size(), 50);

    ASSERT_TRUE(arr.erase(25));
    ASSERT_FALSE(arr.contains(25));
    ASSERT_EQ(arr.size(), 49);

    // Erasing non-existent element
    ASSERT_FALSE(arr.erase(100));

    // Erase multiple elements to trigger shrink check
    for (int i = 0; i < 40; ++i) {
        arr.erase(i);
    }
    // 0..39 was erased (with 25 already erased earlier), remaining elements are 40..49 = 10 elements
    ASSERT_EQ(arr.size(), 10);
    ASSERT_TRUE(arr.contains(49));
}

TEST(DynamicArray, RangeQuery) {
    DynamicArray<int> arr;
    for (int x : {15, 3, 9, 22, 1, 18, 7}) {
        arr.insert(x);
    }

    auto r = arr.range(5, 18);
    // Expected sorted range: 7, 9, 15, 18
    ASSERT_EQ(r.size(), 4);
    ASSERT_EQ(r[0], 7);
    ASSERT_EQ(r[1], 9);
    ASSERT_EQ(r[2], 15);
    ASSERT_EQ(r[3], 18);
}

TEST(DynamicArray, CopyAndMove) {
    DynamicArray<int> original;
    original.insert(1);
    original.insert(2);
    original.insert(3);

    // Copy constructor
    DynamicArray<int> copy = original;
    ASSERT_EQ(copy.size(), 3);
    ASSERT_TRUE(copy.contains(2));

    // Modify copy without affecting original
    copy.insert(4);
    ASSERT_EQ(copy.size(), 4);
    ASSERT_EQ(original.size(), 3);

    // Move constructor
    DynamicArray<int> moved = std::move(copy);
    ASSERT_EQ(moved.size(), 4);
    ASSERT_TRUE(moved.contains(4));
}
