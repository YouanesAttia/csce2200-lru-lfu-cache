# Kickoff message (paste into the team chat)

Team, here's the plan for Milestone 1, due **Tuesday 3 Nov**.

**Project:** A6, LRU/LFU cache with a persistent (disk-based) backing store, in C++. We implement every data structure ourselves, with no STL containers.

**Repo:** <repo link>. Accept the GitHub Classroom invite, then read `README.md`, `CONTRIBUTING.md`, `docs/TASKS.md` and `docs/INTERFACES.md` tonight.

**Who builds what**
- Omar A: DoublyLinkedList, LRU, LFU O(1)
- Omar B: HashTable, CLOCK
- Ebrahim: DynamicArray, MinHeap, LFU with heap
- Ali: MockStore, DiskStore (the persistent store)
- Youanes: repo, ICache and Stats, workload generators, initial plan, integration

**Most urgent:** HashTable v0 by Thursday night, and DoublyLinkedList, DynamicArray, HashTable (with resize) and MockStore by Friday night. Everything else depends on them.

**Rules**
1. We write our own code. No AI-generated code, and every one of us must be able to explain every line, including other people's parts.
2. No STL containers (`make check-stl` checks this).
3. Branch, open a PR, get the assigned review, then merge. Commit under your own name.
4. Comment your code properly. It's graded.

**Kickoff call:** Thursday, 30 minutes. Bring questions about `docs/INTERFACES.md`.
**Feature freeze:** Sunday midnight. Monday is for tests, comments, and rehearsing the explanation of our code.
