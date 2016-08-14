g++ --std=c++14 -Wall -I .. -I . coroutine_example.cpp -o coroutine_example
g++ --std=c++14 -Wall -I .. -I . strprintf_example.cpp -o strprintf_example
# g++ --std=c++14 -Wall -I .. -I . -O2 rw_mutex_benchmark.cpp -o rw_mutex_benchmark
g++-mp-6 --std=c++17 -Wall -I .. -I . -O2 rw_mutex_benchmark.cpp -o rw_mutex_benchmark
