#include "PartitionTableParser.h"

#include <iostream>

int main(int argc, char** argv)
{
	try
	{
		PartitionTableParser parser;
		parser.parse();
	}
	catch (std::runtime_error error)
	{
		std::cout << error.what() << std::endl;
	}

	system("pause");
	return 0;
}