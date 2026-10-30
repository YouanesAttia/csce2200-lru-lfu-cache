#ifndef HASH_TABLE_HPP
#define HASH_TABLE_HPP

#include <cstddef>
#include <string>

/*
    Hash table that maps a string key to an int.

    In the cache the int is where the entry for that key is stored, so this is
    what lets us check whether a key is already cached without searching
    through everything the cache is holding.

    Collisions are handled with separate chaining: each bucket is a linked list,
    and keys that hash to the same bucket all go in the same list.
    Average case O(1), worst case O(n) when every key collides.
*/
class HashTable
{
private:
    // One link in a bucket's chain. The key is stored too, because several
    // different keys can end up in the same bucket.
    struct Node
    {
        std::string key;
        int value;
        Node *next;

        Node(const std::string &k, int v, Node *n)
        {
            key = k;
            value = v;
            next = n;
        }
    };

    Node **buckets_;  // array of chain heads, one per bucket
    size_t capacity_; // how many buckets
    size_t size_;     // how many keys

    // Standard polynomial string hash, the same one Java uses for
    // String.hashCode. 31 because it is odd, an even multiplier would keep
    // pushing zeros into the low bits. size_t is unsigned so it is allowed to
    // overflow here, a signed int would be undefined behaviour.
    size_t hashKey(const std::string &key) const
    {
        size_t h = 0;
        for (size_t i = 0; i < key.size(); ++i)
            h = h * 31 + (unsigned char)key[i]; // cast so bytes over 127 stay positive
        return h % capacity_;
    }

    // Find which node holds a key, or 0 if there is none. insert, update, find
    // and contains all call this so the search is only written once.
    Node *findNode(const std::string &key) const
    {
        Node *current = buckets_[hashKey(key)];
        while (current != 0)
        {
            if (current->key == key)
                return current;
            current = current->next;
        }
        return 0;
    }

    // Double the bucket array and move every node into its new bucket. The
    // nodes are reused, only the links between them change.
    void grow()
    {
        const size_t newCapacity = capacity_ * 2;
        Node **replacement = new Node *[newCapacity];
        for (size_t i = 0; i < newCapacity; ++i)
            replacement[i] = 0; // new[] does not zero them, so we do it

        // Swap these in before the loop, hashKey divides by capacity_ and has
        // to use the new one.
        Node **old = buckets_;
        const size_t oldCapacity = capacity_;
        buckets_ = replacement;
        capacity_ = newCapacity;

        for (size_t i = 0; i < oldCapacity; ++i)
        {
            Node *current = old[i];
            while (current != 0)
            {
                Node *next = current->next; // save it, the line below overwrites it
                size_t bucket = hashKey(current->key);
                current->next = buckets_[bucket];
                buckets_[bucket] = current;
                current = next;
            }
        }
        delete[] old; // only the old array, the nodes were moved not copied
    }

public:
    HashTable(size_t initialCapacity = 16)
    {
        capacity_ = initialCapacity < 1 ? 1 : initialCapacity; // never 0, we divide by it
        size_ = 0;
        buckets_ = new Node *[capacity_];
        for (size_t i = 0; i < capacity_; ++i)
            buckets_[i] = 0;
    }

    // Nodes first, then the array. The other way round would throw away the
    // pointers we need to reach the nodes.
    ~HashTable()
    {
        clear();
        delete[] buckets_;
    }

    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    size_t bucketCount() const { return capacity_; }
    double loadFactor() const { return (double)size_ / (double)capacity_; }

    // Adds a key that is not in the table yet. Returns false and changes
    // nothing if it is already there, it does not overwrite. update() is the
    // one that overwrites.
    bool insert(const std::string &key, int value)
    {
        if (findNode(key) != 0)
            return false;

        // Grow once there is more than 0.75 keys per bucket on average, that
        // keeps the chains short enough for a lookup to stay quick.
        if ((double)(size_ + 1) > 0.75 * (double)capacity_)
            grow();

        // Put the new node at the front of its chain, so we do not have to walk
        // to the end first.
        size_t bucket = hashKey(key); // after grow(), the bucket may have changed
        Node *node = new Node(key, value, buckets_[bucket]);
        buckets_[bucket] = node;
        ++size_;
        return true;
    }

    // Changes the value of a key that is already there. Returns false and adds
    // nothing if the key is missing.
    bool update(const std::string &key, int value)
    {
        Node *node = findNode(key);
        if (node == 0)
            return false;
        node->value = value;
        return true;
    }

    // Pointer to the value, or 0 if the key is not there. No exception like
    // MinHeap::peek, a missing key is normal for a cache and not a mistake.
    // The pointer also lets the caller change the value without searching again.
    int *find(const std::string &key)
    {
        Node *node = findNode(key);
        if (node == 0)
            return 0;
        return &node->value;
    }

    bool contains(const std::string &key) const { return findNode(key) != 0; }

    // Removes a key. Returns false if it was not there.
    bool remove(const std::string &key)
    {
        size_t bucket = hashKey(key);
        Node *current = buckets_[bucket];
        Node *previous = 0; // we need the node before it to unlink

        while (current != 0)
        {
            if (current->key == key)
            {
                if (previous == 0)
                    buckets_[bucket] = current->next; // it was the first one
                else
                    previous->next = current->next;
                delete current;
                --size_;
                return true;
            }
            previous = current;
            current = current->next;
        }
        return false;
    }

    // Deletes every node but keeps the bucket array at its current size.
    void clear()
    {
        for (size_t i = 0; i < capacity_; ++i)
        {
            Node *current = buckets_[i];
            while (current != 0)
            {
                Node *next = current->next; // save it before deleting
                delete current;
                current = next;
            }
            buckets_[i] = 0;
        }
        size_ = 0;
    }

    // The two below are only for the tests, the cache does not use them.
    size_t bucketIndex(const std::string &key) const { return hashKey(key); }

    // Longest chain = the most comparisons a lookup could cost right now.
    size_t longestChain() const
    {
        size_t worst = 0;
        for (size_t i = 0; i < capacity_; ++i)
        {
            size_t length = 0;
            for (Node *n = buckets_[i]; n != 0; n = n->next)
                ++length;
            if (length > worst)
                worst = length;
        }
        return worst;
    }
};

#endif
