rm *.bin
touch *.cpp

g++ --std=c++14 -Wall -I .. -I . coroutine_example.cpp -o coroutine_example
g++ --std=c++14 -Wall -I .. -I . strprintf_example.cpp -o strprintf_example
