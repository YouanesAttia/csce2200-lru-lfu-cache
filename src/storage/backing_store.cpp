#include "backing_store.h"

#include <cerrno>
#include <cstring>

#ifdef _WIN32
#include <io.h>
#else
#include <sys/types.h>
#include <unistd.h>
#endif

namespace storage {

namespace {

const unsigned char FILE_MAGIC[4] = {'B', 'S', 'T', 'R'};
const uint32_t FORMAT_VERSION = 2;
const uint32_t HEADER_SIZE = 8;
// Record: type(1) keyLen(4) valueLen(4) key value checksum(4)
const uint32_t RECORD_HEAD = 9;
const uint32_t RECORD_OVERHEAD = 13;
const unsigned char REC_PUT = 1;
const unsigned char REC_DELETE = 2;
const uint32_t FNV_SEED = 2166136261u;

void encodeU32(unsigned char* p, uint32_t v) {
    for (int i = 0; i < 4; i++) p[i] = (unsigned char)(v >> (8 * i));
}

uint32_t decodeU32(const unsigned char* p) {
    uint32_t v = 0;
    for (int i = 3; i >= 0; i--) v = (v << 8) | p[i];
    return v;
}

uint32_t fnvUpdate(uint32_t h, const unsigned char* p, size_t n) {
    for (size_t i = 0; i < n; i++) {
        h ^= p[i];
        h *= 16777619u;
    }
    return h;
}

uint64_t hashKey(const std::string& key) {
    uint64_t h = 1469598103934665603ull;
    for (size_t i = 0; i < key.size(); i++) {
        h ^= (unsigned char)key[i];
        h *= 1099511628211ull;
    }
    h ^= h >> 32;
    h *= 0x9e3779b97f4a7c15ull;
    h ^= h >> 29;
    return h;
}

bool seekTo(FILE* f, uint64_t pos) {
#ifdef _WIN32
    return _fseeki64(f, (long long)pos, SEEK_SET) == 0;
#else
    return fseeko(f, (off_t)pos, SEEK_SET) == 0;
#endif
}

bool lengthOf(FILE* f, uint64_t& out) {
#ifdef _WIN32
    if (_fseeki64(f, 0, SEEK_END) != 0) return false;
    long long p = _ftelli64(f);
#else
    if (fseeko(f, 0, SEEK_END) != 0) return false;
    off_t p = ftello(f);
#endif
    if (p < 0) return false;
    out = (uint64_t)p;
    return true;
}

bool truncateTo(FILE* f, uint64_t len) {
    if (fflush(f) != 0) return false;
#ifdef _WIN32
    return _chsize_s(_fileno(f), (long long)len) == 0;
#else
    return ftruncate(fileno(f), (off_t)len) == 0;
#endif
}

bool writeHeader(FILE* f) {
    unsigned char h[HEADER_SIZE];
    memcpy(h, FILE_MAGIC, 4);
    encodeU32(h + 4, FORMAT_VERSION);
    return seekTo(f, 0) && fwrite(h, 1, HEADER_SIZE, f) == HEADER_SIZE &&
           fflush(f) == 0;
}

}

const char* statusName(Status s) {
    switch (s) {
        case Status::Ok: return "Ok";
        case Status::Recovered: return "Recovered";
        case Status::NotFound: return "NotFound";
        case Status::ErrNotOpen: return "ErrNotOpen";
        case Status::ErrOpenFailed: return "ErrOpenFailed";
        case Status::ErrBadHeader: return "ErrBadHeader";
        case Status::ErrIo: return "ErrIo";
        case Status::ErrCorrupt: return "ErrCorrupt";
        case Status::ErrTooLarge: return "ErrTooLarge";
        case Status::ErrInvalidArg: return "ErrInvalidArg";
    }
    return "Unknown";
}

HashIndex::HashIndex() : buckets_(nullptr), capacity_(0), size_(0) {
    allocate(64);
}

HashIndex::~HashIndex() {
    clear();
    delete[] buckets_;
}

void HashIndex::allocate(size_t capacity) {
    buckets_ = new Entry*[capacity];
    for (size_t i = 0; i < capacity; i++) buckets_[i] = nullptr;
    capacity_ = capacity;
}

size_t HashIndex::slot(const std::string& key) const {
    return (size_t)(hashKey(key) & (capacity_ - 1));
}

bool HashIndex::find(const std::string& key, uint64_t& offset,
                     uint32_t& valueLen) const {
    for (Entry* e = buckets_[slot(key)]; e != nullptr; e = e->next) {
        if (e->key == key) {
            offset = e->offset;
            valueLen = e->valueLen;
            return true;
        }
    }
    return false;
}

void HashIndex::put(const std::string& key, uint64_t offset, uint32_t valueLen) {
    for (Entry* e = buckets_[slot(key)]; e != nullptr; e = e->next) {
        if (e->key == key) {
            e->offset = offset;
            e->valueLen = valueLen;
            return;
        }
    }
    Entry* e = new Entry;
    e->key = key;
    e->offset = offset;
    e->valueLen = valueLen;
    size_t s = slot(key);
    e->next = buckets_[s];
    buckets_[s] = e;
    size_++;
    if (size_ > capacity_) rehash(capacity_ * 2);
}

bool HashIndex::erase(const std::string& key) {
    Entry** link = &buckets_[slot(key)];
    while (*link != nullptr) {
        if ((*link)->key == key) {
            Entry* dead = *link;
            *link = dead->next;
            delete dead;
            size_--;
            return true;
        }
        link = &(*link)->next;
    }
    return false;
}

void HashIndex::clear() {
    for (size_t i = 0; i < capacity_; i++) {
        Entry* e = buckets_[i];
        while (e != nullptr) {
            Entry* next = e->next;
            delete e;
            e = next;
        }
        buckets_[i] = nullptr;
    }
    size_ = 0;
}

void HashIndex::rehash(size_t newCapacity) {
    Entry** old = buckets_;
    size_t oldCapacity = capacity_;
    allocate(newCapacity);
    for (size_t i = 0; i < oldCapacity; i++) {
        Entry* e = old[i];
        while (e != nullptr) {
            Entry* next = e->next;
            size_t s = slot(e->key);
            e->next = buckets_[s];
            buckets_[s] = e;
            e = next;
        }
    }
    delete[] old;
}

BackingStore::BackingStore()
    : file_(nullptr), endOffset_(0), discarded_(0), reads_(0), writes_(0) {}

BackingStore::~BackingStore() { close(); }

Status BackingStore::open(const char* path, bool createIfMissing) {
    if (path == nullptr) return Status::ErrInvalidArg;
    close();

    FILE* f = fopen(path, "r+b");
    if (f == nullptr) {
        if (errno == ENOENT && createIfMissing) f = fopen(path, "w+b");
        if (f == nullptr) return Status::ErrOpenFailed;
    }

    uint64_t len = 0;
    if (!lengthOf(f, len)) {
        fclose(f);
        return Status::ErrIo;
    }

    if (len == 0) {
        if (!writeHeader(f)) {
            fclose(f);
            return Status::ErrIo;
        }
        file_ = f;
        endOffset_ = HEADER_SIZE;
        discarded_ = 0;
        reads_ = writes_ = 0;
        return Status::Ok;
    }

    unsigned char h[HEADER_SIZE];
    if (len < HEADER_SIZE || !seekTo(f, 0) ||
        fread(h, 1, HEADER_SIZE, f) != HEADER_SIZE ||
        memcmp(h, FILE_MAGIC, 4) != 0 || decodeU32(h + 4) != FORMAT_VERSION) {
        fclose(f);
        return Status::ErrBadHeader;
    }

    file_ = f;
    discarded_ = 0;
    reads_ = writes_ = 0;
    Status s = scanLog(len);
    if (s != Status::Ok && s != Status::Recovered) {
        fclose(file_);
        file_ = nullptr;
        index_.clear();
        endOffset_ = 0;
    }
    return s;
}

// Reads records from the start of the log and applies each to the index.
// Stops at the first record that is truncated, has an unknown type, an
// invalid length, or a failed checksum. Everything from that point on is
// cut from the file so later appends stay reachable.
Status BackingStore::scanLog(uint64_t fileLen) {
    uint64_t pos = HEADER_SIZE;
    unsigned char head[RECORD_HEAD];
    unsigned char keyBuf[MAX_KEY_LEN];
    unsigned char chunk[4096];
    unsigned char tail[4];

    while (pos < fileLen) {
        if (fileLen - pos < RECORD_OVERHEAD) break;
        if (fread(head, 1, RECORD_HEAD, file_) != RECORD_HEAD) break;
        unsigned char type = head[0];
        uint32_t klen = decodeU32(head + 1);
        uint32_t vlen = decodeU32(head + 5);
        if (type != REC_PUT && type != REC_DELETE) break;
        if (type == REC_DELETE && vlen != 0) break;
        if (klen == 0 || klen > MAX_KEY_LEN) break;
        if (vlen > MAX_VALUE_LEN) break;
        if (fileLen - pos < (uint64_t)RECORD_OVERHEAD + klen + vlen) break;

        if (fread(keyBuf, 1, klen, file_) != klen) break;
        uint32_t h = fnvUpdate(FNV_SEED, head, RECORD_HEAD);
        h = fnvUpdate(h, keyBuf, klen);

        uint32_t remaining = vlen;
        bool ok = true;
        while (remaining > 0) {
            size_t want = remaining < sizeof(chunk) ? remaining : sizeof(chunk);
            if (fread(chunk, 1, want, file_) != want) {
                ok = false;
                break;
            }
            h = fnvUpdate(h, chunk, want);
            remaining -= (uint32_t)want;
        }
        if (!ok || fread(tail, 1, 4, file_) != 4) break;
        if (decodeU32(tail) != h) break;

        std::string key((const char*)keyBuf, klen);
        if (type == REC_PUT) index_.put(key, pos, vlen);
        else index_.erase(key);
        pos += RECORD_OVERHEAD + klen + vlen;
    }

    endOffset_ = pos;
    if (pos < fileLen) {
        discarded_ = fileLen - pos;
        if (!truncateTo(file_, pos)) return Status::ErrIo;
        return Status::Recovered;
    }
    return Status::Ok;
}

void BackingStore::close() {
    if (file_ != nullptr) {
        fclose(file_);
        file_ = nullptr;
    }
    index_.clear();
    endOffset_ = 0;
}

Status BackingStore::appendRecord(unsigned char type, const std::string& key,
                                  const std::string& value,
                                  uint64_t& offsetOut) {
    uint32_t klen = (uint32_t)key.size();
    uint32_t vlen = (uint32_t)value.size();
    size_t total = (size_t)RECORD_OVERHEAD + klen + vlen;
    unsigned char* buf = new unsigned char[total];
    buf[0] = type;
    encodeU32(buf + 1, klen);
    encodeU32(buf + 5, vlen);
    memcpy(buf + RECORD_HEAD, key.data(), klen);
    if (vlen > 0) memcpy(buf + RECORD_HEAD + klen, value.data(), vlen);
    uint32_t h = fnvUpdate(FNV_SEED, buf, RECORD_HEAD + klen + vlen);
    encodeU32(buf + RECORD_HEAD + klen + vlen, h);

    bool ok = seekTo(file_, endOffset_) &&
              fwrite(buf, 1, total, file_) == total && fflush(file_) == 0;
    delete[] buf;
    if (!ok) return Status::ErrIo;

    offsetOut = endOffset_;
    endOffset_ += total;
    writes_++;
    return Status::Ok;
}

Status BackingStore::put(const std::string& key, const std::string& value) {
    if (file_ == nullptr) return Status::ErrNotOpen;
    if (key.empty()) return Status::ErrInvalidArg;
    if (key.size() > MAX_KEY_LEN || value.size() > MAX_VALUE_LEN)
        return Status::ErrTooLarge;
    uint64_t offset = 0;
    Status s = appendRecord(REC_PUT, key, value, offset);
    if (s != Status::Ok) return s;
    index_.put(key, offset, (uint32_t)value.size());
    return Status::Ok;
}

Status BackingStore::get(const std::string& key, std::string& out) {
    if (file_ == nullptr) return Status::ErrNotOpen;
    uint64_t offset = 0;
    uint32_t vlen = 0;
    if (!index_.find(key, offset, vlen)) return Status::NotFound;

    uint32_t klen = (uint32_t)key.size();
    size_t total = (size_t)RECORD_OVERHEAD + klen + vlen;
    unsigned char* buf = new unsigned char[total];
    bool ok = seekTo(file_, offset) && fread(buf, 1, total, file_) == total;
    reads_++;
    if (!ok) {
        delete[] buf;
        return Status::ErrIo;
    }

    bool valid = buf[0] == REC_PUT && decodeU32(buf + 1) == klen &&
                 decodeU32(buf + 5) == vlen &&
                 memcmp(buf + RECORD_HEAD, key.data(), klen) == 0 &&
                 fnvUpdate(FNV_SEED, buf, RECORD_HEAD + klen + vlen) ==
                     decodeU32(buf + RECORD_HEAD + klen + vlen);
    if (valid) out.assign((const char*)buf + RECORD_HEAD + klen, vlen);
    delete[] buf;
    return valid ? Status::Ok : Status::ErrCorrupt;
}

Status BackingStore::remove(const std::string& key) {
    if (file_ == nullptr) return Status::ErrNotOpen;
    uint64_t offset = 0;
    uint32_t vlen = 0;
    if (!index_.find(key, offset, vlen)) return Status::NotFound;
    uint64_t unused = 0;
    Status s = appendRecord(REC_DELETE, key, std::string(), unused);
    if (s != Status::Ok) return s;
    index_.erase(key);
    return Status::Ok;
}

bool BackingStore::contains(const std::string& key) const {
    uint64_t offset = 0;
    uint32_t vlen = 0;
    return file_ != nullptr && index_.find(key, offset, vlen);
}

Status BackingStore::flush() {
    if (file_ == nullptr) return Status::ErrNotOpen;
    return fflush(file_) == 0 ? Status::Ok : Status::ErrIo;
}

}
