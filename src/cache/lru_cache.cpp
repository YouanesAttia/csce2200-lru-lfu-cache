/**
 * This file implements a Least Recently Used (LRU) Cache utilizing a combination
 * of a Doubly Linked List (to maintain usage order) and a custom Hash Index
 * (for O(1) average time complexity lookups).
 */

#include "lru_cache.h"

using namespace std;

LRUCache::LRUCache(size_t capacity)
    : capacity_(capacity),
      // Initialize the hash index with a size > capacity (capacity * 2 + 1)
      // to maintain a low load factor (< 0.5) and minimize hash collisions.
      index_(capacity * 2 + 1),
      order_()
{
}

/**
 * Retrieves the value associated with a given key.
 *
 * If the key is found, the accessed item is moved to the front of the
 * internal linked list, marking it as the most recently used.
 */
bool LRUCache::get(const string &key, string &value)
{
    DoublyLinkedList::Node **found = index_.find(key);

    // Key does not exist in the cache
    if (!found)
        return false;

    DoublyLinkedList::Node *node = *found;
    value = node->data;

    // Mark as most recently used by moving it to the front of the list
    order_.moveToFront(node);

    return true;
}

/**
 * Inserts or updates a key-value pair in the cache.
 * If the key already exists, its value is updated and it becomes the most
 * recently used. If the key is new, it is inserted at the front. If the
 * cache is at maximum capacity, the least recently used item is evicted
 * before insertion.
 */
void LRUCache::put(const string &key, const string &value)
{
    // Zero-capacity caches cannot store anything
    if (capacity_ == 0)
        return;

    DoublyLinkedList::Node **found = index_.find(key);

    // Case 1: Key already exists. Update value and promote to MRU.
    if (found)
    {
        (*found)->data = value;
        order_.moveToFront(*found);
        return;
    }

    // Case 2: Key does not exist. Check if we need to evict an old item first.
    if (order_.size() == capacity_)
        evictLeastRecent();

    // Insert the new item at the front (Most Recently Used)
    DoublyLinkedList::Node *node = order_.pushFront(key, value);

    // Attempt to register the new node in the hash index
    if (!index_.insert(key, node))
    {
        // Rollback: Keep the two data structures consistent if index insertion fails.
        // This handles potential memory allocation failures or index limitations.
        order_.erase(node);
    }
}

/**
 * Checks if a key exists in the cache without updating its usage order.
 */
bool LRUCache::contains(const string &key) const
{
    return index_.find(key) != nullptr;
}

/**
 * Evicts the least recently used item from the cache.
 *
 * The LRU item is always located at the back of the doubly linked list.
 * This method removes it from the list, drops it from the index, and
 * frees the allocated memory.
 */
void LRUCache::evictLeastRecent()
{
    // Remove the tail of the list
    DoublyLinkedList::Node *victim = order_.detachBack();

    // Safety check in case the list is already empty
    if (!victim)
        return;

    // Remove the key from the hash index
    index_.remove(victim->key);

    // Free the memory of the evicted node
    delete victim;
}

/**
 * Validates the internal consistency of the cache.
 *
 * Ensures that the internal Doubly Linked List and Hash Index are perfectly
 * synchronized. This is primarily used for debugging and unit testing.
 */
bool LRUCache::validate() const
{
    // Both structures must track the exact same number of elements
    if (order_.size() != index_.size())
        return false;

    // The size must strictly obey the defined capacity limit
    if (order_.size() > capacity_)
        return false;

    // Traverse the list and verify every node matches what the index tracks
    for (DoublyLinkedList::Node *current = order_.head(); current != nullptr; current = current->next)
    {
        DoublyLinkedList::Node *const *found = index_.find(current->key);

        // Discrepancy found: Node exists in the list but missing or points
        // to a completely different node in the hash index.
        if (found == nullptr || *found != current)
            return false;
    }

    return true;
}