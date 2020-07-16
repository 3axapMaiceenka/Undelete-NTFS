#include "DrivesInfo.h"
#include "PartitionTableParser.h"

#include <iostream>

ntfs::DrivesInfo::DrivesInfo(const std::shared_ptr<std::list<PartitionTableEntry>> pDrives)
	: m_pLogicalDrives(pDrives),
	m_pDrivesMFT(std::make_shared<std::vector<MFTInfo>>(pDrives->size())),
	m_wDrivesNumber((WORD)pDrives->size())
{
	if (!m_wDrivesNumber)
	{
		throw std::logic_error("No logical drives with NTFS file sysytem found");
	}
}

/*
	Reads partition boot sector (first sector of partition), stores MFT info for every partition
*/
void ntfs::DrivesInfo::getDrivesInfo()
{
	CHAR caSector[SECTOR_SIZE];
	CHAR caOemId[OEM_ID_LENGTH];
	HANDLE hDrive = CreateFile(PHYSICAL_DRIVE, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	DWORD dwBytesRead;

	int i = 0;
	for (auto entry = m_pLogicalDrives->cbegin(); entry != m_pLogicalDrives->cend(); i++)
	{
		UINT64 uPartitionBeiginning = (UINT64)entry->m_dwLBAFirstSector * SECTOR_SIZE;
		LONG lDistanceToMoveLow = (LONG)(uPartitionBeiginning & 0x00000000ffffffff);
		LONG lDistanceToMoveHigh = (LONG)(uPartitionBeiginning >> 32);

		SetFilePointer(hDrive, lDistanceToMoveLow, &lDistanceToMoveHigh, FILE_BEGIN);
		ReadFile(hDrive, caSector, SECTOR_SIZE, &dwBytesRead, NULL); // read boot sector

		strncpy_s(caOemId, caSector + 3, OEM_ID_LENGTH); // OEM_ID is the special field, which indicates NTFS file system in the partition
		if (strcmp(caOemId, OEM_ID) != 0)
		{		
			auto temp = entry++;
			m_pLogicalDrives->erase(temp);
			m_pDrivesMFT->pop_back();
			m_wDrivesNumber--;
			i--;
			continue;
		}

		if ((CHAR)*(caSector + 0xd) > 0x80) 
		{
			//If the value is greater than 0x80, the amount of sectors is 2 to the power of the absolute value of considering this field to be negative
			m_pDrivesMFT->at(i).m_cSectorsPerCluster = 1 << (-1 * (*(caSector + 0xd)));
		}
		else
		{
			m_pDrivesMFT->at(i).m_cSectorsPerCluster = *(caSector + 0xd);
		}

		/*
		A positive value denotes the number of clusters in a File Record Segment. A negative value denotes the amount of bytes in a File
		Record Segment, in which case the size is 2 to the power of the absolute value
		*/
		if ((CHAR)*(caSector + 0x40) < 0)
		{
			m_pDrivesMFT->at(i).m_wRecordSize = 1 << (-1 * (*(caSector + 0x40)));
		}
		else
		{
			m_pDrivesMFT->at(i).m_wRecordSize = *(caSector + 0x40) * m_pDrivesMFT->at(i).m_cSectorsPerCluster * SECTOR_SIZE;
		}
		
		m_pDrivesMFT->at(i).m_ulFirstMFTCluster = *((UINT64*)(caSector + 0x30));
		m_pDrivesMFT->at(i).m_ulNumnerOfSectors = *((UINT64*)(caSector + 0x28));
		m_pDrivesMFT->at(i).m_dwVolumeStartingAddress = entry->m_dwLBAFirstSector;
		++entry;
	}

	CloseHandle(hDrive);
}

const std::shared_ptr<std::vector<ntfs::MFTInfo>> ntfs::DrivesInfo::getDrivesMFT() const
{
	return m_pDrivesMFT;
}
