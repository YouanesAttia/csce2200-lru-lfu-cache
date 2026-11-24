#include "../src/cache/icache.hpp"
#include "../src/cache/lfu_cache.hpp"
#include "../src/cache/lru_cache.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

// A small shared command format lets the exact same operations run through
// the ICache interface, regardless of which eviction policy is underneath.

struct Operation
{
    enum Type
    {
        Put,
        Get,
        Remove
    };

    Type type;
    string key;
    string value;

    Operation(Type operationType, const string &operationKey,
              const string &operationValue = "")
        : type(operationType), key(operationKey), value(operationValue) {}
};

static vector<string> runScript(ICache *cache, const vector<Operation> &script)
{
    assert(cache != nullptr);
    vector<string> results;

    for (size_t i = 0; i < script.size(); ++i)
    {
        const Operation &operation = script[i];

        if (operation.type == Operation::Put)
        {
            string evictedKey;
            cache->put(operation.key, operation.value, &evictedKey);
            results.push_back("PUT:" + evictedKey);
        }
        else if (operation.type == Operation::Get)
        {
            string value;
            if (cache->get(operation.key, value))
                results.push_back("HIT:" + value);
            else
                results.push_back("MISS");
        }
        else if (operation.type == Operation::Remove)
        {
            results.push_back(cache->remove(operation.key) ? "REMOVED" : "ABSENT");
        }
    }

    return results;
}

static void testSameBasicScriptThroughICache()
{
    const vector<Operation> script = {
        Operation(Operation::Put, "A", "Apple"),
        Operation(Operation::Put, "B", "Banana"),
        Operation(Operation::Get, "A"),
        Operation(Operation::Put, "B", "Blueberry"), // Update existing key.
        Operation(Operation::Get, "B"),
        Operation(Operation::Get, "missing"),
        Operation(Operation::Remove, "A"),
        Operation(Operation::Get, "A"),
        Operation(Operation::Put, "C", "Cherry"),
        Operation(Operation::Get, "C"),
        Operation(Operation::Remove, "missing")
    };

    // These are ordinary local objects. Their destructors run automatically
    // when the test function returns.
    LRUCache lru(5);
    LFUCache lfu(5);
    ICache *caches[] = {&lru, &lfu};

    const vector<string> lruResults = runScript(caches[0], script);
    const vector<string> lfuResults = runScript(caches[1], script);

    assert(lruResults == lfuResults);
    assert(caches[0]->size() == caches[1]->size());

    const vector<string> expected = {
        "PUT:", "PUT:", "HIT:Apple", "PUT:", "HIT:Blueberry",
        "MISS", "REMOVED", "MISS", "PUT:", "HIT:Cherry", "ABSENT"
    };
    assert(lruResults == expected);
}

static void testZeroCapacityThroughICache()
{
    LRUCache lru(0);
    LFUCache lfu(0);
    ICache *caches[] = {&lru, &lfu};

    for (size_t i = 0; i < 2; ++i)
    {
        string evictedKey = "stale value";
        caches[i]->put("A", "Apple", &evictedKey);
        assert(evictedKey.empty());
        assert(caches[i]->size() == 0);
        assert(caches[i]->capacity() == 0);
        assert(!caches[i]->contains("A"));

        string value = "unchanged";
        assert(!caches[i]->get("A", value));
        assert(value == "unchanged");
        assert(!caches[i]->remove("A"));
    }
}

static void testUpdateExistingKeyThroughICache()
{
    LRUCache lru(2);
    LFUCache lfu(2);
    ICache *caches[] = {&lru, &lfu};

    for (size_t i = 0; i < 2; ++i)
    {
        caches[i]->put("A", "old value");
        caches[i]->put("A", "new value");

        string value;
        assert(caches[i]->size() == 1);
        assert(caches[i]->get("A", value));
        assert(value == "new value");
    }
}

static void testRemoveThenReinsertThroughICache()
{
    LRUCache lru(2);
    LFUCache lfu(2);
    ICache *caches[] = {&lru, &lfu};

    for (size_t i = 0; i < 2; ++i)
    {
        caches[i]->put("A", "first value");
        assert(caches[i]->remove("A"));
        assert(!caches[i]->contains("A"));
        assert(!caches[i]->remove("A"));

        caches[i]->put("A", "second value");
        string value;
        assert(caches[i]->get("A", value));
        assert(value == "second value");
        assert(caches[i]->size() == 1);
    }
}

static void testPoliciesChooseDifferentVictims()
{
    LRUCache lruCache(3);
    LFUCache lfuCache(3);
    ICache *caches[] = {&lruCache, &lfuCache};
    ICache *lru = caches[0];
    ICache *lfu = caches[1];

    // Start with two entries. A is accessed three times and B once.
    // C is then inserted as a fresh entry.
    lru->put("A", "Apple");
    lfu->put("A", "Apple");
    lru->put("B", "Banana");
    lfu->put("B", "Banana");

    string value;
    for (int i = 0; i < 3; ++i)
    {
        assert(lru->get("A", value));
        assert(lfu->get("A", value));
    }
    assert(lru->get("B", value));
    assert(lfu->get("B", value));

    lru->put("C", "Cherry");
    lfu->put("C", "Cherry");

    // LRU evicts A, whose last access is oldest. LFU evicts C, whose
    // frequency is only 1 (A has 4 and B has 2).
    string lruVictim;
    string lfuVictim;
    lru->put("D", "Durian", &lruVictim);
    lfu->put("D", "Durian", &lfuVictim);

    assert(lruVictim == "A");
    assert(lfuVictim == "C");
    assert(!lru->contains("A") && lru->contains("C"));
    assert(lfu->contains("A") && !lfu->contains("C"));
}

int main()
{
    testSameBasicScriptThroughICache();
    testZeroCapacityThroughICache();
    testUpdateExistingKeyThroughICache();
    testRemoveThenReinsertThroughICache();
    testPoliciesChooseDifferentVictims();

    cout << "All cache policy parity tests passed!" << endl;
    return 0;
}
