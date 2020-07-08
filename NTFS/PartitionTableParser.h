#pragma once

#include <windows.h>
#include <memory>
#include <list>

#define SECTOR_SIZE 512
#define PHYSICAL_DRIVE "\\\\.\\PhysicalDrive0"

enum PartitionType
{
	EXTENDED_PARTITION = 5,
	NTFS_PARTITION = 7
};

struct PartitionTableEntry
{
	CHAR  m_cStatus;
	CHAR  m_cFirstHead;
	CHAR  m_cFirstSector;   // sector in bits 5-0, bits 6-7 are high bits of cylinder
	CHAR  m_cFirstCylinder; // bits 7-0 of cylinder
	CHAR  m_cPartitionType;
	CHAR  m_cLastHead;
	CHAR  m_cLastSector;    // sector in bits 5-0, bits 6-7 are high bits of cylinder
	CHAR  m_cLastCylinder;  // bits 7-0 of cylinder
	DWORD m_dwLBAFirstSector;
	DWORD m_dwNumberOfSectors;
};

/*
	Finds all logical drives with NTFS file system
*/

class PartitionTableParser
{
public:
	
	void parse();

	PartitionTableParser();

	~PartitionTableParser();

	inline const std::shared_ptr<std::list<PartitionTableEntry>> getLogicalDrives() const;

private:

	void parseExetendedPartition(DWORD dwPrimaryExPartitionFirstSec);

	std::shared_ptr<std::list<PartitionTableEntry>> m_pLogicalDrives; // logical drives with NTFS file system
	HANDLE m_hPhysicalDrive;
};

