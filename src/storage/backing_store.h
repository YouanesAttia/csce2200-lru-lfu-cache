#ifndef BACKING_STORE_H
#define BACKING_STORE_H

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

namespace storage {

// Size limits. They also bound the length fields read back from disk.
const uint32_t MAX_KEY_LEN = 1024;
const uint32_t MAX_VALUE_LEN = 1u << 20;

enum class Status {
    Ok,
    Recovered,
    NotFound,
    ErrNotOpen,
    ErrOpenFailed,
    ErrBadHeader,
    ErrIo,
    ErrCorrupt,
    ErrTooLarge,
    ErrInvalidArg
};

const char* statusName(Status s);

// Hash table with separate chaining, mapping key -> location of the latest
// record for that key in the data file. Doubles its bucket array when
// size exceeds capacity, so find/put/erase are O(1) on average.
class HashIndex {
public:
    struct Entry {
        std::string key;
        uint64_t offset;
        uint32_t valueLen;
        Entry* next;
    };

    HashIndex();
    ~HashIndex();
    HashIndex(const HashIndex&) = delete;
    HashIndex& operator=(const HashIndex&) = delete;

    bool find(const std::string& key, uint64_t& offset, uint32_t& valueLen) const;
    void put(const std::string& key, uint64_t offset, uint32_t valueLen);
    bool erase(const std::string& key);
    void clear();
    size_t size() const { return size_; }

private:
    size_t slot(const std::string& key) const;
    void allocate(size_t capacity);
    void rehash(size_t newCapacity);

    Entry** buckets_;
    size_t capacity_;
    size_t size_;
};

// Append-only log file plus an in-memory HashIndex rebuilt on open.
//
// Contract with the cache manager:
//   get() == Ok          value copied into out (cache can insert it)
//   get() == NotFound    key was never stored or was removed (a true miss)
//   get() == Err*        storage failure; must be reported, not treated as a miss
//   put() == Ok          record is in the file and visible to later get() calls
//   open() == Recovered  usable; a damaged tail was discarded (see discardedBytes())
// Keys must be 1 to MAX_KEY_LEN bytes. Values may be empty, up to MAX_VALUE_LEN
// bytes, and may contain any byte including zero.
// Not thread-safe. One BackingStore object per file.
class BackingStore {
public:
    BackingStore();
    ~BackingStore();
    BackingStore(const BackingStore&) = delete;
    BackingStore& operator=(const BackingStore&) = delete;

    Status open(const char* path, bool createIfMissing = true);
    void close();
    bool isOpen() const { return file_ != nullptr; }

    Status put(const std::string& key, const std::string& value);
    Status get(const std::string& key, std::string& out);
    Status remove(const std::string& key);
    bool contains(const std::string& key) const;
    size_t count() const { return index_.size(); }
    Status flush();

    uint64_t fileSize() const { return endOffset_; }
    uint64_t discardedBytes() const { return discarded_; }
    uint64_t diskReads() const { return reads_; }
    uint64_t diskWrites() const { return writes_; }

private:
    Status appendRecord(unsigned char type, const std::string& key,
                        const std::string& value, uint64_t& offsetOut);
    Status scanLog(uint64_t fileLen);

    FILE* file_;
    HashIndex index_;
    uint64_t endOffset_;
    uint64_t discarded_;
    uint64_t reads_;
    uint64_t writes_;
};

}

#endif
