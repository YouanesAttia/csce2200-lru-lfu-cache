https://www.geeksforgeeks.org/system-design/lru-cache-implementation/

HashMap + Doubly Linked List (Optimal): Uses a hash map to locate cache entries in O(1) time and a doubly linked list to maintain usage order. Both get() and put() operations run in O(1) time, making this the standard and most efficient implementation of an LRU cache.
