#include "../src/cache/lfu_cache.hpp"

#include <cassert>
#include <iostream>
#include <string>

using namespace std;

void testEmptyCacheAndMissingKey()
{
    LFUCache cache(3);
    string value = "unchanged";

    assert(cache.size() == 0);
    assert(cache.capacity() == 3);
    assert(cache.minFrequency() == 0);
    assert(!cache.contains("missing"));
    assert(!cache.get("missing", value));
    assert(value == "unchanged");
    assert(cache.frequencyOf("missing") == 0);
    assert(cache.validate());
}

void testNewEntriesStartAtFrequencyOne()
{
    LFUCache cache(3);

    cache.put("A", "Apple");
    cache.put("B", "Banana");
    cache.put("C", "Cherry");

    assert(cache.size() == 3);
    assert(cache.frequencyOf("A") == 1);
    assert(cache.frequencyOf("B") == 1);
    assert(cache.frequencyOf("C") == 1);
    assert(cache.minFrequency() == 1);
    assert(cache.validate());
}

void testGetIncreasesFrequencyAndReturnsValue()
{
    LFUCache cache(2);
    cache.put("A", "Apple");

    string value;
    assert(cache.get("A", value));
    assert(value == "Apple");
    assert(cache.frequencyOf("A") == 2);
    assert(cache.minFrequency() == 2);

    assert(cache.get("A", value));
    assert(cache.frequencyOf("A") == 3);
    assert(cache.minFrequency() == 3);
    assert(cache.validate());
}

void testFrequencyBucketCreationAndRemoval()
{
    LFUCache cache(3);
    cache.put("A", "Apple");
    cache.put("B", "Banana");

    string value;
    assert(cache.get("A", value));
    assert(cache.minFrequency() == 1);
    assert(cache.frequencyOf("A") == 2);

    assert(cache.get("B", value));
    assert(cache.frequencyOf("B") == 2);
    assert(cache.minFrequency() == 2);
    assert(cache.validate());

    assert(cache.get("A", value));
    assert(cache.frequencyOf("A") == 3);
    assert(cache.minFrequency() == 2);

    assert(cache.get("B", value)); 
    assert(cache.frequencyOf("B") == 3);
    assert(cache.minFrequency() == 3);
    assert(cache.validate());
}

void testEvictsLowestFrequency()
{
    LFUCache cache(3);
    cache.put("A", "Apple");
    cache.put("B", "Banana");
    cache.put("C", "Cherry");

    string value;
    assert(cache.get("A", value));
    assert(cache.get("A", value)); 
    assert(cache.get("B", value)); 

    cache.put("D", "Durian");

    assert(!cache.contains("C")); 
    assert(cache.contains("A"));
    assert(cache.contains("B"));
    assert(cache.contains("D"));
    assert(cache.size() == 3);
    assert(cache.frequencyOf("D") == 1);
    assert(cache.minFrequency() == 1);
    assert(cache.validate());
}

void testLRUTieBreakingWithinSameFrequency()
{
    LFUCache cache(3);
    cache.put("A", "Apple");
    cache.put("B", "Banana");
    cache.put("C", "Cherry");

    string value;

    assert(cache.get("A", value));
    assert(cache.get("B", value));
    assert(cache.get("C", value));
    assert(cache.frequencyOf("A") == 2);
    assert(cache.frequencyOf("B") == 2);
    assert(cache.frequencyOf("C") == 2);

    cache.put("D", "Durian");

    assert(!cache.contains("A")); 
    assert(cache.contains("B"));
    assert(cache.contains("C"));
    assert(cache.contains("D"));
    assert(cache.validate());
}

void testUpdateExistingKeyCountsAsAccess()
{
    LFUCache cache(2);
    cache.put("A", "Apple");
    cache.put("B", "Banana");

    cache.put("A", "Apricot");

    string value;
    assert(cache.get("A", value));
    assert(value == "Apricot");
    assert(cache.frequencyOf("A") == 3);
    assert(cache.frequencyOf("B") == 1);
    assert(cache.size() == 2);
    assert(cache.validate());
}

void testExplicitRemovalAndMinimumUpdate()
{
    LFUCache cache(3);
    cache.put("A", "Apple");
    cache.put("B", "Banana");
    cache.put("C", "Cherry");

    string value;
    assert(cache.get("A", value));
    assert(cache.remove("B"));      
    assert(!cache.remove("B"));
    assert(cache.minFrequency() == 1);
    assert(cache.remove("C"));
    assert(cache.minFrequency() == 2);
    assert(cache.remove("A"));
    assert(cache.minFrequency() == 0);
    assert(cache.size() == 0);
    assert(cache.validate());
}

void testRepeatedEvictionsAndCapacityOne()
{
    LFUCache cache(1);
    cache.put("A", "Apple");

    string value;
    assert(cache.get("A", value));
    assert(cache.get("A", value));
    assert(cache.frequencyOf("A") == 3);

    cache.put("B", "Banana");
    assert(!cache.contains("A"));
    assert(cache.contains("B"));
    assert(cache.frequencyOf("B") == 1);

    cache.put("C", "Cherry");
    assert(!cache.contains("B"));
    assert(cache.contains("C"));
    assert(cache.size() == 1);
    assert(cache.validate());
}

void testZeroCapacity()
{
    LFUCache cache(0);
    cache.put("A", "Apple");

    string value;
    assert(cache.size() == 0);
    assert(cache.capacity() == 0);
    assert(cache.minFrequency() == 0);
    assert(!cache.get("A", value));
    assert(!cache.contains("A"));
    assert(cache.validate());
}

void testClearAndReuse()
{
    LFUCache cache(3);
    cache.put("A", "Apple");
    cache.put("B", "Banana");

    string value;
    assert(cache.get("A", value));
    cache.clear();

    assert(cache.size() == 0);
    assert(cache.minFrequency() == 0);
    assert(cache.validate());

    cache.put("C", "Cherry");
    assert(cache.get("C", value));
    assert(value == "Cherry");
    assert(cache.frequencyOf("C") == 2);
    assert(cache.validate());
}


void testHashIndexGrowthKeepsEntriesReachable()
{
    LFUCache cache(64);

    for (int i = 0; i < 40; ++i)
    {
        const string key = "key-" + to_string(i);
        const string value = "value-" + to_string(i);
        cache.put(key, value);
    }

    assert(cache.size() == 40);
    assert(cache.validate());

    string value;
    for (int i = 0; i < 40; i += 3)
    {
        const string key = "key-" + to_string(i);
        assert(cache.get(key, value));
        assert(value == "value-" + to_string(i));
    }

    assert(cache.validate());
}

void testEmptyKeyAndEmptyValue()
{
    LFUCache cache(2);
    cache.put("", "");

    string value = "not empty";
    assert(cache.get("", value));
    assert(value.empty());
    assert(cache.frequencyOf("") == 2);
    assert(cache.validate());
}

int main()
{
    testEmptyCacheAndMissingKey();
    testNewEntriesStartAtFrequencyOne();
    testGetIncreasesFrequencyAndReturnsValue();
    testFrequencyBucketCreationAndRemoval();
    testEvictsLowestFrequency();
    testLRUTieBreakingWithinSameFrequency();
    testUpdateExistingKeyCountsAsAccess();
    testExplicitRemovalAndMinimumUpdate();
    testRepeatedEvictionsAndCapacityOne();
    testZeroCapacity();
    testClearAndReuse();
    testHashIndexGrowthKeepsEntriesReachable();
    testEmptyKeyAndEmptyValue();

    cout << "All LFU tests passed!" << endl;
    return 0;
}
