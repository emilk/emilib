rm *.bin
touch *.cpp

g++ --std=c++14 -Wall -I .. -I . tests.cpp -o tests.bin
./tests.bin
