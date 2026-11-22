#ifndef STATS_HPP
#define STATS_HPP

#include <cstdint>

/*
    Counters for how the cache is behaving.

    The cache manager owns one of these and bumps the counters as it works.
    The CLI's STATS command prints it.

    hits and misses are counted by the manager. storeReads and storeWrites are
    not counted here at all, they are copied in from the backing store, which
    already tracks them with diskReads() and diskWrites(). Counting them twice
    would just be a second number that can disagree with the first.
*/
struct Stats
{
    uint64_t hits;        // key was in the cache
    uint64_t misses;      // key was not in the cache (whether or not the store had it)
    uint64_t evictions;   // entries thrown out by LRU or LFU to make room
    uint64_t expirations; // entries removed because their TTL ran out
    uint64_t storeReads;  // copied from BackingStore::diskReads()
    uint64_t storeWrites; // copied from BackingStore::diskWrites()

    Stats() { reset(); }

    void reset()
    {
        hits = 0;
        misses = 0;
        evictions = 0;
        expirations = 0;
        storeReads = 0;
        storeWrites = 0;
    }

    // Fraction of lookups that the cache answered by itself.
    // Returns 0 when nothing has been looked up yet, so the caller does not
    // have to guard against dividing by zero.
    double hitRate() const
    {
        uint64_t total = hits + misses;
        if (total == 0)
            return 0.0;
        return (double)hits / (double)total;
    }

    // Called by the manager after it touches the store, so the two disk
    // counters stay in step with the store's own.
    void setStoreCounters(uint64_t reads, uint64_t writes)
    {
        storeReads = reads;
        storeWrites = writes;
    }
};

#endif
