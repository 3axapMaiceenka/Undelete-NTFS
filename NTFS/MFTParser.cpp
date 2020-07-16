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
	delete m_pVolumeInfo;
}

void ntfs::MFTParser::readFirstRecord()
{
	// Calculate LBA of $MFT
	UINT64 uDistanceToMove = (UINT64)(m_pMft->m_dwVolumeStartingAddress + m_pMft->m_ulFirstMFTCluster * m_pMft->m_cSectorsPerCluster) * SECTOR_SIZE;
	CHAR* pszFirstRecord = new CHAR[m_pMft->m_wRecordSize];

	seek(uDistanceToMove, FILE_BEGIN);
	ReadFile(m_hDrive, pszFirstRecord, m_pMft->m_wRecordSize, (DWORD*)&uDistanceToMove, NULL);

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

void ntfs::MFTParser::findVolumeInformation()
{
	CHAR* pszRecord = new CHAR[m_pMft->m_wRecordSize];
	BOOLEAN bFound = FALSE;

	for (int i = 0; !bFound && i < m_pRunlist->size(); i++)
	{
		UINT64 uDistanceToMove = (UINT64)(m_pMft->m_dwVolumeStartingAddress + m_pRunlist->at(0).m_lRunOffset * m_pMft->m_cSectorsPerCluster) * SECTOR_SIZE;
		DWORD dwSectorsRead = 0;

		seek(uDistanceToMove, FILE_BEGIN);

		do
		{		
			ReadFile(m_hDrive, pszRecord, m_pMft->m_wRecordSize, (DWORD*)&uDistanceToMove, NULL);
			dwSectorsRead += m_pMft->m_wRecordSize / SECTOR_SIZE;

			MFTEntryHeader* pHeader = (MFTEntryHeader*)pszRecord;
			AttributeHeader* pAttrHeader = (AttributeHeader*)(pszRecord + pHeader->m_wAttributeOffset);

			if ((bFound = findVolumeAttributes(pAttrHeader)))
			{
				break;
			}

		} while (dwSectorsRead < m_pRunlist->at(i).m_uRunLength / m_pMft->m_cSectorsPerCluster);	
	}

	delete[] pszRecord;
}

inline BOOLEAN ntfs::MFTParser::seek(UINT64 uDistance, DWORD dwMoveMethod) const
{
	LONG lDistanceToMoveLow  = (LONG)(uDistance & 0x00000000ffffffff);
	LONG lDistanceToMoveHigh = (LONG)(uDistance >> 32);

	return SetFilePointer(m_hDrive, lDistanceToMoveLow, &lDistanceToMoveHigh, dwMoveMethod) == INVALID_SET_FILE_POINTER;
}

const ntfs::VolumeInfo ntfs::MFTParser::getVolumeInfo() const
{
	return *m_pVolumeInfo;
}

// pAttrHeader points to the first attribute in a record
BOOLEAN ntfs::MFTParser::findVolumeAttributes(AttributeHeader* pAttrHeader)
{
	BOOLEAN bFoundVolumeName = FALSE;
	BOOLEAN bFoundVolumeInformation = FALSE;

	if (pAttrHeader->m_dwAttributeTypeID == STANDART_INFORMATION)
	{
		STANDART_INFORMATION_ATTR* pStdInfo = (STANDART_INFORMATION_ATTR*)(pAttrHeader->m_Attr.m_Resident.m_wContentOffset + (CHAR*)pAttrHeader);

		// need check only system and hidden files to find $Volume file
		if (pStdInfo->m_dwFags == ((int)SYSTEM_FILE | HIDDEN_FILE)) 
		{
			do
			{
				pAttrHeader = (AttributeHeader*)((CHAR*)pAttrHeader + pAttrHeader->m_dwAttributeSize);

				if (pAttrHeader->m_dwAttributeTypeID == VOLUME_NAME)
				{
					bFoundVolumeName = TRUE;
					WORD wVolumeNameLength = (WORD)pAttrHeader->m_Attr.m_Resident.m_dwContentSize / sizeof(WCHAR);
					
					if (!m_pVolumeInfo) m_pVolumeInfo = new VolumeInfo;

					if (wVolumeNameLength)
					{
						WCHAR* pszVolumeName = new WCHAR[wVolumeNameLength + 1];

						wmemcpy_s(pszVolumeName, 
							      wVolumeNameLength + 1,
								  (const WCHAR*)((CHAR*)pAttrHeader + pAttrHeader->m_Attr.m_Resident.m_wContentOffset),
								  wVolumeNameLength);

						pszVolumeName[wVolumeNameLength] = L'\0';
						m_pVolumeInfo->m_VolumeLabel = CString(pszVolumeName);

						delete[] pszVolumeName;
					}
					else
					{
						m_pVolumeInfo->m_VolumeLabel = CString(L"Empty volume name");
					}
				}
				else
				{
					if (pAttrHeader->m_dwAttributeTypeID == VOLUME_INFORMATION)
					{
						VOLUME_INFORMATION_ATTR* pVolumeInfo = 
						(VOLUME_INFORMATION_ATTR*)((CHAR*)pAttrHeader + pAttrHeader->m_Attr.m_Resident.m_wContentOffset);
						bFoundVolumeInformation = TRUE;
						
						if (!m_pVolumeInfo) m_pVolumeInfo = new VolumeInfo;

						m_pVolumeInfo->m_VolInfoAttr = *pVolumeInfo;
					}
				}
			} while (!(bFoundVolumeInformation & bFoundVolumeName) && *(DWORD*)((CHAR*)pAttrHeader + pAttrHeader->m_dwAttributeSize) != 0xffffffff);
		}
	}

	return bFoundVolumeInformation & bFoundVolumeName;
}

