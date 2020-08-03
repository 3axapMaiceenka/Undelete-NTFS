#include "PartitionTableParser.h"
#include <stdexcept>

ntfs::PartitionTableParser::PartitionTableParser()
	: m_pLogicalDrives(std::make_shared<std::list<PartitionTableEntry>>())
{
	m_hPhysicalDrive = CreateFile(PHYSICAL_DRIVE, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (m_hPhysicalDrive == INVALID_HANDLE_VALUE)
	{
		throw std::runtime_error("Can't open physical drive");
	}
}

ntfs::PartitionTableParser::~PartitionTableParser()
{
	CloseHandle(m_hPhysicalDrive);
}

const std::shared_ptr<std::list<ntfs::PartitionTableEntry>> ntfs::PartitionTableParser::getLogicalDrives() const
{ 
	return m_pLogicalDrives; 
}

void ntfs::PartitionTableParser::parse()
{
	CHAR caSector[SECTOR_SIZE];
	DWORD dwNumberOfBytes = 0;
	DWORD dwPrimaryExPartitionFirstSec = 0;

	if (!ReadFile(m_hPhysicalDrive, caSector, SECTOR_SIZE, &dwNumberOfBytes, NULL)) // read MBR
	{
		throw std::runtime_error("Failed read MBR");
	}

	PartitionTableEntry* pEntry = (PartitionTableEntry*)(caSector + 0x1be); // pEntry is set to the start of the partition table

	for (int i = 0; i < 4; i++, pEntry++)
	{
		if (pEntry->m_cPartitionType == NTFS_PARTITION)
		{
			m_pLogicalDrives->emplace_back(*pEntry);
		}
		else
		{
			if (pEntry->m_cPartitionType == EXTENDED_PARTITION)
			{
				dwPrimaryExPartitionFirstSec = pEntry->m_dwLBAFirstSector;
			}
		}
	}

	if (!dwPrimaryExPartitionFirstSec) return;

	parseExetendedPartition(dwPrimaryExPartitionFirstSec); // throws std::runtime_error
}

void ntfs::PartitionTableParser::parseExetendedPartition(DWORD dwPrimaryExPartitionFirstSec)
{
	CHAR caSector[SECTOR_SIZE];
	DWORD dwPartitionFirstSector = dwPrimaryExPartitionFirstSec;
	DWORD dwNumberOfBytes;
	PartitionTableEntry* pEntry;

	while (1)
	{
		SetFilePointer(m_hPhysicalDrive, dwPartitionFirstSector * SECTOR_SIZE, NULL, FILE_BEGIN);

		if (!ReadFile(m_hPhysicalDrive, caSector, SECTOR_SIZE, &dwNumberOfBytes, NULL))
		{
			throw std::runtime_error("Failed read extended partition tbale");
		}

		pEntry = (PartitionTableEntry*)(caSector + 0x1be); // pEntry is set to the start of the extended partition table

		if (pEntry->m_cPartitionType == NTFS_PARTITION) // first entry corresponds to the logical drive
		{
			m_pLogicalDrives->emplace_back(*pEntry);
			m_pLogicalDrives->back().m_dwLBAFirstSector += dwPartitionFirstSector; // addressing relative to the extended partition table	
		}

		pEntry++;

		if (pEntry->m_cPartitionType == EXTENDED_PARTITION) // second entry is either zero or corresponds to the secondary extended partition
		{
			// addressing relative to the primary extended partition
			dwPartitionFirstSector = pEntry->m_dwLBAFirstSector + dwPrimaryExPartitionFirstSec;
		}
		else
		{
			break;
		}
	}
}

