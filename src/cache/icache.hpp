#ifndef A6_ICACHE_HPP
#define A6_ICACHE_HPP
#include <cstddef>
#include <string>
class ICache
{
public:
    virtual ~ICache() {}
    virtual bool get(const std::string &, std::string &) = 0;
    virtual void put(const std::string &, const std::string &, std::string *evictedKey = 0) = 0;
    virtual bool remove(const std::string &) = 0;
    virtual bool contains(const std::string &) const = 0;
    virtual std::size_t size() const = 0;
    virtual std::size_t capacity() const = 0;
    virtual void clear() = 0;
};
#endif