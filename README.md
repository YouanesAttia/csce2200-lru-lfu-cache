# LRU/LFU Cache with Persistent Backing Store

**Course:** CSCE 2200  
**Milestone:** 1 — Implementation and Initial Design  
**Language:** C++

## 1. Project overview

This project implements two bounded in-memory cache policies—Least Recently Used (LRU) and Least Frequently Used (LFU)—alongside custom supporting data structures and an append-only persistent backing store.

A cache keeps a limited set of records in memory so repeated requests can avoid repeatedly reading the slower backing store. Because the cache has a fixed capacity, it needs an eviction policy to decide which entry to remove when it is full.

The project currently contains the individual cache and storage components. A complete application-level cache manager that routes requests between a selected cache policy and the backing store is still planned work; `src/app/main.cpp` and `tests/test_integration.cpp` are placeholders.

## 2. Repository structure

```text
src/
  app/          Application entry point (currently a placeholder)
  cache/        LRU and LFU cache implementations
  storage/      Append-only persistent backing store
  structures/   HashTable, DoublyLinkedList, MinHeap

tests/          Component-level tests and backing-store test data
benchmarks/     Reserved for future performance experiments
data/           Reserved for application data and results
docs/           Milestone plan, architecture, test plan, and discussion guide
```

## 3. Implemented components

### HashTable

The custom HashTable uses separate chaining to resolve collisions. It supports insertion, lookup, update, removal, clearing, and dynamic growth. Its load factor is the number of stored elements divided by the number of buckets; the implementation grows the table when the load factor exceeds 0.75.

Expected lookup/insert/remove complexity is O(1), assuming a suitable hash distribution and bounded load factor. Worst-case complexity is O(n) if many keys collide into one chain.

### Doubly Linked List

The custom DoublyLinkedList maintains `prev` and `next` links and supports insertion, detachment, movement to the front, and erasing a known node. Operations on a known node are O(1). The node's owner information helps reject operations involving a node that does not belong to the list.

### LRU Cache

The LRU cache combines:

- `HashTable<DoublyLinkedList::Node*>` to find a key's node.
- A DoublyLinkedList to maintain recency order.

The head is most recently used (MRU), and the tail is least recently used (LRU). A successful `get` moves the node to the head. When a new key is inserted into a full cache, the tail is detached and removed from the HashTable.

Expected `get` and `put` complexity is O(1), subject to expected O(1) HashTable operations.

### LFU Cache

The LFU cache combines:

- A HashTable mapping each key to its node and frequency bucket.
- A doubly linked list of frequency buckets in ascending frequency order.
- A DoublyLinkedList inside each bucket to maintain recency among entries with equal frequency.

On access, an entry is promoted to the next frequency bucket and moved to that bucket's front. The tail of the first (minimum-frequency) bucket is the eviction candidate. `minFrequency_` mirrors the first bucket's frequency, or zero when the cache is empty.

Expected `get` and `put` complexity is O(1), subject to expected O(1) HashTable operations and the maintained adjacent-bucket structure.

### Persistent Backing Store

The storage component uses an append-only log file and an in-memory `HashIndex`. The index maps each key to the file offset and value length of its latest record. Updates append a newer record rather than rewriting an earlier record. When the file is opened, the implementation scans the log and rebuilds the index. The status API distinguishes a missing key (`NotFound`) from storage errors.

The in-memory index operations are expected O(1); actual disk operations additionally depend on file I/O, seeking, and record size. Opening/recovery scans the log and is proportional to the amount of data scanned.

### MinHeap

A generic MinHeap is implemented and has its own tests. It is currently a standalone data structure and is not called by the LRU cache, LFU cache, or backing-store implementation. Do not describe it as an integrated part of the cache architecture unless the team later gives it a concrete role.

## 4. Current architecture and integration boundary

The repository currently has separate components:

```text
                 Component-level interfaces

       +----------------+       +----------------+
       |    LRUCache    |       |    LFUCache    |
       | HashTable +    |       | HashTable +    |
       | DoublyLinkedList|      | Frequency      |
       +----------------+       | Buckets + DLLs |
                                +----------------+

       +----------------+       +----------------+
       |  BackingStore  |       |   MinHeap      |
       | Append-only    |       | Standalone     |
       | log + HashIndex|       | implementation |
       +----------------+       +----------------+

       Application-level request coordinator: planned
```

The intended application-level flow is documented in `docs/PROJECT_PLAN.md`. It is not yet implemented in `main.cpp` or covered by an integration test.

## 5. Build and run component tests

The repository currently has no functioning Makefile. The following commands compile the existing component tests with a C++11 compiler. Run them from the repository root so the backing-store sample-data path resolves correctly.

```bash
mkdir -p build
CXXFLAGS="-std=c++11 -Wall -Wextra -Wpedantic -g -Isrc/structures"

g++ $CXXFLAGS tests/test_hash_table.cpp -o build/test_hash_table
g++ $CXXFLAGS tests/test_doubly_linked_list.cpp -o build/test_doubly_linked_list
g++ $CXXFLAGS tests/test_min_heap.cpp -o build/test_min_heap
g++ $CXXFLAGS tests/test_lru_cache.cpp src/cache/lru_cache.cpp -o build/test_lru_cache
g++ $CXXFLAGS tests/test_lfu_cache.cpp -o build/test_lfu_cache
g++ $CXXFLAGS tests/test_backing_store.cpp src/storage/backing_store.cpp -o build/test_backing_store

./build/test_hash_table
./build/test_doubly_linked_list
./build/test_min_heap
./build/test_lru_cache
./build/test_lfu_cache
./build/test_backing_store
```

The backing-store test expects to be run with the repository root as the working directory because it opens `tests/sample_data.txt`.

For sanitizer builds, add `-fsanitize=address,undefined -fno-omit-frame-pointer` to `CXXFLAGS` and rerun each test. Sanitizer availability depends on the local compiler/runtime.

## 6. Milestone 1 status

**Implemented component code:** HashTable, DoublyLinkedList, LRUCache, LFUCache, MinHeap, and BackingStore.  
**Component tests:** present for those components.  
**Application entry point:** placeholder.  
**End-to-end integration test:** placeholder.  
**Build automation:** Makefile is empty.
