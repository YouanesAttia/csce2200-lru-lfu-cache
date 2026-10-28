# Task Assignments — Milestone 1

Today is Wed 28 Oct. Milestone 1 is due **Tue 3 Nov**. Dates below are internal
deadlines that leave room for review and interview prep. Swap assignments if
someone is stronger elsewhere, but keep one owner per file.

"Omar A" and "Omar B" are placeholders. Replace them with full names.

## Dependency map

```
DynamicArray ---> MinHeap -----> LFUHeapCache
DoublyLinkedList -> LRUCache
                 -> LFUO1Cache
HashTable --------> all four policies
MockStore --------> all policy tests
ICache + Stats ---> all policies
```

The critical path is **HashTable and DoublyLinkedList**. Everything else waits on them.

---

## Youanes — Team lead and integration

| Deliverable | Files | Due |
|---|---|---|
| Repo set up, team invited, TAs/instructors given access | GitHub | Wed 28 |
| Shared types, `ICache`, `Stats` | `include/cache/types.h`, `cache.h`, `include/metrics/stats.h` | Thu 29 |
| Seeded RNG + workload generator interfaces (Sequential, Uniform; Zipf started) | `include/workload/*.h` | Sat 31 |
| Test helper conventions, `tests/test_stats.cpp` | `tests/` | Fri 30 |
| Initial plan finalised and slides | `docs/PLAN.md` + slides | Mon 2 |
| Reviews Ali's PRs | | ongoing |
| Final integration, `make test` green on `main` | | Sun 1 – Mon 2 |

Also: run the kickoff call, answer the TA questions in `docs/PLAN.md` section 9, keep the status table in the README current.

## Omar A — Linked list, LRU, LFU O(1)

| Deliverable | Files | Due |
|---|---|---|
| `DoublyLinkedList<T>` + tests | `include/ds/doubly_linked_list.h`, `tests/test_dll.cpp` | **Fri 30** |
| `LRUCache` + tests | `include/cache/lru_cache.h`, `tests/test_lru.cpp` | Sat 31 |
| `LFUO1Cache` (frequency buckets) + tests | `include/cache/lfu_o1_cache.h`, `tests/test_lfu_o1.cpp` | Sun 1 |

LFU O(1) is the hardest policy logic. Draw the structure on paper first: a list of frequency nodes, each owning a list of keys, plus a hash from key to its list node and its frequency node.

## Omar B — Hash table and CLOCK

| Deliverable | Files | Due |
|---|---|---|
| `HashTable<K,V>` v0 (no resize) pushed | `include/ds/hash_table.h` | **Thu 29 night** |
| `HashTable` with resize, mixing hash, tests | `tests/test_hash_table.cpp` | **Fri 30** |
| `ClockCache` + tests | `include/cache/clock_cache.h`, `tests/test_clock.cpp` | Sun 1 |
| Randomised stress test of `HashTable` against a simple reference | `tests/` | Sun 1 |

Everyone is blocked on `HashTable`, so the v0 date matters more than polish.

## Ebrahim — Array, heap, LFU with heap

| Deliverable | Files | Due |
|---|---|---|
| `DynamicArray<T>` + tests | `include/ds/dynamic_array.h`, `tests/test_dynamic_array.cpp` | **Fri 30** |
| Indexed `MinHeap<T>` + tests | `include/ds/min_heap.h`, `tests/test_min_heap.cpp` | Sat 31 |
| `LFUHeapCache` + tests | `include/cache/lfu_heap_cache.h`, `tests/test_lfu_heap.cpp` | Sun 1 |
| Test that LFU-heap and LFU-O(1) evict in the same order | `tests/test_lfu_equivalence.cpp` (with Omar A) | Mon 2 |

## Ali — Persistent backing store

| Deliverable | Files | Due |
|---|---|---|
| `BackingStore` interface + `MockStore` | `include/storage/backing_store.h`, `mock_store.h` | **Fri 30** |
| Disk format written down | `docs/DISK_FORMAT.md` | Sat 31 |
| `DiskStore` (create, read, write, remove, persists across runs) + tests | `include/storage/disk_store.h`, `tests/test_disk_store.cpp` | Sun 1 |
| Test: write, close, reopen, read back; overflow chains | in `tests/test_disk_store.cpp` | Sun 1 |

The disk store is the biggest unknown. Start by deciding page size and record layout on paper. Ask for help early if the file I/O gets stuck.

---

## Everyone

- Work on your own branch and open a PR as soon as something compiles.
- Review the PR assigned to you within 12 hours (rotation in `CONTRIBUTING.md`).
- On **Mon 2 Nov**, each member explains their component to the team for 10 minutes while the others ask questions. This is the rehearsal for the interview.
- Read the other components' headers. You are graded on the whole project.

## Looking ahead (Milestone 2, due 24 Nov)

| Task | Owner |
|---|---|
| CLI (load workload, choose policy, run, print stats) | Youanes |
| Workload generators complete (Zipf, scan mixer) | Youanes + Omar B |
| Benchmark driver and CSV output | Ali |
| LFU heap vs O(1) scaling experiment | Ebrahim + Omar A |
| Scan resistance experiment | Omar A |
| Architecture and algorithm design document | Everyone writes their own section; Youanes merges |
| Stretch policies (2Q / ARC / tree pseudo-LRU) | Decide after Milestone 1 |
