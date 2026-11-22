#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../src/cache/expiration_manager.hpp"

using namespace std;

static void check(const ExpiryHeap &heap, size_t expectedSize)
{
    assert(heap.size() == expectedSize);
    assert(heap.validate());
}

static void testEmptyHeap()
{
    ExpiryHeap heap;
    check(heap, 0);
    assert(!heap.contains("missing"));

    bool threw = false;
    try
    {
        heap.peek();
    }
    catch (const runtime_error &)
    {
        threw = true;
    }
    assert(threw);

    threw = false;
    try
    {
        heap.popMin();
    }
    catch (const runtime_error &)
    {
        threw = true;
    }
    assert(threw);
}

static void testInsertUpdateAndPopOrder()
{
    ExpiryHeap heap(2);
    assert(heap.upsert("later", 300));
    check(heap, 1);
    assert(heap.upsert("first", 100));
    check(heap, 2);
    assert(heap.upsert("middle", 200));
    check(heap, 3); // also exercises capacity growth

    assert(heap.peek().key == "first");
    assert(heap.peek().expiry == 100);

    // Updating an existing key must not change the number of entries.
    assert(!heap.upsert("later", 50));
    check(heap, 3);
    assert(heap.peek().key == "later");

    ExpiryHeap::Entry e = heap.popMin();
    assert(e.key == "later" && e.expiry == 50);
    check(heap, 2);
    e = heap.popMin();
    assert(e.key == "first" && e.expiry == 100);
    check(heap, 1);
    e = heap.popMin();
    assert(e.key == "middle" && e.expiry == 200);
    check(heap, 0);
}

static void testRemoveRoot()
{
    ExpiryHeap heap;
    heap.upsert("root", 10);
    check(heap, 1);
    heap.upsert("a", 20);
    check(heap, 2);
    heap.upsert("b", 30);
    check(heap, 3);

    assert(heap.remove("root"));
    check(heap, 2);
    assert(!heap.contains("root"));
    assert(heap.peek().expiry == 20);
}

static void testRemoveLastElement()
{
    ExpiryHeap heap;
    heap.upsert("a", 10);
    check(heap, 1);
    heap.upsert("b", 20);
    check(heap, 2);
    heap.upsert("c", 30);
    check(heap, 3);

    // In this insertion order, c is the last array element.
    assert(heap.remove("c"));
    check(heap, 2);
    assert(!heap.contains("c"));
    assert(heap.peek().key == "a");
}

static void testRemoveMiddleElement()
{
    ExpiryHeap heap;
    heap.upsert("a", 10);
    check(heap, 1);
    heap.upsert("b", 20);
    check(heap, 2);
    heap.upsert("c", 30);
    check(heap, 3);
    heap.upsert("d", 40);
    check(heap, 4);
    heap.upsert("e", 50);
    check(heap, 5);

    // d occupies an interior heap-array position for this insertion order.
    assert(heap.remove("d"));
    check(heap, 4);
    assert(!heap.contains("d"));
    assert(heap.expiryOf("e") == 50);
    assert(heap.peek().expiry == 10);
    assert(!heap.remove("not-present"));
    check(heap, 4);
}

static void testDuplicateExpiries()
{
    ExpiryHeap heap;
    assert(heap.upsert("one", 100));
    check(heap, 1);
    assert(heap.upsert("two", 100));
    check(heap, 2);
    assert(heap.upsert("three", 100));
    check(heap, 3);
    assert(heap.upsert("earlier", 50));
    check(heap, 4);

    assert(heap.peek().expiry == 50);
    assert(heap.popMin().key == "earlier");
    check(heap, 3);

    // Equal expiry values are all retained; their relative order is unspecified.
    assert(heap.size() == 3);
    for (int i = 0; i < 3; ++i)
    {
        assert(heap.peek().expiry == 100);
        heap.popMin();
        check(heap, static_cast<size_t>(2 - i));
    }
    assert(heap.empty());
}

class RecordingListener : public ExpiryListener
{
public:
    vector<string> expired;
    void onExpired(const string &key) override { expired.push_back(key); }
};

static void testExpirationManager()
{
    ExpirationManager manager;
    RecordingListener listener;

    manager.setExpiry("a", 100);
    assert(manager.validate());
    manager.setExpiry("b", 100);
    assert(manager.validate());
    manager.setExpiry("c", 200);
    assert(manager.validate());
    assert(manager.size() == 3);

    uint64_t next = 0;
    assert(manager.nextExpiry(next) && next == 100);
    assert(manager.sweep(99, &listener) == 0);
    assert(manager.validate());
    assert(listener.expired.empty());

    assert(manager.sweep(100, &listener) == 2);
    assert(manager.validate());
    assert(manager.size() == 1);
    assert(listener.expired.size() == 2);
    assert(manager.contains("c"));

    manager.cancel("c");
    assert(manager.validate());
    assert(manager.size() == 0);
    assert(!manager.nextExpiry(next));

    manager.setExpiry("reuse", 300);
    assert(manager.validate());
    manager.clear();
    assert(manager.validate());
    assert(manager.size() == 0);
}

int main()
{
    testEmptyHeap();
    testInsertUpdateAndPopOrder();
    testRemoveRoot();
    testRemoveLastElement();
    testRemoveMiddleElement();
    testDuplicateExpiries();
    testExpirationManager();

    cout << "All ExpiryHeap and ExpirationManager tests passed!" << endl;
    return 0;
}
