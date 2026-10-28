# Timeline

## Official deadlines (from the project guidelines)

| Date | Item |
|---|---|
| Thu 8 Oct 2026 | Group registration (passed; groups are locked) |
| **Tue 3 Nov 2026** | Milestone 1: data structures + initial plan (25% of project) |
| 24 Nov 2026 | Milestone 2: application + design documentation (15%). The guideline says "Wednesday 24th", but 24 Nov 2026 is a Tuesday. **Confirm the date with the instructor.** |
| Thu 17 Dec 2026 | Final project report on Canvas and GitHub |
| Sun 20 Dec 2026 | Final submission of code on GitHub + marketing presentation (50%). The guidelines list both 17 and 20 Dec. **Confirm which is which.** |

There is no late policy. No submission is accepted after the due date.

## Milestone 1 day by day

| Day | Goal |
|---|---|
| **Wed 28 Oct** (tonight) | Repo created from the Classroom link, scaffold pushed, team invited, kickoff message sent |
| **Thu 29** | 30-min kickoff call: read `docs/INTERFACES.md`, settle the open decisions. `HashTable` v0 pushed by night. `ICache` and `Stats` merged. Ali has the disk page layout on paper. |
| **Fri 30** | `DoublyLinkedList`, `HashTable` (with resize), `DynamicArray`, `MockStore` merged. **Everyone is unblocked.** |
| **Sat 31** | `MinHeap`, `LRUCache`, `DiskStore` first version, `docs/DISK_FORMAT.md`, workload interfaces |
| **Sun 1 Nov** | `LFUHeapCache`, `LFUO1Cache`, `ClockCache`, `DiskStore` tests merged. **Feature freeze at midnight.** |
| **Mon 2** | Tests green on `main`. Comments pass on every file. Explain-your-code session. Plan slides done and rehearsed. |
| **Tue 3 Nov** | Final check of GitHub access for TAs and instructors. Present. **No new code today.** |

Adjust for your own class schedules. If someone falls behind on Friday, the
cut-line is: LRU, LFU-heap, DiskStore are mandatory. CLOCK and LFU-O(1) are the first to simplify.

## After Milestone 1

| Window | Focus |
|---|---|
| 4 – 24 Nov | CLI, workload generators, benchmark driver, first experiments, design document |
| 25 Nov – 10 Dec | All experiments, scan resistance, LFU heap vs O(1) scaling, stretch policy |
| 10 – 17 Dec | Report writing (outline in `report/REPORT_OUTLINE.md`), citations checked |
| 17 – 20 Dec | Marketing presentation, demo rehearsal, final repo cleanup |
