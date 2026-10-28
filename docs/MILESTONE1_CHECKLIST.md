# Milestone 1 Checklist

Milestone 1 is 25% of the project grade: 20% for data structures and 5% for code and documentation. The initial plan must also be presented.

## Code

- [ ] `DynamicArray`, `DoublyLinkedList`, `HashTable`, `MinHeap` implemented from scratch
- [ ] `LRUCache`, `LFUHeapCache`, `LFUO1Cache`, `ClockCache` implemented
- [ ] `BackingStore`, `MockStore`, `DiskStore` implemented, data survives a restart
- [ ] `make check-stl` passes: no forbidden headers in `include/`, `src/`, `benchmarks/`
- [ ] `make test` passes with sanitizers: no leaks, no out-of-bounds access
- [ ] Both LFU variants evict in the same order (equivalence test)

## Documentation (in-code, graded)

- [ ] Every file has a header block (author, purpose, used by)
- [ ] Every class, function and non-trivial variable has a purpose comment
- [ ] Key operations explain their logic
- [ ] Design decisions are recorded (why this structure, why this layout)
- [ ] Complexity noted on every public operation

## GitHub

- [ ] Repo created through the **GitHub Classroom link on Canvas**
- [ ] Teaching assistants and course instructors have access
- [ ] Each member has commits under their own name
- [ ] Work went through PRs, not one big dump
- [ ] `main` builds and passes tests at the deadline
- [ ] `README.md` is current

## Initial plan presentation

- [ ] Problem and why caching policy matters
- [ ] Proposed solution and system diagram
- [ ] Data structures chosen and why (table in `docs/PLAN.md`)
- [ ] Algorithms and complexities
- [ ] Planned experiments
- [ ] Milestones and who owns what
- [ ] Risks
- [ ] Format confirmed with the TA (slides? live? oral?)

Suggested 7-slide structure: problem, solution and architecture, data structures, policies and complexity, disk store, experiment plan, team and timeline.

## Individual evaluation: be ready to answer

Every member should be able to answer these about **any** component.

**General**
- Why these structures and not others? What would change with an STL container?
- Walk through a `get` that misses and forces an eviction of a dirty entry.
- Where is memory allocated and freed in your component?

**DoublyLinkedList / LRU**
- Why can LRU evict and promote in O(1)? What does the hash table store?
- What are sentinel nodes and what bug do they prevent?

**HashTable**
- How do you handle collisions? When and how do you resize? What is the load-factor threshold and why?
- Why does the hash function mix bits? What happens to sequential keys without it?

**MinHeap / LFU heap**
- Why is the heap *indexed*? What breaks without position tracking?
- Why O(log n)? What is the tie-break and why?

**LFU O(1)**
- Draw the frequency-bucket structure. What happens on a hit for frequency 3 when bucket 4 doesn't exist?
- How do you find the eviction victim in O(1)?

**CLOCK**
- What does the hand do? Why is it only an approximation of LRU?

**DiskStore**
- Page size and record layout? How is a key located? What happens on overflow?
- How do you know data survived a restart? What does a read or write cost in page I/Os?

**Evaluation**
- Why does LRU suffer from a large scan while LFU doesn't? Why is LFU bad when popularity changes?
- Why Zipf for the workload?
