rm *.bin
touch *.cpp

g++-mp-6 --std=c++17 -Wall -I .. -I . -O2 rw_mutex_benchmark.cpp -o rw_mutex_benchmark.bin &

g++ --std=c++14 -Wall -I .. -I . -O2 hash_cache_benchmark.cpp -o hash_cache_benchmark_clang_apple.bin &
clang++-mp-3.7 --std=c++14 -Wall -I .. -I . -O2 hash_cache_benchmark.cpp -o hash_cache_benchmark_clang_37.bin &
g++-mp-5 --std=c++14 -Wall -I .. -I . -O2 hash_cache_benchmark.cpp -o hash_cache_benchmark_gcc5.bin &
g++-mp-6 --std=c++14 -Wall -I .. -I . -O2 hash_cache_benchmark.cpp -o hash_cache_benchmark_gcc6.bin &

wait
