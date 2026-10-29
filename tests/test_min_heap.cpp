#include <iostream>
#include <cassert>
#include <stdexcept>
#include "../src/structures/MinHeap.hpp"
using namespace std;

void test_empty_heap()
{
    MinHeap heap;
    assert(heap.empty() == true);
    assert(heap.size() == 0);
    bool caught_peek_exception = false;
    try
    {
        heap.peek();
    }
    catch (const std::runtime_error &e)
    {
        caught_peek_exception = true;
    }
    assert(caught_peek_exception && "peek() should throw runtime_error when empty");

    bool caught_pop_exception = false;
    try
    {
        heap.pop();
    }
    catch (const std::runtime_error &e)
    {
        caught_pop_exception = true;
    }
    assert(caught_pop_exception && "pop() should throw runtime_error when empty");
    cout << "[SUCCESS] test_empty_heap" << endl;
}

void test_single_element()
{
    MinHeap heap;
    heap.push(42);

    assert(heap.empty() == false);
    assert(heap.size() == 1);
    assert(heap.peek() == 42);

    int val = heap.pop();
    assert(val == 42);
    assert(heap.empty() == true);
    assert(heap.size() == 0);

    cout << "[SUCCESS] test_single_element" << endl;
}

void test_min_heap_property()
{
    MinHeap heap;
    heap.push(10);
    heap.push(4);
    heap.push(15);
    heap.push(20);
    heap.push(0);
    assert(heap.size() == 5);
    assert(heap.peek() == 0);
    cout << "[SUCCESS] test_min_heap_property" << endl;
}

void test_pop_order()
{
    MinHeap heap;
    int values[] = {8, 3, 10, 1, 6, 4, 7, 14, 13};
    for (int v : values)
    {
        heap.push(v);
    }
    assert(heap.pop() == 1);
    assert(heap.pop() == 3);
    assert(heap.pop() == 4);
    assert(heap.pop() == 6);
    assert(heap.pop() == 7);
    assert(heap.pop() == 8);
    assert(heap.pop() == 10);
    assert(heap.pop() == 13);
    assert(heap.pop() == 14);
    assert(heap.empty() == true);

    cout << "[SUCCESS] test_pop_order" << endl;
}

void test_capacity_growth()
{
    MinHeap heap(2);
    const int num_elements = 1000;
    for (int i = num_elements; i > 0; --i)
    {
        heap.push(i);
    }
    assert(heap.size() == num_elements);
    assert(heap.peek() == 1);
    for (int i = 1; i <= num_elements; ++i)
    {
        assert(heap.pop() == i);
    }
    assert(heap.empty() == true);
    cout << "[SUCCESS] test_capacity_growth" << endl;
}

void test_duplicate_values()
{
    MinHeap heap;
    heap.push(5);
    heap.push(5);
    heap.push(5);
    heap.push(1);
    assert(heap.size() == 4);
    assert(heap.pop() == 1);
    assert(heap.pop() == 5);
    assert(heap.pop() == 5);
    assert(heap.pop() == 5);
    assert(heap.empty() == true);
    cout << "[SUCCESS] test_duplicate_values" << endl;
}

int main()
{
    test_empty_heap();
    test_single_element();
    test_min_heap_property();
    test_pop_order();
    test_capacity_growth();
    test_duplicate_values();
    cout << "ALL TESTS PASSED!" << endl;
    return 0;
}