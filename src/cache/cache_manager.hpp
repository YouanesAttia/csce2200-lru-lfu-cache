#ifndef CACHE_MANAGER_HPP
#define CACHE_MANAGER_HPP

#include <cstddef>
#include <cstdint>
#include <string>

#include "icache.hpp"
#include "expiration_manager.hpp"
#include "../storage/backing_store.h"

using namespace std;

class Clock {
public:
    virtual ~Clock() {}
    virtual uint64_t now() const = 0;
};

struct CacheStats {
    uint64_t hits = 0;
    uint64_t misses = 0;
    uint64_t evictions = 0;
    uint64_t expirations = 0;

    void reset()
    {
        hits = 0;
        misses = 0;
        evictions = 0;
        expirations = 0;
    }

    double hitRate() const
    {
        uint64_t total = hits + misses;
        if (total == 0)
            return 0.0;
        return static_cast<double>(hits) / static_cast<double>(total);
    }
};

class CacheManager : public ExpiryListener {
public:
    CacheManager(ICache *policy,
                 storage::BackingStore &store,
                 ExpirationManager &expiry,
                 Clock &clock);
    ~CacheManager();

    CacheManager(const CacheManager &) = delete;
    CacheManager &operator=(const CacheManager &) = delete;

    storage::Status get(const string &key, string &out);
    storage::Status set(const string &key, const string &value, uint64_t ttl = 0);
    storage::Status del(const string &key);

    size_t sweep();
    void onExpired(const string &key) override;

    void switchPolicy(ICache *newCache);

    const CacheStats &stats() const { return stats_; }
    void resetStats() { stats_.reset(); }
    const ICache *policy() const { return cache_; }

private:
    static const size_t HEADER_SIZE = 9;

    static string encodeValue(const string &value, bool hasExpiry, uint64_t expiry);
    static bool decodeValue(const string &encoded, string &valueOut, bool &hasExpiryOut, uint64_t &expiryOut);

    ICache *cache_;
    storage::BackingStore &store_;
    ExpirationManager &expiry_;
    Clock &clock_;
    CacheStats stats_;
};

#endif