#include "PartitionTableParser.h"
#include "DrivesInfo.h"
#include "MFTParser.h"

#include <clocale>
#include <iostream>

int main(int argc, char** argv)
{
	try
	{
		PartitionTableParser parser;
		parser.parse();

		try
		{
			ntfs::DrivesInfo di(parser.getLogicalDrives());
			di.getDrivesInfo();

			auto pDrivesMft = di.getDrivesMFT();

			for (auto it = pDrivesMft->cbegin(); it != pDrivesMft->cend(); ++it)
			{
				setlocale(LC_ALL, "Russian");

				ntfs::MFTParser mftParser(*it);
				mftParser.readFirstRecord();

				mftParser.findVolumeInformation();
				ntfs::VolumeInfo vi = mftParser.getVolumeInfo();

				std::cout << "\n" << "Volume name = " << vi.m_VolumeLabel << "\n";
				std::cout << "NTFS main version = " << (int)vi.m_VolInfoAttr.m_cMainVersion << "\n";
				std::cout << "NTFS additional version = " << (int)vi.m_VolInfoAttr.m_cAdditionalVersion << "\n";
			}
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