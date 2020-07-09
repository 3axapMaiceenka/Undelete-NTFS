#include "PartitionTableParser.h"
#include "DrivesInfo.h"

#include <iostream>

int main(int argc, char** argv)
{
	try
	{
		PartitionTableParser parser;
		parser.parse();

		try
		{
			DrivesInfo di(parser.getLogicalDrives());
			di.getDrivesInfo();
		}
		catch (std::logic_error error)
		{
			std::cout << error.what();
		}
	}
	catch (std::runtime_error error)
	{
		std::cout << error.what() << std::endl;
	}

	system("pause");
	return 0;
}