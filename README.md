# LRU/LFU Cache with Persistent Backing Store

**Course:** CSCE 2200  
**Milestone:** 2 — Integrated cache, expiration management, and testing  
**Language:** C++11

## 1. Project overview

This project implements a bounded in-memory cache with selectable Least Recently Used (LRU) and Least Frequently Used (LFU) eviction policies, a persistent append-only backing store, and an application-level `CacheManager` that coordinates reads, writes, deletion, eviction, and TTL expiration.

The cache policy is replaceable at runtime. Values are written to the backing store before being admitted to the in-memory policy. A cache miss can therefore be served from persistent storage, while expired records are removed rather than returned.

## 2. Repository structure

```text
src/
  app/          CLI, clock abstraction, and application entry point
  cache/        LRU/LFU policies, CacheManager, ExpirationManager
  storage/      Append-only backing store and record codec
  structures/   Hash table, doubly linked list, indexed expiry heap
tests/          Component, policy-parity, heap, storage, and integration tests
docs/
  DESIGN.md     Milestone 2 architecture, algorithms, and trade-offs
benchmarks/     Reserved for performance experiments
data/           Reserved for application data and results
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

## 4. Build, test, and run

Run these commands from the repository root:

```bash
make          # compile all test executables
make test     # compile and run the full test suite
make run      # build and start the CLI application
make asan     # compile and run tests with AddressSanitizer/UBSan
make clean    # remove generated binaries
```

The project uses `g++` and C++11 by default. Override `CXX` or `FLAGS` on the make command line if your environment requires a different compiler or flags.

The integration tests use a fake clock so TTL behavior is deterministic and does not depend on waiting in real time. The heap tests call `validate()` after each operation to check both heap order and the key-to-index mapping.

## 5. Milestone 2 scope and verification

The repository includes component tests, an indexed expiry-heap test, policy-parity coverage, and cache/backing-store integration tests. The async worker and concurrent/thread-safe operation are deferred and are not claimed as implemented or verified. Git commit-history quality and TA repository access are external checks and are not established by running the local test suite.

See [`docs/DESIGN.md`](docs/DESIGN.md) for the architecture, algorithms, and design trade-offs.
