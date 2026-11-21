#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>

#include "../src/cache/cache_manager.hpp"
#include "../src/cache/lru_cache.h"
#include "../src/storage/backing_store.h"

using namespace std;

class FakeClock : public Clock
{
    uint64_t now_;

public:
    FakeClock(uint64_t start = 1000) : now_(start) {}
    uint64_t now() const override { return now_; }
    void set(uint64_t t) { now_ = t; }
    void advance(uint64_t delta) { now_ += delta; }
};

static const char *TEST_DB = "test_integration.db";

static void cleanupFile()
{
    remove(TEST_DB);
}

static void testSetAndGetHit()
{
    cleanupFile();
    storage::BackingStore store;
    assert(store.open(TEST_DB) == storage::Status::Ok);
    ExpirationManager expiry;
    FakeClock clock(100);
    CacheManager mgr(new LRUCache(2), store, expiry, clock);

    assert(mgr.set("k1", "v1", 50) == storage::Status::Ok);
    assert(expiry.contains("k1"));
    assert(expiry.expiryOf("k1") == 150);

    string out;
    assert(mgr.get("k1", out) == storage::Status::Ok);
    assert(out == "v1");
    assert(mgr.stats().hits == 1);
    assert(mgr.stats().misses == 0);

    assert(mgr.set("k1", "v1_no_ttl", 0) == storage::Status::Ok);
    assert(!expiry.contains("k1"));
    assert(mgr.get("k1", out) == storage::Status::Ok);
    assert(out == "v1_no_ttl");

    store.close();
    cleanupFile();
    cout << "[PASS] testSetAndGetHit" << endl;
}

static void testGetHitExpired()
{
    cleanupFile();
    storage::BackingStore store;
    assert(store.open(TEST_DB) == storage::Status::Ok);
    ExpirationManager expiry;
    FakeClock clock(100);
    CacheManager mgr(new LRUCache(2), store, expiry, clock);

    assert(mgr.set("k1", "v1", 20) == storage::Status::Ok);
    clock.advance(20);

    string out;
    assert(mgr.get("k1", out) == storage::Status::NotFound);
    assert(!mgr.policy()->contains("k1"));
    assert(!expiry.contains("k1"));
    assert(!store.contains("k1"));
    assert(mgr.stats().expirations == 1);
    assert(mgr.stats().misses == 1);

    store.close();
    cleanupFile();
    cout << "[PASS] testGetHitExpired" << endl;
}

static void testGetMissStoreOkAndEviction()
{
    cleanupFile();
    storage::BackingStore store;
    assert(store.open(TEST_DB) == storage::Status::Ok);
    ExpirationManager expiry;
    FakeClock clock(100);
    CacheManager mgr(new LRUCache(2), store, expiry, clock);

    assert(mgr.set("k1", "v1", 50) == storage::Status::Ok);
    assert(mgr.set("k2", "v2", 60) == storage::Status::Ok);

    assert(mgr.set("k3", "v3", 70) == storage::Status::Ok);
    assert(!mgr.policy()->contains("k1"));
    assert(!expiry.contains("k1"));
    assert(store.contains("k1"));
    assert(mgr.stats().evictions == 1);

    string out;
    assert(mgr.get("k1", out) == storage::Status::Ok);
    assert(out == "v1");
    assert(mgr.policy()->contains("k1"));
    assert(expiry.contains("k1"));
    assert(expiry.expiryOf("k1") == 150);
    assert(!mgr.policy()->contains("k2"));
    assert(!expiry.contains("k2"));
    assert(mgr.stats().misses == 1);
    assert(mgr.stats().evictions == 2);

    store.close();
    cleanupFile();
    cout << "[PASS] testGetMissStoreOkAndEviction" << endl;
}

static void testGetMissStoreExpired()
{
    cleanupFile();
    storage::BackingStore store;
    assert(store.open(TEST_DB) == storage::Status::Ok);
    ExpirationManager expiry;
    FakeClock clock(100);
    CacheManager mgr(new LRUCache(1), store, expiry, clock);

    assert(mgr.set("k1", "v1", 10) == storage::Status::Ok);
    assert(mgr.set("k2", "v2", 100) == storage::Status::Ok);
    assert(!mgr.policy()->contains("k1"));
    assert(store.contains("k1"));

    clock.advance(15);

    string out;
    assert(mgr.get("k1", out) == storage::Status::NotFound);
    assert(!mgr.policy()->contains("k1"));
    assert(!store.contains("k1"));
    assert(mgr.stats().expirations == 1);
    assert(mgr.stats().misses == 1);

    store.close();
    cleanupFile();
    cout << "[PASS] testGetMissStoreExpired" << endl;
}

static void testGetMissTrueNotFound()
{
    cleanupFile();
    storage::BackingStore store;
    assert(store.open(TEST_DB) == storage::Status::Ok);
    ExpirationManager expiry;
    FakeClock clock(100);
    CacheManager mgr(new LRUCache(2), store, expiry, clock);

    string out;
    assert(mgr.get("missing", out) == storage::Status::NotFound);
    assert(mgr.stats().misses == 1);
    assert(mgr.stats().hits == 0);

    store.close();
    cleanupFile();
    cout << "[PASS] testGetMissTrueNotFound" << endl;
}

static void testStoreFailureOnGetAndSet()
{
    cleanupFile();
    storage::BackingStore store;
    ExpirationManager expiry;
    FakeClock clock(100);
    CacheManager mgr(new LRUCache(2), store, expiry, clock);

    assert(mgr.set("k1", "v1", 50) == storage::Status::ErrNotOpen);
    assert(mgr.policy()->size() == 0);
    assert(expiry.size() == 0);

    string out;
    assert(mgr.get("k1", out) == storage::Status::ErrNotOpen);
    assert(mgr.policy()->size() == 0);
    assert(mgr.stats().misses == 0);
    assert(mgr.stats().hits == 0);

    cleanupFile();
    cout << "[PASS] testStoreFailureOnGetAndSet" << endl;
}

static void testDel()
{
    cleanupFile();
    storage::BackingStore store;
    assert(store.open(TEST_DB) == storage::Status::Ok);
    ExpirationManager expiry;
    FakeClock clock(100);
    CacheManager mgr(new LRUCache(2), store, expiry, clock);

    assert(mgr.set("k1", "v1", 50) == storage::Status::Ok);
    assert(mgr.del("k1") == storage::Status::Ok);

    assert(!mgr.policy()->contains("k1"));
    assert(!expiry.contains("k1"));
    assert(!store.contains("k1"));

    string out;
    assert(mgr.get("k1", out) == storage::Status::NotFound);

    store.close();
    cleanupFile();
    cout << "[PASS] testDel" << endl;
}

static void testSweep()
{
    cleanupFile();
    storage::BackingStore store;
    assert(store.open(TEST_DB) == storage::Status::Ok);
    ExpirationManager expiry;
    FakeClock clock(100);
    CacheManager mgr(new LRUCache(3), store, expiry, clock);

    assert(mgr.set("k1", "v1", 10) == storage::Status::Ok);
    assert(mgr.set("k2", "v2", 20) == storage::Status::Ok);
    assert(mgr.set("k3", "v3", 50) == storage::Status::Ok);

    clock.advance(25);
    size_t swept = mgr.sweep();
    assert(swept == 2);
    assert(mgr.stats().expirations == 2);

    assert(!mgr.policy()->contains("k1"));
    assert(!store.contains("k1"));
    assert(!mgr.policy()->contains("k2"));
    assert(!store.contains("k2"));

    string out;
    assert(mgr.get("k3", out) == storage::Status::Ok);
    assert(out == "v3");

    store.close();
    cleanupFile();
    cout << "[PASS] testSweep" << endl;
}

static void testSwitchPolicy()
{
    cleanupFile();
    storage::BackingStore store;
    assert(store.open(TEST_DB) == storage::Status::Ok);
    ExpirationManager expiry;
    FakeClock clock(100);
    CacheManager mgr(new LRUCache(2), store, expiry, clock);

    assert(mgr.set("k1", "v1", 50) == storage::Status::Ok);
    assert(mgr.set("k2", "v2", 0) == storage::Status::Ok);
    assert(expiry.size() == 1);

    mgr.switchPolicy(new LRUCache(2));
    assert(expiry.size() == 0);
    assert(mgr.policy()->size() == 0);
    assert(store.contains("k1"));
    assert(store.contains("k2"));

    string out;
    assert(mgr.get("k1", out) == storage::Status::Ok);
    assert(out == "v1");
    assert(mgr.policy()->contains("k1"));
    assert(expiry.contains("k1"));
    assert(expiry.expiryOf("k1") == 150);

    assert(mgr.get("k2", out) == storage::Status::Ok);
    assert(out == "v2");
    assert(mgr.policy()->contains("k2"));
    assert(!expiry.contains("k2"));

    store.close();
    cleanupFile();
    cout << "[PASS] testSwitchPolicy" << endl;
}

int main()
{
    testSetAndGetHit();
    testGetHitExpired();
    testGetMissStoreOkAndEviction();
    testGetMissStoreExpired();
    testGetMissTrueNotFound();
    testStoreFailureOnGetAndSet();
    testDel();
    testSweep();
    testSwitchPolicy();

    cout << "All integration tests passed!" << endl;
    return 0;
}