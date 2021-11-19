﻿
#define _CRT_SECURE_NO_WARNINGS

#ifdef _DEBUG 
#else
#include "mimalloc-new-delete.h"
#endif

#include <iostream>
#include <string>
#include <ctime>

#include "claujson.h"


int main(int argc, char* argv[])
{
	claujson::UserType ut;
	
	int a = clock();
	int x = claujson::Parse(argv[1], 0, &ut);
	int b = clock();
	std::cout << "total " << b - a << "ms\n";

	claujson::LoadData::_save(std::cout, &ut);
//	claujson::LoadData::save("output.json", ut);
	return x;
}
