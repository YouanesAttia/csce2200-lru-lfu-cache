#include <iostream>
#include <cassert>
#include <string>
#include "doubly_linked_list.hpp"

using namespace std;

// TEST 1: Empty list and null operations
void testEmptyList()
{
    DoublyLinkedList list;

    assert(list.empty());
    assert(list.size() == 0);
    assert(list.head() == nullptr);
    assert(list.tail() == nullptr);

    assert(list.detachFront() == nullptr);
    assert(list.detachBack() == nullptr);

    list.moveToFront(nullptr);
    list.erase(nullptr);
    list.clear();

    assert(list.empty());
    assert(list.size() == 0);

    cout << "PASS: Empty list and null operations" << endl;
}

// TEST 2: Inserting nodes and checking pointers
void testInsertions()
{
    DoublyLinkedList list;

    auto* A = list.pushFront("A", "Apple");

    assert(list.size() == 1);
    assert(!list.empty());
    assert(list.head() == A);
    assert(list.tail() == A);

    assert(A->key == "A");
    assert(A->data == "Apple");
    assert(A->prev == nullptr);
    assert(A->next == nullptr);
    assert(A->owner == &list);

    auto* B = list.pushFront("B", "Banana");
    auto* C = list.pushFront("C", "Cherry");

    // Expected list: C <-> B <-> A

    assert(list.size() == 3);
    assert(list.head() == C);
    assert(list.tail() == A);

    assert(C->prev == nullptr);
    assert(C->next == B);

    assert(B->prev == C);
    assert(B->next == A);

    assert(A->prev == B);
    assert(A->next == nullptr);

    assert(A->owner == &list);
    assert(B->owner == &list);
    assert(C->owner == &list);

    cout << "PASS: Insertions, values, links, and ownership" << endl;
}

// TEST 3: Moving nodes to the front
void testMoveToFront()
{
    DoublyLinkedList list;

    auto* A = list.pushFront("A", "Apple");
    auto* B = list.pushFront("B", "Banana");
    auto* C = list.pushFront("C", "Cherry");

    // Initial list: C <-> B <-> A

    list.moveToFront(B);

    // Expected list: B <-> C <-> A

    assert(list.head() == B);
    assert(list.tail() == A);
    assert(list.size() == 3);

    assert(B->prev == nullptr);
    assert(B->next == C);
    assert(C->prev == B);
    assert(C->next == A);
    assert(A->prev == C);

    // Move the tail to the front
    list.moveToFront(A);

    // Expected list: A <-> B <-> C

    assert(list.head() == A);
    assert(list.tail() == C);
    assert(list.size() == 3);

    assert(A->next == B);
    assert(B->prev == A);
    assert(B->next == C);
    assert(C->prev == B);

    // Moving the head again should change nothing
    list.moveToFront(A);

    assert(list.head() == A);
    assert(list.tail() == C);
    assert(list.size() == 3);

    cout << "PASS: Moving middle/tail/head to front" << endl;
}

// TEST 4: Detaching nodes
void testDetach()
{
    DoublyLinkedList list;

    auto* A = list.pushFront("A", "Apple");
    auto* B = list.pushFront("B", "Banana");
    auto* C = list.pushFront("C", "Cherry");

    // Initial list: C <-> B <-> A

    auto* removedFront = list.detachFront();

    // Expected list: B <-> A

    assert(removedFront == C);
    assert(removedFront->prev == nullptr);
    assert(removedFront->next == nullptr);
    assert(removedFront->owner == nullptr);

    assert(list.head() == B);
    assert(list.tail() == A);
    assert(list.size() == 2);

    delete removedFront;

    auto* removedBack = list.detachBack();

    // Expected list: B

    assert(removedBack == A);
    assert(removedBack->prev == nullptr);
    assert(removedBack->next == nullptr);
    assert(removedBack->owner == nullptr);

    assert(list.head() == B);
    assert(list.tail() == B);
    assert(list.size() == 1);

    delete removedBack;

    // Detach the final node
    auto* lastNode = list.detachFront();

    assert(lastNode == B);
    assert(lastNode->owner == nullptr);

    assert(list.empty());
    assert(list.head() == nullptr);
    assert(list.tail() == nullptr);
    assert(list.size() == 0);

    delete lastNode;

    cout << "PASS: Detaching front, back, and final node" << endl;
}

// TEST 5: Erasing nodes
void testErase()
{
    DoublyLinkedList list;

    auto* A = list.pushFront("A", "Apple");
    auto* B = list.pushFront("B", "Banana");
    auto* C = list.pushFront("C", "Cherry");

    // Initial list: C <-> B <-> A

    // Erase middle node
    list.erase(B);

    // Expected list: C <-> A

    assert(list.size() == 2);
    assert(list.head() == C);
    assert(list.tail() == A);

    assert(C->next == A);
    assert(A->prev == C);

    // Erase head
    list.erase(C);

    // Expected list: A

    assert(list.size() == 1);
    assert(list.head() == A);
    assert(list.tail() == A);

    assert(A->prev == nullptr);
    assert(A->next == nullptr);

    // Erase final node
    list.erase(A);

    assert(list.empty());
    assert(list.size() == 0);
    assert(list.head() == nullptr);
    assert(list.tail() == nullptr);

    cout << "PASS: Erasing middle, head, and final node" << endl;
}

// TEST 6: Clearing and reusing the list
void testClear()
{
    DoublyLinkedList list;

    list.pushFront("A", "Apple");
    list.pushFront("B", "Banana");
    list.pushFront("C", "Cherry");

    assert(list.size() == 3);

    list.clear();

    assert(list.empty());
    assert(list.size() == 0);
    assert(list.head() == nullptr);
    assert(list.tail() == nullptr);

    // Clear an already empty list
    list.clear();

    assert(list.empty());

    // Reuse the list after clearing
    auto* D = list.pushFront("D", "Date");

    assert(list.size() == 1);
    assert(list.head() == D);
    assert(list.tail() == D);
    assert(D->owner == &list);

    list.clear();

    assert(list.empty());

    cout << "PASS: Clearing and reusing the list" << endl;
}

// TEST 7: Checking node ownership
void testOwnership()
{
    DoublyLinkedList list1;
    DoublyLinkedList list2;

    auto* A = list1.pushFront("A", "Apple");
    auto* B = list2.pushFront("B", "Banana");

    assert(A->owner == &list1);
    assert(B->owner == &list2);

    // Try moving a node belonging to another list
    list1.moveToFront(B);

    assert(list1.head() == A);
    assert(list2.head() == B);

    assert(list1.size() == 1);
    assert(list2.size() == 1);

    // Try erasing a node belonging to another list
    list1.erase(B);

    assert(list1.size() == 1);
    assert(list2.size() == 1);

    assert(B->owner == &list2);

    // Detach A from list1
    auto* detached = list1.detachFront();

    assert(detached == A);
    assert(detached->owner == nullptr);

    // Try moving a detached node
    list2.moveToFront(detached);

    assert(list2.head() == B);
    assert(list2.size() == 1);

    // Try erasing a detached node
    list2.erase(detached);

    assert(list2.size() == 1);

    // Detached node still exists, so delete it
    delete detached;

    cout << "PASS: Wrong-owner and detached-node protection" << endl;
}

int main()
{
    testEmptyList();
    testInsertions();
    testMoveToFront();
    testDetach();
    testErase();
    testClear();
    testOwnership();

    cout << "\nAll 7 test groups passed!" << endl;

    return 0;
}