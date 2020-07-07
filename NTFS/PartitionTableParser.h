#pragma once

#include <windows.h>
#include <stdexcept>
#include <list>

const char* m_pszPhysicalDrive = "\\\\.\\PhysicalDrive0";
constexpr WORD m_wSectorSize = 512;

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
	
	PartitionTableParser();

	~PartitionTableParser();

	void parse();

	const std::list<PartitionTableEntry> getLogicalDrives() const { return *m_pLogicalDrives; }

private:

	void parseExetendedPartition(DWORD dwPrimaryExPartitionFirstSec);

	std::list<PartitionTableEntry>* m_pLogicalDrives; // logical drives with NTFS file system
	HANDLE m_hPhysicalDrive;
};

