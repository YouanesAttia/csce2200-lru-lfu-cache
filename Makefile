# Run these from the top folder of the project:
#
#   make         build all the tests
#   make test    build the tests and run them
#   make run     build and start the program
#   make asan    build the tests with the memory checker on and run them
#   make clean   delete everything that was built
#
# Every line below is just the command you would type yourself.
# If any command fails, make stops there.

CXX   = g++
FLAGS = -std=c++11 -Wall -Wextra -g -Isrc/structures
ASAN  = -fsanitize=address,undefined

# Sources that more than one test needs.
STORE   = src/storage/backing_store.cpp
LRU     = src/cache/lru_cache.cpp
MANAGER = src/cache/cache_manager.cpp

# These two tests are not here because they do not compile: both include a
# header that is no longer in the repo.
#   test_min_heap   MinHeap.hpp was replaced by indexed_min_heap.hpp
#   test_avl_tree   avl_tree.hpp was never added
# Delete the tests or point them at the new files, then add them back here.

all:
	mkdir -p bin
	$(CXX) $(FLAGS) -o bin/test_hash_table tests/test_hash_table.cpp
	$(CXX) $(FLAGS) -o bin/test_hash_table_stress tests/test_hash_table_stress.cpp
	$(CXX) $(FLAGS) -o bin/test_cli tests/test_cli.cpp src/app/cli.cpp
	$(CXX) $(FLAGS) -o bin/test_doubly_linked_list tests/test_doubly_linked_list.cpp
	$(CXX) $(FLAGS) -o bin/test_lru_cache tests/test_lru_cache.cpp $(LRU)
	$(CXX) $(FLAGS) -o bin/test_lfu_cache tests/test_lfu_cache.cpp
	$(CXX) $(FLAGS) -o bin/test_backing_store tests/test_backing_store.cpp $(STORE)
	$(CXX) $(FLAGS) -o bin/test_policy_parity tests/test_policy_parity.cpp $(LRU) $(MANAGER) $(STORE)
	$(CXX) $(FLAGS) -o bin/test_integration tests/test_integration.cpp $(LRU) $(MANAGER) $(STORE)

test: all
	./bin/test_hash_table
	./bin/test_hash_table_stress
	./bin/test_cli
	./bin/test_doubly_linked_list
	./bin/test_lru_cache
	./bin/test_lfu_cache
	./bin/test_backing_store
	./bin/test_policy_parity
	./bin/test_integration

run:
	mkdir -p bin
	$(CXX) $(FLAGS) -o bin/cache src/app/main.cpp src/app/cli.cpp $(LRU) $(MANAGER) $(STORE)
	./bin/cache

# The same tests, built with the memory checker switched on. It catches things
# a normal build does not, like using memory after it was freed.
asan:
	mkdir -p bin
	$(CXX) $(FLAGS) $(ASAN) -o bin/test_hash_table tests/test_hash_table.cpp
	$(CXX) $(FLAGS) $(ASAN) -o bin/test_hash_table_stress tests/test_hash_table_stress.cpp
	$(CXX) $(FLAGS) $(ASAN) -o bin/test_cli tests/test_cli.cpp src/app/cli.cpp
	$(CXX) $(FLAGS) $(ASAN) -o bin/test_doubly_linked_list tests/test_doubly_linked_list.cpp
	$(CXX) $(FLAGS) $(ASAN) -o bin/test_lru_cache tests/test_lru_cache.cpp $(LRU)
	$(CXX) $(FLAGS) $(ASAN) -o bin/test_lfu_cache tests/test_lfu_cache.cpp
	$(CXX) $(FLAGS) $(ASAN) -o bin/test_backing_store tests/test_backing_store.cpp $(STORE)
	$(CXX) $(FLAGS) $(ASAN) -o bin/test_policy_parity tests/test_policy_parity.cpp $(LRU) $(MANAGER) $(STORE)
	$(CXX) $(FLAGS) $(ASAN) -o bin/test_integration tests/test_integration.cpp $(LRU) $(MANAGER) $(STORE)
	./bin/test_hash_table
	./bin/test_hash_table_stress
	./bin/test_cli
	./bin/test_doubly_linked_list
	./bin/test_lru_cache
	./bin/test_lfu_cache
	./bin/test_backing_store
	./bin/test_policy_parity
	./bin/test_integration

clean:
	rm -rf bin

# Tells make these are command names, not files to build.
.PHONY: all test run asan clean
