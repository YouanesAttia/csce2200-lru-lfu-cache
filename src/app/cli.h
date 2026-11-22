#ifndef CLI_H
#define CLI_H

#include <cstdint>
#include <string>

/*
    Command line parsing and printing. No cache logic lives here.

    parseLine() turns a typed line into a Command. main() looks at the
    Command, calls the cache manager, and then calls one of the print
    functions below with the result. That split is deliberate: the parser and
    the output format can be tested on their own, with no cache at all
    (tests/test_cli.cpp does exactly that).

    Grammar:
        SET <key> <value> [TTL <seconds>]
        GET <key>
        DEL <key>
        POLICY lru|lfu
        STATS
        SWEEP
        ADVANCE <seconds>          (only with --fake-clock)
        RUN <file>
        HELP
        EXIT

    Command words are not case sensitive, so "get" and "GET" both work. Keys
    and values are.

    Values are normally one token. A value with spaces must be in double
    quotes:  SET greeting "hello world" TTL 30
    There is no escape character, so a value cannot itself contain a quote.
*/

enum class CommandType
{
    Set,
    Get,
    Del,
    Policy,
    Stats,
    Sweep,
    Advance,
    Run,
    Help,
    Exit,
    Empty,  // blank line or a # comment, main should just skip it
    Invalid // did not parse, errorName says why
};

struct Command
{
    CommandType type;

    std::string key;     // SET, GET, DEL
    std::string value;   // SET
    uint64_t ttl;        // SET, only meaningful when hasTtl is true
    bool hasTtl;         // SET without TTL means the entry never expires
    std::string policy;  // POLICY, always "lru" or "lfu" once parsed
    uint64_t seconds;    // ADVANCE
    std::string file;    // RUN
    std::string errorName; // set when type is Invalid

    Command()
    {
        type = CommandType::Empty;
        ttl = 0;
        hasTtl = false;
        seconds = 0;
    }
};

// Startup flags: --policy lru|lfu, --capacity <n>, --db <path>, --fake-clock
struct Options
{
    std::string policy;  // "lru" or "lfu"
    size_t capacity;     // how many entries the cache may hold
    std::string dbPath;  // backing store file
    bool fakeClock;      // use FakeClock instead of SystemClock
    bool ok;             // false if a flag was wrong
    std::string errorName;

    Options()
    {
        policy = "lru";
        capacity = 100;
        dbPath = "data/cache.db";
        fakeClock = false;
        ok = true;
    }
};

Command parseLine(const std::string &line);
Options parseArgs(int argc, char **argv);

/*
    Output.

    Every command prints exactly one of these shapes, and the shape never
    changes between commands. That is what makes RUN <file> output comparable
    between two runs, which is how the policy comparison gets measured.

        OK                      SET stored the value
        EVICTED: <key>          printed before OK when the cache made room
        HIT <value>             GET found it in the cache
        MISS (store) <value>    GET missed the cache, the store had it
        NOT FOUND               GET or DEL, the key is nowhere
        DELETED                 DEL removed it
        POLICY <name>           POLICY switched
        EXPIRED <n>             SWEEP removed n entries
        TIME <seconds>          ADVANCE moved the fake clock
        ERROR: <name>           anything that failed
*/
void printOk();
void printEvicted(const std::string &key);
void printHit(const std::string &value);
void printMissStore(const std::string &value);
void printNotFound();
void printDeleted();
void printPolicy(const std::string &name);
void printExpired(uint64_t count);
void printTime(uint64_t seconds);
void printError(const std::string &name);
void printHelp();
void printUsage();

struct Stats; // metrics/stats.hpp
void printStats(const Stats &stats);

#endif
