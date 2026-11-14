#include "test_framework.hpp"
#include "adaptive_ds/DoublyLinkedList.hpp"
#include <string>

using namespace adaptive_ds;

TEST(DoublyLinkedList, BasicOperations) {
    DoublyLinkedList<int> list;
    ASSERT_EQ(list.size(), 0);
    ASSERT_TRUE(list.empty());

    ASSERT_TRUE(list.insert(100));
    ASSERT_TRUE(list.insert(200));
    ASSERT_TRUE(list.insert(300));
    ASSERT_EQ(list.size(), 3);

    // Duplicates rejected
    ASSERT_FALSE(list.insert(200));
    ASSERT_EQ(list.size(), 3);

    ASSERT_TRUE(list.contains(200));
    ASSERT_FALSE(list.contains(400));
}

TEST(DoublyLinkedList, EraseHeadMiddleTail) {
    DoublyLinkedList<int> list;
    list.insert(10);
    list.insert(20);
    list.insert(30);

    // Erase middle
    ASSERT_TRUE(list.erase(20));
    ASSERT_EQ(list.size(), 2);
    ASSERT_FALSE(list.contains(20));

    // Erase head
    ASSERT_TRUE(list.erase(10));
    ASSERT_EQ(list.size(), 1);
    ASSERT_FALSE(list.contains(10));

    // Erase tail
    ASSERT_TRUE(list.erase(30));
    ASSERT_EQ(list.size(), 0);
    ASSERT_TRUE(list.empty());

    // Erase from empty
    ASSERT_FALSE(list.erase(50));
}

TEST(DoublyLinkedList, RangeAndToVector) {
    DoublyLinkedList<int> list;
    for (int v : {50, 10, 40, 20, 30}) {
        list.insert(v);
    }

    auto vec = list.to_vector();
    ASSERT_EQ(vec.size(), 5);

    auto rng = list.range(15, 45);
    // Expected sorted range: 20, 30, 40
    ASSERT_EQ(rng.size(), 3);
    ASSERT_EQ(rng[0], 20);
    ASSERT_EQ(rng[1], 30);
    ASSERT_EQ(rng[2], 40);
}

TEST(DoublyLinkedList, CloneAndDeepCopy) {
    DoublyLinkedList<int> list1;
    list1.insert(5);
    list1.insert(15);

    auto list2 = list1.clone();
    ASSERT_EQ(list2->size(), 2);
    ASSERT_TRUE(list2->contains(5));
    ASSERT_TRUE(list2->contains(15));

    list1.insert(25);
    ASSERT_EQ(list1.size(), 3);
    ASSERT_EQ(list2->size(), 2);
}
