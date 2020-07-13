#include "MFTParser.h"
#include "DrivesInfo.h"
#include "PartitionTableParser.h"
#include "Runlist.h"

ntfs::MFTParser::MFTParser(const MFTInfo& mftInfo)
	: m_pMft(new MFTInfo(mftInfo)),
	m_pRunlist(NULL)
{
	m_hDrive = CreateFile(PHYSICAL_DRIVE, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

ntfs::MFTParser::~MFTParser()
{
	CloseHandle(m_hDrive);
	delete m_pRunlist;
}

void ntfs::MFTParser::readFirstRecord()
{
	// Calculate LBA of $MFT
	UINT64 uDistanceToMove = (m_pMft->m_dwVolumeStartingAddress + m_pMft->m_ulFirstMFTCluster * m_pMft->m_cSectorsPerCluster) * SECTOR_SIZE;
	LONG lDistanceToMoveLow = (LONG)(uDistanceToMove & 0x00000000ffffffff);
	LONG lDistanceToMoveHigh = (LONG)(uDistanceToMove >> 32);
	CHAR* pszFirstRecord = new CHAR[m_pMft->m_wRecordSize];

	SetFilePointer(m_hDrive, lDistanceToMoveLow, &lDistanceToMoveHigh, FILE_BEGIN);
	ReadFile(m_hDrive, pszFirstRecord, m_pMft->m_wRecordSize, (DWORD*)&lDistanceToMoveHigh, NULL);

	MFTEntryHeader* pHeader = (MFTEntryHeader*)pszFirstRecord;
	AttributeHeader* pAttrHeader = (AttributeHeader*)(pszFirstRecord + pHeader->m_wAttributeOffset);

	while (pAttrHeader->m_dwAttributeTypeID != DATA)
	{
		pAttrHeader = (AttributeHeader*)((CHAR*)pAttrHeader + pAttrHeader->m_dwAttributeSize);
	}

	m_pRunlist = new Runlist(pAttrHeader->m_Attr.m_Nonresident.m_wRunlistOffset + (CHAR*)pAttrHeader,
		pAttrHeader->m_Attr.m_Nonresident.m_uRunlistStartingVCN,
		pAttrHeader->m_Attr.m_Nonresident.m_uRunlistFinalVCN);
	
	delete[] pszFirstRecord;
}


