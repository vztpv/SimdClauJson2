
#define _CRT_SECURE_NO_WARNINGS

#ifdef _DEBUG 
#else
#include "mimalloc-new-delete.h"
#endif

#include <iostream>
#include <string>
#include <ctime>

#include "simdjson.h"
#include "claujson.h"

int main(int argc, char* argv[])
{
	claujson::UserType ut;
	
	int a = clock();
	claujson::Parse(argv[1], 0, &ut);
	int b = clock();
	std::cout << "total " << b - a << "ms\n";

	//claujson::LoadData::_save(std::cout, &ut);

	return 0;
}
