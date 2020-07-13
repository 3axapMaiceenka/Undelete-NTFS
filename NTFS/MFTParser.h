#pragma once

#include <windows.h>

namespace ntfs
{
	struct MFTInfo;
	class  Runlist;

	struct MFTEntryHeader
	{
		CHAR   m_caSignature[4]; //  "FILE" or "BAAD"
		WORD   m_wMarkerArrayOffset;
		WORD   m_wMarkerArraySize;
		UINT64 m_uLSNlogFileNumber;
		WORD   m_wSequenceNumber;
		WORD   m_wHardLinkCount;
		WORD   m_wAttributeOffset;
		WORD   m_wFlags; // 0 - deleted, 0x01 - record in use, 0x02 - catalog
		DWORD  m_dwUsedEntrySize;
		DWORD  m_dwAllocatedEntrySize;
		UINT64 m_uBaseRecordAddress;
		WORD   m_wNextAttributeID;
	};

	enum ATTRIBUTE_TYPES
	{
		STANDARD_INFORMATION = 0x10,
		ATTRIBUTE_LIST = 0x20,
		FILE_NAME = 0x30,
		OBJECT_ID = 0x40,
		SECURITY_DESCRIPTOR = 0x50,
		VOLUME_NAME = 0x60,
		VOLUME_INFORMATION = 0x70,
		DATA = 0x80,
		INDEX_ROOT = 0x90,
		INDEX_ALLOCATION = 0xa0,
		BITMAP = 0xb0,
		REPARSE_POINT = 0xc0,
		EA_INFORMATION = 0xd0,
		EA = 0xe0,
		PROPERTY_SET = 0xf0, // Windows NT only?
		LOGGED_UTILITY_STREAM = 0x100
	};

	struct AttributeHeader
	{
		DWORD m_dwAttributeTypeID;
		DWORD m_dwAttributeSize;
		CHAR  m_cNonresidentFlag; // 1 - non-resident attribute
		CHAR  m_cNameLenght;
		WORD  m_wNameOffset;
		WORD  m_wFlags;
		WORD  m_wAttributeID;

		union
		{
			struct Resident
			{
				DWORD m_dwContentSize;
				WORD  m_wContentOffset;
			} m_Resident;

			struct Nonresident
			{
				UINT64 m_uRunlistStartingVCN;
				UINT64 m_uRunlistFinalVCN;
				WORD   m_wRunlistOffset;
				WORD   m_uCompressionUnitSize;
				CHAR   m_caUnused[4];
				UINT64 m_uAllocatedContentSize;
				UINT64 m_uActualContentSize;
				UINT64 m_uInitializedContentSize;
			} m_Nonresident;
		} m_Attr;
	};

	struct STANDART_INFORMATION_ATTR
	{
		UINT64 m_uCreationTime;
		UINT64 m_uModificationTime;
		UINT64 m_MFTModificationTime;
		UINT64 m_FileAccessTime;
		DWORD  m_dwFags;
		DWORD  m_dwMaxNumberOfVersions;
		DWORD  m_dwVersionNumber;
		DWORD  m_dwClassID;
		DWORD  m_dwOwnerID;
		DWORD  m_dwSecurityID;
		UINT64 m_uQuotaChanged;
		UINT64 m_USN;
	};

	enum STANDART_INFORMATION_FLAGS // the same falgs are used in $FILE_NAME attribute
	{
		READ_ONLY = 1,
		HIDDEN_FILE,
		SYSTEM_FILE = 4,
		ARCHIVE_FILE = 0x20,
		DEVICE = 0x40,
		USUAL_FILE = 0x80,
		TEMP_FILE = 0x100,
		SPARSE_FILE = 0x200,
		CONNECTION_POINT = 0x400,
		COMPRESSED_FILE = 0x800,
		OFFLINE_FILE = 0x1000,
		NONINDEXABLE_CONTENT = 0x2000,
		ENCRYPTED_FILE = 24000
	};

	struct FILE_NAME_ATTR
	{
		UINT64 m_uParentCatalogBaseAddress;
		UINT64 m_uFileCreationTime;
		UINT64 m_uModificationTime;
		UINT64 m_uMFTModificationTime;
		UINT64 m_uFileAccessTime;
		UINT64 m_uAllocatedFileSize;
		UINT64 m_uActualFileSize;
		DWORD  m_dwFalgs;
		DWORD  m_dwConnectionPoint;
		CHAR   m_cNameLength; // in unicode characters
		CHAR   m_cNamespace;

		// file name in unicode is located right after last field of that structure
	};

	class MFTParser
	{
	public:

		MFTParser(const MFTInfo& mftInfo);

		~MFTParser();

		void readFirstRecord(); // make private

	private:
		MFTInfo* m_pMft;
		Runlist* m_pRunlist;
		HANDLE   m_hDrive;
	};

} // namespace