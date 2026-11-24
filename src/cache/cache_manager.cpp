#include "cache_manager.hpp"
#include "../storage/record_codec.hpp"

using namespace std;

CacheManager::CacheManager(ICache *policy, storage::BackingStore &store, ExpirationManager &expiry, Clock &clock)
    : cache_(policy), store_(store), expiry_(expiry), clock_(clock), stats_(){}

CacheManager::~CacheManager(){
    delete cache_;
}

storage::Status CacheManager::get(const string &key, string &out){
    uint64_t now = clock_.now();

    string cachedValue;
    if (cache_ && cache_->get(key, cachedValue)){
        if (expiry_.contains(key) && expiry_.expiryOf(key) <= now){
            cache_->remove(key);
            expiry_.cancel(key);
            store_.remove(key);
            ++stats_.expirations;
            ++stats_.misses;
            return storage::Status::NotFound;
        }

        out = cachedValue;
        ++stats_.hits;
        return storage::Status::Ok;
    }

    string encoded;
    storage::Status st = store_.get(key, encoded);

    if (st == storage::Status::Ok){
        string decodedValue;
        uint64_t expiryTime = 0;

        if (storage::decodeValue(encoded, expiryTime, decodedValue) != storage::Status::Ok)
            return storage::Status::ErrCorrupt;
        if (storage::isExpired(expiryTime, now)){
            store_.remove(key);
            ++stats_.expirations;
            ++stats_.misses;
            return storage::Status::NotFound;
        }

        if (cache_){
            string evictedKey;
            cache_->put(key, decodedValue, &evictedKey);
            if (!evictedKey.empty()){
                expiry_.cancel(evictedKey);
                ++stats_.evictions;
            }
        }

        if (expiryTime != 0)
            expiry_.setExpiry(key, expiryTime);
        else
            expiry_.cancel(key);
        out = decodedValue;
        ++stats_.misses;
        return storage::Status::Ok;
    }

    if (st == storage::Status::NotFound){
        ++stats_.misses;
        return storage::Status::NotFound;
    }

    return st;
}

storage::Status CacheManager::set(const string &key, const string &value, uint64_t ttl){
    bool hasExpiry = (ttl > 0);
    uint64_t expiryTime = hasExpiry ? (clock_.now() + ttl) : 0;

    string encoded = storage::encodeValue(expiryTime, value);
    storage::Status st = store_.put(key, encoded);
    if (st != storage::Status::Ok)
        return st;
    if (cache_){
        string evictedKey;
        cache_->put(key, value, &evictedKey);
        if (!evictedKey.empty()){
            expiry_.cancel(evictedKey);
            ++stats_.evictions;
        }
    }

    if (hasExpiry)
        expiry_.setExpiry(key, expiryTime);
    else
        expiry_.cancel(key);

    return storage::Status::Ok;
}

storage::Status CacheManager::del(const string &key){
    storage::Status st = store_.remove(key);
    if (cache_)
        cache_->remove(key);
    expiry_.cancel(key);
    return st;
}

size_t CacheManager::sweep(){
    return expiry_.sweep(clock_.now(), this);
}

void CacheManager::onExpired(const string &key){
    if (cache_)
        cache_->remove(key);
    store_.remove(key);
    ++stats_.expirations;
}

void CacheManager::switchPolicy(ICache *newCache){
    if (!newCache || newCache == cache_)
        return;
    expiry_.clear();
    delete cache_;
    cache_ = newCache;
}