rm -f *.bin
touch *.cpp

function build
{
	g++ --std=c++14 -Wall -I .. -I . $1.cpp -o $1.bin
}

build coroutine_example
build strprintf_example
