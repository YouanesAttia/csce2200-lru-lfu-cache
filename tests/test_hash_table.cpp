#include <iostream>
#include <cassert>
#include <string>
#include "../src/structures/hash_table.hpp"
#include "../src/structures/doubly_linked_list.hpp"
using namespace std;

void test_empty_table()
{
    HashTable<int> table;
    assert(table.empty() == true);
    assert(table.size() == 0);
    assert(table.find("missing") == 0);
    assert(table.contains("missing") == false);
    assert(table.remove("missing") == false);
    assert(table.update("missing", 5) == false);
    cout << "[SUCCESS] test_empty_table" << endl;
}

void test_insert_and_find()
{
    HashTable<int> table;
    assert(table.insert("alpha", 1) == true);
    assert(table.insert("beta", 2) == true);
    assert(table.insert("gamma", 3) == true);
    assert(table.size() == 3);

    assert(*table.find("alpha") == 1);
    assert(*table.find("beta") == 2);
    assert(*table.find("gamma") == 3);
    assert(table.contains("alpha") == true);
    assert(table.find("delta") == 0);

    // find gives the stored value itself, not a copy, so writing through the
    // pointer has to change what is in the table
    int *value = table.find("alpha");
    *value = 99;
    assert(*table.find("alpha") == 99);

    // an empty string is a normal key, not a missing one
    assert(table.insert("", 0) == true);
    assert(table.contains("") == true);
    cout << "[SUCCESS] test_insert_and_find" << endl;
}

void test_duplicate_insert()
{
    HashTable<int> table;
    assert(table.insert("key", 10) == true);
    assert(table.insert("key", 20) == false);
    assert(*table.find("key") == 10); // the first value stays
    assert(table.size() == 1);        // and no second node was added
    cout << "[SUCCESS] test_duplicate_insert" << endl;
}

void test_update()
{
    HashTable<int> table;
    table.insert("key", 1);
    assert(table.update("key", 2) == true);
    assert(*table.find("key") == 2);

    // update on a missing key must not insert it
    assert(table.update("absent", 3) == false);
    assert(table.size() == 1);
    assert(table.contains("absent") == false);
    cout << "[SUCCESS] test_update" << endl;
}

void test_remove()
{
    HashTable<int> table;
    table.insert("a", 1);
    table.insert("b", 2);

    assert(table.remove("a") == true);
    assert(table.size() == 1);
    assert(table.contains("a") == false);
    assert(*table.find("b") == 2); // the other key is not affected

    assert(table.remove("a") == false); // removing it again says it is gone

    assert(table.insert("a", 7) == true); // and it can be added back
    assert(*table.find("a") == 7);
    cout << "[SUCCESS] test_remove" << endl;
}

void test_collisions()
{
    // "Aa" and "BB" give exactly the same hash:
    //   "Aa" -> 65 * 31 + 97 = 2112
    //   "BB" -> 66 * 31 + 66 = 2112
    // It works for longer keys too, so all six of these collide. This is the
    // weak point of a 31 based string hash, and it is why collisions have to
    // be handled properly instead of assumed to be rare.
    HashTable<int> table;
    const string keys[] = {"Aa", "BB", "AaAa", "AaBB", "BBAa", "BBBB"};
    const int count = sizeof(keys) / sizeof(keys[0]); // how many keys are above

    for (int i = 0; i < count; ++i)
        assert(table.insert(keys[i], i) == true);

    // they all went to the same bucket, and all of them are still there
    for (int i = 0; i < count; ++i)
        assert(table.bucketIndex(keys[i]) == table.bucketIndex(keys[0]));
    assert(table.longestChain() == (size_t)count);

    for (int i = 0; i < count; ++i)
        assert(*table.find(keys[i]) == i);

    // changing one key in the chain must not touch the others
    assert(table.update("AaBB", 100) == true);
    assert(*table.find("AaBB") == 100);
    assert(*table.find("Aa") == 0);
    assert(*table.find("BBBB") == 5);
    cout << "[SUCCESS] test_collisions" << endl;
}

void test_remove_from_chain()
{
    // Removing the first node of a chain is a different case from removing one
    // in the middle, so both are tested. New keys go in at the front, so the
    // chain here is BBBB -> BBAa -> AaBB -> BB -> Aa.
    HashTable<int> table;
    const string keys[] = {"Aa", "BB", "AaBB", "BBAa", "BBBB"};
    const int count = sizeof(keys) / sizeof(keys[0]);
    for (int i = 0; i < count; ++i)
        table.insert(keys[i], i);
    assert(table.longestChain() == (size_t)count);

    assert(table.remove("AaBB") == true); // middle
    assert(table.longestChain() == 4);
    assert(table.contains("AaBB") == false);

    assert(table.remove("BBBB") == true); // first
    assert(table.longestChain() == 3);

    assert(table.remove("Aa") == true); // last
    assert(table.longestChain() == 2);

    // the ones we did not remove are still there and still correct
    assert(*table.find("BB") == 1);
    assert(*table.find("BBAa") == 3);
    assert(table.size() == 2);
    cout << "[SUCCESS] test_remove_from_chain" << endl;
}

void test_growth()
{
    HashTable<int> table(4); // start tiny so it has to grow several times
    const size_t startingBuckets = table.bucketCount();
    const int count = 1000;

    // to_string makes the number into text. "key" + i would not work, "key" is
    // a char pointer so + i would just move the pointer along it.
    for (int i = 0; i < count; ++i)
        assert(table.insert("key" + to_string(i), i) == true);

    assert(table.size() == (size_t)count);
    assert(table.bucketCount() > startingBuckets); // it did grow
    assert(table.loadFactor() <= 0.75);

    // growing rebuilds every bucket, so check nothing got lost on the way
    for (int i = 0; i < count; ++i)
        assert(*table.find("key" + to_string(i)) == i);

    // 1000 keys spread over the buckets should leave the chains short
    assert(table.longestChain() <= 8);

    for (int i = 0; i < count; ++i)
        assert(table.remove("key" + to_string(i)) == true);
    assert(table.empty() == true);
    cout << "[SUCCESS] test_growth" << endl;
}

void test_clear()
{
    HashTable<int> table;
    for (int i = 0; i < 100; ++i)
        table.insert("key" + to_string(i), i);
    const size_t buckets = table.bucketCount();

    table.clear();
    assert(table.empty() == true);
    assert(table.size() == 0);
    assert(table.find("key5") == 0);
    assert(table.bucketCount() == buckets); // the array stays, the keys do not

    // and the table still works after being cleared
    assert(table.insert("key5", 5) == true);
    assert(*table.find("key5") == 5);
    cout << "[SUCCESS] test_clear" << endl;
}

void test_pointer_values()
{
    HashTable<DoublyLinkedList::Node *> index;

    DoublyLinkedList::Node n1("k1", "first_data");
    DoublyLinkedList::Node n2("k2", "second_data");
    DoublyLinkedList::Node n3("k3", "third_data");

    assert(index.empty() == true);

    assert(index.insert("cache_key1", &n1) == true);
    assert(index.insert("cache_key2", &n2) == true);
    assert(index.size() == 2);

    assert(*index.find("cache_key1") == &n1);
    assert(*index.find("cache_key2") == &n2);

    assert(index.update("cache_key1", &n3) == true);
    assert(*index.find("cache_key1") == &n3);

    assert(index.remove("cache_key2") == true);
    assert(index.find("cache_key2") == nullptr);
    assert(index.size() == 1);

    cout << "[SUCCESS] test_pointer_values" << endl;
}

int main()
{
    test_empty_table();
    test_insert_and_find();
    test_duplicate_insert();
    test_update();
    test_remove();
    test_collisions();
    test_remove_from_chain();
    test_growth();
    test_clear();
    test_pointer_values();
    cout << "ALL TESTS PASSED!" << endl;
    return 0;
}
