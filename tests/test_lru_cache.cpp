
#include "../src/cache/lru_cache.h"

#include <cassert>
#include <iostream>
#include <string>
using namespace std;

void testEmptyCache()
{
    LRUCache cache(3);

    assert(cache.size() == 0);
    assert(cache.capacity() == 3);
    assert(!cache.contains("A"));
    assert(cache.validate());
}

void testBasicInsertionAndRetrieval()
{
    LRUCache cache(3);

    cache.put("A", "Apple");
    cache.put("B", "Banana");

    string value;

    assert(cache.get("A", value));
    assert(value == "Apple");

    assert(cache.get("B", value));
    assert(value == "Banana");

    assert(cache.size() == 2);
    assert(cache.validate());
}

void testUpdateExistingKey()
{
    LRUCache cache(3);

    cache.put("A", "Apple");
    cache.put("B", "Banana");

    cache.put("A", "Apricot");

    string value;

    assert(cache.get("A", value));
    assert(value == "Apricot");

    assert(cache.size() == 2);
    assert(cache.validate());
}

void testLRUEviction()
{
    LRUCache cache(2);

    cache.put("A", "Apple");
    cache.put("B", "Banana");

    cache.put("C", "Cherry");

    assert(!cache.contains("A"));
    assert(cache.contains("B"));
    assert(cache.contains("C"));

    assert(cache.size() == 2);
    assert(cache.validate());
}

void testAccessChangesRecency()
{
    LRUCache cache(2);

    cache.put("A", "Apple");
    cache.put("B", "Banana");

    string value;

    assert(cache.get("A", value));

    cache.put("C", "Cherry");

    assert(cache.contains("A"));
    assert(!cache.contains("B"));
    assert(cache.contains("C"));

    assert(cache.validate());
}

void testCapacityOne()
{
    LRUCache cache(1);

    cache.put("A", "Apple");

    assert(cache.size() == 1);
    assert(cache.contains("A"));

    cache.put("B", "Banana");

    assert(!cache.contains("A"));
    assert(cache.contains("B"));

    std::string value;

    assert(cache.get("B", value));
    assert(value == "Banana");

    assert(cache.validate());
}

void testZeroCapacity()
{
    LRUCache cache(0);

    cache.put("A", "Apple");

    std::string value;

    assert(cache.size() == 0);
    assert(!cache.contains("A"));
    assert(!cache.get("A", value));

    assert(cache.validate());
}

void testCacheMiss()
{
    LRUCache cache(2);

    cache.put("A", "Apple");
    cache.put("B", "Banana");

    std::string value;

    assert(!cache.get("X", value));

    assert(cache.size() == 2);
    assert(cache.contains("A"));
    assert(cache.contains("B"));

    assert(cache.validate());
}

void testRepeatedInsertion()
{
    LRUCache cache(3);

    cache.put("A", "Apple");
    cache.put("A", "Apricot");
    cache.put("A", "Avocado");

    std::string value;

    assert(cache.size() == 1);
    assert(cache.get("A", value));
    assert(value == "Avocado");

    assert(cache.validate());
}

void testEmptyValue()
{
    LRUCache cache(2);

    cache.put("A", "");

    std::string value = "initial";

    bool found = cache.get("A", value);

    assert(found);
    assert(value.empty());

    assert(cache.validate());
}

int main()
{
    testEmptyCache();
    testBasicInsertionAndRetrieval();
    testUpdateExistingKey();
    testLRUEviction();
    testAccessChangesRecency();
    testCapacityOne();
    testZeroCapacity();
    testCacheMiss();
    testRepeatedInsertion();
    testEmptyValue();

    std::cout << "All LRU tests passed!\n";

    return 0;
}
