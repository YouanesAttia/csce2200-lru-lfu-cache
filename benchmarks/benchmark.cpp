#include "../src/cache/lru_cache.h"
#include "../src/cache/lfu_cache.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

struct Result
{
    std::string policy;
    std::string workload;
    std::size_t operations;
    std::size_t hits;
    double hitRate;
    double milliseconds;
    double opsPerSecond;
};

static std::uint32_t nextRandom(std::uint32_t &state)
{
    state = state * 1664525u + 1013904223u;
    return state;
}

static std::vector<std::string> makeWorkload(const std::string &kind, std::size_t count, std::size_t capacity)
{
    std::vector<std::string> keys;
    keys.reserve(count);
    std::uint32_t seed = 123456789u;
    for (std::size_t i = 0; i < count; ++i)
    {
        std::size_t key = 0;
        if (kind == "sequential")
        {
            key = i % (capacity / 2);
        }
        else if (kind == "random")
        {
            key = (nextRandom(seed) >> 8) % (capacity * 8);
        }
        else
        { // Zipf-like: minimum of three uniform samples favors low keys.
            std::uint32_t a = (nextRandom(seed) >> 8) % (capacity * 8);
            std::uint32_t b = (nextRandom(seed) >> 8) % (capacity * 8);
            std::uint32_t c = (nextRandom(seed) >> 8) % (capacity * 8);
            key = a < b ? (a < c ? a : c) : (b < c ? b : c);
        }
        keys.push_back("key_" + std::to_string(key));
    }
    return keys;
}

template <typename Cache>
static Result runWorkload(const std::string &policy, const std::string &name,
                          const std::vector<std::string> &keys,
                          std::size_t capacity)
{
    Cache cache(capacity);
    std::size_t hits = 0;
    std::string value;
    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < keys.size(); ++i)
    {
        if (cache.get(keys[i], value))
        {
            ++hits;
        }
        else
        {
            cache.put(keys[i], "value");
        }
    }
    const std::chrono::steady_clock::time_point finish = std::chrono::steady_clock::now();
    const double ms = std::chrono::duration<double, std::milli>(finish - start).count();
    const double seconds = ms / 1000.0;
    Result r = {policy, name, keys.size(), hits,
                keys.empty() ? 0.0 : static_cast<double>(hits) / keys.size(),
                ms, seconds > 0.0 ? keys.size() / seconds : 0.0};
    return r;
}

template <typename Cache>
static Result runScanResistance(const std::string &policy, std::size_t capacity)
{
    Cache cache(capacity);
    std::string value;
    // Warm a small hot set, then issue one scan much larger than the cache.
    for (int repeat = 0; repeat < 20; ++repeat)
    {
        for (std::size_t k = 0; k < capacity / 2; ++k)
        {
            const std::string key = "hot_" + std::to_string(k);
            if (!cache.get(key, value))
                cache.put(key, "value");
        }
    }
    for (std::size_t k = 0; k < capacity * 16; ++k)
        cache.put("scan_" + std::to_string(k), "value");

    std::size_t hits = 0;
    const std::size_t operations = capacity * 4;
    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < operations; ++i)
    {
        const std::string key = "hot_" + std::to_string(i % (capacity / 2));
        if (cache.get(key, value))
            ++hits;
        else
            cache.put(key, "value");
    }
    const std::chrono::steady_clock::time_point finish = std::chrono::steady_clock::now();
    const double ms = std::chrono::duration<double, std::milli>(finish - start).count();
    const double seconds = ms / 1000.0;
    Result r = {policy, "scan_resistance", operations, hits,
                static_cast<double>(hits) / operations, ms,
                seconds > 0.0 ? operations / seconds : 0.0};
    return r;
}

static void writeResult(std::ofstream &out, const Result &r)
{
    out << r.policy << ',' << r.workload << ',' << r.operations << ',' << r.hits << ','
        << std::fixed << std::setprecision(6) << r.hitRate << ','
        << r.milliseconds << ',' << r.opsPerSecond << '\n';
    std::cout << std::left << std::setw(7) << r.policy << std::setw(19) << r.workload
              << " hit rate=" << std::fixed << std::setprecision(2) << r.hitRate * 100.0
              << "%  time=" << r.milliseconds << " ms  ops/s=" << std::setprecision(0)
              << r.opsPerSecond << '\n';
}

int main()
{
    const std::size_t capacity = 256;
    const std::size_t operations = 100000;
    std::ofstream csv("data/results/workload_results.csv");
    if (!csv)
    {
        std::cerr << "Could not open data/results/workload_results.csv\n";
        return 1;
    }
    csv << "policy,workload,operations,hits,hit_rate,elapsed_ms,operations_per_second\n";
    const char *workloads[] = {"sequential", "random", "zipfian"};
    std::cout << "Cache capacity: " << capacity << "; operations per workload: " << operations << "\n";
    for (std::size_t i = 0; i < 3; ++i)
    {
        const std::vector<std::string> keys = makeWorkload(workloads[i], operations, capacity);
        writeResult(csv, runWorkload<LRUCache>("LRU", workloads[i], keys, capacity));
        writeResult(csv, runWorkload<LFUCache>("LFU", workloads[i], keys, capacity));
    }
    writeResult(csv, runScanResistance<LRUCache>("LRU", capacity));
    writeResult(csv, runScanResistance<LFUCache>("LFU", capacity));
    std::cout << "CSV written to data/results/workload_results.csv\n";
    return 0;
}
