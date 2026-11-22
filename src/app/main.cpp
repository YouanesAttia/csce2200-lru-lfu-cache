/*
    The program. Reads commands, hands them to the cache manager, prints what
    came back.

    All the parsing and printing is in cli.cpp, so this file is only the
    wiring: build the pieces, then loop.

    There is no prompt character on purpose. Every line of output has the same
    shape whether a human typed the command or RUN read it from a file, which
    is what makes two runs comparable when we compare LRU against LFU.
*/

#include "cli.h"
#include "clock.hpp"
#include "../cache/cache_manager.hpp"
#include "../cache/lru_cache.h"
#include "../cache/lfu_cache.hpp"
#include "../storage/backing_store.h"
#include "../../include/metrics/stats.hpp"

#include <fstream>
#include <iostream>
using namespace std;

// The manager takes ownership of whatever this returns: switchPolicy deletes
// the old cache and its destructor deletes the last one. So nothing in this
// file ever deletes a cache.
static ICache *makeCache(const string &policy, size_t capacity)
{
    if (policy == "lfu")
        return new LFUCache(capacity);
    return new LRUCache(capacity);
}

// The manager counts hits, misses, evictions and expirations. The store counts
// its own disk reads and writes. STATS shows both together.
static Stats collectStats(const CacheManager &manager, const storage::BackingStore &store)
{
    Stats stats;
    const CacheStats &counters = manager.stats();
    stats.hits = counters.hits;
    stats.misses = counters.misses;
    stats.evictions = counters.evictions;
    stats.expirations = counters.expirations;
    stats.setStoreCounters(store.diskReads(), store.diskWrites());
    return stats;
}

static bool runScript(const string &path, CacheManager &manager, storage::BackingStore &store,
                      FakeClock *fake, const Options &options, int depth);

// Carries out one command. Returns false when the program should stop.
static bool execute(const Command &command, CacheManager &manager, storage::BackingStore &store,
                    FakeClock *fake, const Options &options, int depth)
{
    string value;
    storage::Status status;

    switch (command.type)
    {
    case CommandType::Set:
    {
        // The manager handles eviction inside set() and only counts it, it does
        // not say which key went. So we watch the counter to find out that one
        // happened. Once set() reports the key, pass it here instead.
        const uint64_t evictionsBefore = manager.stats().evictions;

        status = manager.set(command.key, command.value, command.hasTtl ? command.ttl : 0);
        if (status != storage::Status::Ok)
        {
            printError(storage::statusName(status));
            break;
        }
        if (manager.stats().evictions > evictionsBefore)
            printEvicted("(unknown)");
        printOk();
        break;
    }

    case CommandType::Get:
    {
        // get() returns Ok whether the value came from the cache or from the
        // store, so the hit counter is what tells the two apart.
        const uint64_t hitsBefore = manager.stats().hits;

        status = manager.get(command.key, value);
        if (status == storage::Status::Ok)
        {
            if (manager.stats().hits > hitsBefore)
                printHit(value);
            else
                printMissStore(value);
        }
        else if (status == storage::Status::NotFound)
            printNotFound();
        else
            printError(storage::statusName(status));
        break;
    }

    case CommandType::Del:
        status = manager.del(command.key);
        if (status == storage::Status::Ok)
            printDeleted();
        else if (status == storage::Status::NotFound)
            printNotFound();
        else
            printError(storage::statusName(status));
        break;

    case CommandType::Policy:
        // switchPolicy clears the expiry heap, so TTLs set before the switch
        // are forgotten. The store keeps the values, so the new cache refills
        // from disk as keys are asked for.
        manager.switchPolicy(makeCache(command.policy, options.capacity));
        printPolicy(command.policy);
        break;

    case CommandType::Stats:
        printStats(collectStats(manager, store));
        break;

    case CommandType::Sweep:
        printExpired(manager.sweep());
        break;

    case CommandType::Advance:
        if (fake == 0)
        {
            printError("NotFakeClock"); // only works with --fake-clock
            break;
        }
        fake->advance(command.seconds);
        printTime(fake->now());
        break;

    case CommandType::Run:
        if (depth > 0)
        {
            printError("NestedRun"); // a script may not call RUN itself
            break;
        }
        return runScript(command.file, manager, store, fake, options, depth + 1);

    case CommandType::Help:
        printHelp();
        break;

    case CommandType::Exit:
        return false;

    case CommandType::Invalid:
        printError(command.errorName);
        break;

    case CommandType::Empty:
        break;
    }

    return true;
}

// Reads commands from a file, one per line, exactly as if they were typed.
static bool runScript(const string &path, CacheManager &manager, storage::BackingStore &store,
                      FakeClock *fake, const Options &options, int depth)
{
    ifstream input(path.c_str());
    if (!input)
    {
        printError("CannotOpenFile");
        return true; // a bad filename is not a reason to quit
    }

    string line;
    while (getline(input, line))
    {
        Command command = parseLine(line);
        if (command.type == CommandType::Empty)
            continue;
        if (!execute(command, manager, store, fake, options, depth))
            return false; // EXIT inside a script stops the program
    }
    return true;
}

int main(int argc, char **argv)
{
    Options options = parseArgs(argc, argv);
    if (!options.ok)
    {
        printError(options.errorName);
        printUsage();
        return 1;
    }

    storage::BackingStore store;
    storage::Status opened = store.open(options.dbPath.c_str());
    // Recovered means the file had a damaged tail that was thrown away. The
    // store is usable, so carry on.
    if (opened != storage::Status::Ok && opened != storage::Status::Recovered)
    {
        printError(storage::statusName(opened));
        return 1;
    }

    // Both clocks are built, and the flag picks which one the manager gets.
    // fake stays null without --fake-clock, which is how ADVANCE knows to
    // refuse.
    SystemClock systemClock;
    FakeClock fakeClock(0);
    Clock *clock = options.fakeClock ? (Clock *)&fakeClock : (Clock *)&systemClock;
    FakeClock *fake = options.fakeClock ? &fakeClock : 0;

    ExpirationManager expiry;
    CacheManager manager(makeCache(options.policy, options.capacity), store, expiry, *clock);

    string line;
    while (getline(cin, line))
    {
        Command command = parseLine(line);
        if (command.type == CommandType::Empty)
            continue;
        if (!execute(command, manager, store, fake, options, 0))
            break;
    }

    store.close();
    return 0;
}
