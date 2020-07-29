#include "MFTParser.h"
#include "DrivesInfo.h"
#include "PartitionTableParser.h"
#include "Runlist.h"
#include "DeletedFile.h"
#include "NTFSDataStructures.h"

#include <algorithm>
#include <iostream>

ntfs::MFTParser::MFTParser(const MFTInfo& mftInfo)
	: m_pMft(new MFTInfo(mftInfo)),
	m_pRunlist(NULL),
	m_pBitmapRunlist(NULL),
	m_pDeletedFiles(std::make_shared<std::list<DeletedFile>>())
{
	m_hDrive = CreateFile(PHYSICAL_DRIVE, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	readFirstRecord();
	iterate(&MFTParser::findVolumeAttributes);
	iterate(&MFTParser::findBitmap);
}

ntfs::MFTParser::~MFTParser()
{
	CloseHandle(m_hDrive);
	delete m_pRunlist;
	delete m_pBitmapRunlist;
	delete m_pVolumeInfo;
}

void ntfs::MFTParser::readFirstRecord()
{
	// Calculate LBA of $MFT
	UINT64 uDistanceToMove = (UINT64)(m_pMft->m_dwVolumeStartingAddress + m_pMft->m_ulFirstMFTCluster * m_pMft->m_cSectorsPerCluster) * SECTOR_SIZE;
	CHAR* pszFirstRecord = new CHAR[m_pMft->m_wRecordSize];

	seek(uDistanceToMove, FILE_BEGIN);
	ReadFile(m_hDrive, pszFirstRecord, m_pMft->m_wRecordSize, (DWORD*)&uDistanceToMove, NULL);

	const MFTEntryHeader* pHeader = (MFTEntryHeader*)pszFirstRecord;
	const AttributeHeader* pAttrHeader = (AttributeHeader*)(pszFirstRecord + pHeader->m_wAttributeOffset);

	pAttrHeader = findAttr(pAttrHeader, DATA);

	m_pRunlist = new Runlist(pAttrHeader->m_Attr.m_Nonresident.m_wRunlistOffset + (CHAR*)pAttrHeader,
							 pAttrHeader->m_Attr.m_Nonresident.m_uRunlistStartingVCN,
							 pAttrHeader->m_Attr.m_Nonresident.m_uRunlistFinalVCN);
	
	delete[] pszFirstRecord;
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
	const AttributeHeader* pAttrHeader = (AttributeHeader*)((CHAR*)pHeader + pHeader->m_wAttributeOffset);

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
BOOLEAN ntfs::MFTParser::checkForDeleted(ntfs::MFTEntryHeader* pHeader, UINT64 uRecordAddress)
{
	if (pHeader->m_wFlags == DELETED_FILE || pHeader->m_wFlags == DELETED_CATALOG)
	{
		const AttributeHeader* pAttrHeader = (AttributeHeader*)((CHAR*)pHeader + pHeader->m_wAttributeOffset);
		
		if (!pHeader->m_uBaseRecordAddress && pAttrHeader->m_dwAttributeSize)
		{
			if (!(pAttrHeader = findAttr(pAttrHeader, FILE_NAME)))
			{
				return FALSE;
			}

			if (pHeader->m_wFlags == DELETED_FILE)
			{
				LONG lFilePointerLow = 0;
				LONG lFilePointerHigh = 0;
				lFilePointerLow = SetFilePointer(m_hDrive, lFilePointerLow, &lFilePointerHigh, FILE_CURRENT);

				BOOLEAN bIsDataCleared = isDataCleared(pAttrHeader);
				SetFilePointer(m_hDrive, lFilePointerLow, &lFilePointerHigh, FILE_BEGIN);

				if (bIsDataCleared) return FALSE;
			}

			const FILE_NAME_ATTR* pFileNameAttr = (FILE_NAME_ATTR*)((CHAR*)pAttrHeader + pAttrHeader->m_Attr.m_Resident.m_wContentOffset);
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

// For every record in MFT invokes passed pointer to member function (jobFunc), if jobFunc returns TRUE, stops iterating 
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

BOOLEAN ntfs::MFTParser::undelete(const ntfs::DeletedFile* const pDeletedFile, LPCSTR lpszDirectoryName) const
{
	LPSTR lpszCurrentDirectory = new CHAR[BUF_SIZE];
	GetCurrentDirectory(BUF_SIZE, lpszCurrentDirectory);
	SetCurrentDirectory(lpszDirectoryName);

	DWORD dwNumberOfBytes;
	LPSTR lpszRecord = new CHAR[m_pMft->m_wRecordSize];

	seek(pDeletedFile->m_uRecordAddress, FILE_BEGIN);
	ReadFile(m_hDrive, lpszRecord, m_pMft->m_wRecordSize, &dwNumberOfBytes, NULL);

	MFTEntryHeader* pHeader = (MFTEntryHeader*)lpszRecord;

	if (pDeletedFile->m_cType == DELETED_FILE)
	{
		HANDLE hUndeletedFile = CreateFileW(pDeletedFile->m_pszFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (!hUndeletedFile)
		{
			SetCurrentDirectory(lpszCurrentDirectory);
			delete lpszCurrentDirectory;
			return FALSE;
		}

		const AttributeHeader* pAttrHeader = (AttributeHeader*)(lpszRecord + pHeader->m_wAttributeOffset);
		pAttrHeader = findAttr(pAttrHeader, DATA);

		undeleteFile(hUndeletedFile, pAttrHeader);

		CloseHandle(hUndeletedFile);
	}
	else
	{
		CreateDirectoryW(pDeletedFile->m_pszFileName, NULL);
		SetCurrentDirectoryW(pDeletedFile->m_pszFileName);

		undeleteDirectory(pHeader);

		SetCurrentDirectory("..");
	}

	SetCurrentDirectory(lpszCurrentDirectory);
	
	delete lpszCurrentDirectory;
	delete lpszRecord;
	
	return TRUE;
}

// pAttrHeader sould be $DATA attribute
void ntfs::MFTParser::undeleteFile(HANDLE hFile, const AttributeHeader* pAttrHeader) const
{
	DWORD dwNumberOfBytes;

	if (!pAttrHeader->m_cNonresidentFlag)
	{
		WriteFile(hFile,
				  (CHAR*)pAttrHeader + pAttrHeader->m_Attr.m_Resident.m_wContentOffset,
				  pAttrHeader->m_Attr.m_Resident.m_dwContentSize,
				  &dwNumberOfBytes, NULL);
	}
	else
	{
		auto pRunlist = new Runlist(pAttrHeader->m_Attr.m_Nonresident.m_wRunlistOffset + (CHAR*)pAttrHeader,
									pAttrHeader->m_Attr.m_Nonresident.m_uRunlistStartingVCN,
									pAttrHeader->m_Attr.m_Nonresident.m_uRunlistFinalVCN);

		WORD wClusterSize = m_pMft->m_cSectorsPerCluster * SECTOR_SIZE;
		CHAR* pszCluster = new CHAR[wClusterSize];

		for (int i = 0; i < pRunlist->size(); i++)
		{
			DWORD dwClustersWritten = 0;
			UINT64 uDistanceToMove = (UINT64)(m_pMft->m_dwVolumeStartingAddress + pRunlist->at(i).m_lRunOffset * m_pMft->m_cSectorsPerCluster) * SECTOR_SIZE;
			seek(uDistanceToMove, FILE_BEGIN);

			do
			{
				ReadFile(m_hDrive, pszCluster, wClusterSize, &dwNumberOfBytes, NULL);
				WriteFile(hFile, pszCluster, wClusterSize, &dwNumberOfBytes, NULL);
				dwClustersWritten++;
			} while (dwClustersWritten < pRunlist->at(i).m_uRunLength);
		}

		delete pszCluster;
		delete pRunlist;
	}
}

BOOLEAN ntfs::MFTParser::findBitmap(MFTEntryHeader* pHeader, UINT64 uAddress)
{
	const AttributeHeader* pAttrHeader = (AttributeHeader*)((CHAR*)pHeader + pHeader->m_wAttributeOffset);

	if (pAttrHeader->m_dwAttributeSize)
	{
		if (!(pAttrHeader = findAttr(pAttrHeader, FILE_NAME)))
		{
			return FALSE;
		}

		const FILE_NAME_ATTR* pFileNameAttr = (FILE_NAME_ATTR*)((CHAR*)pAttrHeader + pAttrHeader->m_Attr.m_Resident.m_wContentOffset);
		WCHAR* pszFileName = readUtf16String((CHAR*)pFileNameAttr + FILE_NAME_ATTR_SIZE, pFileNameAttr->m_cNameLength);

		if (!wcscmp(pszFileName, L"$Bitmap"))
		{
			m_uBitmapOffset = uAddress;

			while (pAttrHeader->m_dwAttributeTypeID != DATA)
			{
				pAttrHeader = (AttributeHeader*)((CHAR*)pAttrHeader + pAttrHeader->m_dwAttributeSize);
			}

			if (!m_pBitmapRunlist)
			{
				m_pBitmapRunlist = new Runlist(pAttrHeader->m_Attr.m_Nonresident.m_wRunlistOffset + (CHAR*)pAttrHeader,
											   pAttrHeader->m_Attr.m_Nonresident.m_uRunlistStartingVCN,
											   pAttrHeader->m_Attr.m_Nonresident.m_uRunlistFinalVCN);
			}

			delete[] pszFileName;
			return TRUE;
		}

		delete[] pszFileName;
		return FALSE;
	}

	return FALSE;
}

inline BOOLEAN ntfs::MFTParser::intersect(const Runlist* const pLhs, const Runlist* const pRhs, WORD wLhsRunIndx, WORD wwRhsRunIndx) const
{
	INT64 lLhsRunOffset = pLhs->at(wLhsRunIndx).m_lRunOffset;
	INT64 lLhsRunLength = pLhs->at(wLhsRunIndx).m_uRunLength;
	INT64 lRhsRunOffset	= pRhs->at(wwRhsRunIndx).m_lRunOffset;
	INT64 lRhsRunLength	= pRhs->at(wwRhsRunIndx).m_uRunLength;

	return ((lLhsRunOffset >= lRhsRunOffset && lLhsRunOffset <= lRhsRunOffset + lRhsRunLength) ||
		   (lLhsRunOffset + lLhsRunLength >= lRhsRunOffset && lLhsRunOffset + lLhsRunLength <= lRhsRunOffset + lRhsRunLength)) ||
		   ((lRhsRunOffset >= lLhsRunOffset && lRhsRunOffset <= lLhsRunOffset + lLhsRunLength) ||
		   (lRhsRunOffset + lRhsRunLength >= lLhsRunOffset && lRhsRunOffset + lRhsRunLength <= lLhsRunOffset + lLhsRunLength));
}

BOOLEAN ntfs::MFTParser::isDataCleared(const AttributeHeader* pAttrHeader) const
{
	static const DWORD dwClusterSize = m_pMft->m_cSectorsPerCluster * SECTOR_SIZE;
	// number of clusters defined in one cluster in $Bitmap file in $Data attribute (one bit defines one cluster)
	static const DWORD dwClustersDefinedPerCluster = dwClusterSize * 8;
	BOOLEAN bResult = FALSE;

	if (!(pAttrHeader = findAttr(pAttrHeader, DATA)))
	{
		return TRUE;
	}

	if (pAttrHeader->m_cNonresidentFlag)
	{
		auto pRunlist = new Runlist(pAttrHeader->m_Attr.m_Nonresident.m_wRunlistOffset + (CHAR*)pAttrHeader,
									pAttrHeader->m_Attr.m_Nonresident.m_uRunlistStartingVCN,
									pAttrHeader->m_Attr.m_Nonresident.m_uRunlistFinalVCN);
		
		for (unsigned int i = 0; i < pRunlist->size() && !bResult; i++)
		{
			INT64 lFirstCluster = pRunlist->at(i).m_lRunOffset;
			INT64 lLastCluster  = lFirstCluster + pRunlist->at(i).m_uRunLength;
			UINT64 uCluster = 0;
			
			unsigned int j = 0;
			for (j = 0; j < m_pBitmapRunlist->size() && lFirstCluster > uCluster; j++)
			{
				uCluster += m_pBitmapRunlist->at(j).m_uRunLength * dwClustersDefinedPerCluster;
			}			
			j--;

			UINT64 uOffsetInBits = lFirstCluster - (uCluster - m_pBitmapRunlist->at(j).m_uRunLength * dwClustersDefinedPerCluster);
			UINT64 uOffsetInBytes = uOffsetInBits / 8;		
			uOffsetInBits %= 8;

			UINT64 uDistance = (UINT64)(m_pMft->m_dwVolumeStartingAddress + m_pBitmapRunlist->at(j).m_lRunOffset * m_pMft->m_cSectorsPerCluster) * SECTOR_SIZE;
			
			if (uOffsetInBytes > dwClusterSize)
			{
				uDistance += (uOffsetInBytes - (uOffsetInBytes % dwClusterSize));
				uOffsetInBytes %= dwClusterSize;
			}

			seek(uDistance, FILE_BEGIN);
			
			bResult = checkIfAllocated(pRunlist->at(i).m_uRunLength, uOffsetInBytes, uOffsetInBits);
		}

		delete pRunlist;
	}

	return bResult;
}

/* Reads $Bitmap file and checks if uNumberOfClusters clusters are allocated 
   File pointer is set to the needed position in $Data attribute of the $Bitmap file
   uOffsetInBytes is the offset from the current file pointer position to the first byte, which contains bits defining needed clusters state
   uOffsetInBits is the offset in that byte to the first bit, corresponding to the first needed cluster */
BOOLEAN ntfs::MFTParser::checkIfAllocated(UINT64 uNumberOfClusters, UINT64 uOffsetInBytes, UINT64 uOffsetInBits) const
{
	static const DWORD dwClusterSize = m_pMft->m_cSectorsPerCluster * SECTOR_SIZE;
	CHAR* pCluster = new CHAR[dwClusterSize];
	DWORD dwNumberOfBytes;

	while (uNumberOfClusters)
	{
		ReadFile(m_hDrive, pCluster, dwClusterSize, &dwNumberOfBytes, NULL);  

		BOOLEAN bDistance = TRUE;
		UCHAR* pOffset = (UCHAR*)(pCluster + uOffsetInBytes);
		UCHAR cData = *pOffset;
		cData >>= uOffsetInBits;

		while (uNumberOfClusters && bDistance)
		{
			if (uNumberOfClusters >= (8 - uOffsetInBits))
			{
				if (cData)
				{
					delete[] pCluster;
					return TRUE;
				}

				uNumberOfClusters -= (8 - uOffsetInBits);
				uOffsetInBits = uOffsetInBytes = 0;

				if ((bDistance = (std::distance(pCluster, (CHAR*)pOffset) < dwClusterSize)))
				{
					cData = *(++pOffset);
				}
			}
			else
			{
				if ((cData <<= (8 - uNumberOfClusters)))
				{
					delete[] pCluster;
					return TRUE;
				}

				uNumberOfClusters = 0;
			}
		}
	}

	delete[] pCluster;
	return FALSE;
}

/*
	Finds specified attribute in MFT record, returns NULL if error occurred
	uAttrType should be constant from enum ntfs::ATTRIBUTE_TYPES 
*/
inline const ntfs::AttributeHeader* ntfs::MFTParser::findAttr(const AttributeHeader* pAttrHeader, UINT32 uAttrType) const
{
	while (pAttrHeader->m_dwAttributeTypeID != uAttrType)
	{
		if (!pAttrHeader->m_dwAttributeSize || pAttrHeader->m_dwAttributeSize >= m_pMft->m_wRecordSize)
		{
			return NULL;
		}

		pAttrHeader = (AttributeHeader*)((CHAR*)pAttrHeader + pAttrHeader->m_dwAttributeSize);
	}

	return pAttrHeader;
}

void ntfs::MFTParser::undeleteDirectory(MFTEntryHeader* pHeader) const
{
	const AttributeHeader* pAttrHeader = (AttributeHeader*)((CHAR*)pHeader + pHeader->m_wAttributeOffset);
	pAttrHeader = findAttr(pAttrHeader, INDEX_ROOT);

	const INDEX_ROOT_ATTR* pIndexRoot = (INDEX_ROOT_ATTR*)((CHAR*)pAttrHeader + pAttrHeader->m_Attr.m_Resident.m_wContentOffset);
	const NodeHeader* pNodeHeader = (NodeHeader*)(pIndexRoot + 1);
	DWORD dwOffset = pNodeHeader->m_dwIndexEntryListOffset;
	const IndexEntry* pIndexEntry = (IndexEntry*)((CHAR*)pNodeHeader + dwOffset);
	
	while (dwOffset < pNodeHeader->m_dwEndOfListUsedPartOffset)
	{
		if (pIndexEntry->m_wLength && pIndexEntry->m_wFileNameAttrLength)
		{
			DeletedFile df;	
			FILE_NAME_ATTR* pFileNameAttr = (FILE_NAME_ATTR*)(pIndexEntry + 1);
			df.m_pszFileName = readUtf16String((CHAR*)pFileNameAttr + FILE_NAME_ATTR_SIZE, pFileNameAttr->m_cNameLength);
			df.m_pFileNameAttr = new FILE_NAME_ATTR(*pFileNameAttr);

			auto dfIter = std::find(m_pDeletedFiles->cbegin(), m_pDeletedFiles->cend(), df);
			if (dfIter != m_pDeletedFiles->cend())
			{
				undelete(&(*dfIter), ".");
			}
		}																					
		else																				
		{																					
			break;
		}

		pIndexEntry = (IndexEntry*)((CHAR*)pIndexEntry + pIndexEntry->m_wLength);
		dwOffset += pIndexEntry->m_wLength;
	}
}




