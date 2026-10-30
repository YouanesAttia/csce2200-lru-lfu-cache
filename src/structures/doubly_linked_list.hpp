#ifndef A6_DOUBLY_LINKED_LIST_H
#define A6_DOUBLY_LINKED_LIST_H

#include <cstddef>
#include <string>

using namespace std;

/*
Doubly linked list of key/data pairs.
The list manages the memory of its nodes.
External code can keep pointers to nodes while those nodes remain valid.
Known nodes can be moved in constant time, O(1).
*/

class DoublyLinkedList {
public:
    struct DLLNode {               // Structure of the nodes of the list.
        string key;
        string data;
        DLLNode* prev;
        DLLNode* next;

        DLLNode(const string& k, const string& d) : key(k), data(d), prev(nullptr), next(nullptr) {}
        // DLLNode constructor, assigns values to key & data, and sets prev & next = NULL.
    };

    DoublyLinkedList() : head(nullptr), tail(nullptr), size(0) {}
    /* DoublyLinkedList constructor
    initializes pointers to first and last nodes and sets them = NULL
    and sets size of the list = 0. */

    ~DoublyLinkedList() { clear(); }
    // DoublyLinkedList destructor, deletes the list as well as all of its nodes.

    DoublyLinkedList(const DoublyLinkedList&) = delete;
    DoublyLinkedList& operator=(const DoublyLinkedList&) = delete;
    // These lines prevent the list from being copied.

    DLLNode* getHeadNode() const { return head; }
    DLLNode* getTailNode() const { return tail; }
    size_t getDLLSize() const { return size; }
    bool DLLisEmpty() const { return size == 0; }
    // These functions allow retrieval of information about the list.

    DLLNode* pushFront(const string& key, const string& data) { // Inserts a new node at the front of the list.
        DLLNode* node = new DLLNode(key, data);
        attachFront(node);
        return node;
    }

    DLLNode* detachFront() {            // Detaches and returns the front node without deleting it.
        if (!head) return nullptr;
        DLLNode* node = head;
        detach(node);
        return node;
    }

    DLLNode* detachBack() {             // Detaches and returns the back node without deleting it.
        if (!tail) return nullptr;
        DLLNode* node = tail;
        detach(node);
        return node;
    }

    void moveToFront(DLLNode* node) {   // Moves an existing node to the front.
        if (!node || node == head) return;
        detach(node);
        attachFront(node);
    }

    void erase(DLLNode* node) {         // Deletes the given node if it belongs to this list.
        if (!node) return;
        detach(node);
        delete node;
    }

    void clear() {                      //Deletes every node in the list.
        DLLNode* current = head;
        while (current) {
            DLLNode* next = current->next;
            delete current;
            current = next;
        }
        head = tail = nullptr;
        size = 0;
    }

private:
    DLLNode* head;
    DLLNode* tail;
    size_t size;

    void attachFront(DLLNode* node) { //Attaches a node to the front of the list.
        node->prev = nullptr;
        node->next = head;
        if (head) head->prev = node;
        else tail = node;
        head = node;
        ++size;
    }

    void detach(DLLNode* node) {    //Detaches a node from the list.
        if (node->prev) node->prev->next = node->next;
        else head = node->next;

        if (node->next) node->next->prev = node->prev;
        else tail = node->prev;

        node->prev = nullptr;
        node->next = nullptr;
        --size;
    }
};

#endif
