#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include "hash_table.hpp"
using namespace std;

/*
    Indexed min-heap of (key, expiry) entries, ordered by expiry.

    A plain heap can only remove the root quickly. To remove or re-time an
    arbitrary key in O(log n), we keep a HashTable that maps every key to its
    current index in the heap array.

    The rule that keeps invariant 2 true: every movement of an element goes
    through swapEntries(), which updates both array slots and both pos entries
    in one place. Nothing else writes to data_ or pos_ for existing entries.
*/

class ExpiryHeap
{
public:
    struct Entry
    {
        string key;
        uint64_t expiry;
    };

private:
    Entry *data_;
    size_t size_;
    size_t capacity_;
    HashTable<size_t> pos_;

    void grow()
    {
        const size_t newCapaacity = capacity_ * 2;
        Entry *replacement = new Entry[newCapaacity];
        for (size_t i = 0; i < size_; i++)
        {
            replacement[i] = data_[i];
        }
        delete[] data_;
        data_ = replacement;
        capacity_ = newCapaacity;
    }

    // The ONLY place two elements trade slots. Updates both array slots and
    // both pos entries, so the two can never disagree.
    void swapEntries(size_t a, size_t b)
    {
        if (a == b)
            return;
        Entry temp = data_[a];
        data_[a] = data_[b];
        data_[b] = temp;
        *pos_.find(data_[a].key) = a;
        *pos_.find(data_[b].key) = b;
    }

    // Move the element at i toward the root while it is smaller than its
    // parent. Returns where it ended up. Strict < so equal expiries stay put.
    size_t siftUp(size_t i)
    {
        while (i > 0)
        {
            size_t parent = (i - 1) / 2;
            if (!(data_[i].expiry < data_[parent].expiry))
                break;
            swapEntries(i, parent);
            i = parent;
        }
        return i;
    }

    // Move the element at i toward the leaves while a child is smaller.
    void siftDown(size_t i)
    {
        while (true)
        {
            size_t left = 2 * i + 1;
            size_t right = 2 * i + 2;
            size_t smallest = i;
            if (left < size_ && data_[left].expiry < data_[smallest].expiry)
                smallest = left;
            if (right < size_ && data_[right].expiry < data_[smallest].expiry)
                smallest = right;
            if (smallest == i)
                break;
            swapEntries(i, smallest);
            i = smallest;
        }
    }

    // The element at i may now be too small or too large for its place.
    // It can only need one direction: try up, and if it did not move, try down.
    void restore(size_t i)
    {
        if (siftUp(i) == i)
            siftDown(i);
    }

public:
    ExpiryHeap(size_t initialCapacity = 8)
    {
        size_ = 0;
        capacity_ = initialCapacity < 1 ? 1 : initialCapacity;
        data_ = new Entry[capacity_];
    }

    ~ExpiryHeap() { delete[] data_; }

    ExpiryHeap(const ExpiryHeap &) = delete;
    ExpiryHeap &operator=(const ExpiryHeap &) = delete;

    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    bool contains(const std::string &key) const { return pos_.contains(key); }

    // Expiry of a key. Throws if the key is missing, so check contains() first.
    uint64_t expiryOf(const std::string &key) const
    {
        const size_t *index = pos_.find(key);
        if (index == 0)
            throw std::runtime_error("expiryOf on missing key");
        return data_[*index].expiry;
    }

    const Entry &peek() const
    {
        if (empty())
            throw runtime_error("peek on empty ExpiryHeap");
        return data_[0];
    }

    // Insert a new key, or change the expiry of an existing one.
    // Returns true if the key was new, false if it was updated.
    bool upsert(const string &key, uint64_t expiry)
    {
        size_t *index = pos_.find(key);
        if (index != 0)
        {
            size_t i = *index;
            data_[i].expiry = expiry;
            restore(i); // may need to go up or down
            return false;
        }

        if (size_ == capacity_)
            grow();
        size_t i = size_++;
        data_[i].key = key;
        data_[i].expiry = expiry;
        pos_.insert(key, i); // must exist before siftUp calls swapEntries
        siftUp(i);
        return true;
    }

    // Remove and return the entry with the smallest expiry.
    Entry popMin()
    {
        if (empty())
            throw std::runtime_error("popMin on empty ExpiryHeap");
        Entry result = data_[0];
        remove(result.key);
        return result;
    }

    // Remove a key. Returns false if it was not there.
    bool remove(const string &key)
    {
        size_t *index = pos_.find(key);
        if (index == 0)
            return false;

        size_t i = *index;
        size_t last = size_ - 1;

        // Move the victim to the last slot (a no-op if it already is there),
        // then drop it. swapEntries keeps pos correct for the moved element.
        swapEntries(i, last);
        pos_.remove(key);
        --size_;

        // If the victim was last, nothing moved into i and we are done.
        // Otherwise the former last element now sits at i and may be out of place.
        if (i != last)
            restore(i);
        return true;
    }

    // Checks both invariants. Intended for tests and debugging, O(n).
    bool validate() const
    {
        if (pos_.size() != size_)
            return false; // stale or missing pos entries
        for (std::size_t i = 0; i < size_; ++i)
        {
            const std::size_t *index = pos_.find(data_[i].key);
            if (index == 0 || *index != i)
                return false;
            if (i > 0 && data_[i].expiry < data_[(i - 1) / 2].expiry)
                return false;
        }
        return true;
    }
};