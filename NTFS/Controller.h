#pragma once

#include "PartitionTableParser.h"
#include "MFTParser.h"
#include "DrivesInfo.h"
#include "DeletedFile.h"
#include "NTFSDataStructures.h"

namespace ntfs
{

	class Controller
	{
	public:
		Controller();

		BOOLEAN start();

		const std::shared_ptr<VolumeInfo> getVolumeInfo(std::size_t volumeNumb) const;

		const std::shared_ptr<std::vector<MFTInfo>> getDrivesInfo() const;

		void findDeletedFiles(std::size_t volumeNumb);

		const std::shared_ptr<std::list<DeletedFile>> getDeletedFiles(std::size_t volumeNumb) const;

		void undelete(std::size_t volumeNumb, std::size_t fileNumb, const std::wstring& directory) const;

		std::size_t getNumberOfVolumes() const;

	private:
		std::unique_ptr<std::vector<MFTParser>> m_pMftParsers;
		std::shared_ptr<std::vector<MFTInfo>>   m_pDrivesInfo;
	};

}

