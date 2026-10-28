# Interface Contracts

These are the agreements between members. Each owner writes the real header in
`include/...` to match. Change a contract only through a team discussion, and
update this file in the same PR.

## Global decisions (confirm at kickoff)

- Language: C++17, header-only templates in `include/`, builds with `make`.
- `Key` is `uint64_t`. `Value` is a fixed-size struct of **64 bytes** (`kValueSize = 64`). Fixed size keeps the disk layout simple.
- Shared types live in `include/cache/types.h` (Youanes).
- Failure is reported by return value (`bool` / `nullptr`), not exceptions.
- Types that own memory delete their copy constructor and copy assignment.
- Pointers handed out by a structure (list nodes, `V*` from the hash table) stay valid until the element is erased. Memory must not move on growth.

## DynamicArray<T> — Ebrahim

| Operation | Contract | Cost |
|---|---|---|
| `pushBack(const T&)` | Append, grow capacity by doubling | O(1) amortized |
| `popBack()` | Remove last, precondition: not empty | O(1) |
| `operator[](size_t)` | Unchecked access (assert in debug) | O(1) |
| `size()`, `capacity()`, `empty()` | | O(1) |
| `reserve(n)`, `clear()` | | O(n) / O(1) |

## DoublyLinkedList<T> — Omar A

Nodes are heap-allocated and **never move**, so other structures can store `Node*`.

| Operation | Contract | Cost |
|---|---|---|
| `Node` | `{ T value; Node* prev; Node* next; }` | |
| `pushFront(v)`, `pushBack(v)` | Returns `Node*` to the new node | O(1) |
| `popFront(T& out)`, `popBack(T& out)` | `false` if empty | O(1) |
| `remove(Node*)` | Unlink and free a node you hold a pointer to | O(1) |
| `moveToFront(Node*)` | Relink without freeing | O(1) |
| `front()`, `back()` | `Node*` or `nullptr` if empty | O(1) |
| `size()`, `empty()`, `clear()` | | O(1) / O(n) |

Suggested design decision to document: sentinel head/tail nodes versus null checks.

## HashTable<K, V> — Omar B

Separate chaining or open addressing (your choice, document why).

| Operation | Contract | Cost |
|---|---|---|
| `insert(k, v)` | Insert or overwrite | O(1) expected |
| `find(k)` | Returns `V*`, or `nullptr` if absent | O(1) expected |
| `erase(k)` | `true` if it existed | O(1) expected |
| `size()`, `contains(k)`, `loadFactor()` | | O(1) |
| Resizing | Automatic, keeps load factor bounded (chaining ≤ 0.75, open addressing ≤ 0.5) | amortized O(1) |

- Needs a hash function for `uint64_t`. A mixing function is required because raw modulo behaves badly on sequential keys. Explain your choice in a comment.
- **Milestone 1 shortcut:** push a working no-resize v0 within 24 hours so others can integrate. Add resize afterwards.
- The `V*` returned by `find` may be invalidated by a later `insert` that triggers a resize. State this in the comment, or make it stable.

## MinHeap<T> (indexed) — Ebrahim

A binary min-heap over a `DynamicArray<T>`. LFU needs to change an element's priority, so the heap must support position-based updates.

| Operation | Contract | Cost |
|---|---|---|
| `push(x)` | Insert | O(log n) |
| `top()`, `pop()` | Minimum element | O(1) / O(log n) |
| `siftUp(i)`, `siftDown(i)` | Restore the heap after the element at `i` changed | O(log n) |
| `removeAt(i)` | Remove an arbitrary element | O(log n) |
| Position tracking | Document how the cache learns an element's current index (for example, a hook called on every move) | |

Comparison is decided by the element type's `operator<` or a comparator template parameter (no `std::function`).

## Cache interface — `ICache` (Youanes)

| Operation | Contract |
|---|---|
| `ICache(BackingStore&, size_t capacity)` | Capacity counts entries |
| `bool get(Key, Value& out)` | Hit: return the cached value. Miss: read the store, insert (evicting if full). Returns `false` only if the key is nowhere. |
| `void put(Key, const Value&)` | Insert or update, mark dirty. Evict if full. |
| `void flush()` | Write all dirty entries to the store |
| `size()`, `capacity()` | Current entries, maximum entries |
| `const Stats& stats() const` | See below |
| `const char* name() const` | `"LRU"`, `"LFU-heap"`, `"LFU-O1"`, `"CLOCK"` |

Eviction of a dirty entry writes it to the store (write-back). A clean entry is dropped.
Policies: `LRUCache`, `LFUHeapCache`, `LFUO1Cache`, `ClockCache`.

LFU tie-break: among equal frequencies, evict the **least recently used**. Both LFU variants must give the same eviction order so they can be compared fairly, and a test should check this.

## Stats — Youanes

`hits`, `misses`, `evictions`, `dirtyWritebacks`, `storeReads`, `storeWrites` (all `uint64_t`), plus `hitRatio()`.

## BackingStore — Ali

| Operation | Contract |
|---|---|
| `bool read(Key, Value& out)` | `false` if the key is absent |
| `void write(Key, const Value&)` | Insert or overwrite |
| `bool remove(Key)` | `true` if it existed |
| `uint64_t reads() const`, `writes() const` | Page-level I/O counters for experiments |

- `MockStore` (**due Fri 30 Oct**): in-memory, for cache tests. A plain array indexed by key over a fixed key range is fine.
- `DiskStore`: single file of fixed-size pages. Suggested layout: header page (magic number, page size, bucket count), then bucket pages holding fixed-size records `(key, value, occupied)`, with an overflow page pointer. The bucket index is `hash(key) mod bucketCount`. The data persists across runs. Document the layout in `docs/DISK_FORMAT.md` (Ali writes this by Sat 31 Oct).

## Workload generators — Youanes (Milestone 2 start, interface fixed now)

```
struct Op { enum Type { Get, Put } type; Key key; };
class Workload { virtual bool next(Op& out) = 0; };
```

`SequentialWorkload(n)`, `UniformWorkload(n, seed)`, `ZipfWorkload(n, alpha, seed)`, `ScanMixer(base, scanLength, atOp)`.
The random source is our own seeded generator (xorshift64* or splitmix64) so results are reproducible across machines.
