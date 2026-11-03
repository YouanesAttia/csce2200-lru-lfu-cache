#ifndef A6_LFU_CACHE_HPP
#define A6_LFU_CACHE_HPP

#include "../structures/hash_table.hpp"
#include "../structures/doubly_linked_list.hpp"

#include <cstddef>
#include <limits>
#include <string>

/*
 * Least Frequently Used (LFU) cache with an LRU tie-breaker.
 *
 * Current repository interface:
 *   - keys are std::string
 *   - values are std::string
 *   - DoublyLinkedList stores key/value pairs
 *
 * Design:
 *   - The frequency buckets form a doubly linked list sorted by frequency.
 *   - Each frequency bucket owns a DoublyLinkedList of entries with that
 *     frequency. The list head is most-recently used; its tail is least-recently
 *     used.
 *   - The hash table maps each key directly to its entry node and frequency
 *     bucket, avoiding a scan through the buckets.
 *   - The first frequency bucket is always the minimum-frequency bucket.
 *
 * Policy decisions:
 *   - A newly inserted key starts at frequency 1.
 *   - A successful get increments frequency.
 *   - Updating an existing key through put also counts as an access and
 *     increments frequency.
 *   - A put for a new key into a full cache evicts before inserting.
 *   - A zero-capacity cache ignores put calls.
 *   - A missing get returns false and leaves the output value unchanged.
 *
 * Expected time for get/put/remove: O(1), assuming expected O(1) hash-table
 * operations. Hash collisions can make an individual operation O(n) in the
 * worst case. Insertion into the hash table may occasionally resize it.
 */

class LFUCache
{
private:
    struct FrequencyBucket
    {
        std::size_t frequency;
        FrequencyBucket *prev;
        FrequencyBucket *next;
        DoublyLinkedList entries;

        explicit FrequencyBucket(std::size_t f)
            : frequency(f), prev(nullptr), next(nullptr), entries() {}


        ~FrequencyBucket() = default;

        FrequencyBucket(const FrequencyBucket &) = delete;
        FrequencyBucket &operator=(const FrequencyBucket &) = delete;
    };

    // The hash table stores this small locator for every cached key.
    // Both pointers refer to objects owned by this LFUCache.

    struct EntryLocation
    {
        DoublyLinkedList::Node *entryNode;
        FrequencyBucket *bucket;

        EntryLocation(DoublyLinkedList::Node *node = nullptr,
                      FrequencyBucket *frequencyBucket = nullptr)
            : entryNode(node), bucket(frequencyBucket) {}
    };

    std::size_t capacity_;
    std::size_t size_;
    HashTable<EntryLocation> index_;
    FrequencyBucket *firstBucket_;
    std::size_t minFrequency_;

    // Make the bucket the new first bucket. This is used for frequency 1,
    // which is always smaller than every frequency already in the cache.

    FrequencyBucket *createFirstBucket(std::size_t frequency)
    {
        FrequencyBucket *bucket = new FrequencyBucket(frequency);
        bucket->next = firstBucket_;

        if (firstBucket_ != nullptr)
            firstBucket_->prev = bucket;

        firstBucket_ = bucket;
        updateMinimumFrequency();
        return bucket;
    }

    // Insert a new frequency bucket immediately after a known bucket.

    FrequencyBucket *createBucketAfter(FrequencyBucket *previous, std::size_t frequency)
    {
        FrequencyBucket *bucket = new FrequencyBucket(frequency);
        bucket->prev = previous;
        bucket->next = previous->next;

        if (previous->next != nullptr)
            previous->next->prev = bucket;

        previous->next = bucket;
        return bucket;
    }

    // Unlink and delete an empty bucket. Entry lists are required to be empty
    // before this function is called.

    void deleteEmptyBucket(FrequencyBucket *bucket)
    {
        if (bucket == nullptr || !bucket->entries.empty())
            return;

        if (bucket->prev != nullptr)
            bucket->prev->next = bucket->next;
        else
            firstBucket_ = bucket->next;

        if (bucket->next != nullptr)
            bucket->next->prev = bucket->prev;

        delete bucket;
        updateMinimumFrequency();
    }

    // Make sure minfrequency is consistent with first bucket

    void updateMinimumFrequency()
    {
        minFrequency_ = (firstBucket_ == nullptr) ? 0 : firstBucket_->frequency;
    }

    // Promote one entry from frequency f to f+1

    void promote(const std::string &key, EntryLocation *location)
    {
        if (location == nullptr || location->entryNode == nullptr ||
            location->bucket == nullptr)
            return;

        FrequencyBucket *oldBucket = location->bucket;
        DoublyLinkedList::Node *oldNode = location->entryNode;

        // A size_t frequency cannot be incremented beyond its maximum value.

        if (oldBucket->frequency == std::numeric_limits<std::size_t>::max())
        {
            oldBucket->entries.moveToFront(oldNode);
            return;
        }

        const std::size_t newFrequency = oldBucket->frequency + 1;
        FrequencyBucket *newBucket = nullptr;
        bool createdBucket = false;

        if (oldBucket->next != nullptr &&
            oldBucket->next->frequency == newFrequency)
        {
            newBucket = oldBucket->next;
        }
        else
        {
            newBucket = createBucketAfter(oldBucket, newFrequency);
            createdBucket = true;
        }

        // save data before erasing the old list node.

        const std::string savedKey = oldNode->key;
        const std::string savedValue = oldNode->data;

        DoublyLinkedList::Node *newNode =
            newBucket->entries.pushFront(savedKey, savedValue);

        // Update the index before destroying the old node. If the update fails. roll back the new node.
        if (!index_.update(key, EntryLocation(newNode, newBucket)))
        {
            newBucket->entries.erase(newNode);
            if (createdBucket && newBucket->entries.empty())
                deleteEmptyBucket(newBucket);
            return;
        }

        oldBucket->entries.erase(oldNode);
        if (oldBucket->entries.empty())
            deleteEmptyBucket(oldBucket);
    }

    // Remove the least-recently-used entry from the minimum-frequency bucket.
    // The tail is LRU because new/promoted entries are inserted at the head.

    bool evictLeastFrequentlyUsed()
    {
        if (firstBucket_ == nullptr)
            return false;

        FrequencyBucket *bucket = firstBucket_;
        DoublyLinkedList::Node *victim = bucket->entries.tail();
        if (victim == nullptr)
            return false;

        const std::string victimKey = victim->key;

        index_.remove(victimKey);
        bucket->entries.erase(victim);
        --size_;

        if (bucket->entries.empty())
            deleteEmptyBucket(bucket);

        return true;
    }

public:
    explicit LFUCache(std::size_t capacity)
        : capacity_(capacity),
          size_(0),
          index_(16),
          firstBucket_(nullptr),
          minFrequency_(0)
    {
    }

    ~LFUCache()
    {
        clear();
    }

    LFUCache(const LFUCache &) = delete;
    LFUCache &operator=(const LFUCache &) = delete;

    // Return true on a hit. A hit increments the entry's frequency and makes
    // it the most-recently-used entry inside its new frequency bucket.
    // On a miss, return false without changing value.

    bool get(const std::string &key, std::string &value)
    {
        EntryLocation *location = index_.find(key);
        if (location == nullptr || location->entryNode == nullptr)
            return false;

        value = location->entryNode->data;
        promote(key, location);
        return true;
    }


    // Insert a new key at frequency 1. If the key already exists, overwrite
    // its value and count the put as an access (frequency increases by one).
    // If capacity is zero, the cache remains empty.

    void put(const std::string &key, const std::string &value)
    {
        if (capacity_ == 0)
            return;

        EntryLocation *existing = index_.find(key);
        if (existing != nullptr)
        {
            existing->entryNode->data = value;
            promote(key, existing);
            return;
        }

        if (size_ == capacity_)
            evictLeastFrequentlyUsed();

        FrequencyBucket *bucket = nullptr;
        if (firstBucket_ != nullptr && firstBucket_->frequency == 1)
            bucket = firstBucket_;
        else
            bucket = createFirstBucket(1);

        DoublyLinkedList::Node *node = bucket->entries.pushFront(key, value);

        // Rollback incase of inseration refusal
        if (!index_.insert(key, EntryLocation(node, bucket)))
        {
            bucket->entries.erase(node);
            if (bucket->entries.empty())
                deleteEmptyBucket(bucket);
            return;
        }

        ++size_;
        updateMinimumFrequency();
    }


    // Remove an entry. Returns false when the key is absent.

    bool remove(const std::string &key)
    {
        EntryLocation *location = index_.find(key);
        if (location == nullptr || location->entryNode == nullptr ||
            location->bucket == nullptr)
            return false;

        FrequencyBucket *bucket = location->bucket;
        DoublyLinkedList::Node *node = location->entryNode;

        index_.remove(key);
        bucket->entries.erase(node);
        --size_;

        if (bucket->entries.empty())
            deleteEmptyBucket(bucket);

        return true;
    }

    bool contains(const std::string &key) const
    {
        return index_.contains(key);
    }

    std::size_t size() const { return size_; }
    std::size_t capacity() const { return capacity_; }

    std::size_t minFrequency() const { return minFrequency_; }

    std::size_t frequencyOf(const std::string &key) const
    {
        const EntryLocation *location = index_.find(key);
        if (location == nullptr || location->bucket == nullptr)
            return 0;
        return location->bucket->frequency;
    }

    // Delete all entries and buckets.

    void clear()
    {
        index_.clear();

        FrequencyBucket *current = firstBucket_;
        while (current != nullptr)
        {
            FrequencyBucket *next = current->next;
            delete current;
            current = next;
        }

        firstBucket_ = nullptr;
        size_ = 0;
        minFrequency_ = 0;
    }

    // Expensive consistency check for tests/debugging; not part of normal
    // cache operations. It verifies bucket ordering, list ownership, index
    // pointers, entry count, and minimum-frequency tracking.
    
    bool validate() const
    {
        if (size_ != index_.size() || size_ > capacity_)
            return false;

        if (size_ == 0)
            return firstBucket_ == nullptr && minFrequency_ == 0 && index_.empty();

        if (firstBucket_ == nullptr || firstBucket_->prev != nullptr ||
            minFrequency_ != firstBucket_->frequency)
            return false;

        std::size_t countedEntries = 0;
        std::size_t previousFrequency = 0;
        const FrequencyBucket *previousBucket = nullptr;

        for (const FrequencyBucket *bucket = firstBucket_;
             bucket != nullptr; bucket = bucket->next)
        {
            if (bucket->prev != previousBucket || bucket->frequency == 0 ||
                bucket->frequency <= previousFrequency || bucket->entries.empty())
                return false;

            std::size_t bucketCount = 0;
            DoublyLinkedList::Node *previousNode = nullptr;

            for (DoublyLinkedList::Node *node = bucket->entries.head();
                 node != nullptr; node = node->next)
            {
                if (node->prev != previousNode || node->owner != &bucket->entries)
                    return false;

                const EntryLocation *location = index_.find(node->key);
                if (location == nullptr || location->entryNode != node ||
                    location->bucket != bucket)
                    return false;

                previousNode = node;
                ++bucketCount;
            }

            if (previousNode != bucket->entries.tail() ||
                bucketCount != bucket->entries.size())
                return false;

            countedEntries += bucketCount;
            previousFrequency = bucket->frequency;
            previousBucket = bucket;
        }

        return countedEntries == size_;
    }
};

#endif
