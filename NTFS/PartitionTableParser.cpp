#include "PartitionTableParser.h"

PartitionTableParser::PartitionTableParser()
	: m_pLogicalDrives(new std::list<PartitionTableEntry>)
{
	m_hPhysicalDrive = CreateFile(m_pszPhysicalDrive, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (m_hPhysicalDrive == INVALID_HANDLE_VALUE)
	{
		throw std::runtime_error("Can't open physical drive");
	}
}

PartitionTableParser::~PartitionTableParser()
{
	delete m_pLogicalDrives;
	CloseHandle(m_hPhysicalDrive);
}

void PartitionTableParser::parse()
{
	CHAR caSector[m_wSectorSize];
	DWORD dwNumberOfBytes = 0;
	DWORD dwPrimaryExPartitionFirstSec = 0;

	ReadFile(m_hPhysicalDrive, caSector, m_wSectorSize, &dwNumberOfBytes, NULL); // read MBR

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

	parseExetendedPartition(dwPrimaryExPartitionFirstSec);
}

void PartitionTableParser::parseExetendedPartition(DWORD dwPrimaryExPartitionFirstSec)
{
	CHAR caSector[m_wSectorSize];
	DWORD dwPartitionFirstSector = dwPrimaryExPartitionFirstSec;
	DWORD dwNumberOfBytes;
	PartitionTableEntry* pEntry;

	while (1)
	{
		SetFilePointer(m_hPhysicalDrive, dwPartitionFirstSector * m_wSectorSize, NULL, FILE_BEGIN);
		ReadFile(m_hPhysicalDrive, caSector, m_wSectorSize, &dwNumberOfBytes, NULL);
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

