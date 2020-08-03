#pragma once

#include "Controller.h"

#include <filesystem>
#include <string>
#include <fstream>

namespace ntfs
{

	class UserInterface
	{
	public:
		UserInterface(Controller& controller);

		void start();	

	private:
		int undelete(int nVolumeNumber);

		int findDeletedFiles();

		void printDeletdFilesInfo(const std::shared_ptr<std::list<DeletedFile>> pDeletedFiles);

		std::string getInput(std::ostream& os, std::istream& is, const char* pszMessage) const;

		void outputVolumesInformation() const;

		std::ostream& printVolumeInfo(std::ostream& os, const MFTInfo& mftInfo, const VolumeInfo& volumeInfo) const;

		bool isNumber(const std::string& text) const;

		int getVolumeNumber() const;

		long long int getNumberOfFile(int nVolumeNumber) const;

		const std::string getDirectory() const;

	private:
		Controller& m_Controller;
		const std::string m_TempFileRelativePath = "NTFSundelete";
		const std::string m_TempFileName = "NTFSdeletdFilesList.txt";
	};

}