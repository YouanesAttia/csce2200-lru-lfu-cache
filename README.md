# LRU/LFU Cache with Persistent Backing Store

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

## 3. Main components

- **HashTable:** Separate chaining with dynamic growth; expected O(1) lookup, insertion, and removal under a suitable hash distribution.
- **DoublyLinkedList:** Supports O(1) movement and removal when a node is already known.
- **LRUCache:** Combines a hash table with a recency list. The front is most recently used; the tail is the eviction candidate.
- **LFUCache:** Tracks frequency buckets and recency within each frequency. It evicts the least-recently-used entry among entries with the minimum frequency.
- **BackingStore:** Stores records in an append-only file and rebuilds an in-memory index when opened.
- **ExpiryHeap:** An indexed min-heap ordered by absolute expiration time. A key-to-array-index hash table allows a key to be removed or rescheduled in O(log n).
- **ExpirationManager:** Wraps the expiry heap, exposes the next deadline, cancels/reschedules keys, and sweeps all entries whose deadlines are at or before the supplied time.
- **CacheManager:** Coordinates the selected cache policy, backing store, expiration manager, and injected clock.

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
