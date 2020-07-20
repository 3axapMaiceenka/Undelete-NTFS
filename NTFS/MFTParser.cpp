#include "MFTParser.h"
#include "DrivesInfo.h"
#include "PartitionTableParser.h"
#include "Runlist.h"
#include "DeletedFile.h"

ntfs::MFTParser::MFTParser(const MFTInfo& mftInfo)
	: m_pMft(new MFTInfo(mftInfo)),
	m_pRunlist(NULL),
	m_pDeletedFiles(std::make_shared<std::list<DeletedFile>>())
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
	iterate(&MFTParser::findVolumeAttributes);
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

// checks if record corresponds to $Volume file, if so, saves volume attributes
BOOLEAN ntfs::MFTParser::findVolumeAttributes(MFTEntryHeader* pHeader, UINT64 uAddress)
{
	BOOLEAN bFoundVolumeName = FALSE;
	BOOLEAN bFoundVolumeInformation = FALSE;
	AttributeHeader* pAttrHeader = (AttributeHeader*)((CHAR*)pHeader + pHeader->m_wAttributeOffset);

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
						WCHAR* pszVolumeName = readUtf16String((CHAR*)pAttrHeader + pAttrHeader->m_Attr.m_Resident.m_wContentOffset, wVolumeNameLength);
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

void ntfs::MFTParser::findDeletedFiles()
{
	iterate(&MFTParser::checkForDeleted);
}

/*
   Checks MFT record for corresponding to deleted file/catalog
   Always returns FALSE to be callable from iterate() function
 */
BOOLEAN ntfs::MFTParser::checkForDeleted(MFTEntryHeader* pHeader, UINT64 uRecordAddress)
{
	if (pHeader->m_wFlags == DELETED_FILE || pHeader->m_wFlags == DELETED_CATALOG)
	{
		AttributeHeader* pAttrHeader = (AttributeHeader*)((CHAR*)pHeader + pHeader->m_wAttributeOffset);
		
		if (!pHeader->m_uBaseRecordAddress && pAttrHeader->m_dwAttributeSize)
		{
			while (pAttrHeader->m_dwAttributeTypeID != FILE_NAME)
			{			
				if (!pAttrHeader->m_dwAttributeSize || pAttrHeader->m_dwAttributeSize > m_pMft->m_wRecordSize)
				{
					return FALSE;
				}

				pAttrHeader = (AttributeHeader*)((CHAR*)pAttrHeader + pAttrHeader->m_dwAttributeSize);
			}

			FILE_NAME_ATTR* pFileNameAttr = (FILE_NAME_ATTR*)((CHAR*)pAttrHeader + pAttrHeader->m_Attr.m_Resident.m_wContentOffset);
			DeletedFile df;

			df.m_pFileNameAttr = new FILE_NAME_ATTR(*pFileNameAttr);
			df.m_uRecordAddress = uRecordAddress;
			df.m_cType = (CHAR)pHeader->m_wFlags;
			df.m_pszFileName = readUtf16String((CHAR*)pFileNameAttr + FILE_NAME_ATTR_SIZE, pFileNameAttr->m_cNameLength);

			m_pDeletedFiles->emplace_back(std::move(df));
		}
	}

	return FALSE;
}

const std::shared_ptr<std::list<ntfs::DeletedFile>> ntfs::MFTParser::getDeletedFiles() const
{
	return m_pDeletedFiles;
}

inline WCHAR* ntfs::MFTParser::readUtf16String(CHAR* pSource, WORD wLength) const
{
	WCHAR* pStr = new WCHAR[wLength + 1];

	wmemcpy_s(pStr, wLength + 1, (const WCHAR*)pSource, wLength);
	pStr[wLength] = L'\0';

	return pStr;
}

// For every record in MFT invokes passed pointer to member function
void ntfs::MFTParser::iterate(PointerToMemberFunction jobFunc)
{
	CHAR* pszRecord = new CHAR[m_pMft->m_wRecordSize];
	DWORD dwSectorsRead = 0;
	DWORD dwBytesRead;
	BOOLEAN bDone = FALSE;

	for (int i = 0; !bDone && i < m_pRunlist->size(); i++, dwSectorsRead = 0)
	{
		UINT64 uDistanceToMove = (UINT64)(m_pMft->m_dwVolumeStartingAddress + m_pRunlist->at(i).m_lRunOffset * m_pMft->m_cSectorsPerCluster) * SECTOR_SIZE;
		seek(uDistanceToMove, FILE_BEGIN);

		do
		{
			ReadFile(m_hDrive, pszRecord, m_pMft->m_wRecordSize, &dwBytesRead, NULL);
			dwSectorsRead += m_pMft->m_wRecordSize / SECTOR_SIZE;

			if ((bDone = (this->*jobFunc)((MFTEntryHeader*)pszRecord, uDistanceToMove + dwSectorsRead * SECTOR_SIZE - m_pMft->m_wRecordSize)))
			{
				break;
			}

		} while (dwSectorsRead < m_pRunlist->at(i).m_uRunLength * m_pMft->m_cSectorsPerCluster);
	}

	delete[] pszRecord;
}




