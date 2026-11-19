#include "../structures/indexed_min_heap.hpp"
#include <cstdint>
#include <stdexcept>

class ExpiryListener
{
public:
    virtual ~ExpiryListener() {};
    virtual void onExpired(const std::string &key) = 0;
};

class ExpirationManager
{
    ExpiryHeap heap_;

public:
    void setExpiry(const string &key, uint64_t t) { heap_.upsert(key, t); }
    void cancel(const string &key) { heap_.remove(key); }
    bool contains(const string &key) const { return heap_.contains(key); }
    uint64_t expiryOf(const string &key) const { return heap_.expiryOf(key); }
    bool nextExpiry(uint64_t &t) const
    {
        if (heap_.empty())
            return false;
        t = heap_.peek().expiry;
        return true;
    }
    uint64_t nextExpiry() const
    {
        if (heap_.empty())
            throw runtime_error("no scheduled expiry");
        return heap_.peek().expiry;
    }
    size_t size() const { return heap_.size(); }
    void clear()
    {
        while (!heap_.empty())
            heap_.popMin();
    }
    size_t sweep(uint64_t now, ExpiryListener *listener)
    {
        size_t n = 0;
        while (!heap_.empty() && heap_.peek().expiry <= now)
        {
            ExpiryHeap::Entry e = heap_.popMin();
            if (listener)
                listener->onExpired(e.key);
            ++n;
        }
        return n;
    }
    bool validate() const { return heap_.validate(); }
};