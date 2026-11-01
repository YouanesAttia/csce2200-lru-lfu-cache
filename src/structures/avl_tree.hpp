#include <algorithm>
#include <cstddef>
#include <functional>
#include <utility>

// Generic AVL search tree. Keys are unique according to Compare.
template <typename Key, typename Value, typename Compare = std::less<Key>>
class AVLTree
{
private:
    struct Node
    {
        Key key;
        Value value;
        int height;
        Node *left;
        Node *right;

        Node(const Key &k, const Value &v)
            : key(k), value(v), height(1), left(nullptr), right(nullptr) {}
    };

    Node *root_;
    std::size_t size_;
    Compare compare_;

    static int nodeHeight(const Node *n) { return n ? n->height : 0; }

    static void updateHeight(Node *n)
    {
        n->height = 1 + std::max(nodeHeight(n->left), nodeHeight(n->right));
    }

    static int balanceFactor(const Node *n)
    {
        return n ? nodeHeight(n->left) - nodeHeight(n->right) : 0;
    }

    static Node *rotateRight(Node *y)
    {
        Node *x = y->left;
        Node *middle = x->right;
        x->right = y;
        y->left = middle;
        updateHeight(y);
        updateHeight(x);
        return x;
    }

    static Node *rotateLeft(Node *x)
    {
        Node *y = x->right;
        Node *middle = y->left;
        y->left = x;
        x->right = middle;
        updateHeight(x);
        updateHeight(y);
        return y;
    }

    static Node *rebalance(Node *n)
    {
        if (!n)
            return nullptr;
        updateHeight(n);
        const int balance = balanceFactor(n);

        if (balance > 1)
        {
            if (balanceFactor(n->left) < 0)
                n->left = rotateLeft(n->left);
            return rotateRight(n);
        }
        if (balance < -1)
        {
            if (balanceFactor(n->right) > 0)
                n->right = rotateRight(n->right);
            return rotateLeft(n);
        }
        return n;
    }

    Node *insertNode(Node *n, const Key &key, const Value &value,
                     bool &inserted)
    {
        if (!n)
        {
            inserted = true;
            return new Node(key, value);
        }
        if (compare_(key, n->key))
            n->left = insertNode(n->left, key, value, inserted);
        else if (compare_(n->key, key))
            n->right = insertNode(n->right, key, value, inserted);
        else
        {
            n->value = value;
            return n;
        }
        return rebalance(n);
    }

    static Node *minimum(Node *n)
    {
        if (!n)
            return nullptr;
        while (n->left)
            n = n->left;
        return n;
    }

    Node *eraseNode(Node *n, const Key &key, bool &erased)
    {
        if (!n)
            return nullptr;

        if (compare_(key, n->key))
            n->left = eraseNode(n->left, key, erased);
        else if (compare_(n->key, key))
            n->right = eraseNode(n->right, key, erased);
        else
        {
            erased = true;
            if (!n->left || !n->right)
            {
                Node *child = n->left ? n->left : n->right;
                delete n;
                return child;
            }

            Node *successor = minimum(n->right);
            n->key = successor->key;
            n->value = successor->value;
            bool successorErased = false;
            n->right = eraseNode(n->right, successor->key, successorErased);
        }
        return rebalance(n);
    }

    static void destroy(Node *n)
    {
        if (!n)
            return;
        destroy(n->left);
        destroy(n->right);
        delete n;
    }

    Node *findNode(const Key &key) const
    {
        Node *current = root_;
        while (current)
        {
            if (compare_(key, current->key))
                current = current->left;
            else if (compare_(current->key, key))
                current = current->right;
            else
                return current;
        }
        return nullptr;
    }

public:
    AVLTree() : root_(nullptr), size_(0), compare_() {}
    ~AVLTree() { clear(); }

    AVLTree(const AVLTree &) = delete;
    AVLTree &operator=(const AVLTree &) = delete;

    // Returns true for a new key; updates the value and returns false for a duplicate.
    bool insert(const Key &key, const Value &value)
    {
        bool inserted = false;
        root_ = insertNode(root_, key, value, inserted);
        if (inserted)
            ++size_;
        return inserted;
    }

    bool remove(const Key &key)
    {
        bool erased = false;
        root_ = eraseNode(root_, key, erased);
        if (erased)
            --size_;
        return erased;
    }

    bool contains(const Key &key) const { return findNode(key) != nullptr; }

    // Copies the value into out. Returns false when key is absent.
    bool find(const Key &key, Value &out) const
    {
        Node *n = findNode(key);
        if (!n)
            return false;
        out = n->value;
        return true;
    }

    // Returns the smallest key/value without modifying the tree.
    bool getMin(Key &keyOut, Value &valueOut) const
    {
        Node *n = minimum(root_);
        if (!n)
            return false;
        keyOut = n->key;
        valueOut = n->value;
        return true;
    }

    // Removes and returns the smallest key/value.
    bool popMin(Key &keyOut, Value &valueOut)
    {
        Node *n = minimum(root_);
        if (!n)
            return false;
        keyOut = n->key;
        valueOut = n->value;
        const bool erased = remove(n->key);
        return erased;
    }

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    void clear()
    {
        destroy(root_);
        root_ = nullptr;
        size_ = 0;
    }
};