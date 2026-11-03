#include "../src/structures/avl_tree.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <utility>

static void testEmptyTree()
{
    AVLTree<int, std::string> tree;
    int key = -1;
    std::string value = "unchanged";
    assert(tree.empty());
    assert(tree.size() == 0);
    assert(!tree.getMin(key, value));
    assert(!tree.popMin(key, value));
    assert(!tree.find(10, value));
}

static void testInsertFindAndUpdate()
{
    AVLTree<int, std::string> tree;
    assert(tree.insert(20, "twenty"));
    assert(tree.insert(10, "ten"));
    assert(tree.insert(30, "thirty"));
    assert(!tree.insert(20, "updated")); // duplicate updates value
    std::string value;
    assert(tree.find(20, value) && value == "updated");
    assert(tree.contains(10));
    assert(!tree.contains(99));
    assert(tree.size() == 3);
}

static void testAllRotationCases()
{
    // LL, RR, LR, RL insertion patterns. Each must remain searchable.
    const int patterns[4][3] = {
        {30, 20, 10}, {10, 20, 30}, {30, 10, 20}, {10, 30, 20}};
    for (int p = 0; p < 4; ++p)
    {
        AVLTree<int, int> tree;
        for (int i = 0; i < 3; ++i)
            tree.insert(patterns[p][i], patterns[p][i]);
        for (int i = 0; i < 3; ++i)
        {
            int value = 0;
            assert(tree.find(patterns[p][i], value));
            assert(value == patterns[p][i]);
        }
        assert(tree.size() == 3);
    }
}

static void testRemovalCases()
{
    AVLTree<int, int> tree;
    const int values[] = {40, 20, 60, 10, 30, 50, 70, 25};
    for (int i = 0; i < 8; ++i)
        tree.insert(values[i], values[i]);

    assert(tree.remove(10)); // leaf
    assert(tree.remove(30)); // one-child / rebalancing case
    assert(tree.remove(40)); // two children
    assert(!tree.remove(999));

    for (int i = 0; i < 8; ++i)
    {
        if (values[i] == 10 || values[i] == 30 || values[i] == 40)
            assert(!tree.contains(values[i]));
        else
            assert(tree.contains(values[i]));
    }
    assert(tree.size() == 5);
}

static void testMinimumOperations()
{
    AVLTree<std::pair<int, std::string>, std::string> tree;
    tree.insert({120, "currency"}, "currency");
    tree.insert({100, "weather"}, "weather");
    tree.insert({150, "course"}, "course");
    tree.insert({100, "another"}, "another");

    std::pair<int, std::string> key;
    std::string value;

    assert(tree.getMin(key, value));
    assert(key.first == 100);
    assert(key.second == "another");

    assert(tree.popMin(key, value));
    assert(value == "another");
    assert(tree.size() == 3);

    assert(tree.popMin(key, value));
    assert(value == "weather");
    assert(tree.popMin(key, value));
    assert(value == "currency");
    assert(tree.popMin(key, value));
    assert(value == "course");
    assert(tree.empty());
}

static void testClearAndReuse()
{
    AVLTree<int, int> tree;
    for (int i = 0; i < 100; ++i)
        tree.insert(i, i);
    tree.clear();
    assert(tree.empty());
    assert(tree.size() == 0);
    tree.insert(5, 50);
    int value = 0;
    assert(tree.find(5, value) && value == 50);
}

int main()
{
    testEmptyTree();
    testInsertFindAndUpdate();
    testAllRotationCases();
    testRemovalCases();
    testMinimumOperations();
    testClearAndReuse();
    std::cout << "All AVL Tree tests passed!" << std::endl;
    return 0;
}
