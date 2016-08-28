#include <emilib/strprintf.cpp>

#include <iostream>

int main()
{
	std::cout << emilib::strprintf("Format float: %.1f", 1.234) << std::endl;
}
