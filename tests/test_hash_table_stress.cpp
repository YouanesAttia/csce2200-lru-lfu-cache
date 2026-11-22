/*
    Stress test for HashTable.

    The tests in test_hash_table.cpp check behaviour with small, hand-picked
    inputs. This one checks the same table still behaves at a size the cache
    will actually reach: 100,000 keys, which forces the bucket array to double
    about thirteen times.

    Growing is where a hash table loses data silently. Nothing crashes, the
    keys just quietly stop being findable, so every stage below re-checks all
    100,000 keys instead of spot-checking a few.
*/

#include <cassert>
#include <iostream>
#include <string>
#include "../src/structures/hash_table.hpp"
using namespace std;

static const int COUNT = 100000;

// Keys are "key0".."key99999". to_string turns the number into text; "key" + i
// would not, because "key" is a char pointer so + i would move the pointer.
static string keyFor(int i) { return "key" + to_string(i); }

void test_stress_growth()
{
    HashTable<int> table(4); // start tiny so it has to grow all the way up

    for (int i = 0; i < COUNT; ++i)
        assert(table.insert(keyFor(i), i) == true);

    assert(table.size() == (size_t)COUNT);
    assert(table.loadFactor() <= 0.75);
    assert(table.bucketCount() > 4);

    // every key survived every resize
    for (int i = 0; i < COUNT; ++i)
    {
        const int *value = table.find(keyFor(i));
        assert(value != 0);
        assert(*value == i);
    }

    // with a reasonable hash, 100k keys should still leave short chains
    assert(table.longestChain() <= 12);

    cout << "[SUCCESS] test_stress_growth  (" << COUNT << " keys, "
         << table.bucketCount() << " buckets, longest chain "
         << table.longestChain() << ")" << endl;
}

void test_stress_remove_and_reinsert()
{
    HashTable<int> table;

    for (int i = 0; i < COUNT; ++i)
        table.insert(keyFor(i), i);

    // take out every other key
    for (int i = 0; i < COUNT; i += 2)
        assert(table.remove(keyFor(i)) == true);

    assert(table.size() == (size_t)(COUNT / 2));

    // the ones that are left are untouched, the removed ones are really gone
    for (int i = 0; i < COUNT; ++i)
    {
        if (i % 2 == 0)
            assert(table.contains(keyFor(i)) == false);
        else
            assert(*table.find(keyFor(i)) == i);
    }

    // removing something twice reports that it is not there
    for (int i = 0; i < COUNT; i += 2)
        assert(table.remove(keyFor(i)) == false);

    // put them all back, with new values this time
    for (int i = 0; i < COUNT; i += 2)
        assert(table.insert(keyFor(i), i + COUNT) == true);

    assert(table.size() == (size_t)COUNT);

    for (int i = 0; i < COUNT; ++i)
    {
        const int *value = table.find(keyFor(i));
        assert(value != 0);
        assert(*value == (i % 2 == 0 ? i + COUNT : i));
    }

    cout << "[SUCCESS] test_stress_remove_and_reinsert" << endl;
}

void test_stress_update()
{
    // The expiration heap keeps a key -> array position map in a HashTable and
    // rewrites a position on every swap, so update() gets called constantly.
    // This checks it holds up at size.
    HashTable<int> table;

    for (int i = 0; i < COUNT; ++i)
        table.insert(keyFor(i), i);

    for (int i = 0; i < COUNT; ++i)
        assert(table.update(keyFor(i), -i) == true);

    for (int i = 0; i < COUNT; ++i)
        assert(*table.find(keyFor(i)) == -i);

    // update must never create a key
    assert(table.update("not-a-real-key", 1) == false);
    assert(table.size() == (size_t)COUNT);

    cout << "[SUCCESS] test_stress_update" << endl;
}

void test_stress_clear()
{
    HashTable<int> table;

    for (int i = 0; i < COUNT; ++i)
        table.insert(keyFor(i), i);

    table.clear();
    assert(table.empty() == true);
    assert(table.size() == 0);

    for (int i = 0; i < COUNT; i += 1000)
        assert(table.contains(keyFor(i)) == false);

    // still usable after being emptied
    assert(table.insert("after", 1) == true);
    assert(*table.find("after") == 1);

    cout << "[SUCCESS] test_stress_clear" << endl;
}

int main()
{
    test_stress_growth();
    test_stress_remove_and_reinsert();
    test_stress_update();
    test_stress_clear();
    cout << "ALL TESTS PASSED!" << endl;
    return 0;
}
