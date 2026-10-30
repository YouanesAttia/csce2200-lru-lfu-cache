#include "lru_cache.h"
using namespace std;

LRUCache::LRUCache(size_t capacity)
    : capacity_(capacity),
      index_(capacity * 2 + 1),
      order_() {}

LRUCache::~LRUCache()
{
    // The list owns the nodes. The hash table only stores non-owning pointers.
}

bool LRUCache::get(const string &key, string &value)
{
    DoublyLinkedList::Node **found = index_.find(key);
    if (!found)
        return false;
    DoublyLinkedList::Node *node = *found;
    value = node->data;
    order_.moveToFront(node);
    return true;
}

void LRUCache::put(const string &key, const string &value)
{
    if (capacity_ == 0)
        return;

    DoublyLinkedList::Node **found = index_.find(key);
    if (found)
    {
        (*found)->data = value;
        order_.moveToFront(*found);
        return;
    }

    if (order_.size() == capacity_)
        evictLeastRecent();

    DoublyLinkedList::Node *node = order_.pushFront(key, value);
    index_.insert(key, node);
}

bool LRUCache::contains(const string &key) const
{
    return index_.find(key) != nullptr;
}

void LRUCache::evictLeastRecent()
{
    DoublyLinkedList::Node *victim = order_.detachBack();
    if (!victim)
        return;
    index_.erase(victim->key);
    delete victim;
}

bool LRUCache::validate() const
{
    if (order_.size() != index_.size())
        return false;
    const DoublyLinkedList::Node *previous = nullptr;
    size_t counted = 0;
    for (const DoublyLinkedList::Node *node = order_.head(); node; node = node->next)
    {
        if (node->prev != previous)
            return false;
        const DoublyLinkedList::Node *const *indexed = index_.find(node->key);
        if (!indexed || *indexed != node)
            return false;
        previous = node;
        ++counted;
    }
    if (previous != order_.tail())
        return false;
    return counted == order_.size();
}