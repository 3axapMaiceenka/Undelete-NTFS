#pragma once

#include <windows.h>
#include <memory>
#include <list>
#include <vector>

#define OEM_ID "NTFS    "
#define OEM_ID_LENGTH 9

struct PartitionTableEntry;

struct MFTInfo
{
	UINT64 m_ulNumnerOfSectors;
	UINT64 m_ulFirstMFTCluster;
	WORD   m_wRecordSize; // in bytes
	CHAR   m_cSectorsPerCluster;
};

class DrivesInfo

{
public:
	
	DrivesInfo(const std::shared_ptr<std::list<PartitionTableEntry>> pDrives);

	~DrivesInfo();

	void getDrivesInfo();

private:
	std::shared_ptr<std::list<PartitionTableEntry>> m_pLogicalDrives;
	std::vector<MFTInfo>* m_pDrivesMFT;
	WORD m_wDrivesNumber;
};

