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

class DoublyLinkedList
{
public:
    struct Node
    { // Structure of the nodes of the list.
        string key;
        string data;
        Node *prev;
        Node *next;
        DoublyLinkedList *owner;

        Node(const string &k, const string &d) : key(k), data(d), prev(nullptr), next(nullptr), owner(nullptr) {}
        // DLLNode constructor, assigns values to key & data, and sets prev & next = NULL.
    };

    DoublyLinkedList() : head_(nullptr), tail_(nullptr), size_(0) {}
    /* DoublyLinkedList constructor
    initializes pointers to first and last nodes and sets them = NULL
    and sets size of the list = 0. */

    ~DoublyLinkedList() { clear(); }
    // DoublyLinkedList destructor, deletes the list as well as all of its nodes.

    DoublyLinkedList(const DoublyLinkedList &) = delete;
    DoublyLinkedList &operator=(const DoublyLinkedList &) = delete;
    // These lines prevent the list from being copied.

    Node *head() const { return head_; }
    Node *tail() const { return tail_; }
    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    // These functions allow retrieval of information about the list.

    Node *pushFront(const string &key, const string &data)
    { // Inserts a new node at the front of the list.
        Node *node = new Node(key, data);
        attachFront(node);
        return node;
    }

    Node *detachFront()
    { // Detaches and returns the front node without deleting it.
        return detach(head_);
    }

    Node *detachBack()
    { // Detaches and returns the back node without deleting it.
        return detach(tail_);
    }

    void moveToFront(Node *node)
    { // Moves an existing node to the front.
        if (!node || node->owner != this || node == head_)
            return;
        detach(node);
        attachFront(node);
    }

    void erase(Node *node)
    { // Deletes the given node if it belongs to this list.
        if (!node || node->owner != this)
            return;
        detach(node);
        delete node;
    }

    void clear()
    { // Deletes every node in the list.
        Node *current = head_;
        while (current)
        {
            Node *next = current->next;
            current->owner = nullptr;
            delete current;
            current = next;
        }
        head_ = nullptr;
        tail_ = nullptr;
        size_ = 0;
    }

private:
    Node *head_;
    Node *tail_;
    size_t size_;

    void attachFront(Node *node)
    { // Attaches a node to the front of the list.
        node->prev = nullptr;
        node->next = head_;
        node->owner = this;
        if (head_)
            head_->prev = node;
        else
            tail_ = node;
        head_ = node;
        ++size_;
    }

    Node *detach(Node *node)
    { // Detaches a node from the list.
        if (!node || node->owner != this)
            return nullptr;
        if (node->prev)
            node->prev->next = node->next;
        else
            head_ = node->next;

        if (node->next)
            node->next->prev = node->prev;
        else
            tail_ = node->prev;

        node->prev = nullptr;
        node->next = nullptr;
        node->owner = nullptr;
        --size_;
        return node;
    }
};

#endif
