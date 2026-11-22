#ifndef RECORD_CODEC_H
#define RECORD_CODEC_H

#include <cstdint>
#include <cstring>
#include <string>

#include "backing_store.h"

namespace storage {

// The cache manager stores each entry's expiry time together with its value.
// The backing store only sees one opaque string, built by encodeValue():
//
//   offset 0   4 bytes  magic "CVAL"
//   offset 4   4 bytes  codec version (1)
//   offset 8   8 bytes  expiry time, little-endian; 0 means "never expires"
//   offset 16  4 bytes  length of the user value, little-endian
//   offset 20  n bytes  the user value (any bytes, including zero)
//
// The header adds CODEC_HEADER_SIZE bytes, and the store limits a stored
// value to MAX_VALUE_LEN, so the user's value can be at most MAX_USER_VALUE_LEN.
const uint32_t CODEC_HEADER_SIZE = 20;
const uint32_t CODEC_VERSION = 1;
const uint32_t MAX_USER_VALUE_LEN = MAX_VALUE_LEN - CODEC_HEADER_SIZE;

namespace codec_detail {

inline void putU32(std::string& out, uint32_t v) {
    for (int i = 0; i < 4; i++) out.push_back((char)(unsigned char)(v >> (8 * i)));
}

inline void putU64(std::string& out, uint64_t v) {
    for (int i = 0; i < 8; i++) out.push_back((char)(unsigned char)(v >> (8 * i)));
}

inline uint32_t getU32(const std::string& s, size_t pos) {
    uint32_t v = 0;
    for (int i = 3; i >= 0; i--) v = (v << 8) | (unsigned char)s[pos + i];
    return v;
}

inline uint64_t getU64(const std::string& s, size_t pos) {
    uint64_t v = 0;
    for (int i = 7; i >= 0; i--) v = (v << 8) | (unsigned char)s[pos + i];
    return v;
}

}

// Builds the string to hand to BackingStore::put(). Does not check the size;
// put() returns ErrTooLarge if the result is over MAX_VALUE_LEN.
inline std::string encodeValue(uint64_t expiry, const std::string& value) {
    std::string raw;
    raw.reserve(CODEC_HEADER_SIZE + value.size());
    raw.append("CVAL", 4);
    codec_detail::putU32(raw, CODEC_VERSION);
    codec_detail::putU64(raw, expiry);
    codec_detail::putU32(raw, (uint32_t)value.size());
    raw.append(value);
    return raw;
}

// Splits a string returned by BackingStore::get() back into expiry and value.
// Returns Ok, or ErrCorrupt when the header is short, has the wrong magic or
// version, or its length field does not match the bytes present.
// On failure expiry and value are left unchanged.
inline Status decodeValue(const std::string& raw, uint64_t& expiry,
                          std::string& value) {
    if (raw.size() < CODEC_HEADER_SIZE) return Status::ErrCorrupt;
    if (memcmp(raw.data(), "CVAL", 4) != 0) return Status::ErrCorrupt;
    if (codec_detail::getU32(raw, 4) != CODEC_VERSION) return Status::ErrCorrupt;
    uint32_t len = codec_detail::getU32(raw, 16);
    if (raw.size() - CODEC_HEADER_SIZE != len) return Status::ErrCorrupt;
    expiry = codec_detail::getU64(raw, 8);
    value.assign(raw, CODEC_HEADER_SIZE, len);
    return Status::Ok;
}

// An entry with expiry 0 never expires. Otherwise it is expired once the
// current time has reached its expiry time.
inline bool isExpired(uint64_t expiry, uint64_t now) {
    return expiry != 0 && now >= expiry;
}

}

#endif