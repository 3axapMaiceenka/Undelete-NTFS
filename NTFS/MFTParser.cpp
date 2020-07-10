#include "MFTParser.h"
#include "DrivesInfo.h"
#include "PartitionTableParser.h"

#include <iostream>

ntfs::MFTParser::MFTParser(const MFTInfo& mftInfo)
	: m_pMft(new MFTInfo(mftInfo))
{
	m_hDrive = CreateFile(PHYSICAL_DRIVE, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

ntfs::MFTParser::~MFTParser()
{
	CloseHandle(m_hDrive);
}

void ntfs::MFTParser::readFirstRecord()
{
	// LBA of $MFT
	UINT64 uDistanceToMove = (m_pMft->m_dwVolumeStartingAddress + m_pMft->m_ulFirstMFTCluster * m_pMft->m_cSectorsPerCluster) * SECTOR_SIZE;
	LONG lDistanceToMoveLow = (LONG)(uDistanceToMove & 0x00000000ffffffff);
	LONG lDistanceToMoveHigh = (LONG)(uDistanceToMove & 0xffffffff00000000);

	SetFilePointer(m_hDrive, lDistanceToMoveLow, &lDistanceToMoveHigh, FILE_BEGIN);

	CHAR* pszFirstRecord = new CHAR[m_pMft->m_wRecordSize];

	ReadFile(m_hDrive, pszFirstRecord, m_pMft->m_wRecordSize, (DWORD*)&lDistanceToMoveHigh, NULL);

	/*MFTEntryHeader* pHeader = (MFTEntryHeader*)pszFirstRecord;

	std::cout << "******************************";

	std::cout << "SIGNATURE = " << pHeader->m_caSignature << "\n";
	std::cout << "SEQUENCE NUMBER = " << pHeader->m_wSequenceNumber << "\n";
	std::cout << "HARD LINK COUNT = " << pHeader->m_wHardLinkCount << "\n";
	std::cout << "ATTRIBUTE OFFSET = " << pHeader->m_wAttributeOffset << "\n";
	std::cout << "ALLOCATED ENTRY SIZE = " << pHeader->m_dwAllocatedEntrySize << "\n";
	std::cout << "USED ENTRY SIZE = " << pHeader->m_dwUsedEntrySize << "\n";
	std::cout << "NEXT ATTR ID = " << pHeader->m_wNextAttributeID << "\n";
	std::cout << "FLAGS = " << pHeader->m_wFlags << "\n";

	AttributeHeader* pAttrHeader = (AttributeHeader*)(pszFirstRecord + pHeader->m_wAttributeOffset);

	std::cout << "\t" << "First attr:\n";

	std::cout << "Nonresident flag = " << pAttrHeader->m_cNonresidentFlag << "\n";
	std::cout << "Attr type id= " << pAttrHeader->m_dwAttributeTypeID << "\n";
	std::cout << "Attr size = " << pAttrHeader->m_dwAttributeSize << "\n";
	std::cout << "Name lenght = " << pAttrHeader->m_cNameLenght << "\n";

	if (!pAttrHeader->m_cNonresidentFlag)
	{
		pAttrHeader->m_Attr.m_Resident.m_dwContentSize = *((DWORD*)(pAttrHeader + 1));
		pAttrHeader->m_Attr.m_Resident.m_wContentOffset = *((WORD*)(pAttrHeader + 1) + 2);
		
		std::cout << "Content size = " << pAttrHeader->m_Attr.m_Resident.m_dwContentSize << "\n";
		std::cout << "Contenet offset = " << pAttrHeader->m_Attr.m_Resident.m_wContentOffset << "\n";
	}
	else
	{
		
	}

	std::cout << "******************************";*/

	delete[] pszFirstRecord;
}


