# Initial Project Plan (Milestone 1)

*Draft for the team to edit. Milestone 1 requires presenting this plan and showing you understand the problem, the proposed solution, and the system design.*

## 1. Problem

Reading from disk is orders of magnitude slower than reading from memory. A
cache keeps a small set of items in memory. When it is full, an **eviction
policy** decides which item to drop. A good policy keeps the items that will be
requested again. The best policy depends on the access pattern: a sequential
scan, uniform random access, and skewed (Zipfian) access favour different policies.

## 2. Proposed solution

A cache engine with **pluggable eviction policies** sitting in front of a
**persistent, disk-based key-value store**. All structures are implemented from
scratch. A workload generator drives the cache, and a metrics layer records hit
ratio, disk I/O, and speed so we can compare policies with data.

## 3. System design

```
 CLI / benchmark harness
        |
 Workload generators ----> ICache (policy) ----> BackingStore
 (sequential, uniform,     LRU | LFU-heap |      MockStore (tests)
  Zipf, scan mixer)        LFU-O(1) | CLOCK      DiskStore (paged file)
                               |
                               v
        Data-structure layer: DynamicArray, DoublyLinkedList,
                              HashTable, MinHeap
                               |
                             Stats
```

Separation of concerns (required for Milestone 2): data structures know nothing
about caches; policies know nothing about the CLI; the CLI knows nothing about
how a policy stores its data.

Cache semantics: **read-through, write-back**. `get` on a miss loads from the
store and inserts. `put` marks the entry dirty. A dirty entry is written to the
store when evicted or on `flush()`.

## 4. Data structures

| Structure | Used by | Linear? | Owner |
|---|---|---|---|
| DynamicArray | MinHeap, CLOCK ring, Zipf CDF table | Linear | Ebrahim |
| DoublyLinkedList | LRU, LFU-O(1) | Linear | Omar A |
| HashTable | every policy | Associative | Omar B |
| MinHeap (indexed) | LFU-heap | **Non-linear (tree)** | Ebrahim |
| Frequency-bucket list (list of lists) | LFU-O(1) | Composite | Omar A |
| On-disk hash file | DiskStore | Disk-based | Ali |

The rubric needs non-linear structures. The heap is a binary tree in an array. If the TA does not count the hash table as non-linear, add a stretch structure: tree pseudo-LRU (binary tree of bits) or an on-disk B-tree index.

## 5. Algorithms

- **LRU.** Hash table maps key to list node. Front of the list is most recent. A hit moves the node to the front, and eviction removes the tail. O(1).
- **LFU with heap.** Hash table maps key to heap position. The heap orders by (frequency, last-access tick). A hit increments the frequency and sifts down. Eviction pops the root. O(log n).
- **LFU with frequency buckets (O(1)).** A list of frequency nodes in ascending order, each holding a list of keys with that frequency. A hit moves the key from bucket f to bucket f+1 (creating it if needed). Eviction takes the oldest key from the lowest bucket. O(1).
- **CLOCK (second chance).** A circular array with a reference bit per slot. A hand sweeps, clearing bits, and evicts the first slot whose bit is 0. Cheap approximation of LRU.
- **Zipf sampling.** Precompute the cumulative distribution over n keys for exponent α, then sample by binary search on a uniform random number.
- **Stretch.** ARC or 2Q, and tree pseudo-LRU (the hardware-cache variant mentioned in the idea sheet).

## 6. Experiments (Milestone 2 and final)

| Experiment | Variables | Metrics |
|---|---|---|
| Policy comparison | Workload: sequential, uniform, Zipf α in {0.8, 1.0, 1.2}. Cache size: 1%, 5%, 10%, 25% of the key space. | Hit ratio, store reads/writes, ops/sec |
| LFU heap vs LFU O(1) | Number of operations 10^3 to 10^6 | Time per operation, scaling curve |
| Scan resistance | Zipf workload with one large sequential scan injected | Hit ratio before, during and after the scan, per policy |
| Disk cost | Cache size vs store I/O | Page reads and writes per 1000 ops |

All random inputs use a fixed seed so results are reproducible.

## 7. Scope by milestone

| Milestone | Scope |
|---|---|
| 1 (3 Nov) | All data structures and policies implemented and unit-tested. DiskStore working. Interfaces fixed. Initial plan presented. |
| 2 (24 Nov) | CLI application, workload generators wired in, design documentation (architecture, algorithms, trade-offs). |
| Final (17/20 Dec) | Experiments, analysis, report, marketing presentation, stretch policies. |

## 8. Risks

| Risk | Mitigation |
|---|---|
| Everyone depends on `HashTable` and `DoublyLinkedList` | Those two are due first (Fri 30 Oct). Others code against `docs/INTERFACES.md` meanwhile. |
| Disk store design is the biggest unknown | Ali starts today. A `MockStore` unblocks cache testing on day 2. |
| One member can't explain another's code | Cross-review rotation, plus the explain-your-code session on 2 Nov. |
| Merge conflicts | One component per file, one owner per file. |
| Time | Feature freeze Sun 1 Nov night. Mon is for tests, comments, and presentation. |

## 9. Questions to confirm with the TA

1. Are `<string>`, `<fstream>`, `<chrono>`, `<random>` acceptable, or only our own structures?
2. Can tests use STL containers as a reference oracle?
3. Does a hash table count as a non-linear data structure for the rubric?
4. What format does the initial plan presentation take (slides, live demo, oral)?
5. The guideline lists the final report as 17 Dec and the final submission as 20 Dec. Which is the code deadline?
