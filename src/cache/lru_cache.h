#include "../structures/hash_table.hpp"
#include "../structures/doubly_linked_list.hpp"
#include <cstddef>
#include <string>
#include "icache.hpp"
using namespace std;

// LRU cache: list head is most-recently used; tail is least-recently used.
// The hash table maps each key to its list node for expected O(1) lookup.
class LRUCache : public ICache
{
private:
    size_t capacity_;
    HashTable<DoublyLinkedList::Node *> index_;
    DoublyLinkedList order_;
    void evictLeastRecent(string *evictedKey = 0);

public:
    LRUCache(size_t capacity);
    ~LRUCache() = default;

    LRUCache(const LRUCache &) = delete;
    LRUCache &operator=(const LRUCache &) = delete;

    bool get(const string &key, string &value) override;
    void put(const string &key, const string &value, string *evictedKey = 0);
    bool remove(const string &) override;
    bool contains(const string &) const override;
    size_t size() const { return order_.size(); }
    size_t capacity() const { return capacity_; }
    bool validate() const;
    void clear() override;
};