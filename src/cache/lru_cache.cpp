#include "lru_cache.h"
using namespace std;

LRUCache::LRUCache(size_t capacity)
    : capacity_(capacity),
      index_(capacity * 2 + 1),
      order_() {}

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
    if (!index_.insert(key, node))
    {
        // Keep the two data structures consistent if insertion fails.
        order_.erase(node);
    }
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
    index_.remove(victim->key);
    delete victim;
}

bool LRUCache::validate() const
{
    if (order_.size() != index_.size())
        return false;

    if (order_.size() > capacity_)
        return false;

    for (DoublyLinkedList::Node *current = order_.head(); current != nullptr; current = current->next)
    {
        DoublyLinkedList::Node *const *found = index_.find(current->key);

        if (found == nullptr || *found != current)
            return false;
    }

    return true;
}