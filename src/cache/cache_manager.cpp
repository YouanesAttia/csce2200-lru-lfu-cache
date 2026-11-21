#include "cache_manager.hpp"

using namespace std;

CacheManager::CacheManager(ICache *policy, storage::BackingStore &store, ExpirationManager &expiry, Clock &clock)
    : cache_(policy), store_(store), expiry_(expiry), clock_(clock), stats_(){}

CacheManager::~CacheManager(){
    delete cache_;
}

string CacheManager::encodeValue(const string &value, bool hasExpiry, uint64_t expiry){
    string encoded;
    encoded.resize(HEADER_SIZE + value.size());

    encoded[0] = hasExpiry ? static_cast<char>(1) : static_cast<char>(0);
    for (size_t i = 0; i < 8; ++i){
        encoded[1 + i] = static_cast<char>((expiry >> (i * 8)) & 0xFF);
    }

    for (size_t i = 0; i < value.size(); ++i){
        encoded[HEADER_SIZE + i] = value[i];
    }

    return encoded;
}

bool CacheManager::decodeValue(const string &encoded, string &valueOut, bool &hasExpiryOut, uint64_t &expiryOut){
    if (encoded.size() < HEADER_SIZE)
        return false;

    unsigned char flag = static_cast<unsigned char>(encoded[0]);
    if (flag > 1)
        return false;
    hasExpiryOut = (flag == 1);
    expiryOut = 0;
    for (size_t i = 0; i < 8; ++i){
        uint64_t byteVal = static_cast<unsigned char>(encoded[1 + i]);
        expiryOut |= (byteVal << (i * 8));
    }

    valueOut = encoded.substr(HEADER_SIZE);
    return true;
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
        bool hasExpiry = false;
        uint64_t expiryTime = 0;

        if (!decodeValue(encoded, decodedValue, hasExpiry, expiryTime))
            return storage::Status::ErrCorrupt;
        if (hasExpiry && expiryTime <= now){
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

        if (hasExpiry)
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

    string encoded = encodeValue(value, hasExpiry, expiryTime);
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