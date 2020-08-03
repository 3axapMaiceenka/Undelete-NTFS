#include "UserInterface.h"
#include "TempFile.h"

#include <algorithm>

ntfs::UserInterface::UserInterface(Controller& controller)
	: m_Controller(controller)
{
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
}

void ntfs::UserInterface::start()
{
	while (1)
	{
		outputVolumesInformation();

		int nVolumeNumber = findDeletedFiles();
		if (nVolumeNumber == -1)
		{
			break;
		}

		int rc;
		while ((rc = undelete(nVolumeNumber)) == 1)
			;

		if (rc == -1)
		{
			break;
		}
	}
}

void ntfs::UserInterface::outputVolumesInformation() const
{
	const auto pDrivesInfo = m_Controller.getDrivesInfo();
	unsigned int i = 0;

	for (const auto& mftInfo : *pDrivesInfo)
	{
		auto pVolumeInfo = m_Controller.getVolumeInfo(i);

		std::cout << i + 1 << ") NTFS volume " << i + 1 << std::endl;
		printVolumeInfo(std::cout, mftInfo, *pVolumeInfo);
		i++;
	}
}

std::ostream& ntfs::UserInterface::printVolumeInfo(std::ostream& os, const MFTInfo& mftInfo, const VolumeInfo& volumeInfo) const
{
	if (volumeInfo.m_pszVolumeLabel)
	{
		std::cout << "Volume label: " << CString(volumeInfo.m_pszVolumeLabel) << std::endl;
	}
	std::cout << "NTFS version: " << (int)volumeInfo.m_VolInfoAttr.m_cMainVersion << std::endl;
	std::cout << "Number of sectors: " << mftInfo.m_ulNumberOfSectors << std::endl;
	std::cout << "Size: " << (mftInfo.m_ulNumberOfSectors * SECTOR_SIZE) / 1000000000.0 << " GB\n";
	std::cout << std::endl;

	return os;
}

std::string ntfs::UserInterface::getInput(std::ostream& os, std::istream& is, const char* pszMessage) const
{
	os << pszMessage;
	std::string input;
	getline(is, input);

	return input;
}

int ntfs::UserInterface::findDeletedFiles()
{
	int volumeNumber = getVolumeNumber();

	if (volumeNumber != -1)
	{
		std::cout << "\nSearching for deleted files... Wait...\n";

		m_Controller.findDeletedFiles((std::size_t)volumeNumber);

		try
		{
			printDeletdFilesInfo(m_Controller.getDeletedFiles((std::size_t)volumeNumber));
		}
		catch (std::runtime_error error)
		{
			std::cout << error.what() << std::endl;
			return -1;
		}
	}

	return volumeNumber;
}

void ntfs::UserInterface::printDeletdFilesInfo(const std::shared_ptr<std::list<DeletedFile>> pDeletedFiles)
{
	TempFile* pTempFile = new TempFile(m_TempFileName, m_TempFileRelativePath, std::fstream::in | std::fstream::out | std::fstream::trunc);

	unsigned int i = 1;
	for (const auto& df : *pDeletedFiles)
	{
		pTempFile->output(i++, ")", df);
	}

	int result = (int)ShellExecuteW(GetDesktopWindow(), L"open", pTempFile->getFilePath().c_str(), NULL, NULL, SW_SHOW);
	if (result < 32)
	{
		delete pTempFile;
		throw std::runtime_error("Can't show temporary file!");
	}

	delete pTempFile;
}

int ntfs::UserInterface::undelete(int nVolumeNumber)
{
	long long int numberOfFile = getNumberOfFile(nVolumeNumber);
	if (numberOfFile == -1) return -1;
	if (numberOfFile == -2) return 0;

	std::string directory = getDirectory();
	if (directory == "exit") return -1;
	if (directory == "back") return 0;
	
	auto deletedFiles = m_Controller.getDeletedFiles(nVolumeNumber);

	std::cout << "Undeleting...\n";

	std::wstring path = std::filesystem::path(directory).native();
	m_Controller.undelete((std::size_t)nVolumeNumber, (std::size_t)numberOfFile, path);

	std::cout << "Done!\n";

	return 1;
}

bool ntfs::UserInterface::isNumber(const std::string& text) const
{
	return std::find_if(text.cbegin(), text.cend(), [](char c) { return !isdigit(c); }) == text.cend();
}

int ntfs::UserInterface::getVolumeNumber() const
{
	int volumeNumber = -1;
	std::string input;

	while (1)
	{
		input = getInput(std::cout, std::cin, "Enter volume number to find deleted files or 'exit' to finish program: ");
		if (input == "exit")
		{
			return -1;
		}

		if (!isNumber(input))
		{
			std::cout << "\nInput Error! Enter volume number (integer number from 1 to " << m_Controller.getNumberOfVolumes() << ")!\n";
			continue;
		}

		volumeNumber = atoi(input.c_str()) - 1;
		if (volumeNumber >= m_Controller.getNumberOfVolumes() || volumeNumber < 0)
		{
			std::cout << "\nInput Error! Enter volume number (integer number from 1 to " << m_Controller.getNumberOfVolumes() << ")!\n";
			continue;
		}

		return volumeNumber;
	}
}

long long int ntfs::UserInterface::getNumberOfFile(int nVolumeNumber) const
{
	std::string input;
	long long int numberOfFile;

	while (1)
	{
		auto input = getInput(std::cout, std::cin, "Enter number of file to undelete, 'exit' to finish programm or 'back' to change volume: ");
		if (input == "exit")
		{
			return -1;
		}
		if (input == "back")
		{
			return -2;
		}

		if (!isNumber(input))
		{
			std::cout << "\nInput Error! Enter number of file (integer number from 1 to " << m_Controller.getDeletedFiles(nVolumeNumber)->size() << ")!\n";
			continue;
		}

		numberOfFile = atoll(input.c_str()) - 1;
		if (numberOfFile < 0 || numberOfFile >= m_Controller.getDeletedFiles(nVolumeNumber)->size())
		{
			std::cout << "\nInput Error! Enter number of file (integer number from 1 to " << m_Controller.getDeletedFiles(nVolumeNumber)->size() << ")!\n";
			continue;
		}

		return numberOfFile;
	}
}

const std::string ntfs::UserInterface::getDirectory() const
{
	std::string directory;

	while (1)
	{
		directory = getInput(std::cout, std::cin, "Enter path to drectory where to create undeleted file, 'exit' to finish program or 'back' to change volume: ");
		if (directory == "exit" || directory == "back")
		{
			return directory;
		}

		if (directory.empty() || !std::filesystem::exists(directory) || !std::filesystem::is_directory(directory))
		{
			std::cout << "\nInput error! Enter valid path to existing directory! Example: C:\\folder name1\\foldername2\n" << std::endl;
			continue;
		}

		return directory;
	}
}
