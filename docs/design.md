# Milestone 2 Design Document

## 1. Purpose and scope

The system provides a bounded in-memory cache with LRU and LFU replacement policies, a persistent backing store, and time-to-live (TTL) expiration. `CacheManager` is the application-level coordinator. The design in this document describes the synchronous implementation in the repository.

An asynchronous expiration worker and concurrent/thread-safe access are outside the verified scope of this milestone. The current expiration path is invoked synchronously through `CacheManager::sweep()` or expiration checks during reads.

## 2. Architecture

```text
                  Application / CLI
                         |
                         v
                  +--------------+
                  | CacheManager |
                  +--------------+
                    |     |    |
          cache ops |     |    | TTL operations
                    |     |    v
                    |     | +-------------------+
                    |     | | ExpirationManager |
                    |     | +-------------------+
                    |     |          |
                    |     |          v
                    |     |    +------------+
                    |     |    | ExpiryHeap |
                    |     |    +------------+
                    |     |
                    v     v
             +----------+  +----------------+
             | LRU / LFU|  |  BackingStore  |
             | policy   |  | append-only log|
             +----------+  +----------------+
```

`CacheManager` receives an `ICache` policy, a `BackingStore`, an `ExpirationManager`, and a `Clock`. The clock is an interface so tests can inject a deterministic fake clock. The manager owns the selected cache policy; the store, expiration manager, and clock are passed by reference and must outlive the manager.

The cache policy stores in-memory values. The backing store persists encoded values, including whether a TTL exists and its absolute expiry timestamp. The expiration manager independently indexes keys with TTLs.

## 3. Request flows

### Set

1. Determine whether `ttl` is positive. A positive TTL becomes an absolute expiry time (`clock.now() + ttl`); zero means no expiration.
2. Encode the value and expiry metadata.
3. Write the encoded record to the backing store. If this fails, return the storage error without updating the cache or expiry index.
4. Insert the plain value into the selected cache policy.
5. If the cache reports an evicted key, cancel that key's expiry entry.
6. Schedule the new expiry, or cancel an old expiry if the key has no TTL.

### Get

1. Read the current time from the injected clock.
2. If the key is in the in-memory cache, check its expiry entry. If expired, remove it from the cache, expiry index, and store, then return `NotFound`. Otherwise return the cached value and count a hit.
3. On a cache miss, read the encoded value from the backing store.
4. Decode the record. Corrupt encoding returns `ErrCorrupt`; an expired record is removed and returns `NotFound`.
5. Admit a valid value to the cache. If admission evicts another key, cancel that key's expiry entry.
6. Restore the record's expiry in the expiration manager, or cancel any stale expiry for a non-expiring record.
7. Return the value. A successful backing-store read is counted as a cache miss.

### Delete and expiration

`del` removes the key from the store, cache, and expiry manager. `sweep()` asks the expiration manager to remove every deadline less than or equal to `now`; for each removed deadline it calls `CacheManager::onExpired`, which removes the key from the cache and backing store.

## 4. Algorithms and data structures

### LRU policy

LRU uses a hash table to locate an entry's list node and a doubly linked list to maintain recency. A successful access moves the node to the most-recently-used end. When capacity is exceeded, the least-recently-used end is evicted.

Expected operation cost is O(1), assuming expected O(1) hash-table operations.

### LFU policy

LFU groups entries by access frequency. Frequency buckets are ordered, and entries within a bucket retain recency order. Access promotes an entry to a higher-frequency bucket. Eviction selects the least-recently-used entry in the minimum-frequency bucket.

The implementation maintains neighboring frequency-bucket relationships so normal promotion and eviction avoid scanning all entries. Expected operation cost is O(1), subject to the hash-table assumptions and implementation invariants.

### Indexed expiry min-heap

Each heap entry contains `(key, expiry)`. The binary min-heap is ordered by expiry, so the earliest deadline is at the root. A hash table maps each key to its current heap-array index.

- Insert or update: O(log n) heap adjustment, with expected O(1) index lookup.
- Read earliest deadline: O(1).
- Remove an arbitrary key: expected O(1) index lookup plus O(log n) heap repair.
- Pop earliest deadline: O(log n).
- Validate: O(n), intended for tests and debugging.

Every heap swap updates both the array and the key-to-index table. This centralizes maintenance of the index invariant. Equal expiry timestamps are allowed; their relative order is unspecified.

### Persistent backing store

The backing store appends records and keeps an in-memory hash index pointing to the latest record for each key. Updates append rather than rewriting an earlier record. Opening the store scans the log to reconstruct the index. This simplifies updates and recovery logic, at the cost of append-log growth and startup work proportional to the log size.

## 5. Design trade-offs

| Choice                                  | Benefit                                                                        | Cost / limitation                                                                 |
| --------------------------------------- | ------------------------------------------------------------------------------ | --------------------------------------------------------------------------------- |
| LRU and LFU as interchangeable policies | Policy selection is isolated from storage and request coordination             | Both policies must satisfy the same cache interface                               |
| Hash table plus linked list for LRU     | Expected constant-time lookup and recency updates                              | Depends on hash distribution and correct pointer ownership                        |
| Frequency buckets for LFU               | Expected constant-time promotion and minimum-frequency eviction                | More bookkeeping and invariants than LRU                                          |
| Indexed heap for TTL                    | Efficient earliest-deadline lookup and arbitrary key cancellation/rescheduling | Additional key-index table; every swap must update the index                      |
| Append-only persistence                 | Simple writes and recoverable latest-value index                               | File grows; opening requires scanning records                                     |
| Injected clock                          | Deterministic TTL tests without sleeping                                       | Callers must provide a clock whose lifetime exceeds the manager                   |
| Synchronous expiration                  | Simple control flow and deterministic tests                                    | No background worker; expiration is processed on explicit sweep or relevant reads |

## 6. Correctness invariants

1. The cache policy contains at most its configured capacity.
2. For the LRU policy, list order represents recency and the hash table points to the corresponding list nodes.
3. For the LFU policy, buckets are frequency-ordered and each entry belongs to the bucket matching its frequency.
4. For `ExpiryHeap`, every key in the heap has exactly one index entry, every index points back to the matching key, and each parent expiry is no greater than either child expiry.
5. A key without a TTL has no scheduled expiry entry.
6. When an expiry is processed, the key is removed from the cache and backing store.
7. A failed backing-store write does not publish the new value into the in-memory cache or expiry index.

The heap's public `validate()` method checks its two structural invariants and is exercised by the committed tests after heap and expiration-manager operations.

## 7. Testing and verification

Run `make test` from the repository root. The suite includes tests for the custom structures, cache policies, backing store, policy parity, and cache-manager integration. `tests/test_min_heap.cpp` covers root, last-element, and interior-element removal; duplicate deadlines; updates; capacity growth; and `ExpirationManager` sweeping/cancellation. It checks `ExpiryHeap::validate()` after each mutation.

`make asan` builds and runs the test suite with address and undefined-behavior sanitizers where supported by the local toolchain.

This document does not claim verification of asynchronous worker behavior, thread safety, Git history, or teaching-assistant access to the remote repository. Those require separate implementation or external checks.
