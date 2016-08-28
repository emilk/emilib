rm *.bin
touch *.cpp

g++-mp-6 --std=c++17 -Wall -I .. -I . -O2 rw_mutex_benchmark.cpp -o rw_mutex_benchmark.bin
g++ --std=c++14 -Wall -I .. -I . -O2 hash_cache_benchmark.cpp -o hash_cache_benchmark.bin
