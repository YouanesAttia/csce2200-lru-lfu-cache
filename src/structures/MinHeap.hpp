#include <cstddef>
#include <stdexcept>
using namespace std;

class MinHeap
{
private:
    int *data_;
    size_t size_;
    size_t capacity_;

    // Double the size when the capcity is full to increase the efficiency of storing the data
    void grow()
    {
        const size_t newCapacity = capacity_ * 2;
        int *replacement = new int[newCapacity];
        for (size_t i = 0; i < size_; ++i)
            replacement[i] = data_[i];
        delete[] data_;
        data_ = replacement;
        capacity_ = newCapacity;
    }

    // Swap the values by the undex
    void swapValues(size_t a, size_t b)
    {
        int temp = data_[a];
        data_[a] = data_[b];
        data_[b] = temp;
    }

public:
    MinHeap(size_t initialCapacity = 8)
    {
        size_ = 0;
        capacity_ = initialCapacity < 1 ? 1 : initialCapacity;
        data_ = new int[capacity_];
    }

    ~MinHeap() { delete[] data_; }

    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    int peek() const
    {
        if (empty())
            throw runtime_error("Peek on empty MinHeap");
        return data_[0];
    }

    void push(int value)
    {
        if (size_ == capacity_)
            grow();
        size_t i = size_++;
        data_[i] = value;
        // Bubble the new value up until the parent is no larger
        while (i > 0)
        {
            size_t parent = (i - 1) / 2;
            if (data_[parent] <= data_[i])
                break;
            swapValues(parent, i);
            i = parent;
        }
    }

    int pop()
    {
        if (empty())
            throw runtime_error("Peek on empty MinHeap");
        int result = data_[0];
        data_[0] = data_[--size_];
        size_t i = 0;
        // Bubble the root down by swapping with its smaller child
        while (true)
        {
            size_t left = 2 * i + 1;
            size_t right = 2 * i + 2;
            size_t smallest = i;
            if (left < size_ && data_[left] < data_[smallest])
                smallest = left;
            if (right < size_ && data_[right] < data_[smallest])
                smallest = right;
            if (smallest == i)
                break;
            swapValues(i, smallest);
            i = smallest;
        }
        return result;
    }
};