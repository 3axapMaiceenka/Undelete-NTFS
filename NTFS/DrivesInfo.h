#pragma once

#include <windows.h>
#include <memory>
#include <list>
#include <vector>

namespace ntfs
{
	struct PartitionTableEntry;

#define OEM_ID "NTFS    "
#define OEM_ID_LENGTH 9

	struct MFTInfo
	{
		UINT64 m_ulNumberOfSectors;
		UINT64 m_ulFirstMFTCluster;
		DWORD  m_dwVolumeStartingAddress;
		WORD   m_wRecordSize; // in bytes
		CHAR   m_cSectorsPerCluster;
	};

	class DrivesInfo

	{
	public:

		DrivesInfo(const std::shared_ptr<std::list<PartitionTableEntry>> pDrives);

		void getDrivesInfo();

		const std::shared_ptr<std::vector<MFTInfo>> getDrivesMFT() const;

	private:
		std::shared_ptr<std::list<PartitionTableEntry>> m_pLogicalDrives;
		std::shared_ptr<std::vector<MFTInfo>> m_pDrivesMFT;
		WORD m_wDrivesNumber;
	};

} // namespace

