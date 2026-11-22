#ifndef CLOCK_HPP
#define CLOCK_HPP

#include <cstdint>
#include <ctime>

// The Clock interface itself is declared in cache_manager.hpp, so this file
// includes it rather than declaring a second one with the same name, which
// would not compile. The two clocks below are the implementations of it.
#include "../cache/cache_manager.hpp"

/*
    Where the current time comes from.

    TTLs are in seconds, so now() returns seconds. The reason the manager takes
    a Clock instead of just calling time() is testing: a test for "this key
    expires after 5 seconds" should not have to actually wait 5 seconds. With
    FakeClock the test calls advance(5) and the cache believes it.
*/

// The real clock. Seconds since the epoch, same as time(0).
class SystemClock : public Clock
{
public:
    uint64_t now() const { return (uint64_t)time(0); }
};

// A clock that only moves when you tell it to. Used by the tests, and by the
// CLI's ADVANCE command when the program is started with --fake-clock.
class FakeClock : public Clock
{
private:
    uint64_t now_;

public:
    FakeClock(uint64_t start = 0) { now_ = start; }

    uint64_t now() const { return now_; }

    // Move time forward by this many seconds.
    void advance(uint64_t seconds) { now_ += seconds; }

    // Jump to an exact time, for setting up a test.
    void setNow(uint64_t t) { now_ = t; }
};

#endif
