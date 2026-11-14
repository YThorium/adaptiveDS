#include "test_framework.hpp"
#include "adaptive_ds/AVLTree.hpp"
#include <string>
#include <vector>

using namespace adaptive_ds;

TEST(AVLTree, InsertAndBalance) {
    AVLTree<int> tree;
    // Sequential insertions test all rotation types (LL, RR, LR, RL)
    for (int i = 1; i <= 31; ++i) {
        ASSERT_TRUE(tree.insert(i));
        ASSERT_TRUE(tree.is_balanced());
    }
    ASSERT_EQ(tree.size(), 31);
    // Tree height should be at most 6 for 31 elements (optimal is 5)
    ASSERT_TRUE(tree.tree_height() <= 6);

    // Duplicates rejected
    ASSERT_FALSE(tree.insert(15));
    ASSERT_EQ(tree.size(), 31);
}

TEST(AVLTree, RotationsAndSearch) {
    AVLTree<int> tree;
    // Specific pattern to trigger Left-Right and Right-Left rotations
    std::vector<int> vals = {30, 10, 20, 40, 50, 45};
    for (int v : vals) {
        tree.insert(v);
        ASSERT_TRUE(tree.is_balanced());
    }

    for (int v : vals) {
        ASSERT_TRUE(tree.contains(v));
    }
    ASSERT_FALSE(tree.contains(999));
}

TEST(AVLTree, EraseWithRebalance) {
    AVLTree<int> tree;
    for (int i = 1; i <= 20; ++i) {
        tree.insert(i);
    }
    ASSERT_EQ(tree.size(), 20);

    // Erase leaf, single-child node, and two-child node (e.g., root/inner)
    ASSERT_TRUE(tree.erase(1));
    ASSERT_TRUE(tree.is_balanced());
    ASSERT_FALSE(tree.contains(1));

    ASSERT_TRUE(tree.erase(10));
    ASSERT_TRUE(tree.is_balanced());
    ASSERT_FALSE(tree.contains(10));

    ASSERT_EQ(tree.size(), 18);
}

TEST(AVLTree, RangeQueryOrdered) {
    AVLTree<int> tree;
    for (int x : {50, 20, 80, 10, 30, 70, 90, 5, 15, 25, 35, 65, 75, 85, 95}) {
        tree.insert(x);
    }

    auto r = tree.range(25, 75);
    // Expected elements in [25, 75]: 25, 30, 35, 50, 65, 70, 75
    std::vector<int> expected = {25, 30, 35, 50, 65, 70, 75};
    ASSERT_EQ(r.size(), expected.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        ASSERT_EQ(r[i], expected[i]);
    }
}
